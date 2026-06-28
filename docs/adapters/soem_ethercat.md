# SOEM EtherCAT Adapter Plan

## Purpose

Use SOEM as a lightweight EtherCAT master backend for the existing `IBusBackend` process-image contract.

## Build Flag

```bash
cmake -S . -B build -DEMRTC_ENABLE_SOEM=ON
```

## Planned External Dependency

- SOEM headers and library.
- Raw Ethernet interface access on the host.
- Platform-specific permissions for opening the EtherCAT NIC.

When `EMRTC_ENABLE_SOEM=ON`, CMake performs a non-fatal search for `ethercat.h` and the `soem` library. If both are found, `EMRTC_HAS_SOEM=1` is exposed to the C++ build and `emrtc_cli show adapters` reports `dependency_available=true`.

## Mapping Scope

| Process Image Field | EtherCAT Mapping Target |
| --- | --- |
| `controlword` | CiA 402 controlword PDO, object `0x6040` |
| `statusword` | CiA 402 statusword PDO, object `0x6041` |
| `target_position` | Target position, object `0x607A` |
| `target_velocity` | Target velocity, object `0x60FF` |
| `actual_position` | Position actual value, object `0x6064` |
| `actual_velocity` | Velocity actual value, object `0x606C` |

## Bring-Up Evidence Required

- SOEM slave scan log.
- PDO mapping dump.
- Distributed-clock status if available.
- Cycle timing histogram from real exchange loop.
- Cable-pull and heartbeat/fault-reaction trace.
- Filled commissioning logbook section.

## Current Boundary

`SoemEtherCatBackend` is a non-commissioned integration point. It intentionally reports `configured=false`.
