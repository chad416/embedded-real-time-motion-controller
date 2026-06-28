# Timing Budget

Default cycle time is 1 ms. The model records measured period, jitter, missed cycles, and deadline overruns on every cycle.

| Budget Item | Target |
| --- | ---: |
| Nominal cycle period | 1,000,000 ns |
| Deadline tolerance | 100,000 ns |
| Missed cycle threshold | >= 2,000,000 ns |
| Loop watchdog limit | 3 consecutive missed/overrun cycles |
| Heartbeat timeout | 5 missing heartbeat cycles |
| Encoder feedback-loss timeout | 4 ms |

## Cycle Work Order

1. Apply scheduled fault injections.
2. Dispatch pending command.
3. Apply E-stop/safe stop request if present.
4. Sample deterministic timing/jitter.
5. Exchange process image on bus.
6. Update axis state and motion model.
7. Evaluate watchdogs.
8. Record trace, counters, and fault log.

## Jitter Evidence

The deterministic benchmark adds bounded synthetic jitter plus scheduled stress events. Reports include min/max/mean cycle time, min/max/mean absolute jitter, missed cycles, and deadline overrun count.
