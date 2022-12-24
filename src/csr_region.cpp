#include "csr_region.h"

#include <dbghelp.h>

#include "dbghelp_context.h"
#include "logging.h"
#ifndef _WIN64
#include "wow64ext.h"
#endif

#define CSR_REGION_MOD_NAME "ntdll.dll"
#define CSR_REGION_START_SYM "ntdll!CsrServerApiRoutine"
#define CSR_REGION_END_SYM_X64 "ntdll!CsrHeap"
#define CSR_REGION_END_SYM_X86 "ntdll!CsrInitOnceDone"
#define RTL_CUR_DIR_REF_SYM "ntdll!RtlpCurDirRef"

static std::optional<CsrRegion> GetCsrRegionInfoNative();
static std::optional<CsrRegion> GetCsrRegionInfoWow64();

// Intializes symbol handler on construction and cleans it up on destruction
static DbgHelpContext dbghelp_ctx;

std::optional<CsrRegion> CsrRegion::GetForCurrentProcess() {
#ifndef _WIN64
  BOOL bIsWow64;
  if (!IsWow64Process(GetCurrentProcess(), &bIsWow64)) {
    LOG("IsWow64Process failed!\n");
    return {};
  }

  if (bIsWow64) {
    return GetCsrRegionInfoWow64();
  }
#endif  // _WIN64

  return GetCsrRegionInfoNative();
}

void CsrRegion::ResetNative() const {
  ::memset(reinterpret_cast<void*>(data_offset), 0,
           static_cast<size_t>(data_size));
  ::memset(reinterpret_cast<void*>(cur_dir_ref_offset), 0, sizeof(void*));
}

#ifndef _WIN64
void CsrRegion::ResetWow64() const {
  char tmp[512] = {0};
  if (static_cast<size_t>(data_size_wow64) > sizeof(tmp)) {
    LOG("FORKLIB: Csr data size too big! Fix this in fork.cpp\n");
    ::ExitProcess(1);
  }
  ::setMem64(data_offset_wow64, tmp, static_cast<size_t>(data_size_wow64));
  ::setMem64(cur_dir_ref_offset_wow64, tmp, sizeof(ULONG64));
}
#endif  // _WIN64

static std::optional<CsrRegion> GetCsrRegionInfoNative() {
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
    return {};
  }

  char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {0};
  PSYMBOL_INFO p_sym_info = (PSYMBOL_INFO)buffer;
  p_sym_info->SizeOfStruct = sizeof(*p_sym_info);
  p_sym_info->MaxNameLen = MAX_SYM_NAME;

  // CsrServerApiRoutine
  if (!::SymFromName(hProcess, CSR_REGION_START_SYM, p_sym_info)) {
    LOG("SymFromName failed, 0x%X\n", ::GetLastError());
    ::SymUnloadModule64(hProcess, mod_base_addr);
    return {};
  }
  LOG("ntdll!CsrServerApiRoutine is at 0x%I64X\n", p_sym_info->Address);
  CsrRegion region_out{};
  region_out.data_offset = p_sym_info->Address;

  // CsrHeap / CsrInitOnceDone
#ifdef _WIN64
  if (!::SymFromName(hProcess, CSR_REGION_END_SYM_X64, p_sym_info)) {
#elif _WIN32
  if (!::SymFromName(hProcess, CSR_REGION_END_SYM_X86, p_sym_info)) {
#endif
    LOG("SymFromName failed, 0x%X\n", ::GetLastError());
    ::SymUnloadModule64(hProcess, mod_base_addr);
    return {};
  }
  LOG("ntdll!CsrHeap is at 0x%I64X\n", p_sym_info->Address);
  region_out.data_size =
      p_sym_info->Address + p_sym_info->Size - region_out.data_offset;

  // RtlpCurDirRef
  if (!::SymFromName(hProcess, RTL_CUR_DIR_REF_SYM, p_sym_info)) {
    LOG("SymFromName failed, 0x%X\n", ::GetLastError());
    ::SymUnloadModule64(hProcess, mod_base_addr);
    return {};
  }
  LOG("%s is at 0x%I64X\n", RTL_CUR_DIR_REF_SYM, p_sym_info->Address);
  region_out.cur_dir_ref_offset = p_sym_info->Address;

  ::SymUnloadModule64(hProcess, mod_base_addr);
  return region_out;
}

#ifndef _WIN64

static std::optional<CsrRegion> GetCsrRegionInfoWow64() {
  // Get native symbols info
  auto region_out_opt = GetCsrRegionInfoNative();
  if (!region_out_opt.has_value()) {
    return {};
  }
  auto& region_out = region_out_opt.value();

  PVOID old_redir_value{};
  if (!::Wow64DisableWow64FsRedirection(&old_redir_value)) {
    LOG("Wow64DisableWow64FsRedirection failed, 0x%X\n", ::GetLastError());
    return {};
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
    return {};
  }
  ::Wow64RevertWow64FsRedirection(old_redir_value);

  char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {0};
  PSYMBOL_INFO p_sym_info = (PSYMBOL_INFO)buffer;
  p_sym_info->SizeOfStruct = sizeof(*p_sym_info);
  p_sym_info->MaxNameLen = MAX_SYM_NAME;

  // CsrServerApiRoutine
  if (!::SymFromName(hProcess, CSR_REGION_START_SYM, p_sym_info)) {
    LOG("SymFromName failed, 0x%X\n", ::GetLastError());
    ::SymUnloadModule64(hProcess, mod_base_addr);
    return {};
  }
  region_out.data_offset_wow64 = p_sym_info->Address;
  LOG("%s is at 0x%I64X\n", CSR_REGION_START_SYM, region_out.data_offset_wow64);

  // CsrHeap
  if (!::SymFromName(hProcess, CSR_REGION_END_SYM_X64, p_sym_info)) {
    LOG("SymFromName failed, 0x%X\n", ::GetLastError());
    ::SymUnloadModule64(hProcess, mod_base_addr);
    return {};
  }
  LOG("%s is at 0x%I64X\n", CSR_REGION_END_SYM_X64, p_sym_info->Address);
  region_out.data_size_wow64 =
      p_sym_info->Address + p_sym_info->Size - region_out.data_offset_wow64;

  // RtlpCurDirRef
  if (!::SymFromName(hProcess, RTL_CUR_DIR_REF_SYM, p_sym_info)) {
    LOG("SymFromName failed, 0x%X\n", ::GetLastError());
    ::SymUnloadModule64(hProcess, mod_base_addr);
    return {};
  }
  LOG("%s is at 0x%I64X\n", RTL_CUR_DIR_REF_SYM, p_sym_info->Address);
  region_out.cur_dir_ref_offset_wow64 = p_sym_info->Address;

  ::SymUnloadModule64(hProcess, mod_base_addr);
  return region_out_opt;
}

#endif  // _WIN64
