# Local Windows Native Validation

Validation date: 2026-06-29

Validated release baseline: `v0.2.0-simulation-complete`

Validated commit: `b86ba1ac75ac2c51dd40bbc1921f6d363ca241b0`

## Toolchain

| Component | Version | Installation |
| --- | --- | --- |
| CMake | 4.3.3 | Checksum-verified portable archive |
| CTest | 4.3.3 | Included with CMake |
| GCC/G++ | 16.1.0 | WinLibs MinGW-w64 UCRT, POSIX threads, SEH |
| Build generator | MinGW Makefiles | `mingw32-make.exe` |
| Python | Bundled Codex Python runtime | Report generation fallback |

The pinned download URLs and SHA-256 digests are in [`../config/toolchains/windows_portable.json`](../config/toolchains/windows_portable.json). The toolchain is stored outside the Git repository under the workspace `.toolchains/emrtc` directory.

## Reproduction

Install the pinned archives:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/install_portable_toolchain.ps1
```

Run the complete native workflow:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/run_all_portable.ps1
```

## Results

- CMake configured the project with GNU C++ 16.1.0 and C++17 enabled.
- All library and executable targets compiled and linked on Windows.
- CTest passed `1/1` unit and integration test target.
- The SOEM and SocketCAN adapter-enabled build compiled successfully; Linux CI additionally compiles the raw SocketCAN transport.
- The native CLI status, adapter status, two-axis acceptance, JSON fault-matrix scenario, and portfolio demo completed.
- The post-release replay harness compiled and passed nominal plus four-fault recovery traces with zero expectation mismatches.
- The post-release SOEM v2 backend compiled and linked against the pinned dependency, including real context/lifecycle and cyclic exchange calls.
- The default suite now passes 45 controller, adapter, strict JSON/configuration, SocketCAN transport, host timing, replay, scenario, recovery, and performance checks.
- The pinned FreeRTOS V11.3.0 simulator port builds and its cyclic-task smoke test passes.
- The wall-clock host timing probe runs with absolute deadlines and exports CSV/JSON/Markdown evidence.
- The deterministic 10,000-cycle benchmark completed.
- Sample report and capture-manifest generation completed through the bundled Python fallback.

Benchmark summary:

| Metric | Result |
| --- | ---: |
| Cycles | 10,000 |
| Minimum cycle | 990,000 ns |
| Maximum cycle | 3,000,000 ns |
| Mean cycle | 1,000,324 ns |
| Missed cycles | 2 |
| Deadline overruns | 2 |

The missed cycles and deadline overruns are expected injected benchmark events, not uncontrolled host failures.

## Evidence Boundary

The dependency-backed SOEM executable cannot access a raw Windows interface until a compatible packet-capture runtime is installed; SocketCAN runtime access is Linux-specific. The default and injected-transport suites remain fully runnable. This result confirms compilation, linkage, execution-port integration, and deterministic software behavior. It does not validate raw-interface access, real fieldbus timing, physical drive behavior, electrical integration, or a certified safety function.
