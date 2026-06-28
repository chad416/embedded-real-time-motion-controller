# Hardware Checkout Checklist

This checklist defines what must be verified before claiming hardware validation. Until every relevant item is complete, reports must continue to say software simulation only.

## Electrical And Safety-Oriented Checks

| Item | Method | Acceptance Criteria | Status |
| --- | --- | --- | --- |
| Power supply rating | Compare supply to drive/controller load | Rated current and voltage margin documented | Planned |
| Fusing/protection | Inspect wiring and datasheets | Protection appropriate for bench current | Planned |
| E-stop chain | Continuity and functional test | Physical E-stop request reaches safety path | Planned |
| Drive inhibit/STO | Drive manual and bench test | Drive cannot produce torque when inhibited | Planned |
| Limit switch wiring | Manual trigger and telemetry check | State transitions visible and debounced | Planned |
| Encoder wiring | Direction and scale check | Positive command produces expected feedback sign | Planned |
| Grounding/shielding | Visual inspection and continuity check | Cable shields and PE strategy documented | Planned |

## Fieldbus Checks

| Item | Method | Acceptance Criteria | Status |
| --- | --- | --- | --- |
| EtherCAT interface selected | SOEM/EtherLab adapter config | Interface name documented, link detected | Planned |
| CAN interface selected | SocketCAN/CANopen config | Interface name and bitrate documented | Planned |
| PDO/object map | Compare against drive manual | Controlword/statusword/position/velocity mapped | Planned |
| Heartbeat supervision | Timeout test | Timeout produces recorded fault | Planned |
| Cable disconnect | Physical cable pull | Bus watchdog and recovery evidence captured | Planned |
| Invalid response handling | Fault injection or adapter test | Invalid mapping/response is rejected and logged | Planned |

## Motion Checks

| Item | Method | Acceptance Criteria | Status |
| --- | --- | --- | --- |
| Enable sequence | CiA 402 transition trace | Operation Enabled reached deterministically | Planned |
| Low-speed jog | Velocity command | Direction, scale, and stop behavior verified | Planned |
| Absolute move | Profile position command | Overshoot, settling, final error logged | Planned |
| Relative move | Profile position command | Relative displacement within tolerance | Planned |
| Quick stop | Command during motion | Stop time and state transition logged | Planned |
| E-stop during motion | Physical input | Safety-oriented stop evidence captured | Planned |

## Evidence Required Before Hardware Claim

- Capture profile manifest identifying the exact run definitions used.
- Raw trace CSV from hardware run.
- JSON metrics from hardware run.
- Timing PDF from hardware run.
- Fault log from hardware run.
- Photos or screenshots of bench wiring and fieldbus topology.
- Commit hash of software used.
- Drive parameter backup or exported configuration.

No real hardware validation claim should be made until the evidence above exists.
