$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$menu = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Platform\SDL2\jkGUIDisplay.c')
$strings = Get-Content -Raw -LiteralPath (Join-Path $root 'resource\ui\openjkdf2.uni')
$startup = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Main\Main.c')
$gameplay = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Main\jkGame.c')
$window = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Win95\Window.h')
$windowImpl = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Win95\Window.c')
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

foreach ($key in @('GUIEXT_DISPLAY_OPTIONS', 'GUIEXT_DISPLAY_MODE',
                    'GUIEXT_DISPLAY_MONITOR', 'GUIEXT_DISPLAY_RESOLUTION',
                    'GUIEXT_DISPLAY_WIDTH', 'GUIEXT_DISPLAY_HEIGHT',
                    'GUIEXT_DISPLAY_REFRESH', 'GUIEXT_DISPLAY_EFFECTIVE',
                    'GUIEXT_EXCLUSIVE_UNAVAILABLE')) {
    if ($strings -notmatch [regex]::Escape('"' + $key + '"')) { $missing += "localization:$key" }
    if ($menu -notmatch [regex]::Escape('"' + $key + '"')) { $missing += "display-options-menu:$key" }
}
foreach ($label in @('Windowed', 'Borderless Fullscreen (Recommended)', 'Exclusive Fullscreen')) {
    if (-not $menu.Contains($label)) { $missing += "display-mode-label:$label" }
}
foreach ($migrationContract in @('Window_defaultsVersion', 'default_settings_migrate_display', 'WINDOW_DEFAULTS_VERSION')) {
    if (-not $windowImpl.Contains($migrationContract)) { $missing += "display-default-migration:$migrationContract" }
}
foreach ($contract in @('jkGuiDisplay_ShowDisplayOptions', 'Window_GetDisplayInventory',
                         'Window_GetDisplaySettings', 'Window_ApplyDisplaySettings',
                         'Window_CommitDisplaySettings')) {
    if ($menu -notmatch [regex]::Escape($contract)) { $missing += "display-options-behavior:$contract" }
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

foreach ($key in @('Window_displayMonitor', 'Window_windowWidth',
                    'Window_windowHeight', 'Window_refreshHz')) {
    if ($windowImpl -notmatch ('wuRegistry_SaveInt\("' + [regex]::Escape($key) + '"')) {
        $missing += "display-persistence:$key"
    }
}
if ($windowImpl -notmatch 'Window_bDeferDisplayPersistence') {
    $missing += 'display-persistence:confirmation-only'
}
if (-not $windowImpl.Contains('window_focus_gained')) {
    $missing += 'display-lifecycle:window_focus_gained'
}
if (-not $windowImpl.Contains('window_focus_lost')) {
    $missing += 'display-lifecycle:window_focus_lost'
}
foreach ($crashContract in @('OPENJKDF2_VALIDATE_CRASH_MS', 'gameplay_crash requested=true')) {
    if (-not $gameplay.Contains($crashContract)) {
        $missing += "display-lifecycle:$crashContract"
    }
}
if ($windowImpl -notmatch 'if\s*\(settings\.mode\s*==\s*DISPLAY_MODE_WINDOWED\)') {
    $missing += 'display-persistence:preserve-windowed-size-in-borderless'
}
foreach ($dimension in @('Width', 'Height')) {
    if ($windowImpl -notmatch ('wuRegistry_SaveInt\("Window_window' + $dimension + '",\s*Window_window' + $dimension + '\)')) {
        $missing += "display-persistence:committed-window-$($dimension.ToLower())"
    }
}

if ($missing.Count) { throw ('Video menu contract missing: ' + ($missing -join ', ')) }
Write-Host 'Video menu contract passed.'
