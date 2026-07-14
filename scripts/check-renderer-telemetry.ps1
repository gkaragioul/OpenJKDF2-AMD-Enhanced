$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$renderer = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Platform\GL\std3D.c')
$window = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Win95\Window.c')
$missing = @()
foreach ($event in @('framebuffer_created', 'texture_upload', 'texture_upload_failed')) {
    if (-not $renderer.Contains($event)) { $missing += "renderer:$event" }
}
if (-not $window.Contains('swap_started')) { $missing += 'presentation:swap_started' }
if ($missing.Count) { throw ('Renderer telemetry contract missing: ' + ($missing -join ', ')) }
Write-Host 'Renderer telemetry contract passed.'
