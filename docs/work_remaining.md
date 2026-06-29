# Work Remaining From The 2026-06-29 Checkpoint

This is the handoff record for work started after commit `2f5f388`. It separates unfinished software work from tasks that require physical equipment.

## Implemented In This Checkpoint

- Strict structured JSON parsing for scenarios and runtime adapter configuration.
- Hardware-free validation of versioned SOEM and SocketCAN/CANopen configuration examples.
- SocketCAN CANopen backend with NMT, split RPDO/TPDO exchange, CiA 402 scaling, heartbeat supervision, transport diagnostics, and injectable tests.
- Wall-clock host timing probe with absolute deadlines, CPU stress, Linux FIFO/memory-lock capability reporting, and CSV/JSON/Markdown output.
- FreeRTOS periodic-task port built against pinned V11.3.0 commit `9b777ae5c5b8e9e456065a00294d1e5f5f9facf5` and exercised with the local Windows simulator port.
- Quantitative fault detection, re-enable, and post-recovery motion metrics plus a persistent axis-fault logging fix.

## Remaining Non-Hardware Work

1. Make the new Linux SocketCAN build, FreeRTOS POSIX smoke run, reports, and existing jobs pass on the exact hosted GitHub CI run.
2. Add and enforce a software-ready milestone/release gate beyond `v0.2.0-simulation-complete`.
3. Update every adapter, architecture, test, benchmark, demo, status, and recruiter document that still describes SocketCAN or FreeRTOS as planning-only.
4. Gate the new recovery graph, adapter configs, FreeRTOS port, and host timing assets in repository health checks and portfolio packaging.
5. Run final 10,000-cycle Windows validation, SOEM compile, FreeRTOS smoke, adapter dry-runs, evidence tests, report generation, and claim audits.
6. Commit and push any CI corrections, verify the exact final commit, then create the software-ready release record/tag.

## Hardware-Only Work Afterward

- Select and connect the NIC, drive, motor, encoder, power, grounding, and certified stop chain.
- Replace example interface names, PDO offsets, COB-IDs, node IDs, scaling, limits, and timeouts with device-verified values.
- Commission EtherCAT SAFE-OP/OP and CANopen NMT/PDO behavior.
- Capture physical MCU/PREEMPT_RT timing, motion, encoder, cable-loss, heartbeat, recovery, quick-stop, and stop-path evidence.
- Archive raw traces, parameters, photographs, wiring/topology records, and the exact software commit.
- Make no SIL/PL or hardware-validation claim until the real certified chain and evidence support it.

## Checkpoint Status

The new code passes the local default Windows C++ tests and local FreeRTOS simulator smoke test. It is not the final software-ready milestone until the non-hardware list above is closed and the exact hosted commit passes.
