# Preliminary BOM And Hardware Boundary

This BOM is a planning document, not a purchase order. No hardware validation is claimed yet.

| Category | Candidate Item | Purpose | Status |
| --- | --- | --- | --- |
| MCU/control board | STM32 Nucleo, Teensy 4.1, or similar real-time capable board | Future MCU-side deterministic control tasking | Planned |
| Supervisory computer | Linux PC or industrial PC with PREEMPT_RT option | Host supervisor, diagnostics, EtherCAT/CANopen master experiments | Planned |
| EtherCAT master stack | SOEM v2 or EtherLab | SOEM dependency-backed backend exists; physical master commissioning is pending | Software integrated |
| CAN interface | USB-CAN or PCIe CAN with SocketCAN support | Physically commission the implemented SocketCAN/CANopen path | Planned hardware |
| Servo/intelligent actuator | Small industrial servo drive or intelligent actuator | Real CiA 402 motion target | Planned |
| Encoder feedback | Drive encoder or external incremental encoder | Feedback-loss and following-error validation | Planned |
| Safety input | E-stop button plus certified safety relay/drive STO path | Physical safe-stop chain | Planned |
| I/O | 24 V sensors, limit switches, terminal blocks | Fault and permissive inputs | Planned |
| Power | 24 V DC supply and drive supply as required | Bench power | Planned |

## Hardware Boundary

The current repository proves deterministic software behavior, implemented host-side SOEM/SocketCAN exchange paths, execution-port integration, and report generation. Hardware-only work starts when the physical drive, bus, encoder, and safety chain are available. At that point, simulated/replay backends remain in CI while the same acceptance profiles run against bench-specific configuration.
