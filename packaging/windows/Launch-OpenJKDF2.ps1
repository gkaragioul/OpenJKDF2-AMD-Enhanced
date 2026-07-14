[CmdletBinding()]
param(
    [string]$DataDir,
    [switch]$Portable,
    [switch]$NoBrowse,
    [string[]]$GameArguments = @()
)

$ErrorActionPreference = "Stop"
$packageRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Import-Module (Join-Path $packageRoot "PackageTools.psm1") -Force
$executable = Join-Path $packageRoot "OpenJKDF2-AMD-Enhanced.exe"
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) { throw "The game executable is missing: $executable" }

$userRoot = if ($Portable) {
    Join-Path $packageRoot "UserData"
} else {
    Join-Path $env:LOCALAPPDATA "OpenJKDF2 AMD Enhanced"
}
[void](New-Item -ItemType Directory -Path $userRoot -Force)
$launcherConfig = Join-Path $userRoot "launcher.json"
if (-not $DataDir -and (Test-Path -LiteralPath $launcherConfig -PathType Leaf)) {
    try { $DataDir = (Get-Content -Raw -LiteralPath $launcherConfig | ConvertFrom-Json).data_dir } catch {}
}
if (-not $DataDir -or -not (Test-JKDataDirectory -Path $DataDir).Valid) {
    $DataDir = Find-JKDataDirectory
}
if ((-not $DataDir -or -not (Test-JKDataDirectory -Path $DataDir).Valid) -and -not $NoBrowse) {
    Add-Type -AssemblyName System.Windows.Forms
    $browser = [Windows.Forms.FolderBrowserDialog]::new()
    $browser.Description = "Select the legitimate Jedi Knight: Dark Forces II installation folder. No game files will be copied."
    $browser.ShowNewFolderButton = $false
    if ($browser.ShowDialog() -eq [Windows.Forms.DialogResult]::OK) { $DataDir = $browser.SelectedPath }
}
$validation = if ($DataDir) { Test-JKDataDirectory -Path $DataDir } else { $null }
if (-not $validation -or -not $validation.Valid) {
    $missing = if ($validation) { $validation.Missing -join ", " } else { "JK.EXE, Episode\JK1.GOB, Resource\Res1hi.gob, Resource\Res2.gob" }
    throw "A valid Jedi Knight installation was not found. Missing required files: $missing. Install the original Steam or GOG release, then run this launcher again."
}
$DataDir = $validation.Path
@{ schema = 1; data_dir = $DataDir } | ConvertTo-Json | Set-Content -LiteralPath $launcherConfig -Encoding utf8

$arguments = @("--data-dir", ('"' + $DataDir + '"'))
if ($Portable) { $arguments += "--portable" }
$arguments += $GameArguments
$process = Start-Process -FilePath $executable -WorkingDirectory $packageRoot -ArgumentList $arguments -PassThru -Wait
exit $process.ExitCode
