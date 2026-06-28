# Architecture Overview

The controller is split into deterministic control logic, hardware-facing adapters, supervision, diagnostics, and applications.

```text
Diagnostic CLI / Simulator / Benchmark
              |
      RealTimeController
              |
  +-----------+------------+
  |                        |
Axis state machine     Bus abstraction
  |                        |
Motion model           Simulated backend
  |
Watchdogs and fault injector
  |
Telemetry and reports
```

## Layers

| Layer | Responsibility |
| --- | --- |
| Apps | User-facing simulator, benchmark, and diagnostic commands. |
| Controller core | Owns cyclic execution order, timing sample generation, command dispatch, watchdog calls, telemetry, and recovery transitions. |
| Axis | Implements CiA 402-style drive state handling, motion commands, statusword/controlword, and simulated feedback. |
| Bus | Provides a process-image exchange boundary suitable for future SOEM/EtherLab and SocketCAN/CANopenNode adapters. |
| Watchdog | Evaluates loop timing, heartbeat, bus online state, following error, and encoder feedback loss. |
| Telemetry | Produces CSV traces, JSON metrics, fault logs, and Markdown validation reports. |

## Determinism Strategy

The simulation does not use wall-clock sleep for validation. Each cycle advances by a configured period and deterministic jitter pattern. Fault injection events are scheduled by cycle index, making tests repeatable across machines.

## Hardware Boundary

The simulated bus backend is the only active backend in this software-complete phase. Real EtherCAT/CANopen adapters should implement the same process-image contract without changing the axis, watchdog, or scenario logic.
