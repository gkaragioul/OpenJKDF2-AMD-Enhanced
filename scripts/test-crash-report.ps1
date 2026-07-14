[CmdletBinding()]
param(
    [string]$BuildDir = "build/msvc-release",
    [string]$EvidenceRoot = "runtime-evidence/crash-report-probe",
    [int]$TimeoutSeconds = 15
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot $BuildDir))
$evidencePath = [IO.Path]::GetFullPath((Join-Path $repoRoot $EvidenceRoot))
$probe = Join-Path $buildRoot "openjkdf2-crash-probe.exe"
if (-not (Test-Path -LiteralPath $probe -PathType Leaf)) {
    throw "Crash-report probe executable is missing: $probe"
}
if (Test-Path -LiteralPath $evidencePath) { Remove-Item -LiteralPath $evidencePath -Recurse -Force }
[void](New-Item -ItemType Directory -Path $evidencePath -Force)

foreach ($runtime in @("exchndl.dll", "mgwhelp.dll", "symsrv.dll", "symsrv.yes")) {
    $source = Join-Path $buildRoot $runtime
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw "Missing crash runtime: $source" }
    Copy-Item -LiteralPath $source -Destination (Join-Path $evidencePath $runtime)
}
Copy-Item -LiteralPath $probe -Destination (Join-Path $evidencePath "openjkdf2-crash-probe.exe")

$report = Join-Path $evidencePath "OpenJKDF2-crash.RPT"
$process = Start-Process -FilePath (Join-Path $evidencePath "openjkdf2-crash-probe.exe") `
    -WorkingDirectory $evidencePath -ArgumentList @('"' + $report + '"') -PassThru
$deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
while ([DateTime]::UtcNow -lt $deadline -and -not (Test-Path -LiteralPath $report -PathType Leaf)) {
    Start-Sleep -Milliseconds 100
}
if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
$process.WaitForExit()

if (-not (Test-Path -LiteralPath $report -PathType Leaf)) { throw "DrMinGW did not create a crash report" }
$reportInfo = Get-Item -LiteralPath $report
if ($reportInfo.Length -lt 256) { throw "Crash report is unexpectedly small: $($reportInfo.Length) bytes" }
$reportText = Get-Content -Raw -LiteralPath $report
if ($reportText -notmatch '(?i)exception|access violation|stack') {
    throw "Crash report does not contain recognizable exception or stack information"
}

$result = [ordered]@{
    schema = 1
    report_created = $true
    report_bytes = $reportInfo.Length
    contains_exception_or_stack = $true
    process_exit_code = if ($process.HasExited) { $process.ExitCode } else { $null }
}
$result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $evidencePath "crash-report-result.json") -Encoding utf8
$result | ConvertTo-Json
