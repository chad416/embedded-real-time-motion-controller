param(
    [string]$BuildDir = "build",
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $BuildDir)) {
    & "$PSScriptRoot/build.ps1" -BuildDir $BuildDir -Config $Config
}

$ctest = Get-Command ctest -ErrorAction Stop
& $ctest.Source --test-dir $BuildDir --build-config $Config --output-on-failure
if ($LASTEXITCODE -ne 0) {
    throw "CTest failed with exit code $LASTEXITCODE."
}
