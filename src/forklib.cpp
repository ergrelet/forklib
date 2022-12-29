#include "windows_phnt.h"
//
#include <TlHelp32.h>

#include "csr_region.h"
#include "fork.h"
#include "fork_settings.h"
#include "handle_inherit.h"
#include "logging.h"

#ifdef FORKLIB_SHARED_LIB
#define FORKLIB_EXPORT __declspec(dllexport)
#else
#define FORKLIB_EXPORT
#endif

extern "C" {

FORKLIB_EXPORT DWORD Fork(ForkSettings settings,
                          _Out_ LPPROCESS_INFORMATION process_information) {
  static const auto csr_region_opt = forklib::CsrRegion::GetForCurrentProcess();
  if (!csr_region_opt.has_value()) {
    LOG("FORKLIB: GetCsrRegionInfo failed\n");
    return -1;
  }

  static const bool is_windows_11 = []() {
    RTL_OSVERSIONINFOW version_info{};
    RtlGetVersion(&version_info);
    return version_info.dwBuildNumber >= 22000;
  }();

  LOG("FORKLIB: Before the fork, my pid is %d\n",
      GetProcessId(GetCurrentProcess()));

  PS_CREATE_INFO procInfo;
  RtlZeroMemory(&procInfo, sizeof(procInfo));
  HANDLE hProcess = NULL;
  HANDLE hThread = NULL;
  procInfo.Size = sizeof(PS_CREATE_INFO);

#ifndef _WIN64
  // WTF???? Discard *BIZARRE* segfault in ntdll from read fs:[0x18] that you
  // can ignore???
  LPTOP_LEVEL_EXCEPTION_FILTER oldFilter =
      SetUnhandledExceptionFilter(DiscardException);
#endif

  // This is the part that actually does the forking. Everything else is just
  // to clean up after the mess that's created afterwards
  NTSTATUS result = NtCreateUserProcess(
      &hProcess, &hThread, MAXIMUM_ALLOWED, MAXIMUM_ALLOWED, NULL, NULL,
      PROCESS_CREATE_FLAGS_INHERIT_FROM_PARENT |
          PROCESS_CREATE_FLAGS_INHERIT_HANDLES,
      THREAD_CREATE_FLAGS_CREATE_SUSPENDED, NULL, &procInfo, NULL);

#ifndef _WIN64
  // Clear the exception handler installed earlier.
  SetUnhandledExceptionFilter(oldFilter);
#endif

  if (!result) {
    // Parent process
    LOG("FORKLIB: I'm the parent\n");
    LOG("FORKLIB: hThread = %p, hProcess = %p\n", hThread, hProcess);
    LOG("FORKLIB: Thread ID = %x\n", GetThreadId(hThread));
    LOG("FORKLIB: Result = %d\n", result);

    if (settings.notify_csrss_from_parent &&
        !forklib::NotifyCsrssParent(hProcess, hThread)) {
      LOG("FORKLIB: NotifyCsrssParent failed\n");
      TerminateProcess(hProcess, 1);
      return -1;
    }

    if (process_information != nullptr) {
      process_information->hProcess = hProcess;
      process_information->hThread = hThread;
      process_information->dwProcessId = GetProcessId(hProcess);
      process_information->dwThreadId = GetThreadId(hThread);
    }

    ResumeThread(hThread);  // allow the child to connect to Csr.
    return GetProcessId(hProcess);
  } else {
    // Child process
    if (settings.restore_stdio) {
      // Remove these calls to improve performance, at the cost of losing stdio.
      FreeConsole();
      AllocConsole();
      SetStdHandle(STD_INPUT_HANDLE, stdin);
      SetStdHandle(STD_OUTPUT_HANDLE, stdout);
      SetStdHandle(STD_ERROR_HANDLE, stderr);
    }
    LOG("I'm the child\n");

    if (!ConnectCsrChild(csr_region_opt.value(), is_windows_11)) {
      ExitProcess(1);
    }

    if (settings.restore_stdio) {
      // Not safe to do fopen until after ConnectCsrChild
      forklib::ReopenStdioHandles();
    }

    return 0;
  }
}

// Mark all handles inheritble and uncloseable. See markHandle for more info.
// This should be called just before starting the forkserver.
FORKLIB_EXPORT BOOL MarkAllHandlesInheritable() {
  return forklib::EnumerateProcessHandles(&forklib::MarkHandle);
}

FORKLIB_EXPORT BOOL SuspendAllOtherThreads() {
  HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
  THREADENTRY32 te32;
  hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (hThreadSnap == INVALID_HANDLE_VALUE) return (FALSE);
  te32.dwSize = sizeof(THREADENTRY32);
  if (!Thread32First(hThreadSnap, &te32)) {
    CloseHandle(hThreadSnap);
    return FALSE;
  }
  do {
    if (te32.th32OwnerProcessID == GetCurrentProcessId()) {
      if (te32.th32ThreadID != GetCurrentThreadId()) {
        LOG("Yeet thread %d\n", te32.th32ThreadID);
        HANDLE hYeet = OpenThread(THREAD_TERMINATE | THREAD_SUSPEND_RESUME,
                                  FALSE, te32.th32ThreadID);
        if (hYeet != nullptr) {
          SuspendThread(hYeet);
          CloseHandle(hYeet);
        }
      }
    }
  } while (Thread32Next(hThreadSnap, &te32));
  LOG("Yeeted\n");

  CloseHandle(hThreadSnap);
  return TRUE;
}
}
