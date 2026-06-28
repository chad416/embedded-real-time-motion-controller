# Control Loop Architecture Note

`RealTimeController::execute_cycle()` is designed like a FreeRTOS-style high-priority cyclic task while remaining portable on a host PC.

## MCU Mapping

| Simulation Element | MCU/FreeRTOS Mapping |
| --- | --- |
| `run_cycles()` | Periodic task loop or timer-triggered task activation |
| `CycleSample` | Hardware timer capture and period monitor |
| `AxisCommand` | Queue item or lock-free command mailbox |
| `ProcessImage` | DMA-safe or fieldbus process-image buffer |
| `WatchdogSupervisor` | Safety-oriented supervisor task or ISR-fed monitor |

## Linux/PREEMPT_RT Mapping

On Linux RT, the controller loop can run as a `SCHED_FIFO` cyclic thread with `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...)`. The current deterministic timing model can be replaced with real timestamp captures without changing the axis or bus contracts.

## Real-Time Coding Rules Used Here

- No dynamic hardware addresses in core logic.
- Fault events are cycle-indexed, not wall-clock dependent.
- State transitions are explicit and traceable.
- Hardware adapters remain outside the deterministic state machine.
- Persistent reports distinguish software simulation from hardware validation.
