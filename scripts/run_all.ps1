param(
    [string]$BuildDir = "build",
    [string]$Config = "Release",
    [int]$Cycles = 10000
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Invoke-PythonChecked {
    param(
        [string]$Script,
        [string[]]$Arguments = @()
    )

    & $python $Script @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Python command failed with exit code ${LASTEXITCODE}: $Script $($Arguments -join ' ')"
    }
}

& "$PSScriptRoot/build.ps1" -BuildDir $BuildDir -Config $Config
& "$PSScriptRoot/test.ps1" -BuildDir $BuildDir -Config $Config
& "$PSScriptRoot/benchmark.ps1" -BuildDir $BuildDir -Config $Config -Cycles $Cycles
& "$PSScriptRoot/realtime_probe.ps1" -BuildDir $BuildDir -Config $Config -Cycles 500

$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    $candidate = Join-Path $env:USERPROFILE ".cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe"
    if (Test-Path $candidate) {
        $python = $candidate
    }
}
if (-not $python) {
    Write-Error "Python is not available for report generation. Install Python 3.10+ or run scripts/generate_report.py with an available interpreter."
}

Invoke-PythonChecked "$PSScriptRoot/generate_report.py" @("--out", "reports/generated")
Invoke-PythonChecked "$PSScriptRoot/generate_timing_pdf.py" @("--metrics", "reports/sample_metrics.json", "--out", "reports/generated/timing_report.pdf")
Invoke-PythonChecked "$PSScriptRoot/generate_graphs.py" @("--metrics", "reports/sample_metrics.json", "--out", "reports/generated")
Invoke-PythonChecked "$PSScriptRoot/generate_capture_manifest.py" @("--profiles", "config/capture_profiles", "--out", "reports/generated/capture-manifest")
Invoke-PythonChecked "$PSScriptRoot/generate_hardware_target_manifest.py" @("--targets", "config/hardware_targets", "--out", "reports/generated/hardware-targets")
Invoke-PythonChecked "$PSScriptRoot/generate_one_pager.py" @("--metrics", "reports/sample_metrics.json", "--manifest", "reports/sample_capture_manifest.json", "--out", "reports/generated/recruiter_one_pager.md")
Invoke-PythonChecked "$PSScriptRoot/validate_project.py"
Invoke-PythonChecked "$PSScriptRoot/check_simulation_milestone.py"
Invoke-PythonChecked "$PSScriptRoot/check_software_ready_milestone.py"
Invoke-PythonChecked "tests/test_evidence_archive_validator.py" @("-v")

& "$PSScriptRoot/package_portfolio.ps1" -OutDir reports/generated/portfolio-bundle -ArchivePath reports/generated/portfolio-bundle.zip
