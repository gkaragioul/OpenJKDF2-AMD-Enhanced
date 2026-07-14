$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$menu = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Platform\SDL2\jkGUIDisplay.c')
$strings = Get-Content -Raw -LiteralPath (Join-Path $root 'resource\ui\openjkdf2.uni')
$startup = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Main\Main.c')
$window = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Win95\Window.h')
$missing = @()

$declaredCount = [int]([regex]::Match($strings, '(?m)^MSGS\s+(\d+)').Groups[1].Value)
$actualCount = ([regex]::Matches($strings, '(?m)^\s*"[^"]+"\s+\d+\s+"')).Count
if ($declaredCount -ne $actualCount) {
    $missing += "localization-count:$declaredCount-declared-$actualCount-actual"
}

foreach ($key in @('GUIEXT_APPLY', 'GUIEXT_RESET_VIDEO', 'GUIEXT_SAFE_VIDEO',
                    'GUIEXT_RESET_VIDEO_Q', 'GUIEXT_SAFE_VIDEO_Q')) {
    if ($strings -notmatch [regex]::Escape('"' + $key + '"')) { $missing += "localization:$key" }
    if ($menu -notmatch [regex]::Escape('"' + $key + '"')) { $missing += "menu:$key" }
}

if (($menu | Select-String -Pattern 'ELEMENT_TEXTBUTTON,\s+1,\s+2,\s+"GUIEXT_APPLY"' -AllMatches).Matches.Count -lt 2) {
    $missing += 'apply-buttons:display-and-advanced'
}
foreach ($contract in @(
    'display_transaction_begin\(&transaction, proposed, SDL_GetTicks\(\), 15000\)',
    'Window_ConfirmDisplaySettings\(15000\)',
    'display_transaction_confirm\(&transaction\)',
    'display_transaction_cancel\(&transaction\)',
    'display_settings_reverted',
    'video_defaults_recommended\(\)',
    'video_defaults_safe\(\)',
    'jkPlayer_WriteConf\(jkPlayer_playerShortName\)',
    'config_recovery_snapshot\(REGISTRY_FNAME, REGISTRY_LKG_FNAME\)')) {
    $source = if ($contract -like 'config_recovery*') { $startup } else { $menu }
    if ($source -notmatch $contract) { $missing += "behavior:$contract" }
}

foreach ($api in @('Window_GetDisplayInventory', 'Window_GetDisplayName',
                    'Window_GetDisplaySettings', 'Window_ApplyDisplaySettings',
                    'Window_CommitDisplaySettings', 'Window_IsRestorationGuardReady')) {
    if ($window -notmatch [regex]::Escape($api)) { $missing += "display-api:$api" }
}

if ($missing.Count) { throw ('Video menu contract missing: ' + ($missing -join ', ')) }
Write-Host 'Video menu contract passed.'
