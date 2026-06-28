# Hardware Evidence Archive

This convention defines how future bench evidence is stored, reviewed, and retained. It does not convert simulated evidence into hardware validation. A hardware claim is allowed only after the relevant checkout, runbook, raw data, configuration, and review records exist.

## Archive Location

Use one immutable directory per run:

```text
reports/generated/hardware/<YYYY-MM-DD>_<target>_<profile>_<run-sequence>/
```

Example:

```text
reports/generated/hardware/2026-07-15_generic_ethercat_cia402_servo_nominal_profile_position_001/
```

Use lowercase ASCII names with underscores. The date is the local bench date. The manifest records the authoritative UTC start time.

## Required Layout

```text
<run-directory>/
|-- RUN_MANIFEST.json
|-- operator_notes.md
|-- raw/
|   |-- trace.csv
|   `-- bus_capture.<native-format>
|-- metrics/
|   |-- metrics.json
|   `-- fault_log.json
|-- reports/
|   |-- validation_report.md
|   `-- timing_report.pdf
|-- config/
|   |-- mapping_dump.md
|   |-- controller_config.json
|   `-- drive_parameters/
`-- media/
    |-- bench_overview.jpg
    `-- fieldbus_state.png
```

The bus capture, timing PDF, and media files are required when the selected capture profile or runbook asks for them. Do not create placeholder files to make an incomplete run look complete.

## Run Manifest

Create `RUN_MANIFEST.json` from [`../examples/hardware_evidence/RUN_MANIFEST.example.json`](../examples/hardware_evidence/RUN_MANIFEST.example.json). Validate its structure against [`../config/hardware_evidence_run_manifest.schema.json`](../config/hardware_evidence_run_manifest.schema.json).

The manifest records:

- run identity, run type, target profile, and capture profile;
- UTC start time, operator, bench environment, and software version;
- full Git commit hash and whether the worktree was clean;
- hardware inventory and drive firmware;
- every evidence file, its role, byte count, and SHA-256 digest;
- overall result, review state, deviations, and claim boundary.

Set `hardware_claim_allowed` to `false` during capture. It may become `true` only after a reviewer confirms the hardware checkout, required artifacts, acceptance criteria, and manifest hashes.

## Integrity Rules

1. Finish writing all raw captures before calculating hashes.
2. Calculate SHA-256 for every file listed in `artifacts`.
3. Record paths relative to the run directory and use forward slashes.
4. Never edit an accepted run in place. Create a new run sequence for a repeat.
5. If notes must be corrected after review, record the correction as a new file and add a manifest deviation.
6. Keep native bus-capture and drive-parameter formats. Export human-readable summaries in addition to, not instead of, native files.
7. Do not commit secrets, license files, device credentials, network keys, or proprietary vendor material.

PowerShell can calculate a digest with:

```powershell
Get-FileHash -Algorithm SHA256 .\raw\trace.csv
```

## Configuration Records

The `config` directory must identify enough state to reproduce the run:

- controller cycle time and watchdog thresholds;
- drive model, serial or asset identifier, firmware, and parameter-set revision;
- fieldbus interface, topology, bitrate or distributed-clock settings;
- PDO/object dictionary mapping and engineering-unit scaling;
- current, torque, velocity, travel, and following-error limits;
- E-stop, STO, inhibit, and limit-switch test configuration;
- host OS, kernel or real-time runtime, CPU affinity, and timer source when relevant.

Vendor parameter backups may be binary. Add a short Markdown inventory that names the vendor tool and export procedure.

## Media Rules

Photos and screenshots support the engineering record but never replace raw telemetry. Capture the bench overview, protective devices, fieldbus topology, drive state, and relevant wiring labels without exposing credentials or unrelated personal information.

## Review Gate

A hardware run is reviewable only when:

- the matching hardware checkout and runbook steps are complete;
- all capture-profile artifacts are present and hashed;
- the commit in the manifest matches the tested binary;
- deviations and aborted steps are explicit;
- acceptance criteria have a recorded pass or fail;
- a second-person review is recorded for any public hardware claim.

Simulation runs may use the same archive shape with `run_type` set to `simulation` and `hardware_claim_allowed` fixed to `false`.
