[CmdletBinding()]
param(
    [string]$InstallDir = (Split-Path -Parent $MyInvocation.MyCommand.Path),
    [string]$DesktopDir = [Environment]::GetFolderPath("Desktop"),
    [switch]$PreserveInstallDirectory
)

$ErrorActionPreference = "Stop"
$target = [IO.Path]::GetFullPath($InstallDir).TrimEnd('\')
$statePath = Join-Path $target "install-state.json"
if (-not (Test-Path -LiteralPath $statePath -PathType Leaf)) {
    throw "Refusing to uninstall: install-state.json is missing from $target"
}
try { $installState = Get-Content -Raw -LiteralPath $statePath | ConvertFrom-Json } catch {
    throw "Refusing to uninstall: install-state.json is unreadable"
}
if (-not $installState.install_dir -or
    -not ([IO.Path]::GetFullPath([string]$installState.install_dir).TrimEnd('\')).Equals(
        $target, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to uninstall: install-state.json does not match $target"
}
$shortcutPath = Join-Path $DesktopDir "OpenJKDF2 AMD Enhanced.lnk"
if (Test-Path -LiteralPath $shortcutPath -PathType Leaf) {
    $removeShortcut = $false
    try {
        $shell = New-Object -ComObject WScript.Shell
        $shortcut = $shell.CreateShortcut($shortcutPath)
        $shortcutTarget = [IO.Path]::GetFullPath($shortcut.TargetPath)
        $removeShortcut = $shortcutTarget.StartsWith($target + '\', [StringComparison]::OrdinalIgnoreCase)
    } catch {}
    if ($removeShortcut) { Remove-Item -LiteralPath $shortcutPath -Force }
}

if ($PreserveInstallDirectory) {
    Get-ChildItem -LiteralPath $target -Force | Where-Object { $_.Name -ne "UserData" } |
        Remove-Item -Recurse -Force
} else {
    $localUserRoot = Join-Path $target "UserData"
    if (Test-Path -LiteralPath $localUserRoot) {
        throw "Portable UserData exists inside the install directory. Move or back it up, then rerun with -PreserveInstallDirectory to keep it."
    }
    Remove-Item -LiteralPath $target -Recurse -Force
}
Write-Output "Removed OpenJKDF2 AMD Enhanced application files. Original game files and %LOCALAPPDATA% user saves were not touched."
