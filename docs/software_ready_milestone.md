# Software-Ready Milestone

Release version: `0.3.0`

Release tag: `v0.3.0-software-ready`

Release record:

- Tagged commit: `6862f016a1a176b252ce98c9b90ee5ca86a1be83`
- Verified GitHub Actions run: `28344205573`
- Annotated tag pushed: 2026-06-29

## Scope

This milestone closes all executable software, simulation, validation, reporting, documentation, and repository-administration work defined for the hardware-free phase. Only physical hardware commissioning remains.

The release proves:

- the deterministic controller, CiA 402-style axis, watchdog, telemetry, fault, replay, scenario, and coordinated-motion paths;
- strict JSON scenario and adapter configuration parsing;
- dependency-backed SOEM lifecycle/PDO software and Linux SocketCAN CANopen NMT/PDO/heartbeat software;
- injectable adapter tests that exercise transport failure without opening hardware;
- a pinned FreeRTOS periodic-task port and simulator smoke execution;
- a wall-clock host timing probe with absolute deadlines and Linux scheduling-capability reporting;
- quantitative motion, fault-detection, re-enable, post-recovery settling, and final-error evidence;
- deterministic report generation, evidence packaging, archive validation, and CI-enforced release gates.

## Acceptance Gate

Run:

```powershell
python scripts/check_software_ready_milestone.py
```

The gate requires version and tag consistency, at least 45 registered C++ checks, 21 passing scenarios, bounded recovery metrics, strict adapter examples, current evidence manifests, all required CI paths, and an audited hardware-claim boundary.

The historical `v0.2.0-simulation-complete` gate remains enforced as a compatibility baseline.

The tagged commit passed default and adapter-enabled builds, dependency-backed SOEM tests, Linux SocketCAN compilation/tests, FreeRTOS POSIX smoke execution, simulator/JSON/two-axis runs, deterministic benchmark, host timing probe, report and manifest generation, evidence packaging, archive tests, and both milestone gates.

## Evidence Boundary

Host timing evidence is not target real-time evidence. The host probe verifies scheduling mechanics, timing export, and capability reporting; MCU or PREEMPT_RT acceptance requires measurements from the selected physical target.

SOEM and SocketCAN/CANopen are implemented software backends, but this milestone does not claim a commissioned NIC, CAN interface, drive, motor, encoder, wiring system, stop chain, or certified safety function.

## Remaining Boundary

Only physical hardware commissioning remains: device selection and connection, vendor-verified mapping and scaling, bus state bring-up, target timing capture, physical motion/fault/stop testing, and immutable evidence archival.
