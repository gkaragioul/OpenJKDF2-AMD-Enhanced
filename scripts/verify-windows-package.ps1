[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$ZipPath)

$ErrorActionPreference = "Stop"
$archive = (Resolve-Path -LiteralPath $ZipPath).Path
$temp = Join-Path ([IO.Path]::GetTempPath()) ("openjkdf2-package-audit-" + [Guid]::NewGuid().ToString("N"))
try {
    Expand-Archive -LiteralPath $archive -DestinationPath $temp
    $roots = @(Get-ChildItem -LiteralPath $temp -Directory)
    if ($roots.Count -ne 1) { throw "Package must contain exactly one top-level directory" }
    $root = $roots[0].FullName
    foreach ($required in @(
        "OpenJKDF2-AMD-Enhanced.exe", "OpenAL32.dll", "Launch-OpenJKDF2.ps1",
        "OpenJKDF2 AMD Enhanced.cmd", "OpenJKDF2 AMD Enhanced Portable.cmd",
        "Install.ps1", "Uninstall.ps1", "LICENSE.md", "THIRD-PARTY-NOTICES.md",
        "TROUBLESHOOTING.md", "CHANGELOG.md", "CONFIGURATION.example.json",
        "PACKAGE-MANIFEST.json", "BUILD-PROVENANCE.json"
    )) {
        if (-not (Test-Path -LiteralPath (Join-Path $root $required) -PathType Leaf)) { throw "Required package file missing: $required" }
    }

    Import-Module (Join-Path $root "PackageTools.psm1") -Force
    $findings = Get-PackageProprietaryFindings -Path $root
    if ($findings.Count) { throw "Proprietary-looking files found: $($findings -join ', ')" }
    if (Get-ChildItem -LiteralPath $root -Recurse -Directory | Where-Object { $_.Name -in @("Episode", "Resource", "player", "runtime-evidence") }) {
        throw "Package contains a prohibited game-data or runtime-evidence directory"
    }

    $manifest = Get-Content -Raw -LiteralPath (Join-Path $root "PACKAGE-MANIFEST.json") | ConvertFrom-Json
    foreach ($entry in $manifest.files) {
        $file = Join-Path $root ($entry.path -replace '/', '\')
        if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "Manifest file missing: $($entry.path)" }
        if ((Get-Item -LiteralPath $file).Length -ne $entry.size) { throw "Manifest size mismatch: $($entry.path)" }
        $hash = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($hash -ne $entry.sha256) { throw "Manifest hash mismatch: $($entry.path)" }
    }
    $unlisted = @(
        Get-ChildItem -LiteralPath $root -Recurse -File | Where-Object { $_.Name -ne "PACKAGE-MANIFEST.json" } |
            ForEach-Object { $_.FullName.Substring($root.Length + 1).Replace('\', '/') } |
            Where-Object { $_ -notin @($manifest.files.path) }
    )
    if ($unlisted.Count) { throw "Unlisted package files found: $($unlisted -join ', ')" }
    [pscustomobject]@{
        schema = 1
        package = [IO.Path]::GetFileName($archive)
        sha256 = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
        files_verified = @($manifest.files).Count
        proprietary_findings = 0
        manifest_valid = $true
    }
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
