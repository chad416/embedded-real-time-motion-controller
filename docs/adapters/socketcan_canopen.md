# SocketCAN CANopen Adapter

## Purpose

Use Linux SocketCAN as the host-side transport for the implemented, non-commissioned CANopen/CiA 402 backend.

## Build Flag

```bash
cmake -S . -B build -DEMRTC_ENABLE_SOCKETCAN=ON
```

## Runtime Dependency

- SocketCAN-capable Linux system.
- CAN interface such as `can0`.
- Configured bitrate matching the drive/device chain.
- A drive whose PDO mapping matches the validated runtime configuration.

When `EMRTC_ENABLE_SOCKETCAN=ON`, CMake performs a non-fatal search for Linux CAN headers such as `linux/can.h`. If they are found, `EMRTC_HAS_SOCKETCAN=1` is exposed to the C++ build and `emrtc_cli show adapters` reports `dependency_available=true`.

## Mapping Scope

| Process Image Field | CANopen Object |
| --- | --- |
| `controlword` | `0x6040` |
| `statusword` | `0x6041` |
| `target_position` | `0x607A` |
| `target_velocity` | `0x60FF` |
| `actual_position` | `0x6064` |
| `actual_velocity` | `0x606C` |

## Implemented Software Contract

- NMT start and stop commands.
- RPDO1 controlword/target-position and RPDO2 target-velocity encoding.
- TPDO1 statusword/actual-position and TPDO2 actual-velocity decoding.
- Heartbeat supervision, stale PDO handling, transport-loss diagnostics, and CiA 402 scaling.
- Injectable transport tests that do not require a CAN interface.

Validate configuration without opening `can0`:

```bash
emrtc_cli validate adapter-config config/adapters/socketcan_canopen_single_axis.example.json
```

## Commissioning Tests

- CAN interface up/down detection.
- NMT state handling.
- Heartbeat timeout.
- PDO loss or stale data detection.
- Fault reset and re-enable sequence.

## Current Boundary

`SocketCanCanOpenBackend` is implemented but not physically commissioned. Real interface state, drive PDO assignment, bitrate, electrical termination, and fault-recovery evidence remain hardware-only work.
