[CmdletBinding()]
param([string]$PackageRoot)


$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$packageWindowsRoot = if ($PackageRoot) { (Resolve-Path -LiteralPath $PackageRoot).Path } else { Join-Path $repoRoot "packaging\windows" }
$modulePath = Join-Path $packageWindowsRoot "PackageTools.psm1"
Import-Module $modulePath -Force

$root = Join-Path ([IO.Path]::GetTempPath()) ("openjkdf2-package-tools-" + [Guid]::NewGuid().ToString("N"))

function New-TestDataDirectory([string]$Path) {
    [void](New-Item -ItemType Directory -Path (Join-Path $Path "Episode") -Force)
    [void](New-Item -ItemType Directory -Path (Join-Path $Path "Resource") -Force)
    foreach ($relative in @("Episode\JK1.GOB", "Resource\Res1hi.gob", "Resource\Res2.gob", "JK.EXE")) {
        [IO.File]::WriteAllBytes((Join-Path $Path $relative), [byte[]](1, 2, 3))
    }
}

try {
    $valid = Join-Path $root "valid"
    New-TestDataDirectory $valid

    $validation = Test-JKDataDirectory -Path $valid
    if (-not $validation.Valid -or $validation.Missing.Count -ne 0) { throw "Valid data directory was rejected" }

    Remove-Item -LiteralPath (Join-Path $valid "Resource\Res2.gob")
    $invalid = Test-JKDataDirectory -Path $valid
    if ($invalid.Valid -or $invalid.Missing -notcontains "Resource\Res2.gob") { throw "Missing asset was not reported" }
    [IO.File]::WriteAllBytes((Join-Path $valid "Resource\Res2.gob"), [byte[]](1, 2, 3))

    $found = Find-JKDataDirectory -CandidatePaths @((Join-Path $root "missing"), $valid)
    if ($found -ne [IO.Path]::GetFullPath($valid)) { throw "Candidate discovery did not return the valid root" }

    $steamRoot = Join-Path $root "Steam"
    $steamInstall = Join-Path $steamRoot "steamapps\common\JK Custom"
    New-TestDataDirectory $steamInstall
    [void](New-Item -ItemType Directory -Path (Join-Path $steamRoot "steamapps") -Force)
    [IO.File]::WriteAllText(
        (Join-Path $steamRoot "steamapps\appmanifest_32380.acf"),
        '"AppState" { "appid" "32380" "installdir" "JK Custom" }'
    )
    $steamCandidates = @(Get-JKDataDirectoryCandidates -SteamRoots @($steamRoot) -ProgramRoots @() -GogRegistryRoots @() -SkipFixedDriveScan)
    if ((Find-JKDataDirectory -CandidatePaths $steamCandidates) -ne [IO.Path]::GetFullPath($steamInstall)) {
        throw "Steam app-manifest discovery failed"
    }

    $steamClient = Join-Path $root "SteamClient"
    $steamLibrary = Join-Path $root "SteamLibrary"
    $secondaryInstall = Join-Path $steamLibrary "steamapps\common\Star Wars Jedi Knight"
    New-TestDataDirectory $secondaryInstall
    [void](New-Item -ItemType Directory -Path (Join-Path $steamClient "steamapps") -Force)
    $escapedLibrary = $steamLibrary -replace '\\', '\\\\'
    [IO.File]::WriteAllText(
        (Join-Path $steamClient "steamapps\libraryfolders.vdf"),
        '"libraryfolders" { "1" { "path" "' + $escapedLibrary + '" } }'
    )
    $secondaryCandidates = @(Get-JKDataDirectoryCandidates -SteamRoots @($steamClient) -ProgramRoots @() -GogRegistryRoots @() -SkipFixedDriveScan)
    if ((Find-JKDataDirectory -CandidatePaths $secondaryCandidates) -ne [IO.Path]::GetFullPath($secondaryInstall)) {
        throw "Steam secondary-library discovery failed"
    }

    $programRoot = Join-Path $root "Program Files"
    $gogInstall = Join-Path $programRoot "GOG Galaxy\Games\Star Wars Jedi Knight - Dark Forces II"
    New-TestDataDirectory $gogInstall
    $gogCandidates = @(Get-JKDataDirectoryCandidates -SteamRoots @() -ProgramRoots @($programRoot) -GogRegistryRoots @() -SkipFixedDriveScan)
    if ((Find-JKDataDirectory -CandidatePaths $gogCandidates) -ne [IO.Path]::GetFullPath($gogInstall)) {
        throw "GOG common-location discovery failed"
    }

    $automatic = Resolve-JKDataDirectory -RequestedPath (Join-Path $root "invalid") -CandidatePaths @($steamInstall) -NoBrowse
    if (-not $automatic.Valid -or $automatic.Source -ne "automatic" -or $automatic.Path -ne [IO.Path]::GetFullPath($steamInstall)) {
        throw "Automatic resolution did not recover from an invalid requested path"
    }
    $browse = Resolve-JKDataDirectory -CandidatePaths @() -BrowseProvider { $gogInstall }
    if (-not $browse.Valid -or $browse.Source -ne "browse" -or $browse.Path -ne [IO.Path]::GetFullPath($gogInstall)) {
        throw "Browse fallback did not accept the selected valid GOG directory"
    }
    Remove-Item -LiteralPath (Join-Path $gogInstall "Resource\Res2.gob")
    $browseInvalid = Resolve-JKDataDirectory -CandidatePaths @() -BrowseProvider { $gogInstall }
    if ($browseInvalid.Valid -or $browseInvalid.Source -ne "browse" -or $browseInvalid.Missing -notcontains "Resource\Res2.gob") {
        throw "Browse fallback did not explain the selected directory's missing asset"
    }

    $launcherPackage = Join-Path $root "launcher-package"
    $launcherUser = Join-Path $root "launcher-user"
    [void](New-Item -ItemType Directory -Path $launcherPackage)
    Copy-Item -LiteralPath $modulePath -Destination (Join-Path $launcherPackage "PackageTools.psm1")
    Copy-Item -LiteralPath (Join-Path $packageWindowsRoot "Launch-OpenJKDF2.ps1") -Destination $launcherPackage
    [IO.File]::WriteAllBytes((Join-Path $launcherPackage "OpenJKDF2-AMD-Enhanced.exe"), [byte[]](0))
    $launcherOutput = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $launcherPackage "Launch-OpenJKDF2.ps1") `
        -DataDir $steamInstall -UserDir $launcherUser -NoBrowse -DiscoveryOnly
    if ($LASTEXITCODE -ne 0) { throw "Discovery-only launcher invocation failed" }
    $launcherResult = $launcherOutput | ConvertFrom-Json
    $launcherConfig = Get-Content -Raw -LiteralPath (Join-Path $launcherUser "launcher.json") | ConvertFrom-Json
    if (
        -not $launcherResult.valid -or
        $launcherResult.source -ne "requested" -or
        $launcherConfig.data_dir -ne [IO.Path]::GetFullPath($steamInstall)
    ) {
        throw "Discovery-only launcher did not persist the validated explicit directory"
    }

    $package = Join-Path $root "package"
    [void](New-Item -ItemType Directory -Path $package)
    [IO.File]::WriteAllBytes((Join-Path $package "OpenJKDF2.exe"), [byte[]](1))
    [IO.File]::WriteAllBytes((Join-Path $package "OpenAL32.dll"), [byte[]](1))
    if ((Get-PackageProprietaryFindings -Path $package).Count -ne 0) { throw "Open-source package files were falsely flagged" }
    [IO.File]::WriteAllBytes((Join-Path $package "JK1.GOB"), [byte[]](1))
    if ((Get-PackageProprietaryFindings -Path $package).Count -ne 1) { throw "Proprietary GOB was not flagged" }

    [pscustomobject]@{
        schema = 2
        asset_validation = $true
        steam_manifest_discovery = $true
        steam_secondary_library_discovery = $true
        gog_common_location_discovery = $true
        automatic_resolution = $true
        browse_valid_selection = $true
        browse_invalid_selection_explained = $true
        launcher_discovery_only = $true
        proprietary_scan = $true
    } | ConvertTo-Json
} finally {
    Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
}
