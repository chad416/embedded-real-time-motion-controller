# Portfolio Alignment Matrix

This project maps to the embedded real-time motion controller blueprint in the portfolio PDF.

| PDF Expectation | Current Evidence | Gap |
| --- | --- | --- |
| C/C++ controller core | C++17 library and apps with hosted CI plus native Windows build/test evidence | Repeat on the future target-specific runtime. |
| FreeRTOS-style deterministic tasking | Deterministic cycle model and control-loop architecture note | Actual FreeRTOS target adapter not yet implemented. |
| Linux/PREEMPT_RT supervisory layer | Host simulator, benchmark, CLI, GitHub Linux CI | Real PREEMPT_RT timing capture not yet implemented. |
| EtherCAT abstraction | Process-image bus abstraction, simulated and replay backends, SOEM/EtherLab flags, dependency discovery, exchange contract, and replay fault/recovery evidence | Real SOEM/EtherLab exchange loop pending. |
| CANopen/CiA 402 abstraction | CiA 402-style state machine, process-image object contract, replayed PDO-style exchange checks, SocketCAN/CANopenNode flags, and adapter docs | Real CANopen PDO/SDO exchange pending. |
| Timing instrumentation | Cycle samples, jitter, missed-cycle, deadline counters | Real hardware timestamp source pending. |
| Fault injector | Required fault classes covered in code, tests, and JSON scenario suite | More cross-fault combinations can still be added. |
| Quantitative report | Markdown, JSON, CSV, timing PDF, jitter SVG, scenario SVG, two-axis path/acceptance SVGs, and capture-profile manifest | Hardware reports pending. |
| Diagnostic shell | CLI with status, scenario, fault injection, reset, report export, two-axis demo | Interactive shell mode could be added later. |
| Graph-first article | `docs/technical_article.md` with capture-profile and adapter-contract evidence | Add screenshots/plots after more generated artifacts. |
| Recruiter-facing summary | `docs/recruiter_one_pager.md` generated from metrics and capture manifest | Keep synchronized with future hardware evidence. |
| Evidence packaging | `scripts/package_portfolio.ps1` exports docs, reports, graphs, capture profiles, and manifest | Add live bench evidence once available. |
| Hardware evidence archive | Immutable run-folder convention, structured manifest schema, archive validator, digest/completeness tests, parameter backups, raw traces, media, and review rules | Populate it with real bench evidence after hardware exists. |
| Software release gate | Versioned `0.2.0` milestone definition and CI-enforced quantitative checks | Preserve the gate as future hardware work is added. |
| Hardware target planning | Vendor-neutral simulated/EtherCAT/CANopen target profiles and generated manifest | Select a real drive only after current availability and safety documentation are reviewed. |
| Local native workflow | Pinned portable CMake/MinGW-w64 installer, runner, toolchain checker, and passing Windows validation record | Repeat on any future target-specific toolchain. |
| Bench-run tracking | GitHub issue templates and bench-run playbook | Real hardware issues will be opened once hardware exists. |
| Hardware runbooks | Simulated, EtherCAT CiA 402, and CANopen CiA 402 runbooks | Target-specific values need a selected real drive family. |
| Live bench demo | Hardware integration plan and repeatable portfolio demo script | Real bench unavailable in this phase. |
| Commissioning-grade package | README, architecture, timing, bus, watchdog, fault matrix, test plan, validation, benchmark, tradeoffs, status, BOM, commissioning logbook, hardware checkout, capture profiles, archive validator, replay harness, adapter lifecycle checklist | Production adapter and real bench evidence remain. |

## Honest Completion Estimate

The software/simulation/documentation package is about 90% complete. The full flagship artifact, including real adapter integration, hardware commissioning, target timing evidence, and a live bench demonstration, is about 75% complete.

The implemented software core compiles and passes both hosted CI and native Windows validation. The repository now has graph-first software evidence, versioned capture profiles, adapter contracts, a repeatable demo walkthrough, a recruiter-facing evidence summary, and a machine-enforced release boundary. The lower full-project estimate reflects the real engineering still needed on fieldbus hardware, physical motion, timing capture, and stop-path evidence.
