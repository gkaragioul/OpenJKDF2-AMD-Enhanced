$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$control = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Devices\sithControl.c')
$header = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Devices\sithControl.h')
$game = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Main\jkGame.c')
$missing = @()
foreach($token in @('sithControl_HasBinding','sithControl_ValidatePresetBindings','CONTROL_PRESET_CLASSIC','DIK_S','DIK_LSHIFT','KEY_MOUSE_B1','KEY_MOUSE_B6','KEY_MOUSE_B7')) {
    if(-not $control.Contains($token) -and -not $header.Contains($token)){ $missing += $token }
}
if(-not $game.Contains('OPENJKDF2_VALIDATE_INPUT_PRESET')){ $missing += 'runtime preset selector' }
if(-not $game.Contains('bindings_valid=')){ $missing += 'binding validation telemetry' }
if($missing.Count){ throw ('Control preset mapping contract missing: '+($missing -join ', ')) }
Write-Host 'Control preset mapping contract passed.'
