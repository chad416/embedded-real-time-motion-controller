# Adapter Lifecycle Checklist

This checklist defines the minimum lifecycle behavior a real EtherCAT or CANopen backend must implement before it can replace the simulated bus in a bench run.

## Common Lifecycle

| Phase | Required behavior | Evidence |
| --- | --- | --- |
| Discover | Identify interface, stack version, and available devices without enabling motion. | Interface name, stack version, device IDs. |
| Configure | Bind the process-image contract to actual PDO/object entries. | Mapping dump tied to drive manual. |
| Pre-operational exchange | Verify cyclic reads before torque-producing commands. | Idle trace and heartbeat counters. |
| Enable | Drive CiA 402 state transitions through the controller command path. | Statusword/controlword trace. |
| Cyclic exchange | Copy process image once per control cycle without blocking calls. | Timing histogram and missed-cycle count. |
| Fault reaction | Convert stale data, invalid response, timeout, or bus down into watchdog-visible diagnostics. | Fault log and recovery trace. |
| Recovery | Require explicit reset and re-enable after a bus or drive fault. | Recovery-cycle count and final state. |
| Shutdown | Disable motion command path and leave drive in a known non-motion state. | Final statusword and operator log entry. |

## Stack-Specific Gates

| Backend | Discovery gate | Exchange gate | Fault gate |
| --- | --- | --- | --- |
| SOEM EtherCAT | Slave scan returns expected vendor/product IDs. | Process-data exchange completes inside cycle budget. | Cable pull or slave loss records bus dropout. |
| EtherLab EtherCAT | Master service online and domain activated. | Domain process data updates statusword and actual position. | Master/domain error maps to bus watchdog. |
| SocketCAN CANopen | CAN interface is up with expected bitrate. | PDO receive timestamps stay fresh across control cycles. | Heartbeat timeout and bus-off are reported separately. |
| CANopenNode | Object dictionary loads and node enters operational state. | CANopen process tick is isolated from control-loop blocking. | NMT error, heartbeat loss, and stale PDOs enter fault handling. |

## Process-Image Contract

All adapters must use the shared contract in [`../../include/emrtc/process_image_mapping.hpp`](../../include/emrtc/process_image_mapping.hpp):

- `0x6040` controlword, 16 bit, controller to drive.
- `0x6041` statusword, 16 bit, drive to controller.
- `0x607A` target position, 32 bit, controller to drive.
- `0x60FF` target velocity, 32 bit, controller to drive.
- `0x6064` actual position, 32 bit, drive to controller.
- `0x606C` actual velocity, 32 bit, drive to controller.

Adapter-specific scaling may vary by drive, but the conversion must be documented in the commissioning logbook and validated by a low-speed move before nominal motion.

## Claim Boundary

Passing this checklist still does not create a SIL/PL claim. It only moves the project from simulation-only evidence toward a hardware commissioning record.
