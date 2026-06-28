# Release Notes Template

## `v0.2.0-simulation-complete`

Release date: `YYYY-MM-DD`

Commit: `<full-commit-hash>`

GitHub Actions run: `<run-url>`

## Summary

State what the release proves in software and simulation. Hardware validation is not included in this milestone.

## Added

- `<controller, validation, evidence, or documentation capability>`

## Changed

- `<behavioral or interface change>`

## Fixed

- `<defect and its validation evidence>`

## Validation

- Milestone gate: `python3 scripts/check_simulation_milestone.py`
- Scenario result: `<passed>/<total>`
- Unit and integration tests: `<result>`
- Adapter smoke build: `<result>`
- Simulator and benchmark: `<result>`
- Portfolio bundle: `<artifact name or path>`

## Quantitative Evidence

| Metric | Result | Limit |
| --- | ---: | ---: |
| Single-axis overshoot | `<value>` | `<limit>` |
| Single-axis settling time | `<value>` | `<limit>` |
| Two-axis maximum path error | `<value>` | `<limit>` |
| Two-axis settling time | `<value>` | `<limit>` |
| Missed cycles | `<value>` | `<expected range>` |

## Known Limitations

- Local Windows native build status: `<result or reason blocked>`
- Real EtherCAT adapter: `<status>`
- Real CANopen adapter: `<status>`
- Physical motion and stop-path evidence: `<status>`

## Hardware Boundary

This release is software/simulation complete. It does not claim commissioned hardware behavior, electrical correctness, real-time target performance, or certified safety integrity.

## Artifacts

- `<generated-validation-reports artifact>`
- `<portfolio evidence bundle>`
- `<checksums or manifest>`

## Release Approval

- Release commit CI passed: `<yes/no>`
- Milestone gate passed on release commit: `<yes/no>`
- Annotated tag points to release commit: `<yes/no>`
- Claim language reviewed: `<reviewer/date>`
