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

The committed sample report shows a 1 ms nominal cycle with bounded synthetic jitter plus injected stress cases. The high-jitter buckets are intentional: one CPU-load spike and one missed-cycle injection prove the timing instrumentation and watchdog path are not decorative.

Visual evidence is generated under `reports/`:

- `sample_jitter_histogram.svg`
- `sample_scenario_matrix.svg`
- `sample_control_response.svg`
- `sample_two_axis_path.svg`
- `sample_timing_report.pdf`

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
- What changes when the simulated bus becomes SOEM, EtherLab, SocketCAN, or CANopenNode?

Those are the right conversations for Siemens, Honeywell, Schneider, Swisslog, Ericsson-style embedded/software roles, and industrial robotics teams because they move beyond API familiarity into system behavior.

## Current Boundary

This is still software/simulation evidence. It does not claim real EtherCAT/CANopen hardware validation, real encoder measurements, or certified safety. The hardware commissioning boundary is explicit: adapters, PDO/object mapping, real timing capture, drive parameters, and physical E-stop/safe-stop wiring remain future work.
