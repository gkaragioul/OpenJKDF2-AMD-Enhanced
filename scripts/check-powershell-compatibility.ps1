[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$failures = [System.Collections.Generic.List[string]]::new()

Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.ps1' -File | ForEach-Object {
    $contents = Get-Content -Raw -LiteralPath $_.FullName
    if ($contents -match '\.Environment(?:Variables)?\s*\[') {
        $failures.Add("$($_.Name) indexes ProcessStartInfo environment dictionaries; use a process-scoped environment variable for Windows PowerShell 5.1")
    }
}

$crashHarness = Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'test-gameplay-crash-restoration.ps1')
$clearStart = $crashHarness.IndexOf('foreach ($name in @(')
$clearEnd = $crashHarness.IndexOf('    if ($Crash)', $clearStart)
if ($clearStart -lt 0 -or $clearEnd -le $clearStart) {
    $failures.Add('test-gameplay-crash-restoration.ps1 has no pre-launch environment clear block')
} else {
    $clearBlock = $crashHarness.Substring($clearStart, $clearEnd - $clearStart)
    foreach ($name in @(
        'OPENJKDF2_VALIDATE_CRASH_MS',
        'OPENJKDF2_VALIDATE_PRESENTATION_MS',
        'OPENJKDF2_VALIDATE_PRESENTATION_VSYNC',
        'OPENJKDF2_VALIDATE_PRESENTATION_FRAME_CAP',
        'OPENJKDF2_VALIDATE_PRESENTATION_SCREENSHOT'
    )) {
        if (-not $clearBlock.Contains('"' + $name + '"')) {
            $failures.Add("test-gameplay-crash-restoration.ps1 does not clear $name before a mutually exclusive launch")
        }
    }
    if ($clearBlock -notmatch 'SetEnvironmentVariable\(\s*\$name\s*,\s*\$null\s*,') {
        $failures.Add('test-gameplay-crash-restoration.ps1 clear block does not remove each process-scoped variable')
    }
}

if ($failures.Count) {
    $failures | ForEach-Object { Write-Error $_ }
    exit 1
}

$probeName = 'OPENJKDF2_COMPATIBILITY_PROBE'
[Environment]::SetEnvironmentVariable($probeName, '1', [EnvironmentVariableTarget]::Process)
if ([Environment]::GetEnvironmentVariable($probeName, [EnvironmentVariableTarget]::Process) -ne '1') {
    Write-Error 'Process-scoped environment assignment failed'
    exit 1
}
[Environment]::SetEnvironmentVariable($probeName, $null, [EnvironmentVariableTarget]::Process)

Write-Host 'PASS: PowerShell process harnesses use the Windows PowerShell 5.1-compatible environment API.'
