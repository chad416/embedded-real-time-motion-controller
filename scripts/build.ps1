param(
    [string]$BuildDir = "build",
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"
$cmake = Get-Command cmake -ErrorAction SilentlyContinue

if (-not $cmake) {
    Write-Error "CMake is not available on PATH. Install CMake 3.20+ and a C++17 compiler, then rerun scripts/build.ps1."
}

cmake -S . -B $BuildDir -DCMAKE_BUILD_TYPE=$Config
cmake --build $BuildDir --config $Config
