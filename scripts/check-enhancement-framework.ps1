$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$quality = Get-Content -Raw -LiteralPath (Join-Path $root 'src\General\QualityPreset.h')
$player = Get-Content -Raw -LiteralPath (Join-Path $root 'src\World\jkPlayer.c')
$renderer = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Platform\GL\std3D.c')
$loader = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Platform\GL\jkgm.cpp')
$packageBuild = Get-Content -Raw -LiteralPath (Join-Path $root 'scripts\build-windows-package.ps1')
$packageVerify = Get-Content -Raw -LiteralPath (Join-Path $root 'scripts\verify-windows-package.ps1')
$runtimeProbe = Get-Content -Raw -LiteralPath (Join-Path $root 'scripts\test-data-overlay.ps1')
$guidePath = Join-Path $root 'packaging\windows\ENHANCEMENT-PACKS.md'
$guide = if (Test-Path -LiteralPath $guidePath) { Get-Content -Raw -LiteralPath $guidePath } else { '' }
$missing = @()

foreach ($field in @('ssaaMultiple', 'texturePrecache', 'assetEnhancements')) {
    if ($quality -notmatch ('\b' + $field + '\b')) { $missing += "preset-field:$field" }
}
if ($player -notmatch 'QualityPreset_ClampSsaa') { $missing += 'profile:ssaa-sanitized' }
if ($renderer -notmatch 'QualityPreset_ClampSsaa') { $missing += 'renderer:ssaa-sanitized' }
if ($loader -notmatch 'path_overlay_resolve_read\("jkgm/materials"') { $missing += 'loader:user-overlay' }
if ($loader -notmatch 'original_asset_fallback') { $missing += 'loader:original-fallback-telemetry' }
if ($loader -notmatch 'original_asset_fallback reason=no_matching_override') { $missing += 'loader:pack-miss-telemetry' }
if ($loader -notmatch 'texture->cache_entry\s*=\s*&path_match->second') { $missing += 'loader:path-match-entry' }
if ($loader -notmatch 'texture->cache_entry\s*=\s*&hash_match->second') { $missing += 'loader:hash-match-entry' }

foreach ($phrase in @('original assets', 'license', 'attribution', 'jkgm/materials',
                       'does not include', 'high-resolution textures', 'models')) {
    if ($guide -notmatch [regex]::Escape($phrase)) { $missing += "guide:$phrase" }
}
if ($packageBuild -notmatch 'ENHANCEMENT-PACKS\.md') { $missing += 'package:guide-copy' }
if ($packageVerify -notmatch 'ENHANCEMENT-PACKS\.md') { $missing += 'package:guide-required' }
if ($runtimeProbe -notmatch 'EmptyEnhancementPack') { $missing += 'runtime:empty-pack-mode' }
if ($runtimeProbe -notmatch 'no_matching_override') { $missing += 'runtime:fallback-assertion' }

if ($missing.Count) { throw ('Enhancement framework contract missing: ' + ($missing -join ', ')) }
Write-Host 'Enhancement framework contract passed.'
