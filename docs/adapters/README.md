# Hardware Adapter Integration Notes

The repository is simulation-first. These notes define how future fieldbus adapters should be brought up without weakening the deterministic controller core or making unsupported hardware claims.

## Build Flags

| Adapter | CMake Flag | Default | Current Status |
| --- | --- | --- | --- |
| SOEM EtherCAT | `EMRTC_ENABLE_SOEM` | `OFF` | Skeleton only |
| EtherLab EtherCAT | `EMRTC_ENABLE_ETHERLAB` | `OFF` | Skeleton only |
| SocketCAN CANopen | `EMRTC_ENABLE_SOCKETCAN` | `OFF` | Skeleton only |
| CANopenNode | `EMRTC_ENABLE_CANOPENNODE` | `OFF` | Skeleton only |

Example:

```bash
cmake -S . -B build -DEMRTC_ENABLE_SOEM=ON
```

Enabling a flag currently marks the adapter integration point at compile time. It does not link or commission real hardware libraries yet.

Runtime status can be inspected with:

```bash
emrtc_cli show adapters
```

## Commissioning Rule

Do not change report language from software simulation to hardware validation until:

- The adapter links the real fieldbus stack.
- Interface names and device identities are documented.
- PDO/object dictionary mapping is verified against the drive manual.
- Hardware traces, metrics JSON, timing PDF, and fault logs are captured.
- The commissioning logbook and hardware checkout checklist are filled out.
