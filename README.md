# Forklib

Standalone version of `forklib` from [winnie](https://github.com/sslab-gatech/winnie).
This version retrieves required `ntdll` symbols at run time and should work on
all Windows 10 versions.

## How to Build

```
$ cmake -B x64
$ cmake --build x64 --config Release -- -maxcpucount
```
Note: You may want to set the `FORKLIB_NOTIFY_CSRSS_FROM_PARENT` and
`FORKLIB_RESTORE_STDIO` CMake options to `OFF` if you plan to build the library
for `winnie`.

## How to Use

PDBs for the `ntdll` DLLs that your system uses must be available, either from
the current working directory of the application or from a properly configured
symbol server path in the `_NT_SYMBOL_PATH` environment variable.
