param(
    [string]$BuildDir = "build",
    [string]$Config = "Release",
    [int]$Cycles = 10000,
    [string]$Out = "reports/generated"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $BuildDir)) {
    & "$PSScriptRoot/build.ps1" -BuildDir $BuildDir -Config $Config
}

$exe = Join-Path $BuildDir "emrtc_benchmark.exe"
if (-not (Test-Path $exe)) {
    $exe = Join-Path $BuildDir "apps/Release/emrtc_benchmark.exe"
}
if (-not (Test-Path $exe)) {
    $exe = Join-Path $BuildDir "Release/emrtc_benchmark.exe"
}
if (-not (Test-Path $exe)) {
    Write-Error "Cannot find emrtc_benchmark executable under $BuildDir."
}

& $exe --cycles $Cycles --out $Out
if ($LASTEXITCODE -ne 0) {
    throw "Benchmark failed with exit code $LASTEXITCODE."
}
