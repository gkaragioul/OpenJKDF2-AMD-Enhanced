[CmdletBinding()]
param(
    [string]$InstallDir = (Join-Path $env:LOCALAPPDATA "Programs\OpenJKDF2 AMD Enhanced"),
    [string]$DesktopDir = [Environment]::GetFolderPath("Desktop"),
    [switch]$NoShortcut
)

$ErrorActionPreference = "Stop"
$source = Split-Path -Parent $MyInvocation.MyCommand.Path
$target = [IO.Path]::GetFullPath($InstallDir)
if ($target.Equals([IO.Path]::GetFullPath($source), [StringComparison]::OrdinalIgnoreCase)) {
    throw "Choose an install directory different from the extracted package"
}
[void](New-Item -ItemType Directory -Path $target -Force)
foreach ($item in Get-ChildItem -LiteralPath $source -Force) {
    if ($item.Name -in @("UserData", "install-state.json")) { continue }
    Copy-Item -LiteralPath $item.FullName -Destination $target -Recurse -Force
}

$shortcutPath = $null
if (-not $NoShortcut) {
    [void](New-Item -ItemType Directory -Path $DesktopDir -Force)
    $shortcutPath = Join-Path $DesktopDir "OpenJKDF2 AMD Enhanced.lnk"
    $shell = New-Object -ComObject WScript.Shell
    $shortcut = $shell.CreateShortcut($shortcutPath)
    $shortcut.TargetPath = Join-Path $target "OpenJKDF2 AMD Enhanced.cmd"
    $shortcut.WorkingDirectory = $target
    $shortcut.IconLocation = (Join-Path $target "OpenJKDF2-AMD-Enhanced.exe") + ",0"
    $shortcut.Description = "Community OpenJKDF2 source port; requires legally owned Jedi Knight data"
    $shortcut.Save()
}
@{ schema = 1; install_dir = $target; shortcut = $shortcutPath } | ConvertTo-Json |
    Set-Content -LiteralPath (Join-Path $target "install-state.json") -Encoding utf8
Write-Output "Installed OpenJKDF2 AMD Enhanced to $target"
if ($shortcutPath) { Write-Output "Created desktop shortcut $shortcutPath" }
