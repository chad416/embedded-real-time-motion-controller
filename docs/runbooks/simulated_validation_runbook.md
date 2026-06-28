# Simulated Validation Runbook

Use this runbook for CI, local host validation, and portfolio evidence refreshes.

## Target Profile

`config/hardware_targets/simulated_loopback_target.json`

## Preconditions

- Hosted CI is green for the commit under review, or local toolchain check passes.
- No hardware validation wording is used.
- Generated output goes under `reports/generated/`.

## Command Sequence

```powershell
scripts/check_toolchain.ps1
python scripts/generate_report.py --out reports/generated/simulated_sample
python scripts/generate_capture_manifest.py --profiles config/capture_profiles --out reports/generated/simulated_sample
python scripts/generate_hardware_target_manifest.py --targets config/hardware_targets --out reports/generated/simulated_sample
scripts/package_portfolio.ps1 -OutDir reports/generated/portfolio_bundle
```

If a native build exists:

```powershell
scripts/run_all.ps1
scripts/demo_portfolio.ps1 -BuildDir build -OutDir reports/generated/demo
```

## Acceptance Criteria

- Repository health checks pass.
- Sample metrics include all expected scenarios.
- Two-axis acceptance passes.
- Capture profile and hardware target manifests generate.
- Portfolio bundle creates a folder and archive.

## Evidence To Attach

- CI run URL or local transcript.
- `reports/generated` output folder.
- Portfolio bundle archive.

## Boundary

This runbook validates deterministic software behavior and evidence generation only.
