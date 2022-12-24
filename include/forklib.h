#pragma once

#include <Windows.h>
#include <winternl.h>

#ifdef FORKLIB_SHARED_LIB
#define FORKLIB_IMPORT __declspec(dllimport)
#else
#define FORKLIB_IMPORT
#endif

typedef struct _OBJECT_TYPE_INFORMATION {
  UNICODE_STRING TypeName;
  ULONG TotalNumberOfObjects;
  ULONG TotalNumberOfHandles;
  ULONG TotalPagedPoolUsage;
  ULONG TotalNonPagedPoolUsage;
  ULONG TotalNamePoolUsage;
  ULONG TotalHandleTableUsage;
  ULONG HighWaterNumberOfObjects;
  ULONG HighWaterNumberOfHandles;
  ULONG HighWaterPagedPoolUsage;
  ULONG HighWaterNonPagedPoolUsage;
  ULONG HighWaterNamePoolUsage;
  ULONG HighWaterHandleTableUsage;
  ULONG InvalidAttributes;
  GENERIC_MAPPING GenericMapping;
  ULONG ValidAccessMask;
  BOOLEAN SecurityRequired;
  BOOLEAN MaintainHandleCount;
  UCHAR TypeIndex;  // since WINBLUE
  CHAR ReservedByte;
  ULONG PoolType;
  ULONG DefaultPagedPoolCharge;
  ULONG DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

#ifdef __cplusplus
extern "C" {
#endif

FORKLIB_IMPORT DWORD Fork(_Out_ LPPROCESS_INFORMATION process_information);
FORKLIB_IMPORT BOOL MarkAllHandlesInheritable();
FORKLIB_IMPORT BOOL SuspendAllOtherThreads();

#ifdef __cplusplus
}
#endif
