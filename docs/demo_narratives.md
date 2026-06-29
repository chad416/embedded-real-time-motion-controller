# Demo Narratives

## 90-Second Demo

This project is a software-first embedded real-time motion controller. I built the controller core in C++ with a 1 ms deterministic cycle model, a CiA 402-style axis state machine, watchdogs, telemetry, dependency-backed SOEM, and Linux SocketCAN/CANopen. The simulator can enable a drive, run position/velocity/coordinated moves, inject bus and feedback faults, and export CSV/JSON/Markdown reports. A pinned FreeRTOS port and a wall-clock host probe exercise execution outside the deterministic simulator. The project records cycle timing, jitter, missed cycles, packet loss, watchdog trips, recovery behavior, and pass/fail evidence. The remaining work is physical commissioning, not a missing software backend.

## 6-Minute Engineering Walkthrough

Start with the architecture. The `RealTimeController` owns the cyclic execution order: fault injection, command dispatch, process-image exchange, axis update, watchdog evaluation, and telemetry. The axis implements CiA 402-style states from Not Ready through Operation Enabled and Fault. Motion commands are only accepted in valid states, and rejected commands become explicit fault evidence.

The bus layer is deliberately abstract. It uses deterministic simulation, recorded-exchange replay, dependency-backed SOEM v2, and Linux SocketCAN/CANopen. Controller injection tests prove outgoing controlwords/targets and incoming statusword/position/velocity traverse the same cyclic boundary. SocketCAN transport tests cover NMT, split PDOs, heartbeat, stale data, and transport failure without opening a physical interface.

The safety-oriented layer includes loop, bus, heartbeat, following-error, encoder-feedback, and E-stop/safe-stop supervision. It does not claim certified safety. Instead, it shows disciplined fault handling and documents where certified hardware/process would be required.

The validation story is quantitative. The benchmark records cycle histograms, min/max/mean cycle time, jitter distribution, missed cycles, deadline overruns, packet loss, and fault recovery evidence. The tests cover normal startup, enable, moves, quick stop, E-stop, fault reset, invalid transitions, missed cycles, load spikes, heartbeat timeout, bus disconnect, packet loss, encoder loss, following error, sensor noise, warm restart, cold restart, replayable fault logs, adapter mapping validation, stale PDO detection, and invalid adapter responses.

For a repeatable demonstration, run `scripts/demo_portfolio.ps1 -BuildDir build -OutDir reports/generated/demo`.

The native Windows suite passes through a pinned portable CMake/MinGW-w64 workflow, and hosted Linux CI compiles SocketCAN, links the pinned SOEM dependency, executes the FreeRTOS POSIX smoke target, and runs both milestone gates. The next boundary is a selected interface and drive, followed by the documented low-energy commissioning sequence.
