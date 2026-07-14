[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$ZipPath,
    [Parameter(Mandatory = $true)][string]$DataDir,
    [Parameter(Mandatory = $true)][string]$EvidenceRoot
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$archive = (Resolve-Path -LiteralPath $ZipPath).Path
$assetRoot = (Resolve-Path -LiteralPath $DataDir).Path
$root = [IO.Path]::GetFullPath($EvidenceRoot)
if (Test-Path -LiteralPath $root) { throw "EvidenceRoot must be a fresh path: $root" }
[void](New-Item -ItemType Directory -Path $root)
$extract = Join-Path $root "Extracted"
$install = Join-Path $root "Installed"
$desktop = Join-Path $root "Desktop"
$user = Join-Path $root "UserData"

& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "verify-windows-package.ps1") -ZipPath $archive
if ($LASTEXITCODE -ne 0) { throw "Package audit failed" }
Expand-Archive -LiteralPath $archive -DestinationPath $extract
$packageRoots = @(Get-ChildItem -LiteralPath $extract -Directory)
if ($packageRoots.Count -ne 1) { throw "Unexpected package layout" }
$package = $packageRoots[0].FullName

& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $package "Install.ps1") `
    -InstallDir $install -DesktopDir $desktop
if ($LASTEXITCODE -ne 0) { throw "Clean install failed" }
$shortcut = Join-Path $desktop "OpenJKDF2 AMD Enhanced.lnk"
if (-not (Test-Path -LiteralPath $shortcut -PathType Leaf)) { throw "Desktop shortcut was not created" }

Import-Module (Join-Path $install "PackageTools.psm1") -Force
$discovered = Find-JKDataDirectory
if (-not $discovered -or -not $discovered.Equals($assetRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Installed first-run discovery did not locate the legitimate game directory"
}

$installedExe = Join-Path $install "OpenJKDF2-AMD-Enhanced.exe"
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "test-save-load-lifecycle.ps1") `
    -DataDir $assetRoot -UserDir $user -Executable $installedExe
if ($LASTEXITCODE -ne 0) { throw "Installed gameplay/save acceptance failed" }
$saveResult = Get-Content -Raw -LiteralPath (Join-Path $user "save-load-result.json") | ConvertFrom-Json

& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $install "Uninstall.ps1") `
    -InstallDir $install -DesktopDir $desktop
if ($LASTEXITCODE -ne 0) { throw "Clean uninstall failed" }
$summary = [ordered]@{
    schema = 1
    package_sha256 = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
    automatic_data_discovery = $true
    installed_gameplay_launched = $saveResult.same_process_exit -eq 1
    save_created = $saveResult.validation_save_exists -and $saveResult.validation_save_bytes -gt 0
    save_restored_same_process = $saveResult.same_process_restored
    save_restored_fresh_process = $saveResult.fresh_process_restored
    display_invariant = $saveResult.display_invariant
    original_assets_invariant = $saveResult.asset_metadata_invariant
    application_removed = -not (Test-Path -LiteralPath $install)
    shortcut_removed = -not (Test-Path -LiteralPath $shortcut)
    user_data_preserved = Test-Path -LiteralPath (Join-Path $user "save-load-result.json")
}
$summaryPath = Join-Path $root "clean-install-result.json"
$summary | ConvertTo-Json | Set-Content -LiteralPath $summaryPath -Encoding utf8
$summary | ConvertTo-Json
if ($summary.Values -contains $false) { throw "Clean-install acceptance failed; inspect $summaryPath" }
