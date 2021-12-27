#include "csrss_offsets.h"

#include <dbghelp.h>

#include "logging.h"
#ifndef _WIN64
#include "wow64ext.h"
#endif

#define CSR_REGION_MOD_NAME "ntdll.dll"
#define CSR_REGION_START_SYM_X64 "ntdll!CsrServerApiRoutine"
#define CSR_REGION_END_SYM_X64 "ntdll!CsrHeap"
#define CSR_REGION_START_SYM_X86 "ntdll!CsrServerApiRoutine"
#define CSR_REGION_END_SYM_X86 "ntdll!CsrInitOnceDone"

static bool GetCsrRegionInfoNative(CsrRegion* region_out_ptr);
static bool GetCsrRegionInfoWow64(CsrRegion* region_out_ptr);

bool GetCsrRegionInfo(CsrRegion* region_out_ptr) {
  if (region_out_ptr == nullptr) {
    return false;
  }

#ifndef _WIN64
  BOOL bIsWow64;
  if (!IsWow64Process(GetCurrentProcess(), &bIsWow64)) {
    LOG("IsWow64Process failed!\n");
    return FALSE;
  }

  if (bIsWow64) {
    return GetCsrRegionInfoWow64(region_out_ptr);
  }
#endif  // _WIN64

  return GetCsrRegionInfoNative(region_out_ptr);
}

static bool GetCsrRegionInfoNative(CsrRegion* region_out_ptr) {
  const HANDLE hProcess = ::GetCurrentProcess();
  const auto mod_base_addr =
      reinterpret_cast<DWORD64>(::GetModuleHandleA(CSR_REGION_MOD_NAME));
  if (!::SymLoadModuleEx(hProcess,             // target process
                         NULL,                 // handle to image - not used
                         CSR_REGION_MOD_NAME,  // name of image file
                         nullptr,              // name of module - not required
                         mod_base_addr,        // base address - not required
                         0,                    // size of image - not required
                         nullptr,  // MODLOAD_DATA used for special cases
                         0))       // flags - not required
  {
    LOG("SymLoadModuleEx failed, 0x%X\n", ::GetLastError());
    return false;
  }

  char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {0};
  PSYMBOL_INFO p_sym_info = (PSYMBOL_INFO)buffer;
  p_sym_info->SizeOfStruct = sizeof(*p_sym_info);
  p_sym_info->MaxNameLen = MAX_SYM_NAME;

#ifdef _WIN64
  if (!::SymFromName(hProcess, CSR_REGION_START_SYM_X64, p_sym_info)) {
#elif _WIN32
  if (!::SymFromName(hProcess, CSR_REGION_START_SYM_X86, p_sym_info)) {
#endif
    LOG("SymFromName failed, 0x%X\n", ::GetLastError());
    ::SymUnloadModule64(hProcess, mod_base_addr);
    return false;
  }
  LOG("Success, ntdll!CsrServerApiRoutine is at 0x%I64X\n",
      p_sym_info->Address);
  region_out_ptr->data_offset = reinterpret_cast<void*>(p_sym_info->Address);

#ifdef _WIN64
  if (!::SymFromName(hProcess, CSR_REGION_END_SYM_X64, p_sym_info)) {
#elif _WIN32
  if (!::SymFromName(hProcess, CSR_REGION_END_SYM_X86, p_sym_info)) {
#endif
    LOG("SymFromName failed, 0x%X\n", ::GetLastError());
    ::SymUnloadModule64(hProcess, mod_base_addr);
    return false;
  }
  LOG("Success, ntdll!CsrHeap is at 0x%I64X\n", p_sym_info->Address);
  region_out_ptr->data_size =
      p_sym_info->Address + p_sym_info->Size -
      reinterpret_cast<ULONG64>(region_out_ptr->data_offset);

  ::SymUnloadModule64(hProcess, mod_base_addr);
  return true;
}

#ifndef _WIN64

static bool GetCsrRegionInfoWow64(CsrRegion* region_out_ptr) {
  // Get native symbols info
  if (!GetCsrRegionInfoNative(region_out_ptr)) {
    return false;
  }

  PVOID old_redir_value{};
  if (!::Wow64DisableWow64FsRedirection(&old_redir_value)) {
    LOG("Wow64DisableWow64FsRedirection failed, 0x%X\n", ::GetLastError());
    return false;
  }

  const HANDLE hProcess = ::GetCurrentProcess();
  const auto mod_base_addr = ::GetModuleHandle64(L"ntdll.dll");
  if (!::SymLoadModuleEx(
          hProcess,                            // target process
          NULL,                                // handle to image - not used
          "C:\\Windows\\System32\\ntdll.dll",  // name of image file
          nullptr,                             // name of module - not required
          mod_base_addr,                       // base address - not required
          0,                                   // size of image - not required
          nullptr,  // MODLOAD_DATA used for special cases
          0))       // flags - not required
  {
    LOG("SymLoadModuleEx failed, 0x%X\n", ::GetLastError());
    ::Wow64RevertWow64FsRedirection(old_redir_value);
    return false;
  }
  ::Wow64RevertWow64FsRedirection(old_redir_value);

  char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {0};
  PSYMBOL_INFO p_sym_info = (PSYMBOL_INFO)buffer;
  p_sym_info->SizeOfStruct = sizeof(*p_sym_info);
  p_sym_info->MaxNameLen = MAX_SYM_NAME;

  if (!::SymFromName(hProcess, CSR_REGION_START_SYM_X64, p_sym_info)) {
    LOG("SymFromName failed, 0x%X\n", ::GetLastError());
    ::SymUnloadModule64(hProcess, mod_base_addr);
    return false;
  }
  region_out_ptr->data_offset_wow64 = p_sym_info->Address;
  LOG("Success, ntdll!CsrServerApiRoutine is at 0x%I64X\n",
      region_out_ptr->data_offset_wow64);

  if (!::SymFromName(hProcess, CSR_REGION_END_SYM_X64, p_sym_info)) {
    LOG("SymFromName failed, 0x%X\n", ::GetLastError());
    ::SymUnloadModule64(hProcess, mod_base_addr);
    return false;
  }
  LOG("Success, ntdll!CsrHeap is at 0x%I64X\n", p_sym_info->Address);
  region_out_ptr->data_size_wow64 = p_sym_info->Address + p_sym_info->Size -
                                    region_out_ptr->data_offset_wow64;

  ::SymUnloadModule64(hProcess, mod_base_addr);
  return true;
}

#endif  // _WIN64
