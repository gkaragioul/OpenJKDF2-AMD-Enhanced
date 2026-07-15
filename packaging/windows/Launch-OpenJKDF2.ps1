[CmdletBinding()]
param(
    [string]$DataDir,
    [string]$UserDir,
    [switch]$Portable,
    [switch]$NoBrowse,
    [switch]$DiscoveryOnly,
    [string[]]$GameArguments = @()
)

$ErrorActionPreference = "Stop"
$packageRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Import-Module (Join-Path $packageRoot "PackageTools.psm1") -Force
$executable = Join-Path $packageRoot "OpenJKDF2-AMD-Enhanced.exe"
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) { throw "The game executable is missing: $executable" }

$userRoot = if ($UserDir) {
    [IO.Path]::GetFullPath($UserDir)
} elseif ($Portable) {
    Join-Path $packageRoot "UserData"
} else {
    Join-Path $env:LOCALAPPDATA "OpenJKDF2 AMD Enhanced"
}
[void](New-Item -ItemType Directory -Path $userRoot -Force)
$launcherConfig = Join-Path $userRoot "launcher.json"
$savedDataDir = $null
if (Test-Path -LiteralPath $launcherConfig -PathType Leaf) {
    try { $savedDataDir = (Get-Content -Raw -LiteralPath $launcherConfig | ConvertFrom-Json).data_dir } catch {}
}

$resolution = Resolve-JKDataDirectory -RequestedPath $DataDir -SavedPath $savedDataDir -NoBrowse:$NoBrowse
if (-not $resolution.Valid) {
    $missing = if ($resolution.Missing.Count -gt 0) {
        $resolution.Missing -join ", "
    } else {
        "JK.EXE, Episode\JK1.GOB, Resource\Res1hi.gob, Resource\Res2.gob"
    }
    throw "A valid Jedi Knight installation was not found. Missing required files: $missing. Install the original Steam or GOG release, or select its installation folder. No game data will be downloaded or copied."
}

$DataDir = $resolution.Path
[ordered]@{
    schema = 2
    data_dir = $DataDir
    discovery_source = $resolution.Source
} | ConvertTo-Json | Set-Content -LiteralPath $launcherConfig -Encoding utf8

if ($DiscoveryOnly) {
    [pscustomobject]@{
        schema = 1
        valid = $true
        source = $resolution.Source
        data_dir = $DataDir
        config_path = $launcherConfig
    } | ConvertTo-Json
    exit 0
}

$arguments = @("--data-dir", ('"' + $DataDir + '"'))
if ($Portable) { $arguments += "--portable" }
$arguments += $GameArguments
$process = Start-Process -FilePath $executable -WorkingDirectory $packageRoot -ArgumentList $arguments -PassThru -Wait
exit $process.ExitCode
