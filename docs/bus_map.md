# Bus Map

The project exposes a fieldbus-neutral process image that can be adapted to EtherCAT PDOs or CANopen objects later.

## Process Image

| Field | Direction | Future EtherCAT/CANopen Mapping |
| --- | --- | --- |
| `controlword` | Controller to drive | CiA 402 controlword, object 0x6040 |
| `statusword` | Drive to controller | CiA 402 statusword, object 0x6041 |
| `target_position` | Controller to drive | Target position, object 0x607A |
| `target_velocity` | Controller to drive | Target velocity, object 0x60FF |
| `actual_position` | Drive to controller | Position actual value, object 0x6064 |
| `actual_velocity` | Drive to controller | Velocity actual value, object 0x606C |
| `drive_fault` | Drive to controller | Emergency/fault register mapped by adapter |

## Adapter Roadmap

| Backend | Status | Notes |
| --- | --- | --- |
| Simulated bus | Implemented | Deterministic packet loss, dropout, heartbeat timeout, invalid response. |
| EtherCAT/SOEM | Planned | Implement `IBusBackend` using SOEM process-data exchange. |
| EtherCAT/EtherLab | Planned | Same process-image contract, different master lifecycle. |
| CANopen/SocketCAN | Planned | Map process image to SDO/PDO and heartbeat supervision. |
| CANopenNode | Planned | Useful for embedded target-side CANopen integration. |

## Adapter Skeletons

The repository includes non-commissioned adapter skeletons for:

- SOEM EtherCAT.
- EtherLab EtherCAT.
- SocketCAN CANopen.
- CANopenNode.

These classes intentionally report `configured=false` until real hardware libraries, interface names, PDO/object mapping, and bench validation are added.
