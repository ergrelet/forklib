#pragma once

#ifndef __cplusplus
#include <stdbool.h>
#endif

// Settings for tweaking the forking behavior.
struct ForkSettings {
  // Notify CSR subsystem, from the parent, that a new child process has been
  // created.
  bool notify_csrss_from_parent;
  // Restore standard IO (e.g., stdin, stdout, stderr) in the child process.
  bool restore_stdio;
};
