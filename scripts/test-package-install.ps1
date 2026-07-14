[CmdletBinding()]
param([string]$PackageRoot = "dist\OpenJKDF2-AMD-Enhanced-windows-x64")

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$source = (Resolve-Path -LiteralPath (Join-Path $repoRoot $PackageRoot)).Path
$root = Join-Path ([IO.Path]::GetTempPath()) ("openjkdf2-install-test-" + [Guid]::NewGuid().ToString("N"))
try {
    $desktop = Join-Path $root "Desktop"
    $install = Join-Path $root "Installed"
    $unrelated = Join-Path $root "Unrelated"
    [void](New-Item -ItemType Directory -Path $unrelated)
    $unrelatedMarker = Join-Path $unrelated "keep.txt"
    [IO.File]::WriteAllText($unrelatedMarker, "keep")
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $source "Uninstall.ps1") `
        -InstallDir $unrelated -DesktopDir $desktop
    if ($LASTEXITCODE -eq 0 -or -not (Test-Path -LiteralPath $unrelatedMarker)) {
        throw "Uninstaller did not reject an unrelated directory"
    }
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $source "Install.ps1") `
        -InstallDir $install -DesktopDir $desktop
    if ($LASTEXITCODE -ne 0) { throw "Installer failed" }
    $shortcut = Join-Path $desktop "OpenJKDF2 AMD Enhanced.lnk"
    if (-not (Test-Path -LiteralPath (Join-Path $install "OpenJKDF2-AMD-Enhanced.exe")) -or
        -not (Test-Path -LiteralPath $shortcut)) { throw "Installer output or shortcut missing" }
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $install "Uninstall.ps1") `
        -InstallDir $install -DesktopDir $desktop
    if ($LASTEXITCODE -ne 0 -or (Test-Path -LiteralPath $install) -or (Test-Path -LiteralPath $shortcut)) {
        throw "Normal uninstall did not remove application and shortcut"
    }

    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $source "Install.ps1") `
        -InstallDir $install -DesktopDir $desktop
    if ($LASTEXITCODE -ne 0) { throw "Second installer run failed" }
    $marker = Join-Path $install "UserData\player\preserve-marker.jks"
    [void](New-Item -ItemType Directory -Path (Split-Path -Parent $marker) -Force)
    [IO.File]::WriteAllText($marker, "preserve")
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $install "Uninstall.ps1") `
        -InstallDir $install -DesktopDir $desktop -PreserveInstallDirectory
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $marker) -or (Test-Path -LiteralPath $shortcut)) {
        throw "Portable-data-preserving uninstall failed"
    }
    $remaining = @(Get-ChildItem -LiteralPath $install -Recurse -File)
    if ($remaining.Count -ne 1 -or $remaining[0].FullName -ne $marker) { throw "Uninstaller left unexpected application files" }
    [pscustomobject]@{
        schema = 1
        normal_uninstall_removed_application = $true
        shortcut_removed = $true
        portable_save_preserved = $true
        original_game_directory_touched = $false
    }
} finally {
    Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
}
