Set-StrictMode -Version Latest

function Test-JKDataDirectory {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string]$Path)

    $required = @("JK.EXE", "Episode\JK1.GOB", "Resource\Res1hi.gob", "Resource\Res2.gob")
    $missing = @()
    foreach ($relative in $required) {
        $candidate = Join-Path $Path $relative
        if (-not (Test-Path -LiteralPath $candidate -PathType Leaf) -or (Get-Item -LiteralPath $candidate).Length -le 0) {
            $missing += $relative
        }
    }
    [pscustomobject]@{
        Valid = $missing.Count -eq 0
        Path = if (Test-Path -LiteralPath $Path) { [IO.Path]::GetFullPath($Path) } else { $Path }
        Missing = $missing
    }
}

function Get-JKDataDirectoryCandidates {
    [CmdletBinding()]
    param()

    $candidates = [Collections.Generic.List[string]]::new()
    $steamRoots = [Collections.Generic.List[string]]::new()
    foreach ($registryPath in @(
        "HKCU:\Software\Valve\Steam",
        "HKLM:\Software\WOW6432Node\Valve\Steam",
        "HKLM:\Software\Valve\Steam"
    )) {
        try {
            $value = Get-ItemProperty -LiteralPath $registryPath -ErrorAction Stop
            foreach ($name in @("SteamPath", "InstallPath")) {
                if ($value.$name) { $steamRoots.Add([string]$value.$name) }
            }
        } catch {}
    }
    if (${env:ProgramFiles(x86)}) { $steamRoots.Add((Join-Path ${env:ProgramFiles(x86)} "Steam")) }
    if ($env:ProgramFiles) { $steamRoots.Add((Join-Path $env:ProgramFiles "Steam")) }
    foreach ($drive in [IO.DriveInfo]::GetDrives()) {
        if ($drive.DriveType -eq [IO.DriveType]::Fixed) {
            $steamRoots.Add((Join-Path $drive.RootDirectory.FullName "SteamLibrary"))
        }
    }
    foreach ($steamRoot in $steamRoots) {
        $candidates.Add((Join-Path $steamRoot "steamapps\common\Star Wars Jedi Knight"))
        $libraryFile = Join-Path $steamRoot "steamapps\libraryfolders.vdf"
        if (Test-Path -LiteralPath $libraryFile -PathType Leaf) {
            foreach ($line in Get-Content -LiteralPath $libraryFile -ErrorAction SilentlyContinue) {
                if ($line -match '"path"\s+"([^"]+)"') {
                    $libraryRoot = $Matches[1] -replace '\\\\', '\'
                    $candidates.Add((Join-Path $libraryRoot "steamapps\common\Star Wars Jedi Knight"))
                }
            }
        }
    }

    foreach ($base in @($env:ProgramFiles, ${env:ProgramFiles(x86)}, ${env:ProgramW6432})) {
        if ($base) {
            $candidates.Add((Join-Path $base "GOG Galaxy\Games\Star Wars Jedi Knight - Dark Forces II"))
            $candidates.Add((Join-Path $base "GOG.com\Star Wars Jedi Knight - Dark Forces II"))
        }
    }
    foreach ($registryRoot in @("HKLM:\Software\WOW6432Node\GOG.com\Games", "HKLM:\Software\GOG.com\Games")) {
        try {
            foreach ($gameKey in Get-ChildItem -LiteralPath $registryRoot -ErrorAction Stop) {
                $game = Get-ItemProperty -LiteralPath $gameKey.PSPath -ErrorAction SilentlyContinue
                if ($game.path) { $candidates.Add([string]$game.path) }
                if ($game.gamePath) { $candidates.Add([string]$game.gamePath) }
            }
        } catch {}
    }
    @($candidates | Where-Object { $_ } | Select-Object -Unique)
}

function Find-JKDataDirectory {
    [CmdletBinding()]
    param([string[]]$CandidatePaths = (Get-JKDataDirectoryCandidates))

    foreach ($candidate in $CandidatePaths) {
        $validation = Test-JKDataDirectory -Path $candidate
        if ($validation.Valid) { return $validation.Path }
    }
    return $null
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

Export-ModuleMember -Function Test-JKDataDirectory, Get-JKDataDirectoryCandidates, Find-JKDataDirectory, Get-PackageProprietaryFindings
