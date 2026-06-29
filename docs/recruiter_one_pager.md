# Embedded Real-Time Motion Controller - Portfolio One-Pager

## Role Fit

Industrial automation, robotics, embedded real-time software, fieldbus integration, controls validation, and commissioning-oriented engineering roles.

## Project In One Sentence

A C++17 software-ready motion-controller bench with deterministic simulation, CiA 402 behavior, implemented SOEM and SocketCAN/CANopen paths, FreeRTOS/host execution probes, fault recovery, visual evidence, and explicit hardware commissioning boundaries.

## Evidence Snapshot

| Evidence | Value |
| --- | ---: |
| Automated scenarios passing | 21/21 |
| Benchmark cycles | 2500 |
| Max jitter (ns) | 2000000 |
| Missed cycles | 2 |
| Deadline overruns | 2 |
| Packet-loss evidence | 6 |
| Motion overshoot | 0.0 |
| Motion settling time (s) | 0.8 |
| Two-axis acceptance | True |
| Two-axis max path error | 0.0 |
| Two-axis path limit | 0.01 |
| Capture profiles | 6 |
| Fault detection latency (cycles) | 0 |
| Recovery cycles | 7 |

## Engineering Highlights

- Deterministic 1 ms cyclic controller model with jitter, missed-cycle, and deadline instrumentation.
- CiA 402-style axis state machine with command guards, statusword/controlword evidence, and fault reset behavior.
- Simulated fieldbus backend covering packet loss, cable dropout, heartbeat timeout, and invalid responses.
- Dependency-backed SOEM plus Linux SocketCAN/CANopen NMT, split PDO, heartbeat, and transport diagnostics.
- Pinned FreeRTOS cyclic-task smoke path and absolute-deadline host timing probe.
- Adapter contracts for CiA 402 object mapping, stale receive data, heartbeat health, bus-offline/restored-exchange behavior, and invalid responses.
- Quantitative evidence: JSON metrics, CSV traces, fault logs, Markdown report, timing PDF, and SVG plots.
- Repeatable portfolio demo script: `scripts/demo_portfolio.ps1`.

## Best Interview Talking Points

- How a timing anomaly becomes telemetry first, then watchdog evidence.
- Why simulated bus faults are separated from future hardware claims.
- How adapter contracts and injected transport tests reduce risk before SOEM or SocketCAN commissioning.
- How capture profiles define pass/fail evidence for future bench runs.
- How the same validation suite runs through the pinned Windows toolchain and hosted Linux CI.

## Honest Boundary

This is a safety-oriented software simulation and host-validation project. It does not claim certified SIL/PL safety, real EtherCAT/CANopen hardware validation, real encoder measurements, or commissioned drive behavior.

## Next Hardware Step

Select one EtherCAT or CANopen drive family and physical interface, replace example mappings with vendor-verified values, perform low-energy commissioning, and archive the predefined profile evidence from `config/capture_profiles/`.
