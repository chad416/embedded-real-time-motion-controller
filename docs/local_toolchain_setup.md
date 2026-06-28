# Local Toolchain Setup

This project builds in GitHub Actions and now has a validated project-local Windows toolchain path. A global CMake or compiler installation is optional.

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

## Pinned Portable Path

The repository includes a manifest with versioned download URLs and SHA-256 digests:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/install_portable_toolchain.ps1
powershell -ExecutionPolicy Bypass -File scripts/run_all_portable.ps1
```

By default, the installer writes to the workspace `.toolchains/emrtc` directory outside the Git repository. The runner adds the selected CMake and MinGW-w64 directories to the current child process only, selects the `MinGW Makefiles` generator, and runs the existing build, test, benchmark, and report scripts.

Use `-ToolchainRoot` on both scripts to choose another location.

## Verify And Build

For a global installation, open a fresh terminal and run:

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

The completed local result is recorded in [`local_windows_validation.md`](local_windows_validation.md).
