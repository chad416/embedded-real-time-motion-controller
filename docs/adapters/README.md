# Hardware Adapter Integration Notes

The repository is simulation-first. These notes define how fieldbus adapters are brought up without weakening the deterministic controller core or making unsupported hardware claims.

## Build Flags

| Adapter | CMake Flag | Default | Discovery |
| --- | --- | --- | --- |
| SOEM EtherCAT | `EMRTC_ENABLE_SOEM` | `OFF` | SOEM v2 CMake package or `soem/soem.h` + library discovery |
| EtherLab EtherCAT | `EMRTC_ENABLE_ETHERLAB` | `OFF` | Non-fatal `ecrt.h` + `ethercat` library discovery |
| SocketCAN CANopen | `EMRTC_ENABLE_SOCKETCAN` | `OFF` | Non-fatal Linux CAN header discovery |
| CANopenNode | `EMRTC_ENABLE_CANOPENNODE` | `OFF` | Non-fatal `CANopen.h` + CANopenNode library discovery |

Example:

```bash
cmake -S . -B build -DEMRTC_ENABLE_SOEM=ON -DEMRTC_REQUIRE_SOEM=ON
```

Enabling a flag asks CMake to discover the matching external dependency. If SOEM is required and missing, configuration fails. A discovered SOEM dependency enables its lifecycle and exchange implementation. On Linux, discovered CAN headers enable the raw SocketCAN transport, CANopen NMT/PDO exchange, and heartbeat supervision. EtherLab and CANopenNode remain optional placeholders. Runtime status remains unconfigured until explicit interface and mapping configuration succeeds.

Runtime status can be inspected with:

```bash
emrtc_cli show adapters
```

The adapter-neutral CiA 402 object contract is implemented in [`../../include/emrtc/process_image_mapping.hpp`](../../include/emrtc/process_image_mapping.hpp) and tested by the no-dependency C++ test runner. SOEM and SocketCAN/CANopen use this contract for bounded little-endian PDO encoding and decoding.

`RealTimeController` supports constructor injection of an `IBusBackend`. External feedback therefore reaches the same CiA 402 state, following-error watchdog, telemetry, and fault path used by simulation.

Stale process-data and invalid object-map handling are captured in [Adapter Exchange Contract](exchange_contract.md).

Recorded process-image exchanges can be exercised through the [Adapter Replay Harness](replay_harness.md).

The required runtime bring-up sequence is captured in [Adapter Lifecycle Checklist](lifecycle_checklist.md).

## Commissioning Rule

Do not change report language from software simulation to hardware validation until:

- The adapter links the real fieldbus stack.
- Interface names and device identities are documented.
- PDO/object dictionary mapping is verified against the drive manual.
- Hardware traces, metrics JSON, timing PDF, and fault logs are captured.
- The commissioning logbook and hardware checkout checklist are filled out.
