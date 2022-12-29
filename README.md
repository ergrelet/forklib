# Forklib

Standalone version of `forklib` from [winnie](https://github.com/sslab-gatech/winnie).
This version retrieves required `ntdll` symbols at run time and should work on
all Windows 10 and Windows 11 versions.

Keep in mind that this library is mainly aimed at providing a fork API for
fuzzing. Many Windows APIs (*e.g.*, `user32` APIs) won't work seemlessly in forked
processes.

## How to Build

```
cmake -B x64
cmake --build x64 --config Release -- -maxcpucount
```

## How to Use

PDBs for the `ntdll` DLLs that your system uses must be available, either from
the current working directory of the application or from a properly configured
symbol server path in the `_NT_SYMBOL_PATH` environment variable.

You must copy `dbghelp.dll` and `symsrv.dll` from the `ext` folder in your
harness' working directory for the symbols to be automatically fetched from a
SymSrv path.

Note: the public interface is different from winnie's original forklib.
