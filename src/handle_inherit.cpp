#include "handle_inherit.h"

#include <cstdlib>

#include "logging.h"

namespace forklib {

// Marks a handle as inheritable by child (as handles are not inherited
// by children by default on Windows), and also mark them as uncloseable
// to prevent the forked child from closing any important handles. We
// don't know what the fuzzing target will do, and we need to make sure
// that we can reuse the same handles over and over again in our forkserver.
void MarkHandle(HANDLE handle, POBJECT_TYPE_INFORMATION, PUNICODE_STRING) {
  // SetHandleInformation(handle, HANDLE_FLAG_INHERIT |
  // HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_INHERIT |
  // HANDLE_FLAG_PROTECT_FROM_CLOSE);
  SetHandleInformation(handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
}

// This method simply iterates through all of the handles that are opened by the
// current process, and call a callback on each one. This can also be used for
// closing any dangling or leaked file handles to the input binary that we are
// mutating in the fuzzer.
BOOL EnumerateProcessHandles(void (*callback)(HANDLE, POBJECT_TYPE_INFORMATION,
                                              PUNICODE_STRING)) {
  if (callback == nullptr) {
    return FALSE;
  }

  NTSTATUS status;
  PSYSTEM_HANDLE_INFORMATION_EX handleInfo;
  ULONG handleInfoSize = 0x10000;
  HANDLE processHandle = GetCurrentProcess();
  ULONG i;

  handleInfo = (PSYSTEM_HANDLE_INFORMATION_EX)malloc(handleInfoSize);

  // NtQuerySystemInformation won't give us the correct buffer size,
  //  so we guess by doubling the buffer size.
  while ((status = NtQuerySystemInformation(
              SystemExtendedHandleInformation, handleInfo, handleInfoSize,
              NULL)) == STATUS_INFO_LENGTH_MISMATCH)
    handleInfo =
        (PSYSTEM_HANDLE_INFORMATION_EX)realloc(handleInfo, handleInfoSize *= 2);

  // NtQuerySystemInformation stopped giving us STATUS_INFO_LENGTH_MISMATCH.
  if (!NT_SUCCESS(status)) {
    LOG("NtQuerySystemInformation failed!\n");
    return FALSE;
  }

  LOG("WOW, %zd\n", handleInfo->NumberOfHandles);
  for (i = 0; i < handleInfo->NumberOfHandles; i++) {
    SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handle = handleInfo->Handles[i];
    POBJECT_TYPE_INFORMATION objectTypeInfo;
    PVOID objectNameInfo;
    ULONG returnLength;

    // Check if this handle belongs to the PID the user specified.
    if (handle.UniqueProcessId != GetCurrentProcessId()) {
      continue;
    }

    // we don't need to duplicate ... these are OUR handles lol
    HANDLE handleValue = (HANDLE)handle.HandleValue;

    // Query the object type.
    objectTypeInfo = (POBJECT_TYPE_INFORMATION)malloc(0x1000);
    if (!NT_SUCCESS(NtQueryObject(handleValue, ObjectTypeInformation,
                                  objectTypeInfo, 0x1000, NULL))) {
      LOG("[%#zx] Error!\n", handle.HandleValue);
      continue;
    }

    // Query the object name (unless it has an access of
    //   0x0012019f, on which NtQueryObject could hang.
    if (handle.GrantedAccess == 0x0012019f) {
      // We have the type, so display that.
      callback(handleValue, objectTypeInfo, NULL);
      free(objectTypeInfo);
      continue;
    }

    objectNameInfo = malloc(0x1000);
    if (!NT_SUCCESS(NtQueryObject(handleValue, ObjectNameInformation,
                                  objectNameInfo, 0x1000, &returnLength))) {
      // Reallocate the buffer and try again.
      objectNameInfo = realloc(objectNameInfo, returnLength);
      if (!NT_SUCCESS(NtQueryObject(handleValue, ObjectNameInformation,
                                    objectNameInfo, returnLength, NULL))) {
        // We have the type name, so just display that.
        callback(handleValue, objectTypeInfo, NULL);

        free(objectTypeInfo);
        free(objectNameInfo);
        continue;
      }
    }

    // Cast our buffer into an UNICODE_STRING.
    callback(handleValue, objectTypeInfo, (PUNICODE_STRING)objectNameInfo);

    free(objectTypeInfo);
    free(objectNameInfo);
  }

  free(handleInfo);

  return 0;
}

}  // namespace forklib
