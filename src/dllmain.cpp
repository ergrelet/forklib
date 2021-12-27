#include <windows.h>
//
#include <dbghelp.h>

#include "logging.h"

static bool InitializeSymbols();
static void CleanupSymbols();

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
      if (!InitializeSymbols()) {
        return FALSE;
      }
      break;
    case DLL_PROCESS_DETACH:
      CleanupSymbols();
      break;
    default:
      break;
  }
  return TRUE;
}

static bool InitializeSymbols() {
  if (!::SymInitialize(::GetCurrentProcess(), nullptr, FALSE)) {
    LOG("SymInitialize failed, 0x%X\n", ::GetLastError());
    return false;
  }

  auto sym_options = (::SymGetOptions() & ~SYMOPT_DEFERRED_LOADS) |
                     SYMOPT_ALLOW_ABSOLUTE_SYMBOLS;
#ifdef _DEBUG
  sym_options |= SYMOPT_DEBUG;
#endif
  ::SymSetOptions(sym_options);
  return true;
}

static void CleanupSymbols() { ::SymCleanup(::GetCurrentProcess()); }