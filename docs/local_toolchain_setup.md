# Local Toolchain Setup

This project already builds in GitHub Actions. Local Windows builds are still blocked until CMake and a C++17 compiler are available on PATH.

## Quick Check

Run:

```powershell
scripts/check_toolchain.ps1
```

For a build-required check:

```powershell
scripts/check_toolchain.ps1 -RequireCMake -RequireCompiler
```

The script checks for:

- `cmake`
- `ctest`
- one C++ compiler: `cl`, `clang++`, `g++`, or `c++`
- `python3` or `python`
- `git`
- `gh`

## Recommended Windows Paths

Choose one compiler path and keep the rest of the project unchanged.

| Option | When to use it | Notes |
| --- | --- | --- |
| MSVC Build Tools | Best default for Windows portfolio demos. | Install C++ build tools and CMake integration, then run commands from a Developer PowerShell or Developer Command Prompt. |
| LLVM/Clang | Good lightweight compiler path. | Install LLVM, ensure `clang++` and `cmake` are on PATH. |
| MinGW/GCC | Useful if you already have a GCC-based environment. | Ensure `g++`, `mingw32-make` or Ninja, and CMake are on PATH. |

## Verify And Build

After installation, open a fresh terminal and run:

```powershell
cmake --version
scripts/check_toolchain.ps1 -RequireCMake -RequireCompiler
scripts/run_all.ps1
```

If PowerShell script execution is blocked:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/run_all.ps1
```

## Expected Local Outputs

Successful local validation should produce:

- Native executables under `build/`.
- CTest pass output.
- Benchmark output under `reports/generated`.
- Generated sample report artifacts.

## Boundary

Installing a local toolchain enables native Windows validation. It still does not create a real EtherCAT/CANopen hardware validation claim. Hardware claims require the commissioning evidence defined in the capture profiles and hardware target profiles.
