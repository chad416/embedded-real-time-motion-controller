# Website Project Entry: Embedded Real-Time Motion Controller

## Work Card

**Title:** Embedded Real-Time Motion Controller

**Subtitle:** Deterministic C++/FreeRTOS-style motion-control bench with CiA 402 drive behavior, SOEM/SocketCAN software paths, timing instrumentation, watchdogs, fault injection, telemetry, and replayable validation evidence.

**One-liner:** A software-ready motion-controller validation project that proves deterministic loop design, fieldbus abstraction, fault recovery, and quantitative timing evidence before physical EtherCAT/CANopen commissioning.

**Status label:** Software-ready; hardware commissioning remains Phase 2.

**Suggested CTA:** Open timing brief

## Evidence To Show

- Release gate: `v0.3.0-software-ready`.
- Native portable validation passes with CMake 4.3.3 and MinGW GCC 16.1.0.
- C++ checks: 45 registered checks.
- Scenario suite: 21/21 passing deterministic motion, timing, bus, and fault scenarios.
- Benchmark evidence: 2,500 cycles, 1 ms nominal loop, 2 injected missed cycles, 2 injected deadline overruns.
- Host timing probe: absolute-deadline wall-clock probe exports CPU-stress timing evidence and capability flags.
- Recovery evidence: 0-cycle detection latency, 7-cycle recovery to operation re-enabled, post-recovery final error 0.0.
- Motion evidence: overshoot 0.0, settling time 0.8 s, two-axis acceptance pass.
- Visual artifacts: jitter histogram, scenario matrix, control response, two-axis path, acceptance plot, recovery comparison, and timing PDF.
- Evidence tooling: `scripts/package_portfolio.ps1` exports a review bundle with reports, runbooks, capture profiles, and hardware-boundary docs.

## Suggested Detail Sections

### Overview

C++17 software-first embedded motion-controller bench for deterministic cyclic control, CiA 402-style drive state handling, bus timing diagnostics, watchdog strategy, and recovery under injected fault conditions.

### Control And Bus Architecture

The control loop separates scheduling, guarded axis commands, bus exchange, axis-state update, watchdog supervision, telemetry, and persistent fault logging. The fieldbus boundary supports deterministic simulation, replay traces, dependency-backed SOEM EtherCAT software integration, and Linux SocketCAN/CANopen software integration.

### Timing And Fault Evidence

The repo publishes quantitative evidence rather than a happy-path demo: jitter distributions, missed-cycle counters, deadline-overrun counters, packet-loss counters, recovery-cycle counts, closed-loop motion metrics, two-axis acceptance metrics, CSV traces, JSON metrics, Markdown reports, SVG plots, and a timing PDF.

### Hardware Boundary

Do not claim commissioned EtherCAT/CANopen hardware, real encoder measurements, physical stop-path verification, or certified SIL/PL/CE safety validation. The honest claim is that the software, simulation, adapter contracts, host execution probes, documentation, and evidence tooling are complete and ready for the Phase 2 bench.

## Tech Tags

C++17, FreeRTOS, deterministic control, real-time scheduling, CiA 402, EtherCAT, SOEM, CANopen, SocketCAN, PREEMPT_RT readiness, watchdogs, fault injection, telemetry, jitter analysis, motion control, embedded Linux, CMake, GitHub Actions, safety-oriented software, validation reports.
