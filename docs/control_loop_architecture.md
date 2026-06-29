# Control Loop Architecture Note

`RealTimeController::execute_cycle()` remains portable on a host PC and is also wrapped by the implemented FreeRTOS high-priority periodic-task port.

## MCU Mapping

| Simulation Element | MCU/FreeRTOS Mapping |
| --- | --- |
| `run_cycles()` | `emrtc_freertos_task` periodic activation using `vTaskDelayUntil` semantics |
| `CycleSample` | Hardware timer capture and period monitor |
| `AxisCommand` | Queue item or lock-free command mailbox |
| `ProcessImage` | DMA-safe or fieldbus process-image buffer |
| `WatchdogSupervisor` | Safety-oriented supervisor task or ISR-fed monitor |

## Linux/PREEMPT_RT Mapping

The host timing probe uses absolute monotonic deadlines and can request `SCHED_FIFO` plus memory locking on Linux. It reports whether those capabilities were actually granted and exports CSV/JSON/Markdown timing evidence. PREEMPT_RT acceptance still requires capture on the selected kernel and hardware; host timing evidence is not target real-time evidence.

## Real-Time Coding Rules Used Here

- No dynamic hardware addresses in core logic.
- Fault events are cycle-indexed, not wall-clock dependent.
- State transitions are explicit and traceable.
- Hardware adapters remain outside the deterministic state machine.
- Persistent reports distinguish software simulation from hardware validation.
