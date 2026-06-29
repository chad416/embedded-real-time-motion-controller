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
Motion/feedback        Simulated / replay / SOEM
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
| Axis | Implements CiA 402-style drive state handling, motion commands, controlword generation, and simulated or externally reported feedback. |
| Bus | Provides an injectable process-image exchange boundary with simulated, replay, and dependency-backed SOEM implementations. |
| Watchdog | Evaluates loop timing, heartbeat, bus online state, following error, and encoder feedback loss. |
| Telemetry | Produces CSV traces, JSON metrics, fault logs, and Markdown validation reports. |

## Determinism Strategy

The simulation does not use wall-clock sleep for validation. Each cycle advances by a configured period and deterministic jitter pattern. Fault injection events are scheduled by cycle index, making tests repeatable across machines.

## Hardware Boundary

`RealTimeController` accepts any `IBusBackend`. The simulated backend remains the default, while external backends feed statusword, position, velocity, and drive-fault data through the same axis and watchdog path. The SOEM implementation performs guarded single-slave configuration and cyclic process-data exchange, but it remains non-commissioned until a selected NIC and drive produce archived bench evidence.
