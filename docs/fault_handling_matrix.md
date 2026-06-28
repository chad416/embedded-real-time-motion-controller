# Fault Handling Matrix

| Fault | Injection Support | Expected Detection | Expected State/Action |
| --- | --- | --- | --- |
| Missed cycle | Yes | Cycle sample missed-cycle counter | Telemetry evidence, watchdog if repeated. |
| CPU load spike | Yes | Deadline overrun counter | Telemetry evidence, watchdog if repeated. |
| Heartbeat timeout | Yes | Heartbeat watchdog | Fault Reaction Active -> Fault. |
| EtherCAT/CANopen dropout | Yes | Bus online watchdog | Fault Reaction Active -> Fault. |
| Packet loss | Yes | Bus diagnostics counter | Counter evidence; fault only if supervision threshold is crossed. |
| Invalid drive transition | Yes | Axis transition guard | Fault Reaction Active -> Fault. |
| Encoder feedback loss | Yes | Encoder feedback-loss watchdog | Fault Reaction Active -> Fault. |
| Sensor noise | Yes | Feedback trace and following-error monitor | No trip while below threshold. |
| Following error | Yes | Following-error watchdog | Fault Reaction Active -> Fault. |
| Drive fault | Yes | Drive fault record | Fault Reaction Active -> Fault. |
| Watchdog timeout | Yes | Loop watchdog path | Fault Reaction Active -> Fault. |
| Command abuse | Yes | Command acceptance guard | CommandRejected fault. |

## Notes

Packet loss is not automatically a drive fault because industrial buses can tolerate isolated packet events depending on the device profile and supervision settings. Persistent loss should be escalated through heartbeat/bus watchdogs.
