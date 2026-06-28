# Hardware Adapter Integration Notes

The repository is simulation-first. These notes define how future fieldbus adapters should be brought up without weakening the deterministic controller core or making unsupported hardware claims.

## Build Flags

| Adapter | CMake Flag | Default | Discovery |
| --- | --- | --- | --- |
| SOEM EtherCAT | `EMRTC_ENABLE_SOEM` | `OFF` | Non-fatal `ethercat.h` + `soem` library discovery |
| EtherLab EtherCAT | `EMRTC_ENABLE_ETHERLAB` | `OFF` | Non-fatal `ecrt.h` + `ethercat` library discovery |
| SocketCAN CANopen | `EMRTC_ENABLE_SOCKETCAN` | `OFF` | Non-fatal Linux CAN header discovery |
| CANopenNode | `EMRTC_ENABLE_CANOPENNODE` | `OFF` | Non-fatal `CANopen.h` + CANopenNode library discovery |

Example:

```bash
cmake -S . -B build -DEMRTC_ENABLE_SOEM=ON
```

Enabling a flag asks CMake to discover the matching external dependency. If the dependency is missing, the build stays in skeleton mode and reports `dependency_available=false`. If the dependency is found, the project records `dependency_available=true`, but the adapter still reports `configured=false` until process-image mapping, bench configuration, and commissioning evidence are complete.

Runtime status can be inspected with:

```bash
emrtc_cli show adapters
```

The adapter-neutral CiA 402 object contract is implemented in [`../../include/emrtc/process_image_mapping.hpp`](../../include/emrtc/process_image_mapping.hpp) and tested by the no-dependency C++ test runner. Hardware adapters should use that contract before adding stack-specific PDO/SDO exchange code.

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
