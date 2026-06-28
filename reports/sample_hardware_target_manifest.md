# Hardware Target Manifest

Generated from `config/hardware_targets/*.json`. These records are vendor-neutral planning profiles, not purchase recommendations.

| Target | Fieldbus | Hardware required | Capture profiles | Required evidence | Primary risks |
| --- | --- | --- | ---: | --- | --- |
| `generic_canopen_cia402_servo` | CANopen | True | 4 | CAN interface configuration<br>node ID and bitrate record<br>object dictionary excerpt<br>PDO mapping record<br>heartbeat timeout trace<br>nominal move trace<br>commissioning logbook entry | CAN bitrate, termination, and node-ID conflicts can look like software faults.<br>Blocking SDO access must not enter the cyclic control path.<br>Heartbeat timeout evidence is not a certified safety function. |
| `generic_ethercat_cia402_servo` | EtherCAT | True | 4 | slave scan log<br>vendor/product IDs<br>PDO mapping dump<br>idle timing capture<br>nominal move trace<br>fault log from heartbeat or bus dropout test<br>commissioning logbook entry | Wrong PDO mapping can make statusword or position feedback misleading.<br>EtherCAT interface selection and permissions can fail before the controller code is reached.<br>Physical cable-pull tests must be low energy and must not be used as certified safety evidence. |
| `simulated_loopback_target` | simulated | False | 6 | CI run URL<br>metrics.json<br>trace.csv<br>fault_log.json<br>validation_report.md<br>sample graphs | Simulation can validate software behavior but cannot validate electrical wiring, real drive timing, encoder scaling, or physical stop paths. |

## Boundary

These profiles help plan future bench work. They do not claim hardware validation and do not recommend a specific vendor, drive, or purchase.
