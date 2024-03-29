// todo: investigate why MessageBoxA doesnt work
#include "fork.h"

#include <wchar.h>

#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>

// Uncodumented headers csrss stuff
#include "csr_region.h"
#include "csrss.h"

// ReWolf's library to fuck with 64-bit memory from 32-bit WoW64 programs
#ifndef _WIN64
#include "wow64ext.h"
#endif

#include "logging.h"

namespace forklib {

// Populate this from the parent only once, to reduce the overhead on the child
// side
static const DWORD kSessionId = []() {
  DWORD session_id{};
  ProcessIdToSessionId(GetProcessId(GetCurrentProcess()), &session_id);
  return session_id;
}();
static void* pCtrlRoutine =
    (void*)GetProcAddress(GetModuleHandleA("kernelbase"), "CtrlRoutine");

// When a new child process is spawned, the parent must call
// CsrClientCallServer with API number BasepCreateProcess to notify
// the csrss subsystem of the new process. However, this seems to
// be optional as the child process will work without doing this
// call.
BOOL NotifyCsrssParent(HANDLE hProcess, HANDLE hThread) {
  PROCESS_BASIC_INFORMATION info;
  if (!NT_SUCCESS(NtQueryInformationProcess(hProcess, ProcessBasicInformation,
                                            &info, sizeof(info), 0))) {
    LOG("FORKLIB: NtQueryInformationProcess failed!\n");
    return FALSE;
  }

  BOOL bIsWow64;
  if (!IsWow64Process(GetCurrentProcess(), &bIsWow64)) {
    LOG("FORKLIB: IsWow64Process failed!\n");
    return FALSE;
  }

  NTSTATUS result;
  if (bIsWow64) {
    CSR_API_MSG64 csrmsg;
    RtlZeroMemory(&csrmsg, sizeof(csrmsg));
    csrmsg.CreateProcessRequest.PebAddressNative =
        (ULONGLONG)info.PebBaseAddress;
    csrmsg.CreateProcessRequest.ProcessorArchitecture =
        PROCESSOR_ARCHITECTURE_INTEL;
    csrmsg.CreateProcessRequest.ProcessHandle = (ULONGLONG)hProcess;
    csrmsg.CreateProcessRequest.ThreadHandle = (ULONGLONG)hThread;
    csrmsg.CreateProcessRequest.ClientId.UniqueProcess = GetProcessId(hProcess);
    csrmsg.CreateProcessRequest.ClientId.UniqueThread = GetThreadId(hThread);
    result = CsrClientCallServer64(
        &csrmsg, NULL,
        CSR_MAKE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepCreateProcess),
        sizeof(csrmsg.CreateProcessRequest));
  } else {
    CSR_API_MSG csrmsg;
    RtlZeroMemory(&csrmsg, sizeof(csrmsg));
    csrmsg.CreateProcessRequest.PebAddressNative = info.PebBaseAddress;
#ifdef _WIN64
    csrmsg.CreateProcessRequest.ProcessorArchitecture =
        PROCESSOR_ARCHITECTURE_AMD64;
#else
    csrmsg.CreateProcessRequest.ProcessorArchitecture =
        PROCESSOR_ARCHITECTURE_INTEL;
#endif
    csrmsg.CreateProcessRequest.ProcessHandle = hProcess;
    csrmsg.CreateProcessRequest.ThreadHandle = hThread;
    csrmsg.CreateProcessRequest.ClientId.UniqueProcess =
        (HANDLE)((size_t)GetProcessId(hProcess));
    csrmsg.CreateProcessRequest.ClientId.UniqueThread =
        (HANDLE)((size_t)GetThreadId(hThread));
    result = CsrClientCallServer(
        &csrmsg, NULL,
        CSR_MAKE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepCreateProcess),
        sizeof(csrmsg.CreateProcessRequest));
  }

  if (!NT_SUCCESS(result)) {
    LOG("CsrClientCallServer(BasepCreateThread) failed!\n");
    return FALSE;
  }

  LOG("FORKLIB: Successfully notified Csr of child!\n");
  return TRUE;
}

// When the a new process is spawned, it must call CsrClientConnectToServer
// and RtlRegisterThreadWithCsrss to connect to the various csrss subsystems
// (such as Windows subsystem, Console subsystem, etc). If this is not done,
// then nearly every function in the Win32 API will lead to segfault. It seems
// that internally the APIs depend on csrss in some way.
//
// j00ru documented csrss on his blog:
// https://j00ru.vexillium.org/2010/07/windows-csrss-write-up-inter-process-communication-part-1/
// https://j00ru.vexillium.org/2010/07/windows-csrss-write-up-inter-process-communication-part-2/
//
// However, our situation is even trickier than usual, since we are a *forked*
// process, meaning that all memory values are cloned from the parent process.
// This is important because CsrClientConnectToServer and
// RtlRegisterThreadWithCsrss seem to initialize some of the global variables in
// ntdll, and the two functions will not work if these variable are already
// initialized. Therefore, it's our responsibility to also *manually
// de-initialize* these global variables by zeroing them before reconnecting
// with csrss.
//
// Yet *another* complication is WoW64: WoW64 enables the execution of 32-bit
// executeables on 64-bit Windows. The program sees a 32-bit address space and
// the 32-bit version of all system dlls. However, the 32-bit ntdll is really
// just a shim to call the 64-bit version of all of the functions it exposes. In
// other words on WoW64, there are actually *two* copies of ntdll loaded: the
// 32-bit version exposed by WoW64, and the 64-bit version that is loaded into
// every process. Therefore, we need to de-initialize the global variables in
// *both the 64- and 32-bit version of ntdll*. This is accomplished using some
// tRicKErY by jumping to 64-bit code from 32-bit.
//
// This method only supports Windows 10. Somewhere between Windows 7 and Windows
// 10, Microsoft refactored Windows to rely less and less on csrss. Hence the
// API and structures are much simpler on Windows 10 than Windows 7, and as a
// result our job is much easier.
BOOL ConnectCsrChild(const CsrRegion& csr_region, bool is_windows_11) {
  BOOL bIsWow64{};
  if (!IsWow64Process(GetCurrentProcess(), &bIsWow64)) {
    LOG("FORKLIB: IsWow64Process failed!\n");
    return FALSE;
  }

  // Zero Csr fields???
  // Required or else Csr calls will crash
  LOG("FORKLIB: De-initialize ntdll csr data\n");
#ifdef _WIN64
  LOG("FORKLIB: Csr data = %llx\n", csr_region.data_offset);
  csr_region.ResetNative();
#else
  LOG("FORKLIB: Csr data = %llx\n", csr_region.data_offset);
  csr_region.ResetNative();

  if (bIsWow64) {
    LOG("FORKLIB: ntdll 64 = %llx\n", GetModuleHandle64(L"ntdll.dll"));
    LOG("FORKLIB: Csr data 64 = %llx\n", csr_region.data_offset_wow64);
    csr_region.ResetWow64();
  }
#endif

  wchar_t ObjectDirectory[100]{};
  swprintf(ObjectDirectory, ARRAYSIZE(ObjectDirectory),
           L"\\Sessions\\%d\\Windows", kSessionId);
  LOG("FORKLIB: Session_id: %d\n", kSessionId);

  // Not required?
  LOG("FORKLIB: Link Console subsystem...\n");
  BOOLEAN ServerToServerCall{};
  if (!NT_SUCCESS(CsrClientConnectToServer(ObjectDirectory, 1, &pCtrlRoutine,
                                           sizeof(void*),
                                           &ServerToServerCall))) {
    LOG("FORKLIB: CsrClientConnectToServer failed!\n");
    return FALSE;
  }

  LOG("FORKLIB: Link Windows subsystem...\n");
  // passing &gfServerProcess is not necessary, actually? passing
  // &ServerToServerCall is okay?
  char buf[0x248]{};  // this seem to just be all zero everytime?
  const ULONG buf_size = is_windows_11 ? 0x248 : 0x240;
  if (!NT_SUCCESS(CsrClientConnectToServer(ObjectDirectory, 3, buf, buf_size,
                                           &ServerToServerCall))) {
    LOG("FORKLIB: CsrClientConnectToServer failed!\n");
    return FALSE;
  }

  LOG("FORKLIB: Connect to Csr...\n");
  if (!NT_SUCCESS(RtlRegisterThreadWithCsrss())) {
    LOG("FORKLIB: RtlRegisterThreadWithCsrss failed!\n");
    return FALSE;
  }

  LOG("FORKLIB: Connected to Csr!\n");
  return TRUE;
}

// Fix stdio handles of the child. If this isn't done, the child
// will inherit the stdio of the parent, and operations to those
// file descriptors will just not work.
void ReopenStdioHandles() {
  freopen_s((FILE**)stdout, "CONOUT$", "w+", stdout);
  freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
  freopen_s((FILE**)stderr, "CONOUT$", "w+", stdout);

  SetStdHandle(STD_INPUT_HANDLE, stdin);
  SetStdHandle(STD_OUTPUT_HANDLE, stdout);
  SetStdHandle(STD_ERROR_HANDLE, stderr);
}

#ifndef _WIN64
LONG WINAPI DiscardException(EXCEPTION_POINTERS* ExceptionInfo) {
  LOG("FORKLIB: Discarding exception %08x to %p, at instruction %08x\n",
      ExceptionInfo->ExceptionRecord->ExceptionCode,
      ExceptionInfo->ExceptionRecord->ExceptionAddress,
      ExceptionInfo->ContextRecord->Eip);
  return EXCEPTION_CONTINUE_EXECUTION;
}
#endif

}  // namespace forklib
