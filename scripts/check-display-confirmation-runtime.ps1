$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$display = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Platform\SDL2\jkGUIDisplay.c')
$displayHeader = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Gui\jkGUIDisplay.h')
$mainMenu = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Gui\jkGUIMain.c')
$missing = @()
if (-not $mainMenu.Contains('OPENJKDF2_VALIDATE_DISPLAY_REVERT')) { $missing += 'opt-in runtime route' }
if (-not $display.Contains('jkGuiDisplay_ValidateTimedRevert') -and -not $displayHeader.Contains('jkGuiDisplay_ValidateTimedRevert')) { $missing += 'production apply/revert probe' }
foreach ($token in @('display_confirmation_validation', 'display_settings_reverted')) {
    if (-not $display.Contains($token) -and -not $mainMenu.Contains($token)) { $missing += $token }
}
if ($missing.Count) { throw ('Timed display confirmation runtime contract missing: ' + ($missing -join ', ')) }
Write-Host 'Timed display confirmation runtime contract passed.'
