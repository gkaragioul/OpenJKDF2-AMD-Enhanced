Set-StrictMode -Version Latest

$script:RequiredJKDataFiles = @(
    "JK.EXE",
    "Episode\JK1.GOB",
    "Resource\Res1hi.gob",
    "Resource\Res2.gob"
)

function Test-JKDataDirectory {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string]$Path)

    $missing = [Collections.Generic.List[string]]::new()
    foreach ($relative in $script:RequiredJKDataFiles) {
        $candidate = Join-Path $Path $relative
        try {
            $item = Get-Item -LiteralPath $candidate -ErrorAction Stop
            if ($item.PSIsContainer -or $item.Length -le 0) { $missing.Add($relative) }
        } catch {
            $missing.Add($relative)
        }
    }

    $resolvedPath = $Path
    if (Test-Path -LiteralPath $Path -PathType Container) {
        $resolvedPath = [IO.Path]::GetFullPath($Path)
    }
    [pscustomobject]@{
        Valid = $missing.Count -eq 0
        Path = $resolvedPath
        Missing = @($missing)
    }
}

function Add-JKCandidate {
    param(
        [Collections.Generic.List[string]]$Candidates,
        [string]$Path
    )
    if (-not [string]::IsNullOrWhiteSpace($Path)) { $Candidates.Add($Path) }
}

function Get-JKDataDirectoryCandidates {
    [CmdletBinding()]
    param(
        [string[]]$SteamRoots,
        [string[]]$ProgramRoots,
        [string[]]$GogRegistryRoots,
        [switch]$SkipFixedDriveScan
    )

    $candidates = [Collections.Generic.List[string]]::new()
    $steamRootList = [Collections.Generic.List[string]]::new()
    if ($PSBoundParameters.ContainsKey("SteamRoots")) {
        foreach ($root in @($SteamRoots)) { Add-JKCandidate $steamRootList $root }
    } else {
        foreach ($registryPath in @(
            "HKCU:\Software\Valve\Steam",
            "HKLM:\Software\WOW6432Node\Valve\Steam",
            "HKLM:\Software\Valve\Steam"
        )) {
            try {
                $value = Get-ItemProperty -LiteralPath $registryPath -ErrorAction Stop
                foreach ($name in @("SteamPath", "InstallPath")) {
                    $property = $value.PSObject.Properties[$name]
                    if ($property -and $property.Value) {
                        Add-JKCandidate $steamRootList ([string]$property.Value)
                    }
                }
            } catch {}
        }
        if (${env:ProgramFiles(x86)}) {
            Add-JKCandidate $steamRootList (Join-Path ${env:ProgramFiles(x86)} "Steam")
        }
        if ($env:ProgramFiles) {
            Add-JKCandidate $steamRootList (Join-Path $env:ProgramFiles "Steam")
        }
    }
    if (-not $SkipFixedDriveScan) {
        foreach ($drive in [IO.DriveInfo]::GetDrives()) {
            if ($drive.DriveType -eq [IO.DriveType]::Fixed) {
                Add-JKCandidate $steamRootList (Join-Path $drive.RootDirectory.FullName "SteamLibrary")
            }
        }
    }

    $steamLibraryRoots = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($steamRoot in $steamRootList) {
        [void]$steamLibraryRoots.Add($steamRoot)
        $libraryFile = Join-Path $steamRoot "steamapps\libraryfolders.vdf"
        if (Test-Path -LiteralPath $libraryFile -PathType Leaf) {
            try {
                $libraryText = [IO.File]::ReadAllText($libraryFile)
                foreach ($match in [regex]::Matches($libraryText, '"path"\s+"([^"]+)"', [Text.RegularExpressions.RegexOptions]::IgnoreCase)) {
                    $libraryRoot = $match.Groups[1].Value.Replace('\\', '\')
                    if ($libraryRoot) { [void]$steamLibraryRoots.Add($libraryRoot) }
                }
            } catch {}
        }
    }

    foreach ($libraryRoot in $steamLibraryRoots) {
        $manifest = Join-Path $libraryRoot "steamapps\appmanifest_32380.acf"
        if (Test-Path -LiteralPath $manifest -PathType Leaf) {
            try {
                $manifestText = [IO.File]::ReadAllText($manifest)
                $installMatch = [regex]::Match(
                    $manifestText,
                    '"installdir"\s+"([^"]+)"',
                    [Text.RegularExpressions.RegexOptions]::IgnoreCase
                )
                if ($installMatch.Success) {
                    $installDir = $installMatch.Groups[1].Value.Replace('\\', '\')
                    Add-JKCandidate $candidates (Join-Path $libraryRoot ("steamapps\common\" + $installDir))
                }
            } catch {}
        }
        Add-JKCandidate $candidates (Join-Path $libraryRoot "steamapps\common\Star Wars Jedi Knight")
    }

    $programRootList = if ($PSBoundParameters.ContainsKey("ProgramRoots")) {
        @($ProgramRoots)
    } else {
        @($env:ProgramFiles, ${env:ProgramFiles(x86)}, ${env:ProgramW6432})
    }
    foreach ($base in $programRootList) {
        if ($base) {
            Add-JKCandidate $candidates (Join-Path $base "GOG Galaxy\Games\Star Wars Jedi Knight - Dark Forces II")
            Add-JKCandidate $candidates (Join-Path $base "GOG.com\Star Wars Jedi Knight - Dark Forces II")
        }
    }

    $gogRootList = if ($PSBoundParameters.ContainsKey("GogRegistryRoots")) {
        @($GogRegistryRoots)
    } else {
        @(
            "HKCU:\Software\GOG.com\Games",
            "HKLM:\Software\WOW6432Node\GOG.com\Games",
            "HKLM:\Software\GOG.com\Games"
        )
    }
    foreach ($registryRoot in $gogRootList) {
        try {
            foreach ($gameKey in Get-ChildItem -LiteralPath $registryRoot -ErrorAction Stop) {
                $game = Get-ItemProperty -LiteralPath $gameKey.PSPath -ErrorAction SilentlyContinue
                foreach ($name in @("path", "gamePath")) {
                    $property = $game.PSObject.Properties[$name]
                    if ($property -and $property.Value) {
                        Add-JKCandidate $candidates ([string]$property.Value)
                    }
                }
            }
        } catch {}
    }

    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $unique = [Collections.Generic.List[string]]::new()
    foreach ($candidate in $candidates) {
        $canonical = $candidate
        try { $canonical = [IO.Path]::GetFullPath($candidate) } catch {}
        if ($seen.Add($canonical)) { $unique.Add($canonical) }
    }
    @($unique)
}

function Find-JKDataDirectory {
    [CmdletBinding()]
    param([string[]]$CandidatePaths = (Get-JKDataDirectoryCandidates))

    foreach ($candidate in @($CandidatePaths)) {
        if (-not $candidate) { continue }
        $validation = Test-JKDataDirectory -Path $candidate
        if ($validation.Valid) { return $validation.Path }
    }
    return $null
}

function New-JKDataResolution {
    param(
        [bool]$Valid,
        [string]$Path,
        [string]$Source,
        [string[]]$Missing
    )
    [pscustomobject]@{
        Valid = $Valid
        Path = $Path
        Source = $Source
        Missing = @($Missing)
    }
}

function Resolve-JKDataDirectory {
    [CmdletBinding()]
    param(
        [string]$RequestedPath,
        [string]$SavedPath,
        [string[]]$CandidatePaths,
        [switch]$NoBrowse,
        [scriptblock]$BrowseProvider
    )

    $lastInvalid = $null
    foreach ($entry in @(
        [pscustomobject]@{ Source = "requested"; Path = $RequestedPath },
        [pscustomobject]@{ Source = "saved"; Path = $SavedPath }
    )) {
        if (-not $entry.Path) { continue }
        $validation = Test-JKDataDirectory -Path $entry.Path
        if ($validation.Valid) {
            return New-JKDataResolution $true $validation.Path $entry.Source @()
        }
        $lastInvalid = New-JKDataResolution $false $validation.Path $entry.Source $validation.Missing
    }

    if (-not $PSBoundParameters.ContainsKey("CandidatePaths")) {
        $CandidatePaths = Get-JKDataDirectoryCandidates
    }
    foreach ($candidate in @($CandidatePaths)) {
        if (-not $candidate) { continue }
        $validation = Test-JKDataDirectory -Path $candidate
        if ($validation.Valid) {
            return New-JKDataResolution $true $validation.Path "automatic" @()
        }
        $lastInvalid = New-JKDataResolution $false $validation.Path "automatic" $validation.Missing
    }

    if (-not $NoBrowse) {
        if (-not $BrowseProvider) {
            $BrowseProvider = {
                Add-Type -AssemblyName System.Windows.Forms
                $browser = [Windows.Forms.FolderBrowserDialog]::new()
                try {
                    $browser.Description = "Select the legitimate Jedi Knight: Dark Forces II installation folder. No game files will be copied."
                    $browser.ShowNewFolderButton = $false
                    if ($browser.ShowDialog() -eq [Windows.Forms.DialogResult]::OK) {
                        return $browser.SelectedPath
                    }
                    return $null
                } finally {
                    $browser.Dispose()
                }
            }
        }
        $selectedPath = & $BrowseProvider
        if ($selectedPath) {
            $validation = Test-JKDataDirectory -Path $selectedPath
            return New-JKDataResolution $validation.Valid $validation.Path "browse" $validation.Missing
        }
    }

    if ($lastInvalid) { return $lastInvalid }
    return New-JKDataResolution $false $null "none" $script:RequiredJKDataFiles
}

function Get-PackageProprietaryFindings {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string]$Path)

    $assetExtensions = @(
        ".gob", ".jkl", ".mat", ".bm", ".wav", ".smk", ".san", ".mp3", ".ogg",
        ".jks", ".3do", ".key", ".cog", ".snd", ".ai", ".spr", ".cmp"
    )
    @(
        Get-ChildItem -LiteralPath $Path -Recurse -File | Where-Object {
            $assetExtensions -contains $_.Extension.ToLowerInvariant()
        } | ForEach-Object { $_.FullName.Substring([IO.Path]::GetFullPath($Path).TrimEnd('\').Length + 1) }
    )
}

Export-ModuleMember -Function Test-JKDataDirectory, Get-JKDataDirectoryCandidates, Find-JKDataDirectory, Resolve-JKDataDirectory, Get-PackageProprietaryFindings
