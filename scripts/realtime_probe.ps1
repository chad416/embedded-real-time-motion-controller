param(
    [string]$BuildDir = "build",
    [string]$Config = "Release",
    [int]$Cycles = 500,
    [int]$PeriodUs = 1000,
    [int]$ToleranceUs = 100,
    [int]$StressThreads = 0,
    [string]$Out = "reports/generated/realtime-probe"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $BuildDir)) {
    & "$PSScriptRoot/build.ps1" -BuildDir $BuildDir -Config $Config
}

$candidates = @(
    (Join-Path $BuildDir "emrtc_realtime_probe.exe"),
    (Join-Path $BuildDir "emrtc_realtime_probe"),
    (Join-Path $BuildDir "apps/$Config/emrtc_realtime_probe.exe"),
    (Join-Path $BuildDir "$Config/emrtc_realtime_probe.exe")
)
$exe = $candidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $exe) {
    throw "Cannot find emrtc_realtime_probe executable under $BuildDir."
}

& $exe --cycles $Cycles --period-us $PeriodUs --tolerance-us $ToleranceUs `
    --stress-threads $StressThreads --out $Out
if ($LASTEXITCODE -ne 0) {
    throw "Host real-time probe failed with exit code $LASTEXITCODE."
}
