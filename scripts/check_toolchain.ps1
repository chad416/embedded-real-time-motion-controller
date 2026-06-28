param(
    [switch]$RequireCMake,
    [switch]$RequireCompiler,
    [switch]$Json
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Find-CommandName {
    param([string[]]$Names)

    foreach ($name in $Names) {
        $command = Get-Command $name -ErrorAction SilentlyContinue
        if ($null -ne $command) {
            return $command.Source
        }
    }
    return $null
}

function Get-CommandVersion {
    param(
        [string]$Command,
        [string[]]$Args = @("--version")
    )

    if ([string]::IsNullOrEmpty($Command)) {
        return ""
    }

    try {
        $output = & $Command @Args 2>&1 | Select-Object -First 1
        return [string]$output
    } catch {
        return "version probe failed: $($_.Exception.Message)"
    }
}

$cmakePath = Find-CommandName @("cmake")
$ctestPath = Find-CommandName @("ctest")
$compilerPath = Find-CommandName @("cl", "clang++", "g++", "c++")
$pythonPath = Find-CommandName @("python3", "python")
$gitPath = Find-CommandName @("git")
$ghPath = Find-CommandName @("gh")

$result = [ordered]@{
    cmake_available = -not [string]::IsNullOrEmpty($cmakePath)
    cmake_path = $cmakePath
    cmake_version = Get-CommandVersion $cmakePath
    ctest_available = -not [string]::IsNullOrEmpty($ctestPath)
    ctest_path = $ctestPath
    compiler_available = -not [string]::IsNullOrEmpty($compilerPath)
    compiler_path = $compilerPath
    compiler_version = Get-CommandVersion $compilerPath
    python_available = -not [string]::IsNullOrEmpty($pythonPath)
    python_path = $pythonPath
    python_version = Get-CommandVersion $pythonPath
    git_available = -not [string]::IsNullOrEmpty($gitPath)
    git_path = $gitPath
    github_cli_available = -not [string]::IsNullOrEmpty($ghPath)
    github_cli_path = $ghPath
}

if ($Json) {
    $result | ConvertTo-Json -Depth 3
} else {
    Write-Host "Toolchain check"
    Write-Host "  CMake:      $($result.cmake_available) $($result.cmake_path)"
    Write-Host "  CTest:      $($result.ctest_available) $($result.ctest_path)"
    Write-Host "  C++:        $($result.compiler_available) $($result.compiler_path)"
    Write-Host "  Python:     $($result.python_available) $($result.python_path)"
    Write-Host "  Git:        $($result.git_available) $($result.git_path)"
    Write-Host "  GitHub CLI: $($result.github_cli_available) $($result.github_cli_path)"
}

if ($RequireCMake -and -not $result.cmake_available) {
    throw "CMake is required but was not found on PATH."
}

if ($RequireCompiler -and -not $result.compiler_available) {
    throw "A C++ compiler is required but cl, clang++, g++, or c++ was not found on PATH."
}
