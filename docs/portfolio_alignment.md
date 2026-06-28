# Portfolio Alignment Matrix

This project maps to the embedded real-time motion controller blueprint in the portfolio PDF.

| PDF Expectation | Current Evidence | Gap |
| --- | --- | --- |
| C/C++ controller core | C++17 library and apps with hosted CI build | Local Windows CMake/compiler still missing. |
| FreeRTOS-style deterministic tasking | Deterministic cycle model and control-loop architecture note | Actual FreeRTOS target adapter not yet implemented. |
| Linux/PREEMPT_RT supervisory layer | Host simulator, benchmark, CLI, GitHub Linux CI | Real PREEMPT_RT timing capture not yet implemented. |
| EtherCAT abstraction | Process-image bus abstraction, simulated backend, SOEM/EtherLab flags, and adapter docs | Real SOEM/EtherLab library linkage pending. |
| CANopen/CiA 402 abstraction | CiA 402-style drive state machine, object mapping notes, SocketCAN/CANopenNode flags, and adapter docs | Real CANopen stack linkage pending. |
| Timing instrumentation | Cycle samples, jitter, missed-cycle, deadline counters | Real hardware timestamp source pending. |
| Fault injector | Required fault classes covered in code, tests, and JSON scenario suite | More cross-fault combinations can still be added. |
| Quantitative report | Markdown, JSON, CSV, timing PDF, jitter SVG, and scenario SVG | Hardware reports pending. |
| Diagnostic shell | CLI with status, scenario, fault injection, reset, report export, two-axis demo | Interactive shell mode could be added later. |
| Graph-first article | `docs/technical_article.md` | Add screenshots/plots after more generated artifacts. |
| Live bench demo | Hardware integration plan and demo scripts | Real bench unavailable in this phase. |
| Commissioning-grade package | README, architecture, timing, bus, watchdog, fault matrix, test plan, validation, benchmark, tradeoffs, status, BOM, commissioning logbook, hardware checkout | Adapter design needs expansion before hardware purchase. |

## Honest Completion Estimate

Against the portfolio PDF, this project is about 65% complete as a flagship portfolio artifact.

The implemented software core is strong enough to compile and pass hosted CI, and the repo now has graph-first software evidence. The PDF target still includes more than software: real hardware commissioning, deeper adapter integration, richer plots, and eventually a live bench demo.
