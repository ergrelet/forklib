#include "dbghelp_context.h"

#include "windows_phnt.h"
//
#include <DbgHelp.h>

#include "logging.h"

namespace forklib {

bool DbgHelpContext::InitializeSymbols() {
  if (!::SymInitialize(::GetCurrentProcess(), nullptr, FALSE)) {
    LOG("SymInitialize failed, 0x%X\n", ::GetLastError());
    return false;
  }

  auto sym_options = ::SymGetOptions() | SYMOPT_ALLOW_ABSOLUTE_SYMBOLS;
#ifdef _DEBUG
  sym_options |= SYMOPT_DEBUG;
#endif
  ::SymSetOptions(sym_options);
  return true;
}

void DbgHelpContext::CleanupSymbols() { ::SymCleanup(::GetCurrentProcess()); }

}  // namespace forklib
