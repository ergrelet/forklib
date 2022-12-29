#pragma once

#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>

#include "fork_settings.h"

#ifdef FORKLIB_SHARED_LIB
#define FORKLIB_IMPORT __declspec(dllimport)
#else
#define FORKLIB_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Forks the current process and continue execution in both processes.
// Returns -1 (0xFFFFFFFF) in case of error. In case of success, returns 0
// inside of the child process and returns the child's PID in the parent
// process.
FORKLIB_IMPORT DWORD Fork(ForkSettings settings,
                          _Out_ LPPROCESS_INFORMATION process_information);
// Marks all handles in the current process as inheritable.
FORKLIB_IMPORT BOOL MarkAllHandlesInheritable();
// Suspends all threads in the process but the calling one.
FORKLIB_IMPORT BOOL SuspendAllOtherThreads();

#ifdef __cplusplus
}
#endif
