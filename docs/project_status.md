# Project Status

Last updated: 2026-06-29

## Completion Snapshot

| Area | Status | Notes |
| --- | --- | --- |
| Standalone repository | Complete | Local Git repo created and pushed to private GitHub remote. |
| CMake/C++ source layout | Complete | Library, simulator, benchmark, CLI, and test runner are present. |
| Real-time controller core | Implemented | Deterministic 1 ms cycle model, jitter, missed cycle, deadline, watchdog, and trace instrumentation. |
| Drive and motion abstraction | Implemented | Axis abstraction, CiA 402-style states, profile position/velocity, command guards, statusword/controlword. |
| Bus abstraction | Implemented for simulation | Simulated process image with packet loss, dropout, heartbeat timeout, invalid response, counters. |
| Watchdog/safety-oriented layer | Implemented | Loop, bus, heartbeat, following-error, encoder feedback-loss, and E-stop/safe-stop handling. |
| Fault injector | Implemented | Required fault classes are represented in code and validation scenarios. |
| Telemetry and diagnostics | Implemented | CSV trace, JSON metrics, fault log, Markdown reports, and CLI commands. |
| Automated scenarios | Expanded | Built-in scenarios plus JSON scenario suite covering fault matrix, heartbeat timeout, bus disconnect, command abuse, encoder loss, and timing stress. |
| Two-axis coordination | Expanded | Optional two-axis straight-line coordinated move simulation now reports path error, final position error, settling limits, and acceptance pass/fail. |
| Adapter skeletons | Expanded | SOEM, EtherLab, SocketCAN, and CANopenNode skeletons have CMake build flags, status reporting, CI smoke build, and adapter-specific README files. |
| Adapter dependency discovery | Added | CMake now performs non-fatal discovery for SOEM, EtherLab, SocketCAN headers, and CANopenNode when flags are enabled. |
| Process-image mapping contract | Added | CiA 402 object indexes, bit widths, directions, and counts/unit encode-decode helpers are tested independently of hardware stacks. |
| Adapter exchange contract | Expanded | Mapping validation, stale receive-data detection, heartbeat health, bus-offline, restored exchange, and invalid-response handling are covered by adapter-level tests. |
| Adapter lifecycle checklist | Added | Discovery, configure, cyclic exchange, fault reaction, recovery, and shutdown gates are documented for each future backend. |
| Portfolio demo script | Added | `scripts/demo_portfolio.ps1` runs CLI status, adapters, two-axis demo, JSON scenario, benchmark, sample evidence, and capture manifest generation. |
| Recruiter one-pager | Added | Generated from sample metrics and capture manifest to summarize evidence, role fit, talking points, and honest boundaries. |
| Portfolio evidence bundle | Added | `scripts/package_portfolio.ps1` exports docs, reports, graphs, capture profiles, and a manifest into a review-ready folder/zip. |
| Hardware target profiles | Added | Vendor-neutral target records define evidence requirements for simulated, EtherCAT CiA 402, and CANopen CiA 402 bench paths without making purchase recommendations. |
| Local toolchain setup | Added | Windows-oriented CMake/C++ setup guide and `scripts/check_toolchain.ps1` are present; actual local compiler install remains a machine-level task. |
| Bench-run tracking | Added | GitHub issue templates and a bench-run playbook define required evidence for future simulated and hardware runs. |
| Motion performance metrics | Added | Runtime JSON/Markdown reports include overshoot, settling time, steady-state error, max following error, and two-axis path metrics. |
| Visual evidence | Added | Timing PDF plus SVG jitter histogram, scenario pass/fail matrix, control response, two-axis path plot, and two-axis acceptance plot. |
| Data capture profiles | Added | Versioned profiles define simulation and future bench evidence requirements for timing, motion, jitter stress, heartbeat, bus dropout, and feedback faults. |
| Commissioning templates | Added | Hardware checkout checklist and commissioning logbook template are present for future bench validation. |
| Documentation | Implemented | Architecture, timing, bus map, drive state machine, watchdogs, fault matrix, test plan, validation, benchmark, tradeoffs, hardware plan. |
| Sample reports | Implemented | Deterministic sample artifacts committed under `reports/`. |
| Native local build/test | Blocked | This machine currently lacks CMake and a C++17 compiler on PATH. |
| GitHub CI | Passing | Latest hosted run builds, tests, benchmarks, generates reports, runs health checks, and uploads artifacts. |
| Portfolio PDF alignment | In progress | Core software is strong, but graph-first article, timing PDF, scenario library, adapter skeletons, and hardware path are still being expanded. |
| Hardware commissioning | Not started | Remaining hardware-only work: real EtherCAT/CANopen adapter, drive PDO/OD mapping, timing measurements, commissioning. |

## Current Completion Estimate

Software/simulation/documentation side against the portfolio PDF target: about 86%.

The earlier 90% estimate was too optimistic because it measured the codebase scaffold, not the full flagship portfolio artifact described in the PDF. The remaining work includes target-specific hardware runbooks, hardware evidence archive conventions, and eventually live bench commissioning.

## Immediate Next Actions

1. Add hardware-target-specific capture runbooks once a real drive family is selected.
2. Add optional hardware evidence archive conventions for photos, drive parameter backups, and raw traces.
3. Add a final software-complete tag checklist.
4. Install local CMake plus a C++17 compiler when convenient, then rerun `scripts/run_all.ps1`.
