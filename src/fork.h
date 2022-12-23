#pragma once

#include "windows_phnt.h"

#ifdef FORKLIB_SHARED_LIB
#define FORKLIB_EXPORT __declspec(dllexport)
#else
#define FORKLIB_EXPORT
#endif

extern "C" {

FORKLIB_EXPORT DWORD fork(_Out_ LPPROCESS_INFORMATION lpProcessInformation);
FORKLIB_EXPORT BOOL EnumerateProcessHandles(
    void (*callback)(HANDLE, POBJECT_TYPE_INFORMATION, PUNICODE_STRING));
FORKLIB_EXPORT BOOL MarkAllHandles();
FORKLIB_EXPORT BOOL SuspendOtherThreads();
}
