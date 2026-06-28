# Portfolio Alignment Matrix

This project maps to the embedded real-time motion controller blueprint in the portfolio PDF.

| PDF Expectation | Current Evidence | Gap |
| --- | --- | --- |
| C/C++ controller core | C++17 library and apps with hosted CI build | Local Windows CMake/compiler still missing. |
| FreeRTOS-style deterministic tasking | Deterministic cycle model and control-loop architecture note | Actual FreeRTOS target adapter not yet implemented. |
| Linux/PREEMPT_RT supervisory layer | Host simulator, benchmark, CLI, GitHub Linux CI | Real PREEMPT_RT timing capture not yet implemented. |
| EtherCAT abstraction | Process-image bus abstraction, simulated backend, SOEM/EtherLab flags, non-fatal dependency discovery, adapter exchange contract, and adapter docs | Real SOEM/EtherLab exchange loop pending. |
| CANopen/CiA 402 abstraction | CiA 402-style drive state machine, tested process-image object contract, stale-PDO/restored-exchange contract checks, SocketCAN/CANopenNode flags, and adapter docs | Real CANopen PDO/SDO exchange pending. |
| Timing instrumentation | Cycle samples, jitter, missed-cycle, deadline counters | Real hardware timestamp source pending. |
| Fault injector | Required fault classes covered in code, tests, and JSON scenario suite | More cross-fault combinations can still be added. |
| Quantitative report | Markdown, JSON, CSV, timing PDF, jitter SVG, scenario SVG, two-axis path/acceptance SVGs, and capture-profile manifest | Hardware reports pending. |
| Diagnostic shell | CLI with status, scenario, fault injection, reset, report export, two-axis demo | Interactive shell mode could be added later. |
| Graph-first article | `docs/technical_article.md` with capture-profile and adapter-contract evidence | Add screenshots/plots after more generated artifacts. |
| Recruiter-facing summary | `docs/recruiter_one_pager.md` generated from metrics and capture manifest | Keep synchronized with future hardware evidence. |
| Evidence packaging | `scripts/package_portfolio.ps1` exports docs, reports, graphs, capture profiles, and manifest | Add live bench evidence once available. |
| Hardware evidence archive | Immutable run-folder convention, structured manifest schema, hashes, parameter backups, raw traces, media, and review rules | Populate it with real bench evidence after hardware exists. |
| Software release gate | Versioned `0.2.0` milestone definition and CI-enforced quantitative checks | Preserve the gate as future hardware work is added. |
| Hardware target planning | Vendor-neutral simulated/EtherCAT/CANopen target profiles and generated manifest | Select a real drive only after current availability and safety documentation are reviewed. |
| Local native workflow | Toolchain checker and Windows setup guide | Actual local compiler install remains pending on this machine. |
| Bench-run tracking | GitHub issue templates and bench-run playbook | Real hardware issues will be opened once hardware exists. |
| Hardware runbooks | Simulated, EtherCAT CiA 402, and CANopen CiA 402 runbooks | Target-specific values need a selected real drive family. |
| Live bench demo | Hardware integration plan and repeatable portfolio demo script | Real bench unavailable in this phase. |
| Commissioning-grade package | README, architecture, timing, bus, watchdog, fault matrix, test plan, validation, benchmark, tradeoffs, status, BOM, commissioning logbook, hardware checkout, capture profiles, adapter lifecycle checklist | Adapter design needs expansion before hardware purchase. |

## Honest Completion Estimate

Against the portfolio PDF, this project is about 89% complete as a flagship portfolio artifact.

The implemented software core is strong enough to compile and pass hosted CI, and the repo now has graph-first software evidence, versioned capture profiles, adapter contracts, a repeatable demo walkthrough, a recruiter-facing evidence summary, and a machine-enforced release boundary. The PDF target still includes more than software: real hardware commissioning, deeper adapter integration, target timing capture, and a live bench demo.
