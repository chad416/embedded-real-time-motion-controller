# Embedded Real-Time Motion Controller - Portfolio One-Pager

## Role Fit

Industrial automation, robotics, embedded real-time software, fieldbus integration, controls validation, and commissioning-oriented engineering roles.

## Project In One Sentence

A C++17 software-first motion-controller validation bench with deterministic cyclic execution, CiA 402-style drive behavior, simulated EtherCAT/CANopen process image, watchdogs, fault injection, telemetry, visual reports, and explicit hardware commissioning boundaries.

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

## Engineering Highlights

- Deterministic 1 ms cyclic controller model with jitter, missed-cycle, and deadline instrumentation.
- CiA 402-style axis state machine with command guards, statusword/controlword evidence, and fault reset behavior.
- Simulated fieldbus backend covering packet loss, cable dropout, heartbeat timeout, and invalid responses.
- Adapter contracts for CiA 402 object mapping, stale receive data, heartbeat health, bus-offline state, and invalid responses.
- Quantitative evidence: JSON metrics, CSV traces, fault logs, Markdown report, timing PDF, and SVG plots.
- Repeatable portfolio demo script: `scripts/demo_portfolio.ps1`.

## Best Interview Talking Points

- How a timing anomaly becomes telemetry first, then watchdog evidence.
- Why simulated bus faults are separated from future hardware claims.
- How adapter contracts reduce risk before SOEM/EtherLab/SocketCAN/CANopenNode commissioning.
- How capture profiles define pass/fail evidence for future bench runs.
- How the same validation suite runs in hosted CI even before local Windows CMake/compiler setup is finished.

## Honest Boundary

This is a safety-oriented software simulation and host-validation project. It does not claim certified SIL/PL safety, real EtherCAT/CANopen hardware validation, real encoder measurements, or commissioned drive behavior.

## Next Hardware Step

Install local CMake plus a C++17 compiler, pick one EtherCAT or CANopen drive family, bind the process-image contract to real PDO/object dictionary entries, then capture the predefined profile evidence from `config/capture_profiles/`.
