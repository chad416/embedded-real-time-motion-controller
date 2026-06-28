# Watchdog Design Note

The watchdog layer is safety-oriented but not certified safety logic.

## Monitors

| Watchdog | Trip Condition | Recovery |
| --- | --- | --- |
| Loop watchdog | Consecutive missed/deadline-overrun cycles exceed limit | Fault reaction, fault reset, re-enable. |
| Bus watchdog | Simulated bus online state false | Fault reaction, bus restoration, fault reset. |
| Heartbeat watchdog | Missing heartbeat exceeds configured cycles | Fault reaction, heartbeat restoration, fault reset. |
| Following-error watchdog | Absolute following error exceeds limit | Fault reaction, investigate command/load/feedback, fault reset. |
| Encoder feedback-loss watchdog | Feedback loss exceeds timeout | Fault reaction, restore feedback, fault reset. |
| E-stop safe stop | Safe stop request asserted | Quick stop behavior plus EmergencyStop record. |

## Recovery Principle

Fault reset does not silently resume motion. The axis returns to Switch On Disabled and must be explicitly re-enabled before accepting motion commands.
