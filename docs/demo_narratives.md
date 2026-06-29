# Demo Narratives

## 90-Second Demo

This project is a software-first embedded real-time motion controller. I built the controller core in C++ with a 1 ms deterministic cycle model, a CiA 402-style axis state machine, simulated EtherCAT/CANopen process image, watchdogs, and telemetry. The simulator can enable a drive, run position and velocity moves, inject bus and feedback faults, and export CSV/JSON/Markdown reports. The important point is that the project does not just say "real-time"; it records cycle timing, jitter, missed cycles, packet loss, watchdog trips, recovery behavior, and pass/fail scenario evidence. Hardware integration is cleanly isolated behind the bus backend, so the remaining work is commissioning actual EtherCAT or CANopen devices, not rewriting the controller.

## 6-Minute Engineering Walkthrough

Start with the architecture. The `RealTimeController` owns the cyclic execution order: fault injection, command dispatch, process-image exchange, axis update, watchdog evaluation, and telemetry. The axis implements CiA 402-style states from Not Ready through Operation Enabled and Fault. Motion commands are only accepted in valid states, and rejected commands become explicit fault evidence.

The bus layer is deliberately abstract. It uses a deterministic simulated backend for controller scenarios, a replay backend for recorded exchanges, and a dependency-backed SOEM v2 implementation for future physical EtherCAT work. Controller injection tests prove outgoing controlwords and targets plus incoming statusword, position, and velocity traverse the same cyclic boundary.

The safety-oriented layer includes loop, bus, heartbeat, following-error, encoder-feedback, and E-stop/safe-stop supervision. It does not claim certified safety. Instead, it shows disciplined fault handling and documents where certified hardware/process would be required.

The validation story is quantitative. The benchmark records cycle histograms, min/max/mean cycle time, jitter distribution, missed cycles, deadline overruns, packet loss, and fault recovery evidence. The tests cover normal startup, enable, moves, quick stop, E-stop, fault reset, invalid transitions, missed cycles, load spikes, heartbeat timeout, bus disconnect, packet loss, encoder loss, following error, sensor noise, warm restart, cold restart, replayable fault logs, adapter mapping validation, stale PDO detection, and invalid adapter responses.

For a repeatable demonstration, run `scripts/demo_portfolio.ps1 -BuildDir build -OutDir reports/generated/demo`.

The native Windows suite passes through a pinned portable CMake/MinGW-w64 workflow, and the SOEM path compiles/links against the pinned dependency. The next major boundary is a selected NIC and drive, followed by the documented low-energy commissioning sequence.
