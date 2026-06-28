# Bench Run Playbook

This playbook explains how to track future simulated and hardware bench runs without weakening the project boundary.

## Issue Templates

The repository includes two GitHub issue templates:

- [`../.github/ISSUE_TEMPLATE/bench-run.md`](../.github/ISSUE_TEMPLATE/bench-run.md)
- [`../.github/ISSUE_TEMPLATE/adapter-commissioning.md`](../.github/ISSUE_TEMPLATE/adapter-commissioning.md)

Use a bench-run issue for each capture-profile execution. Use an adapter-commissioning issue when a real SOEM, EtherLab, SocketCAN, or CANopenNode backend is being brought up.

## Bench Run Sequence

1. Pick a hardware target profile from [`../config/hardware_targets`](../config/hardware_targets).
2. Pick a capture profile from [`../config/capture_profiles`](../config/capture_profiles).
3. Select the matching runbook from [`runbooks`](runbooks).
4. Open a bench-run issue and copy the profile acceptance criteria into it.
5. Confirm hosted CI is green for the commit being tested.
6. For hardware runs, complete the hardware checkout checklist first.
7. Run the profile and attach raw artifacts.
8. Review pass/fail criteria before changing any project claim language.

## Adapter Commissioning Sequence

1. Open an adapter-commissioning issue.
2. Record dependency discovery and build flags.
3. Validate CiA 402 object mapping before motion.
4. Run idle cyclic exchange before torque-producing commands.
5. Execute one low-energy nominal move.
6. Execute one supervised fault and recovery path.
7. Attach logs, traces, mapping dumps, and commissioning notes.

## Evidence Naming

Use the complete archive convention in [`hardware_evidence_archive.md`](hardware_evidence_archive.md). The stable run-directory form is:

```text
reports/generated/hardware/<date>_<target>_<profile>/
```

Expected files:

- `trace.csv`
- `metrics.json`
- `fault_log.json`
- `validation_report.md`
- `timing_report.pdf`
- `operator_notes.md`
- `mapping_dump.md`
- photos or screenshots, if applicable

Each run also requires a `RUN_MANIFEST.json` with relative artifact paths and SHA-256 hashes.

## Boundary

Do not mark a hardware run complete if the issue contains only screenshots or informal notes. Raw traces, metrics, fault logs, configuration records, and operator notes are required.
