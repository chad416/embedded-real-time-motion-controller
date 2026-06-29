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
Motion/feedback        Simulated / replay / SOEM / SocketCAN
  |
Watchdogs and fault injector
  |
Telemetry and reports
```

## Layers

| Layer | Responsibility |
| --- | --- |
| Apps | User-facing simulator, benchmark, wall-clock timing probe, and diagnostic commands. |
| Controller core | Owns cyclic execution order, timing sample generation, command dispatch, watchdog calls, telemetry, and recovery transitions. |
| Axis | Implements CiA 402-style drive state handling, motion commands, controlword generation, and simulated or externally reported feedback. |
| Bus | Provides an injectable process-image exchange boundary with simulated, replay, dependency-backed SOEM, and Linux SocketCAN/CANopen implementations. |
| Watchdog | Evaluates loop timing, heartbeat, bus online state, following error, and encoder feedback loss. |
| Telemetry | Produces CSV traces, JSON metrics, fault logs, and Markdown validation reports. |

## Determinism Strategy

The deterministic simulation does not use wall-clock sleep for functional validation. Each cycle advances by a configured period and deterministic jitter pattern, while the separate host probe uses absolute monotonic deadlines to capture real scheduler behavior. Fault injection events are scheduled by cycle index, making controller tests repeatable across machines.

## Hardware Boundary

`RealTimeController` accepts any `IBusBackend`. The simulated backend remains the default, while external backends feed statusword, position, velocity, and drive-fault data through the same axis and watchdog path. SOEM performs guarded single-slave lifecycle/PDO exchange, and SocketCAN performs CANopen NMT/PDO/heartbeat exchange. Both remain non-commissioned until selected interfaces and drives produce archived bench evidence.
