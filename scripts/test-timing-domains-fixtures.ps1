[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$subject = Join-Path $PSScriptRoot 'test-timing-domains.ps1'
$temp = Join-Path $root 'build/timing-domains-fixtures'
if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp -Recurse -Force }
[void](New-Item -ItemType Directory -Path $temp)
$domains = @('weapon','ai','physics','animation','particle','script','dialogue','cutscene','level_transition')

function Write-Run([string]$Path, [int]$Cap, [string]$Mutation) {
    $lines = [System.Collections.Generic.List[string]]::new()
    $tick = 100
    foreach ($domain in $domains) {
        $start = "timing_domain_start domain=$domain simulation_tick=$tick wall_time_us=$($tick * 1000) expected_events=1 observed_events=0 status=1 reason=none frame_limit=$Cap"
        $duration = if ($Mutation -eq 'threshold' -and $domain -eq 'physics') { 400 } else { 10 }
        $end = $tick + $duration
        $complete = "timing_domain_complete domain=$domain simulation_tick=$end wall_time_us=$($end * 1000) expected_events=1 observed_events=1 status=2 reason=none frame_limit=$Cap"
        $lines.Add((@{schema=1;subsystem='timing_validation';event=$start;fields=@{}} | ConvertTo-Json -Compress))
        if (-not ($Mutation -eq 'missing' -and $domain -eq 'dialogue')) {
            $lines.Add((@{schema=1;subsystem='timing_validation';event=$complete;fields=@{}} | ConvertTo-Json -Compress))
        }
        if ($Mutation -eq 'duplicate' -and $domain -eq 'script') {
            $lines.Add((@{schema=1;subsystem='timing_validation';event=$complete;fields=@{}} | ConvertTo-Json -Compress))
        }
        $tick += 100
    }
    if ($Mutation -eq 'failed') {
        $lines[3] = (@{schema=1;subsystem='timing_validation';event='timing_domain_complete domain=ai simulation_tick=210 wall_time_us=210000 expected_events=1 observed_events=1 status=3 reason=timeout frame_limit=60';fields=@{}} | ConvertTo-Json -Compress)
    }
    if ($Mutation -eq 'malformed') { $lines.Add('{not-json') }
    $lines.Add((@{schema=1;subsystem='timing_validation';event="timing_domains_summary passed=true domains=9 frame_limit=$Cap simulation_tick=1000 wall_time_us=1000000";fields=@{}} | ConvertTo-Json -Compress))
    $lines | Set-Content -LiteralPath $Path -Encoding utf8
}

function Invoke-Fixture([string]$Name, [string]$Mutation60, [string]$Mutation120, [bool]$ShouldPass) {
    $dir = Join-Path $temp $Name
    [void](New-Item -ItemType Directory -Path $dir)
    $run60 = Join-Path $dir '60.jsonl'; $run120 = Join-Path $dir '120.jsonl'
    Write-Run $run60 60 $Mutation60
    Write-Run $run120 120 $Mutation120
    $savedPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $subject -FixtureOnly -Input60 $run60 -Input120 $run120 -OutputDir (Join-Path $dir 'out') *> (Join-Path $dir 'console.txt')
    $exitCode = $LASTEXITCODE
    $ErrorActionPreference = $savedPreference
    $passed = $exitCode -eq 0
    if ($passed -ne $ShouldPass) { throw "Fixture '$Name' expected pass=$ShouldPass but exit code was $exitCode" }
}

Invoke-Fixture 'valid' '' '' $true
Invoke-Fixture 'missing' 'missing' '' $false
Invoke-Fixture 'duplicate' 'duplicate' '' $false
Invoke-Fixture 'failed' 'failed' '' $false
Invoke-Fixture 'malformed' 'malformed' '' $false
Invoke-Fixture 'threshold' '' 'threshold' $false
Write-Host 'PASS: timing-domain harness accepted the valid fixture and rejected five invalid fixtures.'
