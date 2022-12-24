#pragma once

#include <Windows.h>

#include <optional>

namespace forklib {

struct CsrRegion {
  // Csr*
  ULONG64 data_offset{};
  ULONG64 data_size{};
  // RtlpCurDirRef
  ULONG64 cur_dir_ref_offset{};

  // Only populated when in WoW64
  // Csr*
  ULONG64 data_offset_wow64{};
  ULONG64 data_size_wow64{};
  // RtlpCurDirRef
  ULONG64 cur_dir_ref_offset_wow64{};

  static std::optional<CsrRegion> GetForCurrentProcess();

  void ResetNative() const;
#ifndef _WIN64
  void ResetWow64() const;
#endif  // _WIN64
};

}  // namespace forklib
