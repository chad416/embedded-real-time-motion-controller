param(
    [string]$OutDir = "reports/generated/portfolio_bundle",
    [string]$ArchivePath = "",
    [switch]$SkipArchive
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Copy-RequiredFile {
    param(
        [string]$Source,
        [string]$DestinationRelative
    )

    if (-not (Test-Path $Source)) {
        throw "Missing required portfolio artifact: $Source"
    }
    $destination = Join-Path $OutDir $DestinationRelative
    $destinationDirectory = Split-Path $destination -Parent
    New-Item -ItemType Directory -Force $destinationDirectory | Out-Null
    Copy-Item -LiteralPath $Source -Destination $destination -Force
}

function Copy-RequiredDirectory {
    param(
        [string]$Source,
        [string]$DestinationRelative
    )

    if (-not (Test-Path $Source)) {
        throw "Missing required portfolio directory: $Source"
    }
    $destination = Join-Path $OutDir $DestinationRelative
    New-Item -ItemType Directory -Force $destination | Out-Null
    Copy-Item -Path (Join-Path $Source "*") -Destination $destination -Recurse -Force
}

New-Item -ItemType Directory -Force $OutDir | Out-Null

$rootFiles = @(
    "README.md",
    "CMakeLists.txt",
    "CHANGELOG.md"
)

foreach ($file in $rootFiles) {
    Copy-RequiredFile $file $file
}

$docs = @(
    "docs/recruiter_one_pager.md",
    "docs/demo_walkthrough.md",
    "docs/demo_narratives.md",
    "docs/project_status.md",
    "docs/portfolio_alignment.md",
    "docs/technical_article.md",
    "docs/architecture.md",
    "docs/timing_budget.md",
    "docs/control_loop_architecture.md",
    "docs/bus_map.md",
    "docs/drive_state_machine.md",
    "docs/watchdog_design.md",
    "docs/fault_handling_matrix.md",
    "docs/test_plan.md",
    "docs/validation_report.md",
    "docs/benchmark_appendix.md",
    "docs/design_tradeoffs.md",
    "docs/final_report.md",
    "docs/hardware_integration_plan.md",
    "docs/hardware_checkout.md",
    "docs/hardware_target_profiles.md",
    "docs/hardware_evidence_archive.md",
    "docs/local_toolchain_setup.md",
    "docs/local_windows_validation.md",
    "docs/software_complete_milestone.md",
    "docs/release_notes_template.md",
    "docs/bench_run_playbook.md",
    "docs/commissioning_logbook.md",
    "docs/data_capture_profiles.md",
    "docs/bom.md"
)

foreach ($doc in $docs) {
    Copy-RequiredFile $doc $doc
}

Copy-RequiredDirectory "docs/adapters" "docs/adapters"
Copy-RequiredDirectory "docs/runbooks" "docs/runbooks"
Copy-RequiredDirectory ".github/ISSUE_TEMPLATE" ".github/ISSUE_TEMPLATE"
Copy-RequiredDirectory "config/capture_profiles" "config/capture_profiles"
Copy-RequiredDirectory "config/hardware_targets" "config/hardware_targets"
Copy-RequiredDirectory "config/milestones" "config/milestones"
Copy-RequiredDirectory "config/toolchains" "config/toolchains"
Copy-RequiredFile "config/hardware_evidence_run_manifest.schema.json" "config/hardware_evidence_run_manifest.schema.json"
Copy-RequiredDirectory "examples/hardware_evidence" "examples/hardware_evidence"
Copy-RequiredDirectory "examples/scenarios" "examples/scenarios"
Copy-RequiredDirectory "examples/replay" "examples/replay"
Copy-RequiredFile "scripts/check_simulation_milestone.py" "scripts/check_simulation_milestone.py"
Copy-RequiredFile "scripts/validate_evidence_archive.py" "scripts/validate_evidence_archive.py"
Copy-RequiredFile "scripts/install_portable_toolchain.ps1" "scripts/install_portable_toolchain.ps1"
Copy-RequiredFile "scripts/run_all_portable.ps1" "scripts/run_all_portable.ps1"

$reports = @(
    "reports/sample_metrics.json",
    "reports/sample_fault_log.json",
    "reports/sample_trace.csv",
    "reports/sample_validation_report.md",
    "reports/sample_timing_report.pdf",
    "reports/sample_jitter_histogram.svg",
    "reports/sample_scenario_matrix.svg",
    "reports/sample_control_response.svg",
    "reports/sample_two_axis_path.svg",
    "reports/sample_two_axis_acceptance.svg",
    "reports/sample_capture_manifest.json",
    "reports/sample_capture_manifest.md",
    "reports/sample_hardware_target_manifest.json",
    "reports/sample_hardware_target_manifest.md"
)

foreach ($report in $reports) {
    Copy-RequiredFile $report $report
}

$manifestPath = Join-Path $OutDir "MANIFEST.md"
$manifestLines = @(
    "# Portfolio Evidence Bundle",
    "",
    "This bundle contains review-oriented documentation, deterministic sample evidence, graphs, capture profiles, and hardware-commissioning boundaries.",
    "",
    "## Contents",
    ""
)

$bundleRoot = (Resolve-Path $OutDir).Path
$files = Get-ChildItem -Path $OutDir -Recurse -File | Sort-Object FullName
foreach ($file in $files) {
    $relative = $file.FullName.Substring($bundleRoot.Length).TrimStart([char[]]"\/")
    $manifestLines += "- $relative"
}

Set-Content -Path $manifestPath -Value $manifestLines -Encoding UTF8

if (-not $SkipArchive) {
    $archive = if ($ArchivePath.Length -gt 0) { $ArchivePath } else { "$OutDir.zip" }
    $archiveDirectory = Split-Path $archive -Parent
    if (-not [string]::IsNullOrEmpty($archiveDirectory)) {
        New-Item -ItemType Directory -Force $archiveDirectory | Out-Null
    }
    Compress-Archive -Path (Join-Path $OutDir "*") -DestinationPath $archive -Force
    Write-Host "Wrote portfolio archive: $archive"
}

Write-Host "Wrote portfolio bundle: $OutDir"
