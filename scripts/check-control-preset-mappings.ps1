$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$control = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Devices\sithControl.c')
$header = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Devices\sithControl.h')
$game = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Main\jkGame.c')
$player = Get-Content -Raw -LiteralPath (Join-Path $root 'src\World\jkPlayer.c')
$playerHeader = Get-Content -Raw -LiteralPath (Join-Path $root 'src\World\jkPlayer.h')
$migration = Get-Content -Raw -LiteralPath (Join-Path $root 'src\General\DefaultSettingsMigration.h')
$allSources = $control + $header + $game + $player + $playerHeader + $migration
$missing = @()
foreach($token in @(
    'sithControl_HasBinding',
    'sithControl_ValidatePresetBindings',
    'CONTROL_PRESET_CLASSIC',
    'ControlPreset_Default()',
    'sithControl_ApplyModernPreset();',
    'sithControl_CaptureClassicPresetSnapshot',
    'sithControl_MatchesCapturedClassicPreset',
    'controlpresetversion',
    'CONTROL_DEFAULTS_VERSION',
    'DIK_S',
    'DIK_LSHIFT',
    'KEY_MOUSE_B1',
    'KEY_MOUSE_B2',
    'KEY_MOUSE_B6',
    'KEY_MOUSE_B7',
    'DIK_SPACE',
    'DIK_E'
)) {
    if(-not $allSources.Contains($token)){ $missing += $token }
}
if(-not $game.Contains('OPENJKDF2_VALIDATE_INPUT_PRESET')){ $missing += 'runtime preset selector' }
if(-not $game.Contains('bindings_valid=')){ $missing += 'binding validation telemetry' }
if($player.Contains('jkPlayer_controlPreset = CONTROL_PRESET_CLASSIC')){ $missing += 'classic player default remains' }
if($player -match 'sithCvar_RegisterInt\("in_controlPreset",\s*CONTROL_PRESET_CLASSIC'){ $missing += 'classic cvar default remains' }
if($missing.Count){ throw ('Control preset mapping contract missing: '+($missing -join ', ')) }
Write-Host 'Control preset mapping contract passed.'
