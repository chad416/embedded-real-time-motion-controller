# Simulation-Complete Software Milestone

## Release Identity

- Version: `0.2.0`
- Tag: `v0.2.0-simulation-complete`
- Scope: software, simulation, validation evidence, and documentation
- Excluded scope: commissioned EtherCAT/CANopen hardware and physical safety validation

This milestone does not claim hardware validation. It states that the hosted software and simulation package has a reproducible, machine-enforced evidence baseline suitable for portfolio review and future bench integration.

Release record:

- Tagged commit: `b86ba1ac75ac2c51dd40bbc1921f6d363ca241b0`
- Verified GitHub Actions run: `28338578406`
- Annotated tag pushed: 2026-06-29

## Automated Release Gate

The versioned definition is [`../config/milestones/simulation_complete_v0.2.0.json`](../config/milestones/simulation_complete_v0.2.0.json). Run:

```powershell
python scripts/check_simulation_milestone.py
```

The gate first runs the repository health checks, then verifies:

- CMake project version and milestone tag identity;
- at least 21 passing deterministic scenarios;
- settled single-axis motion within the overshoot bound;
- passing two-axis path and settling acceptance;
- at least six versioned capture profiles;
- at least three vendor-neutral hardware target profiles;
- no target profile presented as a purchase recommendation;
- required reports, release records, and hardware evidence conventions;
- CI execution of the same milestone gate;
- explicit simulation-only claim language.

## Tag Checklist

The following is the release operator checklist. The automated items are enforced by the gate; the release operations are performed only after the exact release commit passes hosted CI.

- [x] Hosted C++17 configure, build, adapter smoke build, and tests pass.
- [x] Simulator, JSON scenario loader, two-axis demonstration, benchmark, and CLI demo pass.
- [x] Deterministic reports, graphs, manifests, and recruiter one-pager generate successfully.
- [x] Portfolio evidence bundle generates successfully.
- [x] Repository health checks pass.
- [x] Simulation milestone gate passes.
- [x] Hardware evidence archive and run-manifest rules are documented.
- [x] Changelog and release-note template identify the hardware boundary.
- [x] Push the release commit and confirm its exact GitHub Actions run passes.
- [x] Create the annotated tag from that verified commit and push it.

The final two items are release operations. They must be completed in order; never tag a commit whose hosted run has not passed.

## Evidence Baseline

The milestone includes:

- deterministic C++ controller, axis, bus, watchdog, and fault-injection logic;
- adapter discovery and exchange contracts for future SOEM, EtherLab, SocketCAN, and CANopenNode work;
- single-axis and coordinated two-axis motion evidence;
- 21-scenario validation evidence with fault and recovery coverage;
- timing, control-response, scenario, and two-axis visual reports;
- versioned capture profiles, target profiles, runbooks, and commissioning templates;
- PowerShell demo, toolchain check, and portfolio packaging workflows;
- Linux hosted CI and the pinned portable Windows workflow as native build authorities for their respective paths.

## Remaining Work

- Commission the dependency-backed SOEM path on a selected EtherCAT NIC and CiA 402 drive, or implement the selected alternative backend.
- Record physical drive mapping, timing, motion, fault, wiring, and stop-path evidence.
- Validate a selected real-time target such as PREEMPT_RT or an embedded RTOS platform.
- Review any public hardware or safety claim against the archived bench evidence.

The broader flagship portfolio remains incomplete until those hardware tasks are performed.
