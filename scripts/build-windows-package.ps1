[CmdletBinding()]
param(
    [string]$BuildDir = "build/msvc-release",
    [string]$OutputDir = "dist"
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildRoot = (Resolve-Path -LiteralPath (Join-Path $repoRoot $BuildDir)).Path
$outputRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot $OutputDir))
$packageName = "OpenJKDF2-AMD-Enhanced-windows-x64"
$stage = Join-Path $outputRoot $packageName
$zipPath = Join-Path $outputRoot ($packageName + ".zip")
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
[void](New-Item -ItemType Directory -Path $stage -Force)
[void](New-Item -ItemType Directory -Path (Join-Path $stage "Licenses") -Force)

$binaryMap = [ordered]@{
    "openjkdf2-64.exe" = "OpenJKDF2-AMD-Enhanced.exe"
    "openjkdf2-renderer-smoke.exe" = "OpenJKDF2-Renderer-Smoke.exe"
    "openjkdf2-display-watchdog.exe" = "OpenJKDF2-Display-Watchdog.exe"
    "OpenAL32.dll" = "OpenAL32.dll"
    "exchndl.dll" = "exchndl.dll"
    "mgwhelp.dll" = "mgwhelp.dll"
    "symsrv.dll" = "symsrv.dll"
    "symsrv.yes" = "symsrv.yes"
}
foreach ($entry in $binaryMap.GetEnumerator()) {
    $source = Join-Path $buildRoot $entry.Key
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw "Required Release artifact is missing: $source" }
    Copy-Item -LiteralPath $source -Destination (Join-Path $stage $entry.Value)
}

$packageSource = Join-Path $repoRoot "packaging\windows"
foreach ($name in @(
    "PackageTools.psm1", "Launch-OpenJKDF2.ps1", "OpenJKDF2 AMD Enhanced.cmd",
    "OpenJKDF2 AMD Enhanced Portable.cmd", "Install.ps1", "Uninstall.ps1",
    "README-PACKAGE.md", "CONFIGURATION.example.json", "THIRD-PARTY-NOTICES.md",
    "TROUBLESHOOTING.md", "CHANGELOG.md", "ENHANCEMENT-PACKS.md"
)) {
    Copy-Item -LiteralPath (Join-Path $packageSource $name) -Destination (Join-Path $stage $name)
}
Copy-Item -LiteralPath (Join-Path $repoRoot "LICENSE.md") -Destination (Join-Path $stage "LICENSE.md")
Copy-Item -LiteralPath (Join-Path $repoRoot "LICENSING.md") -Destination (Join-Path $stage "LICENSING.md")
Copy-Item -LiteralPath (Join-Path $repoRoot "docs\building-windows.md") -Destination (Join-Path $stage "BUILDING.md")
Copy-Item -LiteralPath (Join-Path $repoRoot "docs\diagnostics.md") -Destination (Join-Path $stage "DIAGNOSTICS.md")

$licenseMap = [ordered]@{
    "lib\SDL\LICENSE.txt" = "SDL-LICENSE.txt"
    "lib\SDL_mixer\LICENSE.txt" = "SDL_mixer-LICENSE.txt"
    "lib\SDL_mixer\external\ogg\COPYING" = "Ogg-COPYING.txt"
    "lib\SDL_mixer\external\vorbis\COPYING" = "Vorbis-COPYING.txt"
    "lib\SDL_mixer\external\opus\COPYING" = "Opus-COPYING.txt"
    "lib\SDL_mixer\external\opusfile\COPYING" = "Opusfile-COPYING.txt"
    "lib\SDL_mixer\src\dr_libs\LICENSE" = "dr_libs-LICENSE.txt"
    "lib\openal\COPYING" = "OpenAL-Soft-COPYING.txt"
    "lib\freeglut\COPYING" = "FreeGLUT-COPYING.txt"
    "lib\glew\LICENSE.txt" = "GLEW-LICENSE.txt"
    "lib\zlib\LICENSE" = "zlib-LICENSE.txt"
    "lib\libpng\LICENSE" = "libpng-LICENSE.txt"
    "src\external\libsmacker\COPYING" = "libsmacker-COPYING.txt"
    "src\external\libsmusher\LICENSE" = "libsmusher-LICENSE.txt"
    "src\external\nativefiledialog-extended\LICENSE" = "nativefiledialog-LICENSE.txt"
    "src\external\fcaseopen\LICENSE.txt" = "fcaseopen-LICENSE.txt"
    "3rdparty\json\LICENSE.MIT" = "nlohmann-json-LICENSE.txt"
    "3rdparty\drmingw-0.9.3-win64\doc\LICENSE.txt" = "DrMinGW-LICENSE.txt"
    "3rdparty\drmingw-0.9.3-win64\doc\LICENSE-zlib.txt" = "DrMinGW-zlib-LICENSE.txt"
    "3rdparty\drmingw-0.9.3-win64\doc\LICENSE-libdwarf.txt" = "DrMinGW-libdwarf-LICENSE.txt"
}
foreach ($entry in $licenseMap.GetEnumerator()) {
    Copy-Item -LiteralPath (Join-Path $repoRoot $entry.Key) -Destination (Join-Path $stage "Licenses\$($entry.Value)")
}

$commit = (& git -C $repoRoot rev-parse HEAD).Trim()
& git -C $repoRoot diff --quiet
$dirty = $LASTEXITCODE -ne 0
@{
    schema = 1
    product = "OpenJKDF2 AMD Enhanced"
    source_commit = $commit
    source_dirty = $dirty
    configuration = "Release"
    architecture = "x64"
    generated_utc = [DateTime]::UtcNow.ToString("o")
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $stage "BUILD-PROVENANCE.json") -Encoding utf8

Import-Module (Join-Path $packageSource "PackageTools.psm1") -Force
$findings = Get-PackageProprietaryFindings -Path $stage
if ($findings.Count) { throw "Proprietary-looking files found in stage: $($findings -join ', ')" }

$manifestFiles = @(
    Get-ChildItem -LiteralPath $stage -Recurse -File | Sort-Object FullName | ForEach-Object {
        [ordered]@{
            path = $_.FullName.Substring($stage.Length + 1).Replace('\', '/')
            size = $_.Length
            sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    }
)
@{ schema = 1; product = "OpenJKDF2 AMD Enhanced"; files = $manifestFiles } | ConvertTo-Json -Depth 5 |
    Set-Content -LiteralPath (Join-Path $stage "PACKAGE-MANIFEST.json") -Encoding utf8

Compress-Archive -LiteralPath $stage -DestinationPath $zipPath -CompressionLevel Optimal
$zipHash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
"$zipHash  $([IO.Path]::GetFileName($zipPath))" | Set-Content -LiteralPath (Join-Path $outputRoot "SHA256SUMS.txt") -Encoding ascii
[pscustomobject]@{ package = $zipPath; sha256 = $zipHash; staged_files = $manifestFiles.Count }
