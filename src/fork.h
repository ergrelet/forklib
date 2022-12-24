#pragma once

#include "csr_region.h"
#include "windows_phnt.h"

namespace forklib {

BOOL NotifyCsrssParent(HANDLE hProcess, HANDLE hThread);
BOOL ConnectCsrChild(const CsrRegion& csr_region, bool is_windows_11);
void ReopenStdioHandles();

}  // namespace forklib
