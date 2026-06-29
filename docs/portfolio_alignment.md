# Portfolio Alignment Matrix

This project maps to the embedded real-time motion controller blueprint in the portfolio PDF.

| PDF Expectation | Current Evidence | Gap |
| --- | --- | --- |
| C/C++ controller core | C++17 library and apps with hosted CI plus native Windows build/test evidence | Repeat on the future target-specific runtime. |
| FreeRTOS-style deterministic tasking | Static periodic-task port built against pinned FreeRTOS V11.3.0 and exercised on Windows simulator plus Linux POSIX ports | Deployment and timing capture on the selected MCU require that physical target. |
| Linux/PREEMPT_RT supervisory layer | Host simulator, benchmark, CLI, absolute-deadline timing probe, CPU stress, FIFO/memory-lock capability reporting, and Linux CI | PREEMPT_RT acceptance capture requires the selected kernel and machine. |
| EtherCAT abstraction | Injectable process-image bus, simulated/replay backends, dependency-backed SOEM v2 lifecycle and cyclic exchange, PDO codec, working-counter checks, and CI compile/test path | Selected NIC/drive mapping, recovery behavior, and physical exchange evidence pending. |
| CANopen/CiA 402 abstraction | Linux SocketCAN transport, NMT start/stop, split RPDO/TPDO exchange, heartbeat/stale-data supervision, CiA 402 scaling, strict runtime config, and injected transport tests | Device-specific PDO assignment/SDO provisioning and physical exchange evidence require the selected drive. |
| Timing instrumentation | Deterministic cycle evidence plus wall-clock absolute-deadline host probe and capability reporting | MCU/PREEMPT_RT target measurements require the physical target. |
| Fault injector | Required fault classes, cross-layer recovery metrics, tests, and JSON scenario suite | Repeat the same profiles against physical failure modes. |
| Quantitative report | Markdown, JSON, CSV, timing PDF, jitter/scenario/control/two-axis/recovery SVGs, and capture-profile manifest | Populate equivalent reports from bench captures. |
| Diagnostic shell | CLI commands for status, adapters, strict config validation, scenario/replay/two-axis execution, fault injection/reset, and report export | Bind commands to the selected physical interface during commissioning. |
| Graph-first article | `docs/technical_article.md` linked to committed timing, motion, scenario, and recovery visuals | Add physical plots without replacing the software baseline. |
| Recruiter-facing summary | `docs/recruiter_one_pager.md` generated from metrics and capture manifest | Keep synchronized with future hardware evidence. |
| Evidence packaging | `scripts/package_portfolio.ps1` exports docs, reports, graphs, capture profiles, and manifest | Add live bench evidence once available. |
| Hardware evidence archive | Immutable run-folder convention, structured manifest schema, archive validator, digest/completeness tests, parameter backups, raw traces, media, and review rules | Populate it with real bench evidence after hardware exists. |
| Software release gate | Historical `0.2.0` simulation gate plus `0.3.0` software-ready gate covering adapters, execution ports, recovery, evidence, CI, and claim language | Preserve both gates as hardware evidence is added. |
| Hardware target planning | Vendor-neutral simulated/EtherCAT/CANopen target profiles and generated manifest | Select a real drive only after current availability and safety documentation are reviewed. |
| Local native workflow | Pinned portable CMake/MinGW-w64 installer, runner, toolchain checker, and passing Windows validation record | Repeat on any future target-specific toolchain. |
| Bench-run tracking | GitHub issue templates and bench-run playbook | Real hardware issues will be opened once hardware exists. |
| Hardware runbooks | Simulated, EtherCAT CiA 402, and CANopen CiA 402 runbooks | Target-specific values need a selected real drive family. |
| Live bench demo | Hardware integration plan and repeatable portfolio demo script | Real bench unavailable in this phase. |
| Commissioning-grade package | README, architecture, timing, bus, watchdog, fault matrix, test plan, validation, benchmark, tradeoffs, status, BOM, commissioning logbook, checkout, profiles, archive validator, replay harness, lifecycle checklist, and implemented SOEM/SocketCAN paths | Populate target-specific values and real bench evidence. |

## Honest Completion Estimate

The defined software/simulation/documentation/admin package is 100% complete once the `0.3.0` exact-commit gate and tag pass. The full flagship artifact, including hardware commissioning, target timing evidence, and a live bench demonstration, is about 85% complete.

The lower full-project estimate reflects physical engineering that cannot be manufactured by simulation: fieldbus electrical state, device mapping, actual motion, target timing, recovery under cable/device faults, and stop-path evidence. Software backends and commissioning records are ready for that phase.
