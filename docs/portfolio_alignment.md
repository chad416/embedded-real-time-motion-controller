# Portfolio Alignment Matrix

This project maps to the embedded real-time motion controller blueprint in the portfolio PDF.

| PDF Expectation | Current Evidence | Gap |
| --- | --- | --- |
| C/C++ controller core | C++17 library and apps with hosted CI build | Local Windows CMake/compiler still missing. |
| FreeRTOS-style deterministic tasking | Deterministic cycle model and control-loop architecture note | Actual FreeRTOS target adapter not yet implemented. |
| Linux/PREEMPT_RT supervisory layer | Host simulator, benchmark, CLI, GitHub Linux CI | Real PREEMPT_RT timing capture not yet implemented. |
| EtherCAT abstraction | Process-image bus abstraction and simulated backend | SOEM/EtherLab adapter skeleton still needed. |
| CANopen/CiA 402 abstraction | CiA 402-style drive state machine and object mapping notes | CANopenNode/SocketCAN adapter skeleton still needed. |
| Timing instrumentation | Cycle samples, jitter, missed-cycle, deadline counters | Real hardware timestamp source pending. |
| Fault injector | Required fault classes covered in code and tests | More scenario files and fault combinations needed. |
| Quantitative report | Markdown, JSON, CSV, and generated timing PDF | More visual plots and hardware reports pending. |
| Diagnostic shell | CLI with status, scenario, fault injection, reset, report export, two-axis demo | Interactive shell mode could be added later. |
| Graph-first article | `docs/technical_article.md` | Add screenshots/plots after more generated artifacts. |
| Live bench demo | Hardware integration plan and demo scripts | Real bench unavailable in this phase. |
| Commissioning-grade package | README, architecture, timing, bus, watchdog, fault matrix, test plan, validation, benchmark, tradeoffs, status | BOM and adapter design need expansion before hardware purchase. |

## Honest Completion Estimate

Against the portfolio PDF, this project is about 65% complete as a flagship portfolio artifact.

The implemented software core is strong enough to compile and pass hosted CI, but the PDF target includes more than software: graph-first presentation, hardware commissioning path, adapter skeletons, benchmark PDF, deeper scenario library, and eventually a live bench demo.
