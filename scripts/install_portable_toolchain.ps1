param(
    [string]$ToolchainRoot = ""
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
$downloads = Join-Path $ToolchainRoot "downloads"
New-Item -ItemType Directory -Force $downloads | Out-Null

$curl = Get-Command curl.exe -ErrorAction SilentlyContinue
$tar = Get-Command tar.exe -ErrorAction SilentlyContinue
if (-not $curl -or -not $tar) {
    throw "curl.exe and tar.exe are required to install the portable toolchain."
}

foreach ($tool in $manifest.tools) {
    $archivePath = Join-Path $downloads $tool.archive_name
    $extractPath = Join-Path $ToolchainRoot $tool.extract_subdirectory
    $executablePath = Join-Path $extractPath ($tool.executable -replace "/", "\")

    if (-not (Test-Path -LiteralPath $archivePath)) {
        Write-Host "Downloading $($tool.name) $($tool.version)..."
        & $curl.Source -L --fail --retry 3 --output $archivePath $tool.download_url
        if ($LASTEXITCODE -ne 0) {
            throw "Download failed for $($tool.name)."
        }
    }

    $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $archivePath).Hash.ToLowerInvariant()
    if ($actualHash -ne $tool.sha256) {
        throw "Checksum mismatch for $archivePath. Expected $($tool.sha256), got $actualHash."
    }

    if (-not (Test-Path -LiteralPath $executablePath)) {
        if (Test-Path -LiteralPath $extractPath) {
            $existing = Get-ChildItem -LiteralPath $extractPath -Force | Select-Object -First 1
            if ($existing) {
                throw "Incomplete extraction exists at $extractPath. Review it before retrying."
            }
        } else {
            New-Item -ItemType Directory -Force $extractPath | Out-Null
        }

        Write-Host "Extracting $($tool.name) to $extractPath..."
        & $tar.Source -xf $archivePath -C $extractPath
        if ($LASTEXITCODE -ne 0) {
            throw "Extraction failed for $($tool.name)."
        }
    }

    if (-not (Test-Path -LiteralPath $executablePath)) {
        throw "Expected executable was not found after extraction: $executablePath"
    }

    Write-Host "Ready: $($tool.name) $($tool.version)"
}

Write-Host "Portable toolchain root: $ToolchainRoot"
Write-Host "Run scripts/run_all_portable.ps1 to build and validate with this toolchain."
