# Hardware Commissioning Remaining After `v0.3.0`

This is the handoff from the software-ready phase to the physical bench phase. The strict JSON/configuration, SOEM, SocketCAN/CANopen, host timing, FreeRTOS, recovery metrics, reports, documentation, packaging, and release gates are complete in software.

## Closed Non-Hardware Scope

- Strict structured JSON parsing for scenarios and runtime adapter configuration.
- Hardware-free validation of versioned SOEM and SocketCAN/CANopen configuration examples.
- SocketCAN CANopen backend with NMT, split RPDO/TPDO exchange, CiA 402 scaling, heartbeat supervision, transport diagnostics, and injectable tests.
- Wall-clock host timing probe with absolute deadlines, CPU stress, Linux FIFO/memory-lock capability reporting, and CSV/JSON/Markdown output.
- FreeRTOS periodic-task port built against pinned V11.3.0 commit `9b777ae5c5b8e9e456065a00294d1e5f5f9facf5` and exercised with the local Windows simulator port.
- Quantitative fault detection, re-enable, and post-recovery motion metrics plus a persistent axis-fault logging fix.
- Machine-enforced `v0.3.0-software-ready` definition, claim audit, local validation path, hosted CI path, evidence bundle, and release record.

## Hardware-Only Work

- Select and connect the NIC, drive, motor, encoder, power, grounding, and certified stop chain.
- Replace example interface names, PDO offsets, COB-IDs, node IDs, scaling, limits, and timeouts with device-verified values.
- Commission EtherCAT SAFE-OP/OP and CANopen NMT/PDO behavior.
- Capture physical MCU/PREEMPT_RT timing, motion, encoder, cable-loss, heartbeat, recovery, quick-stop, and stop-path evidence.
- Archive raw traces, parameters, photographs, wiring/topology records, and the exact software commit.
- Make no SIL/PL or hardware-validation claim until the real certified chain and evidence support it.

## Release Boundary

Only physical hardware commissioning remains. Host timing evidence is not target real-time evidence, software adapter tests are not physical bus evidence, and simulated/replayed fault recovery is not a certified stop-path result. Hardware claims stay blocked until a complete evidence archive passes the repository validator.
