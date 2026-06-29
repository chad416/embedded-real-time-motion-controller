# Project Status

Last updated: 2026-06-29

Release note: annotated tag `v0.3.0-software-ready` points to commit `6862f016a1a176b252ce98c9b90ee5ca86a1be83`; exact-commit hosted run `28344205573` passed the complete workflow.

## Completion Snapshot

| Area | Status | Notes |
| --- | --- | --- |
| Standalone repository | Complete | Local Git repo created and pushed to private GitHub remote. |
| CMake/C++ source layout | Complete | Library, simulator, benchmark, CLI, and test runner are present. |
| Real-time controller core | Implemented | Deterministic 1 ms cycle model, jitter, missed cycle, deadline, watchdog, and trace instrumentation. |
| Drive and motion abstraction | Implemented | Axis abstraction, CiA 402-style states, profile position/velocity, command guards, statusword/controlword. |
| Bus abstraction | Implemented | Injectable `IBusBackend` with simulated, replay, dependency-backed SOEM, and Linux SocketCAN/CANopen implementations. |
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
| Adapter lifecycle checklist | Added | Discovery, configure, cyclic exchange, fault reaction, recovery, and shutdown gates are documented for each backend and optional alternative. |
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
| Software-ready milestone | Released | Tag `v0.3.0-software-ready` points to CI-verified commit `6862f01`; all defined non-hardware work is closed. |
| Motion performance metrics | Added | Runtime JSON/Markdown reports include overshoot, settling time, steady-state error, max following error, and two-axis path metrics. |
| Visual evidence | Added | Timing PDF plus SVG jitter histogram, scenario pass/fail matrix, control response, two-axis path/acceptance, and recovery-comparison plots. |
| Data capture profiles | Added | Versioned profiles define simulation and future bench evidence requirements for timing, motion, jitter stress, heartbeat, bus dropout, and feedback faults. |
| Commissioning templates | Added | Hardware checkout checklist and commissioning logbook template are present for future bench validation. |
| Documentation | Implemented | Architecture, timing, bus map, drive state machine, watchdogs, fault matrix, test plan, validation, benchmark, tradeoffs, hardware plan. |
| Sample reports | Implemented | Deterministic sample artifacts committed under `reports/`. |
| Native local build/test | Complete | Windows CMake/GCC build, CTest, 10,000-cycle benchmark, and report generation pass through the portable runner. |
| GitHub CI | Passing | Latest hosted run builds, tests, benchmarks, generates reports, runs health checks, and uploads artifacts. |
| Software-ready release gate | Implemented | `0.3.0` definition enforces tests, scenarios, recovery, adapters, execution ports, CI paths, evidence, and claim boundaries. |
| Portfolio PDF alignment | Software scope complete | Every hardware-free deliverable is implemented; target timing, physical fieldbus/motion, and stop-path evidence require the bench. |
| Hardware commissioning | Not started | Software adapters exist; remaining work is physical device selection/connection, vendor-verified mapping, target measurements, motion/fault/stop tests, and evidence capture. |

## Current Completion Estimate

Defined software/simulation/documentation/admin scope: 100% complete and released at `v0.3.0-software-ready`.

Full PDF flagship scope including hardware commissioning and live evidence: about 85%.

The remaining approximately 15% is not a loose software backlog. It is the physical commissioning phase: selected equipment, real bus state, target timing, actual motion and recovery, electrical stop behavior, and archived bench evidence.

## Immediate Next Actions

Follow [Hardware Commissioning Remaining](work_remaining.md) when physical equipment is available. No additional software deliverable is being held open as a substitute for missing hardware evidence.
