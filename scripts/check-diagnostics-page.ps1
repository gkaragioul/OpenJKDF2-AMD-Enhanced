$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$header = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Gui\jkGUIDisplay.h')
$display = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Platform\SDL2\jkGUIDisplay.c')
$mainMenu = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Gui\jkGUIMain.c')
$missing = @()
if (-not $header.Contains('jkGuiDisplay_ShowDiagnostics')) { $missing += 'public diagnostics entry point' }
if (-not $display.Contains('diagnostics_page displayed=true')) { $missing += 'displayed telemetry' }
if (-not $display.Contains('diagnostics_page dismissed=true')) { $missing += 'dismissed telemetry' }
if (-not $mainMenu.Contains('OPENJKDF2_VALIDATE_DIAGNOSTICS_PAGE')) { $missing += 'opt-in menu-context validation hook' }
if (-not $mainMenu.Contains('OPENJKDF2_VALIDATE_DISPLAY_PAGE')) { $missing += 'opt-in display-page validation hook' }
if ($missing.Count) { throw ('Diagnostics page contract missing: ' + ($missing -join ', ')) }
Write-Host 'Diagnostics page contract passed.'
