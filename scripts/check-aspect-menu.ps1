$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$source = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Platform\SDL2\jkGUIDisplay.c')
$strings = Get-Content -Raw -LiteralPath (Join-Path $root 'resource\ui\openjkdf2.uni')

$localizationKeys = @(
    'GUIEXT_ASPECT_GAMEPLAY',
    'GUIEXT_ASPECT_MENUS',
    'GUIEXT_ASPECT_HUD',
    'GUIEXT_ASPECT_VIDEOS'
)

$settingBindings = @(
    'jkPlayer_enableOrigAspect',
    'jkPlayer_preserveMenuAspect',
    'jkPlayer_preserveHudAspect',
    'jkPlayer_preserveVideoAspect'
)

$missing = @()
foreach ($key in $localizationKeys) {
    if ($strings -notmatch [regex]::Escape('"' + $key + '"')) {
        $missing += "localization:$key"
    }
    if ($source -notmatch [regex]::Escape('"' + $key + '"')) {
        $missing += "menu-label:$key"
    }
}

foreach ($binding in $settingBindings) {
    if ($source -notmatch ('selectedTextEntry\s*=\s*' + [regex]::Escape($binding))) {
        $missing += "checkbox-load:$binding"
    }
    if ($source -notmatch ([regex]::Escape($binding) + '\s*=\s*[^;]*selectedTextEntry')) {
        $missing += "checkbox-save:$binding"
    }
}

if ($source -notmatch 'GUI_ASPECT_OPTIONS' -or $source -notmatch 'jkGuiDisplay_ShowAspectOptions') {
    $missing += 'aspect-options-dialog'
}

if ($missing.Count -gt 0) {
    throw ('Aspect menu contract missing: ' + ($missing -join ', '))
}

Write-Host 'Aspect menu contract passed.'
