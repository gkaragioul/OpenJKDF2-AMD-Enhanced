[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$DataDir,
    [Parameter(Mandatory = $true)][string]$EvidenceRoot,
    [string]$Executable = "build/msvc-release/openjkdf2-64.exe"
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$evidencePath = [IO.Path]::GetFullPath($EvidenceRoot)
if (Test-Path -LiteralPath $evidencePath) { throw "EvidenceRoot must be a fresh path: $evidencePath" }
[void](New-Item -ItemType Directory -Path $evidencePath)

$doorHarness = Join-Path $PSScriptRoot "test-first-door.ps1"
$run60 = Join-Path $evidencePath "60-fps"
$run120 = Join-Path $evidencePath "120-fps"

& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $doorHarness `
    -DataDir $DataDir -UserDir $run60 -Executable $Executable -FrameCap 60 | Out-Host
if ($LASTEXITCODE -ne 0) { throw "60 FPS first-door capture failed" }
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $doorHarness `
    -DataDir $DataDir -UserDir $run120 -Executable $Executable -FrameCap 120 | Out-Host
if ($LASTEXITCODE -ne 0) { throw "120 FPS first-door capture failed" }

$result60 = Get-Content -Raw -LiteralPath (Join-Path $run60 "first-door-result.json") | ConvertFrom-Json
$result120 = Get-Content -Raw -LiteralPath (Join-Path $run120 "first-door-result.json") | ConvertFrom-Json
$budget60 = 1000.0 / 60.0
$budget120 = 1000.0 / 120.0
$median60Error = [Math]::Abs($result60.timing.median_ms - $budget60) / $budget60
$median120Error = [Math]::Abs($result120.timing.median_ms - $budget120) / $budget120
$doorMaximum = [Math]::Max($result60.timing.door_movement_ms, $result120.timing.door_movement_ms)
$doorDeltaMs = [Math]::Abs($result60.timing.door_movement_ms - $result120.timing.door_movement_ms)
$doorDeltaRatio = if ($doorMaximum) { $doorDeltaMs / $doorMaximum } else { 1.0 }

$summary = [ordered]@{
    schema = 1
    cap_60 = $result60.timing
    cap_120 = $result120.timing
    median_60_error_ratio = $median60Error
    median_120_error_ratio = $median120Error
    door_duration_delta_ms = $doorDeltaMs
    door_duration_delta_ratio = $doorDeltaRatio
    pacing_pass = $median60Error -le 0.05 -and $median120Error -le 0.05 -and `
        $result60.timing.p95_ms -le $budget60 * 1.15 -and `
        $result120.timing.p95_ms -le $budget120 * 1.15
    simulation_invariant = $doorDeltaMs -le 20 -and $doorDeltaRatio -le 0.05
}
$summaryPath = Join-Path $evidencePath "timing-invariance-result.json"
$summary | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $summaryPath -Encoding utf8
$summary | ConvertTo-Json -Depth 4
if (-not $summary.pacing_pass -or -not $summary.simulation_invariant) {
    throw "Timing invariance verification failed; inspect $summaryPath"
}
