# SocketCAN CANopen Adapter Plan

## Purpose

Use Linux SocketCAN as the host-side CAN transport for a future CANopen/CiA 402 backend.

## Build Flag

```bash
cmake -S . -B build -DEMRTC_ENABLE_SOCKETCAN=ON
```

## Planned External Dependency

- SocketCAN-capable Linux system.
- CAN interface such as `can0`.
- Configured bitrate matching the drive/device chain.
- CANopen SDO/PDO layer implemented above SocketCAN.

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

## Required Tests

- CAN interface up/down detection.
- NMT state handling.
- Heartbeat timeout.
- PDO loss or stale data detection.
- Fault reset and re-enable sequence.

## Current Boundary

`SocketCanCanOpenBackend` is a non-commissioned integration point. It intentionally reports `configured=false`.
