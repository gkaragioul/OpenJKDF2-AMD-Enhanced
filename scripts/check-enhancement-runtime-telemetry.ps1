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
$harness = Get-Content -Raw -LiteralPath (Join-Path $root 'scripts\test-enhancement-performance.ps1')
foreach ($token in @(
    'ConvertFrom-Json',
    'Get-WinEvent',
    'shader_stage_events',
    'distinct_shader_stages',
    'shader_link_events',
    'distinct_shader_programs',
    'incomplete_framebuffers',
    'texture_upload_errors',
    'diagnostic_errors',
    'software_renderer_markers',
    'application_errors',
    'minimum_frames'
)) {
    if (-not $harness.Contains($token)) { $missing += $token }
}

if ($missing.Count) { throw ('Enhancement runtime telemetry missing: ' + ($missing -join ', ')) }
Write-Host 'Enhancement runtime telemetry contract passed.'
