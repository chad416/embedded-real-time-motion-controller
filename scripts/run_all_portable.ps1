param(
    [string]$ToolchainRoot = "",
    [string]$BuildDir = "build/portable-mingw",
    [string]$Config = "Release",
    [int]$Cycles = 10000
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if ([string]::IsNullOrWhiteSpace($ToolchainRoot)) {
    $workspaceRoot = Split-Path $repositoryRoot -Parent
    $ToolchainRoot = Join-Path $workspaceRoot ".toolchains\emrtc"
}

$manifestPath = Join-Path $repositoryRoot "config\toolchains\windows_portable.json"
$manifest = Get-Content -Raw $manifestPath | ConvertFrom-Json
$cmakeTool = $manifest.tools | Where-Object name -eq "cmake" | Select-Object -First 1
$compilerTool = $manifest.tools | Where-Object name -eq "winlibs" | Select-Object -First 1

if (-not $cmakeTool -or -not $compilerTool) {
    throw "Portable toolchain manifest must define cmake and winlibs entries."
}

$cmakeExe = Join-Path (Join-Path $ToolchainRoot $cmakeTool.extract_subdirectory) ($cmakeTool.executable -replace "/", "\")
$compilerExe = Join-Path (Join-Path $ToolchainRoot $compilerTool.extract_subdirectory) ($compilerTool.executable -replace "/", "\")
$makeExe = Join-Path (Split-Path $compilerExe -Parent) "mingw32-make.exe"

foreach ($required in @($cmakeExe, $compilerExe, $makeExe)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Portable toolchain file is missing: $required. Run scripts/install_portable_toolchain.ps1 first."
    }
}

$cmakeBin = Split-Path $cmakeExe -Parent
$compilerBin = Split-Path $compilerExe -Parent
$env:PATH = "$cmakeBin;$compilerBin;$env:PATH"
$env:CMAKE_GENERATOR = $manifest.default_generator
$env:CMAKE_CXX_COMPILER = $compilerExe
$env:CMAKE_MAKE_PROGRAM = $makeExe

Write-Host "Using CMake: $cmakeExe"
Write-Host "Using C++ compiler: $compilerExe"
Write-Host "Using generator: $($manifest.default_generator)"

& "$PSScriptRoot/check_toolchain.ps1" -RequireCMake -RequireCompiler
& "$PSScriptRoot/run_all.ps1" -BuildDir $BuildDir -Config $Config -Cycles $Cycles
