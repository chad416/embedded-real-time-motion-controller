# Embedded Real-Time Motion Controller

[![Motion Controller CI](https://github.com/chad416/embedded-real-time-motion-controller/actions/workflows/ci.yml/badge.svg)](https://github.com/chad416/embedded-real-time-motion-controller/actions/workflows/ci.yml)

Software-first embedded real-time motion controller for an industrial automation portfolio. The project demonstrates deterministic cyclic control, CiA 402-style drive handling, injectable bus backends, watchdog supervision, fault injection, telemetry, diagnostics, and validation reporting.

This repository does not claim certified SIL/PL safety and does not claim real EtherCAT/CANopen hardware validation. The current scope includes a safety-oriented deterministic software bench plus a dependency-backed, non-commissioned SOEM v2 adapter path. Physical hardware commissioning remains future work.

Current release line: `0.2.0`, the machine-gated simulation-complete software milestone.

## What Is Included

- C++17 controller core with a 1 ms default cyclic loop model.
- CiA 402-style axis state machine.
- Profile position, profile velocity, and cyclic synchronous mode abstractions.
- EtherCAT/CANopen-ready bus abstraction with a deterministic simulated backend.
- Replay-backed adapter harness for recorded nominal, stale-data, bus, heartbeat, invalid-response, and recovery exchanges.
- Pinned SOEM v2 compile path with guarded NIC/slave configuration, CiA 402 PDO byte exchange, working-counter supervision, and controller backend injection.
- Watchdogs for loop timing, heartbeat, bus health, following error, and encoder feedback loss.
- Fault injector for missed cycle, CPU spike, heartbeat timeout, bus dropout, packet loss, invalid transition, encoder loss, sensor noise, following error, drive fault, watchdog timeout, and command abuse.
- Timestamped telemetry, CSV trace, JSON metrics, persistent fault log, and Markdown validation report export.
- No-dependency C++ test runner and CMake build.
- Deterministic Python sample report generator for review workstations without a native compiler.

## Repository Layout

```text
apps/                 Simulator, benchmark, and diagnostic CLI entry points
config/capture_profiles/ Hardware-facing simulation and future bench capture profiles
config/hardware_targets/ Vendor-neutral future hardware target planning profiles
docs/                 Architecture, timing, safety-oriented design, tests, reports
examples/scenarios/   Deterministic controller scenario definitions
examples/replay/      Recorded process-image exchange fixtures
include/emrtc/        Public C++ interfaces
reports/              Sample generated validation artifacts
scripts/              Build, test, benchmark, and report generation helpers
src/                  C++ implementation
tests/                No-dependency unit/integration test runner
```

## Build And Test

Requires CMake 3.20+ and a C++17 compiler.

```powershell
scripts/build.ps1
scripts/test.ps1
scripts/benchmark.ps1 -Cycles 10000
```

Optional hardware-adapter integration flags are available for bench work:

```powershell
cmake -S . -B build-soem -DEMRTC_ENABLE_SOEM=ON -DEMRTC_REQUIRE_SOEM=ON
cmake -S . -B build-etherlab -DEMRTC_ENABLE_ETHERLAB=ON
cmake -S . -B build-socketcan -DEMRTC_ENABLE_SOCKETCAN=ON
cmake -S . -B build-canopennode -DEMRTC_ENABLE_CANOPENNODE=ON
```

The SOEM path becomes dependency-backed when SOEM v2 is discoverable. CI builds and tests that path against a pinned upstream commit. EtherLab, SocketCAN, and CANopenNode remain planned integration points. None of these flags claim commissioned hardware behavior.

If Windows blocks local PowerShell scripts, run the same commands through a process-local bypass:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build.ps1
```

One-command native validation:

```powershell
scripts/run_all.ps1
```

Pinned portable Windows toolchain and validation:

```powershell
scripts/install_portable_toolchain.ps1
scripts/run_all_portable.ps1
```

Portfolio demo walkthrough:

```powershell
scripts/demo_portfolio.ps1 -BuildDir build -OutDir reports/generated/demo
```

Portfolio evidence bundle:

```powershell
scripts/package_portfolio.ps1 -OutDir reports/generated/portfolio_bundle
```

Simulation-complete release gate:

```powershell
python scripts/check_simulation_milestone.py
```

Evidence archive integrity check:

```powershell
python scripts/validate_evidence_archive.py reports/generated/hardware/<run-directory> --require-complete
```

If CMake or a compiler is missing, use the pinned portable installer above or install a C++17 toolchain such as MSVC Build Tools, LLVM/Clang, or GCC/MinGW.

Use the local toolchain checker before a native build:

```powershell
scripts/check_toolchain.ps1 -RequireCMake -RequireCompiler
```

## Simulation And CLI

After building:

```powershell
build\emrtc_sim.exe --cycles 2500 --out reports/generated
build\emrtc_benchmark.exe --cycles 10000 --out reports/generated
build\emrtc_cli.exe show status
build\emrtc_cli.exe show adapters
build\emrtc_cli.exe run scenario
build\emrtc_cli.exe run replay examples/replay/fault_recovery_exchange.csv
build\emrtc_cli.exe inject fault heartbeat_timeout
build\emrtc_cli.exe export report reports/generated
```

## Sample Reports Without Native Toolchain

For review machines that do not yet have CMake/C++ installed:

```powershell
python scripts/generate_report.py --out reports/generated
python scripts/generate_capture_manifest.py --profiles config/capture_profiles --out reports
```

The committed files under `reports/` are deterministic sample artifacts. Native C++ validation should be rerun after installing the toolchain.

Key committed visuals include jitter, scenario matrix, control response, two-axis path, and two-axis acceptance plots under `reports/`.

## Documentation Index

- [Project Status](docs/project_status.md)
- [Checkpoint And Work Remaining](docs/work_remaining.md)
- [Portfolio Alignment](docs/portfolio_alignment.md)
- [Recruiter One-Pager](docs/recruiter_one_pager.md)
- [Portfolio Demo Walkthrough](docs/demo_walkthrough.md)
- [Graph-First Technical Article](docs/technical_article.md)
- [Preliminary BOM](docs/bom.md)
- [Commissioning Logbook Template](docs/commissioning_logbook.md)
- [Hardware Checkout Checklist](docs/hardware_checkout.md)
- [Data Capture Profiles](docs/data_capture_profiles.md)
- [Hardware Target Profiles](docs/hardware_target_profiles.md)
- [Hardware Evidence Archive](docs/hardware_evidence_archive.md)
- [Local Toolchain Setup](docs/local_toolchain_setup.md)
- [Local Windows Validation](docs/local_windows_validation.md)
- [Simulation-Complete Milestone](docs/software_complete_milestone.md)
- [Release Notes Template](docs/release_notes_template.md)
- [Changelog](CHANGELOG.md)
- [Bench Run Playbook](docs/bench_run_playbook.md)
- [Hardware Runbooks](docs/runbooks/simulated_validation_runbook.md)
- [Hardware Adapter Integration Notes](docs/adapters/README.md)
- [SOEM EtherCAT Backend](docs/adapters/soem_ethercat.md)
- [Architecture Overview](docs/architecture.md)
- [Timing Budget](docs/timing_budget.md)
- [Control Loop Architecture](docs/control_loop_architecture.md)
- [Bus Map](docs/bus_map.md)
- [Drive State Machine](docs/drive_state_machine.md)
- [Watchdog Design](docs/watchdog_design.md)
- [Fault Handling Matrix](docs/fault_handling_matrix.md)
- [Test Plan](docs/test_plan.md)
- [Validation Report](docs/validation_report.md)
- [Benchmark Appendix](docs/benchmark_appendix.md)
- [Design Tradeoffs](docs/design_tradeoffs.md)
- [Hardware Integration Plan](docs/hardware_integration_plan.md)
- [Final Software-Complete Report](docs/final_report.md)
- [Demo Narratives](docs/demo_narratives.md)
