param(
    [string]$BuildDir = "build",
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $BuildDir)) {
    & "$PSScriptRoot/build.ps1" -BuildDir $BuildDir -Config $Config
}

ctest --test-dir $BuildDir --build-config $Config --output-on-failure
