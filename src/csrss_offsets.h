#pragma once

#include "windows_phnt.h"

struct CsrRegion {
  void* data_offset{};
  size_t data_size{};
  // Only populated when in WoW64
  ULONG64 data_offset_wow64{};
  ULONG64 data_size_wow64{};
};

bool GetCsrRegionInfo(CsrRegion* region_out_ptr);
