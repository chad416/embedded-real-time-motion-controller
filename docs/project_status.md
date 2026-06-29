# Project Status

Last updated: 2026-06-29

Checkpoint note: post-`2f5f388` software-readiness work is implemented locally and being preserved in Git. It is not a final release until its exact hosted CI run passes. See [Work Remaining](work_remaining.md).

## Completion Snapshot

| Area | Status | Notes |
| --- | --- | --- |
| Standalone repository | Complete | Local Git repo created and pushed to private GitHub remote. |
| CMake/C++ source layout | Complete | Library, simulator, benchmark, CLI, and test runner are present. |
| Real-time controller core | Implemented | Deterministic 1 ms cycle model, jitter, missed cycle, deadline, watchdog, and trace instrumentation. |
| Drive and motion abstraction | Implemented | Axis abstraction, CiA 402-style states, profile position/velocity, command guards, statusword/controlword. |
| Bus abstraction | Implemented | Injectable `IBusBackend` with simulated, replay, and dependency-backed SOEM implementations. |
| Watchdog/safety-oriented layer | Implemented | Loop, bus, heartbeat, following-error, encoder feedback-loss, and E-stop/safe-stop handling. |
| Fault injector | Implemented | Required fault classes are represented in code and validation scenarios. |
| Telemetry and diagnostics | Implemented | CSV trace, JSON metrics, fault log, Markdown reports, and CLI commands. |
| Automated scenarios | Expanded | Built-in scenarios plus JSON scenario suite covering fault matrix, heartbeat timeout, bus disconnect, command abuse, encoder loss, and timing stress. |
| Two-axis coordination | Expanded | Optional two-axis straight-line coordinated move simulation now reports path error, final position error, settling limits, and acceptance pass/fail. |
| Adapter implementations | Expanded | SOEM and SocketCAN/CANopen now have cyclic software implementations; physical commissioning remains pending. |
| Adapter dependency discovery | Expanded | CMake discovers SOEM v2 packages or headers/library, supports strict required mode, and retains optional discovery for the other stacks. |
| External controller backend | Added | Controller constructor injection forwards controlword/targets and applies external statusword, position, velocity, and drive faults through the normal watchdog path. |
| SOEM dependency validation | Added | Pinned upstream SOEM builds in CI; local Windows dependency-backed compile/link passes, with hardware-free guards and PDO codec tests. |
| SocketCAN/CANopen path | Implemented, not commissioned | Linux transport, NMT, split RPDO/TPDO codec, heartbeat timeout, transport-loss diagnostics, and fake-transport tests are present. |
| Runtime adapter configuration | Implemented | Strict JSON and dry-run CLI validation cover mappings, scaling, layouts, COB-IDs, node ID, and timeouts without opening hardware. |
| Host real-time execution | Implemented | Absolute-deadline wall-clock probe exports measured timing and Linux FIFO/memory-lock capability results. |
| FreeRTOS execution port | Implemented, target-independent | Static periodic task and pinned V11.3.0 simulator smoke path exist; target timing remains hardware-specific. |
| Process-image mapping contract | Added | CiA 402 object indexes, bit widths, directions, and counts/unit encode-decode helpers are tested independently of hardware stacks. |
| Adapter exchange contract | Expanded | Mapping, stale receive data, heartbeat, bus-offline, restoration, invalid response, and outgoing command expectations are covered by adapter-level tests. |
| Adapter replay harness | Added | Recorded nominal and fault/recovery process-image traces run through `IBusBackend`, CiA 402 scaling, and the exchange monitor with zero expected-fault mismatches. |
| Adapter lifecycle checklist | Added | Discovery, configure, cyclic exchange, fault reaction, recovery, and shutdown gates are documented for each future backend. |
| Portfolio demo script | Added | `scripts/demo_portfolio.ps1` runs CLI status, adapters, replay, two-axis demo, JSON scenario, benchmark, sample evidence, and capture manifest generation. |
| Recruiter one-pager | Added | Generated from sample metrics and capture manifest to summarize evidence, role fit, talking points, and honest boundaries. |
| Portfolio evidence bundle | Added | `scripts/package_portfolio.ps1` exports docs, reports, graphs, capture profiles, and a manifest into a review-ready folder/zip. |
| Hardware target profiles | Added | Vendor-neutral target records define evidence requirements for simulated, EtherCAT CiA 402, and CANopen CiA 402 bench paths without making purchase recommendations. |
| Local toolchain setup | Complete | Pinned portable CMake and MinGW-w64 assets have checksums, an installer, a runner, and a validated local workflow. |
| Bench-run tracking | Added | GitHub issue templates and a bench-run playbook define required evidence for future simulated and hardware runs. |
| Hardware runbooks | Added | Simulated, EtherCAT CiA 402, and CANopen CiA 402 runbooks define future run order, evidence, and stop conditions. |
| Hardware evidence archive | Added | Immutable run folders, SHA-256 artifact records, structured run manifest, configuration backups, and review gates are defined. |
| Evidence archive validator | Added | CI tests manifest fields, safe paths, exact files, byte counts, SHA-256 digests, profile requirements, and hardware-claim prerequisites. |
| Simulation-complete milestone | Released | Tag `v0.2.0-simulation-complete` points to CI-verified commit `b86ba1a`; the release gate remains enforced on `master`. |
| Motion performance metrics | Added | Runtime JSON/Markdown reports include overshoot, settling time, steady-state error, max following error, and two-axis path metrics. |
| Visual evidence | Added | Timing PDF plus SVG jitter histogram, scenario pass/fail matrix, control response, two-axis path plot, and two-axis acceptance plot. |
| Data capture profiles | Added | Versioned profiles define simulation and future bench evidence requirements for timing, motion, jitter stress, heartbeat, bus dropout, and feedback faults. |
| Commissioning templates | Added | Hardware checkout checklist and commissioning logbook template are present for future bench validation. |
| Documentation | Implemented | Architecture, timing, bus map, drive state machine, watchdogs, fault matrix, test plan, validation, benchmark, tradeoffs, hardware plan. |
| Sample reports | Implemented | Deterministic sample artifacts committed under `reports/`. |
| Native local build/test | Complete | Windows CMake/GCC build, CTest, 10,000-cycle benchmark, and report generation pass through the portable runner. |
| GitHub CI | Passing | Latest hosted run builds, tests, benchmarks, generates reports, runs health checks, and uploads artifacts. |
| Portfolio PDF alignment | In progress | Software evidence is mature; the remaining gap is concentrated in production adapter integration, real-time target capture, and hardware commissioning. |
| Hardware commissioning | Not started | Remaining hardware-only work: real EtherCAT/CANopen adapter, drive PDO/OD mapping, timing measurements, commissioning. |

## Current Completion Estimate

Software/simulation/documentation side at this checkpoint: about 96%.

Full flagship scope including hardware commissioning and live evidence: about 83%.

The split estimate prevents the mature simulation package from hiding the substantial hardware work that remains.

## Immediate Next Actions

Complete the non-hardware checkpoint list in [Work Remaining](work_remaining.md), verify the exact hosted commit, and only then proceed to physical commissioning.
