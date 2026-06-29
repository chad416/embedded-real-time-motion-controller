param(
    [string]$BuildDir = "build",
    [string]$Config = "Release",
    [int]$Cycles = 10000
)

$ErrorActionPreference = "Stop"

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

& $python "$PSScriptRoot/generate_report.py" --out reports/generated
if ($LASTEXITCODE -ne 0) {
    throw "Python report generation failed with exit code $LASTEXITCODE."
}
