# Capture Profile Manifest

Generated from `config/capture_profiles/*.json`. These profiles define the evidence that must be collected during simulation and future hardware commissioning runs.

This manifest does not claim real EtherCAT/CANopen hardware validation. Profiles with bench extensions describe future evidence requirements only.

| Profile | Backend | Hardware required now | Cycles | Fault injections | Required artifacts | Acceptance criteria |
| --- | --- | --- | ---: | ---: | --- | --- |
| `bus_dropout_recovery` | simulated_or_hardware | False | 4500 | 1 | trace.csv, metrics.json, fault_log.json, validation_report.md | `bus_dropout_count_min` = `1`<br>`fault_expected` = `Bus Watchdog`<br>`final_fault` = `None`<br>`operation_enabled_required` = `True`<br>`recovery_cycles_max` = `300` |
| `cpu_jitter_stress` | simulated_host | False | 5000 | 2 | trace.csv, metrics.json, timing_report.pdf, jitter_histogram.svg | `deadline_overruns_min` = `1`<br>`final_fault` = `None`<br>`max_jitter_ns` = `1500000`<br>`missed_cycles_max` = `1`<br>`watchdog_fault_expected` = `False` |
| `encoder_loss_following_error` | simulated_or_hardware | False | 5000 | 2 | trace.csv, metrics.json, fault_log.json, control_response.svg | `encoder_loss_count_min` = `1`<br>`fault_expected` = `Encoder Feedback Loss`<br>`final_fault` = `None`<br>`following_error_count_min` = `1`<br>`max_abs_following_error` = `0.25`<br>`recovery_cycles_max` = `350` |
| `heartbeat_timeout_recovery` | simulated_or_hardware | False | 3500 | 1 | trace.csv, metrics.json, fault_log.json, validation_report.md | `fault_expected` = `Heartbeat Timeout`<br>`final_fault` = `None`<br>`heartbeat_timeout_count_min` = `1`<br>`missed_cycles_max` = `0`<br>`recovery_cycles_max` = `250` |
| `idle_bus_online` | simulated_or_hardware | False | 3000 | 0 | trace.csv, metrics.json, fault_log.json, timing_report.pdf | `deadline_overruns_max` = `0`<br>`final_fault` = `None`<br>`max_jitter_ns` = `100000`<br>`missed_cycles_max` = `0`<br>`operation_enabled_required` = `True`<br>`packet_loss_max` = `0` |
| `nominal_profile_position` | simulated_or_hardware | False | 4000 | 0 | trace.csv, metrics.json, validation_report.md, control_response.svg | `final_fault` = `None`<br>`max_abs_following_error` = `0.05`<br>`overshoot_max` = `0.02`<br>`settling_time_max_s` = `2.5`<br>`steady_state_error_abs_max` = `0.01` |

## Hardware Boundary

A profile can be simulation-ready while still carrying a bench-extension note. Hardware evidence must include raw traces, metrics, fault logs, timing plots, and operator notes before the run is described as hardware validated.
