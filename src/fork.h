#pragma once

#include "csr_region.h"
#include "windows_phnt.h"

namespace forklib {

BOOL NotifyCsrssParent(HANDLE hProcess, HANDLE hThread);
BOOL ConnectCsrChild(const CsrRegion& csr_region);
void ReopenStdioHandles();

}  // namespace forklib
