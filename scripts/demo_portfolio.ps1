param(
    [string]$BuildDir = "build",
    [string]$OutDir = "reports/generated/demo",
    [switch]$SkipNative
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Write-Step {
    param([string]$Name)
    Write-Host ""
    Write-Host "== $Name =="
}

function Invoke-Checked {
    param(
        [string]$Command,
        [string[]]$Arguments
    )

    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $Command $($Arguments -join ' ')"
    }
}

function Get-ExecutablePath {
    param([string]$Name)

    $isUnix = $PSVersionTable.ContainsKey("Platform") -and $PSVersionTable.Platform -eq "Unix"
    $suffix = if ($isUnix) { "" } else { ".exe" }
    $path = Join-Path $BuildDir "$Name$suffix"
    if (-not (Test-Path $path)) {
        throw "Missing native executable: $path. Build first or run with -SkipNative."
    }
    return (Resolve-Path $path).Path
}

function Get-PythonCommand {
    foreach ($candidate in @("python3", "python")) {
        $command = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($null -ne $command) {
            return $command.Source
        }
    }
    $bundledPython = Join-Path $env:USERPROFILE ".cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe"
    if (Test-Path -LiteralPath $bundledPython) {
        return $bundledPython
    }
    return $null
}

New-Item -ItemType Directory -Force $OutDir | Out-Null

if (-not $SkipNative) {
    $cli = Get-ExecutablePath "emrtc_cli"
    $sim = Get-ExecutablePath "emrtc_sim"
    $benchmark = Get-ExecutablePath "emrtc_benchmark"

    Write-Step "Diagnostic status"
    Invoke-Checked -Command $cli -Arguments @("show", "status")

    Write-Step "Adapter status"
    Invoke-Checked -Command $cli -Arguments @("show", "adapters")

    Write-Step "Two-axis coordinated motion"
    Invoke-Checked -Command $cli -Arguments @("run", "two-axis")

    Write-Step "Fault-matrix scenario"
    $scenarioOut = Join-Path $OutDir "fault_matrix"
    Invoke-Checked -Command $sim -Arguments @("--scenario", "examples/scenarios/fault_matrix.json", "--out", $scenarioOut)

    Write-Step "Deterministic benchmark"
    $benchmarkOut = Join-Path $OutDir "benchmark"
    Invoke-Checked -Command $benchmark -Arguments @("--cycles", "5000", "--out", $benchmarkOut)
}

$python = Get-PythonCommand
if ($null -ne $python) {
    Write-Step "Sample software evidence"
    $sampleOut = Join-Path $OutDir "sample_report"
    Invoke-Checked -Command $python -Arguments @("scripts/generate_report.py", "--out", $sampleOut)

    Write-Step "Capture profile manifest"
    $manifestOut = Join-Path $OutDir "capture_manifest"
    Invoke-Checked -Command $python -Arguments @("scripts/generate_capture_manifest.py", "--profiles", "config/capture_profiles", "--out", $manifestOut)
} else {
    Write-Step "Python evidence generation skipped"
    Write-Host "No configured Python interpreter was found."
}

Write-Step "Demo artifacts"
Write-Host "Generated portfolio demo artifacts under $OutDir"
