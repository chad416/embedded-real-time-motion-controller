# SOEM EtherCAT Backend

## Purpose

`SoemEtherCatBackend` is a dependency-backed SOEM v2 implementation of the existing `IBusBackend` process-image contract. It is designed for one CiA 402 slave and remains non-commissioned until physical bench evidence is archived.

## Build Flag

```bash
cmake -S . -B build-soem \
  -DEMRTC_ENABLE_SOEM=ON \
  -DEMRTC_REQUIRE_SOEM=ON \
  -DCMAKE_PREFIX_PATH=/path/to/soem/install
```

CI checks out SOEM commit `304d1c05eab77dc0d426f1a5cf09c8cc7dc03713`, builds it, installs its CMake package, then compiles and runs the hardware-free guard and PDO codec tests against that dependency.

On Windows, SOEM also requires a packet-capture development/runtime installation. Set `EMRTC_SOEM_PLATFORM_INCLUDE_DIR` to the directory containing `pcap.h` when using the vendored SOEM Windows path.

## Runtime Configuration

`SoemAdapterConfig` requires:

- a non-empty raw Ethernet interface name;
- exactly one expected slave for the current single-axis backend;
- the expected six-object CiA 402 mapping;
- positive engineering-unit scaling;
- bounded output/input PDO byte offsets;
- at least 4096 bytes of I/O map storage;
- a positive cyclic receive timeout, defaulting to 500 microseconds.

Configuration opens the interface, discovers the expected slave, maps group process data, configures distributed clocks when supported, verifies the working-counter expectation, and requests SAFE-OP then OPERATIONAL. Any failed gate leaves the backend offline and unconfigured.

## Mapping Scope

| Process Image Field | EtherCAT Mapping Target |
| --- | --- |
| `controlword` | CiA 402 controlword PDO, object `0x6040` |
| `statusword` | CiA 402 statusword PDO, object `0x6041` |
| `target_position` | Target position, object `0x607A` |
| `target_velocity` | Target velocity, object `0x60FF` |
| `actual_position` | Position actual value, object `0x6064` |
| `actual_velocity` | Velocity actual value, object `0x606C` |

The current default byte layout is:

| Direction | Offset | Field |
| --- | ---: | --- |
| Output | 0 | 16-bit controlword |
| Output | 2 | 32-bit target position |
| Output | 6 | 32-bit target velocity |
| Input | 0 | 16-bit statusword |
| Input | 2 | 32-bit actual position |
| Input | 6 | 32-bit actual velocity |

All values are encoded explicitly as little-endian bytes. Actual drive PDO offsets must be verified against its manual and captured mapping before use.

## Cyclic Behavior

Each exchange:

1. Encodes one controller process image into the mapped output PDO.
2. Sends and receives SOEM process data with the configured timeout.
3. Compares the returned working counter with the mapped expectation.
4. Marks the bus offline on a short working counter.
5. Decodes statusword and actual position/velocity into the controller process image.
6. Aligns the command model to the first measured position, then lets the controller apply external CiA 402 feedback and watchdog checks.

Relative moves are based on the latest measured position. Locally detected bus, timing, and following-error faults remain latched even if fresh drive statuswords resume. The axis emits the standard shutdown, switch-on, enable-operation, quick-stop, and one-cycle fault-reset controlwords. Automatic EtherCAT slave recovery/reconfiguration is intentionally deferred until the first real target supplies failure modes and acceptance evidence.

## Bring-Up Evidence Required

- SOEM slave scan log.
- PDO mapping dump.
- Selected interface and packet-capture/runtime version.
- Configured receive timeout and expected working counter.
- Distributed-clock status if available.
- Cycle timing histogram from real exchange loop.
- Cable-pull and heartbeat/fault-reaction trace.
- Filled commissioning logbook section.

## Evidence Boundary

The project proves dependency-backed compilation/linking, pre-NIC configuration guards, byte-bounded PDO conversion, controller/backend injection, external statusword transitions, measured feedback propagation, and fault-reset output. It does not yet prove raw-interface access, slave discovery, actual PDO correctness, working-counter behavior, timing, or motion on physical hardware.
