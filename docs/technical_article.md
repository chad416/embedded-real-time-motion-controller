# Graph-First Technical Article

## Deterministic Motion Control Under Fault Injection

The point of this project is not to show a motor moving on the happy path. The point is to prove that the controller measures and explains what happens when timing, communication, and feedback assumptions break.

## Evidence First

The deterministic benchmark currently generates:

- Cycle-time histogram.
- Min, max, and mean cycle period.
- Jitter distribution.
- Missed-cycle count.
- Deadline-overrun count.
- Packet-loss count.
- Fault recovery evidence.
- Scenario pass/fail table.
- Replayable CSV trace and persistent JSON fault log.
- Closed-loop motion performance: overshoot, settling time, steady-state error, and following error.
- Coordinated two-axis path performance: RMS path error, max path error, final position error, settling time, and acceptance thresholds.
- Capture profile manifest for simulation and future bench evidence.
- Adapter contract evidence for object-map validation, stale receive data, and invalid responses.

The committed sample report shows a 1 ms nominal cycle with bounded synthetic jitter plus injected stress cases. The high-jitter buckets are intentional: one CPU-load spike and one missed-cycle injection prove the timing instrumentation and watchdog path are not decorative.

Visual evidence is generated under `reports/`:

- `sample_jitter_histogram.svg`
- `sample_scenario_matrix.svg`
- `sample_control_response.svg`
- `sample_two_axis_path.svg`
- `sample_two_axis_acceptance.svg`
- `sample_recovery_comparison.svg`
- `sample_timing_report.pdf`
- `sample_capture_manifest.md`

## Adapter Boundary Evidence

The adapter layer now has two explicit contracts before real hardware is commissioned:

- The CiA 402 process-image mapping contract defines object indexes, bit widths, direction, and engineering-unit scaling.
- The exchange contract rejects invalid object maps, stale receive data, heartbeat loss, bus-offline state, and invalid PDO/object responses.

That matters because hardware integration should not start by trusting whatever a fieldbus stack hands back. The software now exercises dependency-backed SOEM and injected SocketCAN/CANopen paths in CI before either is connected to a real drive; EtherLab and CANopenNode remain optional alternatives.

The host timing probe adds a second evidence class. It schedules against absolute monotonic deadlines, applies configurable CPU stress, reports Linux FIFO/memory-lock capability results, and exports measured timing. Those measurements prove the host instrumentation path, not a future MCU or PREEMPT_RT target.

## Capture Profiles

The capture profiles define the evidence package for future bench work:

- `idle_bus_online` proves timing and supervision before motion.
- `nominal_profile_position` proves baseline command/response metrics.
- `cpu_jitter_stress` proves timing stress visibility.
- `heartbeat_timeout_recovery` proves supervision and recovery.
- `bus_dropout_recovery` proves bus-watchdog behavior.
- `encoder_loss_following_error` proves feedback and following-error watchdog behavior.

The profiles keep the project honest: simulation-ready runs are separated from future bench-extension evidence.

## Controller Structure

The C++ controller executes a deterministic cycle:

1. Apply scheduled fault injections.
2. Dispatch pending axis command.
3. Apply safe-stop request.
4. Record cycle timing sample.
5. Exchange process image through the bus abstraction.
6. Update the CiA 402-style axis model.
7. Evaluate watchdogs.
8. Record telemetry and persistent fault evidence.

## What Makes This Interview-Useful

The project gives a reviewer something concrete to challenge:

- What threshold creates a missed cycle?
- How does heartbeat loss become a fault?
- What happens after a rejected motion command?
- Which layer owns bus dropout detection?
- What changes when the simulated bus becomes the implemented SOEM or SocketCAN backend?
- Which artifacts prove a bench run passed the same criteria as the simulation?

Those are the right conversations for Siemens, Honeywell, Schneider, Swisslog, Ericsson-style embedded/software roles, and industrial robotics teams because they move beyond API familiarity into system behavior.

## Current Boundary

This is still software/simulation and host-execution evidence. It does not claim real EtherCAT/CANopen hardware validation, real encoder measurements, target real-time performance, or certified safety. The adapter software is implemented; selected interfaces/drives, device parameters, target timing capture, and physical E-stop/safe-stop wiring remain hardware commissioning work.
