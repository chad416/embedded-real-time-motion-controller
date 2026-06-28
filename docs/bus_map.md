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

## CiA 402 Object Contract

The code-level contract lives in [`../include/emrtc/process_image_mapping.hpp`](../include/emrtc/process_image_mapping.hpp). It keeps the simulation process image and future hardware adapters aligned on object indexes, direction, and scaling.

| Field | Object | Sub-index | Width | Direction |
| --- | --- | ---: | ---: | --- |
| `controlword` | `0x6040` | `0x00` | 16 bit | Controller to drive |
| `statusword` | `0x6041` | `0x00` | 16 bit | Drive to controller |
| `target_position` | `0x607A` | `0x00` | 32 bit | Controller to drive |
| `target_velocity` | `0x60FF` | `0x00` | 32 bit | Controller to drive |
| `actual_position` | `0x6064` | `0x00` | 32 bit | Drive to controller |
| `actual_velocity` | `0x606C` | `0x00` | 32 bit | Drive to controller |

The helper functions encode engineering units to signed 32-bit counts using a configurable counts-per-unit scale. They are adapter-neutral and are covered by the no-dependency C++ test runner.

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

These classes intentionally report `configured=false` until real hardware libraries, interface names, live PDO/object mapping, and bench validation are added.

See [Hardware Adapter Integration Notes](adapters/README.md) for build flags and adapter-specific bring-up plans.
