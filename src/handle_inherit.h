#pragma once

#include "windows_phnt.h"

namespace forklib {

void MarkHandle(HANDLE handle, POBJECT_TYPE_INFORMATION, PUNICODE_STRING);
BOOL EnumerateProcessHandles(void (*)(HANDLE, POBJECT_TYPE_INFORMATION,
                                      PUNICODE_STRING));

}  // namespace forklib
