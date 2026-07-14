$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$requiredFiles = @(
    'docs\building-windows.md',
    'docs\architecture.md',
    'docs\timing-design.md',
    'docs\upstreaming.md',
    'COMPATIBILITY.md',
    'BENCHMARKS.md',
    'FINAL-REPORT.md',
    'LICENSE.md',
    'packaging\windows\THIRD-PARTY-NOTICES.md',
    'packaging\windows\TROUBLESHOOTING.md'
)
$missing = @()
foreach ($relative in $requiredFiles) {
    $path = Join-Path $root $relative
    if (-not (Test-Path -LiteralPath $path -PathType Leaf) -or (Get-Item -LiteralPath $path).Length -lt 200) {
        $missing += $relative
    }
}
$readme = Get-Content -Raw -LiteralPath (Join-Path $root 'README.md')
$report = Get-Content -Raw -LiteralPath (Join-Path $root 'FINAL-REPORT.md')
$workflow = Get-Content -Raw -LiteralPath (Join-Path $root '.github\workflows\win64.yml')
foreach ($token in @('OpenJKDF2 AMD Enhanced', 'community fork', 'not affiliated', 'game assets')) {
    if (-not $readme.Contains($token)) { $missing += "README:$token" }
}
foreach ($token in @('requirements.csv', 'Unresolved limitations', 'Reproduction and artifacts')) {
    if (-not $report.Contains($token)) { $missing += "FINAL-REPORT:$token" }
}
foreach ($token in @('windows-latest', '-Configuration Release', '-Test')) {
    if (-not $workflow.Contains($token)) { $missing += "win64.yml:$token" }
}
if ($report.Contains('Live display-change timed-dialog interaction')) {
    $missing += 'FINAL-REPORT:stale display-dialog limitation'
}
if ($missing.Count) { throw ('Release documentation contract missing: ' + ($missing -join ', ')) }
Write-Host 'Release documentation contract passed.'
