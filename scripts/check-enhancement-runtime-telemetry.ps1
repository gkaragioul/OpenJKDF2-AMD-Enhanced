$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$game = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Main\jkGame.c')
$missing = @()
foreach ($token in @(
    'OPENJKDF2_VALIDATE_ENHANCEMENTS_MS',
    'OPENJKDF2_VALIDATE_ENHANCEMENTS_SCREENSHOT',
    'enhancements applied preset=Ultra',
    'enhancements complete preset=Ultra',
    'total_frames=%llu',
    'QualityPreset_Get(QUALITY_PRESET_ULTRA)',
    'jkPlayer_enableBloom = settings.bloom',
    'jkPlayer_enableSSAO = settings.ssao',
    'jkPlayer_ssaaMultiple = settings.ssaaMultiple',
    'jkPlayer_bEnableJkgm = settings.assetEnhancements',
    'jkPlayer_bEnableTexturePrecache = settings.texturePrecache',
    'jkPlayer_enableVsync = PRESENTATION_VSYNC_OFF',
    'jkPlayer_fpslimit = 60',
    'FrameTelemetry_CalculateStatistics'
)) {
    if (-not $game.Contains($token)) { $missing += $token }
}
if ($missing.Count) { throw ('Enhancement runtime telemetry missing: ' + ($missing -join ', ')) }
Write-Host 'Enhancement runtime telemetry contract passed.'
