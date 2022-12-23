#include "dbghelp_context.h"

#include <Windows.h>
//
#include <DbgHelp.h>

#include "logging.h"

bool DbgHelpContext::InitializeSymbols() {
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

void DbgHelpContext::CleanupSymbols() { ::SymCleanup(::GetCurrentProcess()); }
