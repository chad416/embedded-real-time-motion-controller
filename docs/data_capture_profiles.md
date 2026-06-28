# Data Capture Profiles

The capture profiles in [`../config/capture_profiles`](../config/capture_profiles) turn commissioning intent into versioned inputs. Each JSON file defines the run purpose, backend, command sequence, injected faults, required artifacts, and acceptance criteria.

These files are deliberately separate from the current JSON scenario suite. Scenarios exercise software behavior inside the simulator. Capture profiles define the evidence package expected from simulation and future bench runs.

## Run Order

| Order | Profile | Why it runs there |
| ---: | --- | --- |
| 1 | `idle_bus_online` | Proves cyclic timing and bus supervision before motion. |
| 2 | `nominal_profile_position` | Establishes the baseline response plot and motion metrics. |
| 3 | `cpu_jitter_stress` | Shows deadline-pressure instrumentation without hiding jitter. |
| 4 | `heartbeat_timeout_recovery` | Proves device-supervision timeout and recovery evidence. |
| 5 | `bus_dropout_recovery` | Proves bus-watchdog fault logging and controlled recovery. |
| 6 | `encoder_loss_following_error` | Proves feedback and following-error watchdog evidence. |

## Required Evidence

Every profile requires machine-readable artifacts, not only screenshots:

- `trace.csv` for cycle-by-cycle state and signal review.
- `metrics.json` for thresholds, counters, timing, and motion metrics.
- `fault_log.json` when a fault is expected or recovery is under test.
- A rendered report or graph such as `timing_report.pdf`, `control_response.svg`, or `jitter_histogram.svg`.

The generated manifest files in [`../reports/sample_capture_manifest.json`](../reports/sample_capture_manifest.json) and [`../reports/sample_capture_manifest.md`](../reports/sample_capture_manifest.md) are review artifacts built from the JSON profile definitions.

## Hardware Boundary

Most profiles are marked `hardware_required: false` because the current repository can execute them as simulation or host-validation work. The `bench_extension` block describes what must be added before the same profile can support a hardware claim.

Until raw hardware traces, metrics, fault logs, timing plots, operator notes, and bench configuration are attached, the project remains a safety-oriented software validation package, not a hardware-validated EtherCAT/CANopen controller.
