$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$window = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Win95\Window.c')
$control = Get-Content -Raw -LiteralPath (Join-Path $root 'src\Platform\SDL2\stdControl.c')
$missing = @()
foreach ($token in @('OPENJKDF2_VALIDATE_MOUSE_LATENCY', 'mouse_latency stage=dispatch', 'queue_us=')) {
    if (-not $window.Contains($token)) { $missing += "Window.c:$token" }
}
foreach ($token in @('mouse_latency stage=consume', 'total_us=', 'consume_us=')) {
    if (-not $control.Contains($token)) { $missing += "stdControl.c:$token" }
}
if ($missing.Count) { throw ('Mouse latency telemetry contract missing: ' + ($missing -join ', ')) }
Write-Host 'Mouse latency telemetry contract passed.'
