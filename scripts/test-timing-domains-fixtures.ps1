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
        $duration = if ($Mutation -eq 'threshold' -and $domain -eq 'physics') {
            400
        } elseif ($Mutation -match '^(animation|dialogue)-spread-([0-9]+)$' -and $domain -eq $Matches[1]) {
            10 + [int]$Matches[2]
        } else {
            10
        }
        $end = $tick + $duration
        $wallDuration = $duration * 1000
        if ($domain -eq 'physics' -and $Mutation -eq 'wall-relative-base') { $wallDuration = 200000 }
        if ($domain -eq 'physics' -and $Mutation -eq 'wall-relative-over') { $wallDuration = 211000 }
        if ($domain -eq 'physics' -and $Mutation -eq 'wall-short-base') { $wallDuration = 28000 }
        if ($domain -eq 'physics' -and $Mutation -eq 'wall-short-within') { $wallDuration = 30000 }
        if ($domain -eq 'physics' -and $Mutation -eq 'wall-absolute-base') { $wallDuration = 2000000 }
        if ($domain -eq 'physics' -and $Mutation -eq 'wall-absolute-over') { $wallDuration = 2060000 }
        if ($domain -eq 'dialogue' -and $Mutation -eq 'media-wall-base') { $wallDuration = 2000000 }
        if ($domain -eq 'dialogue' -and $Mutation -eq 'media-wall-within') { $wallDuration = 2080000 }
        if ($domain -eq 'level_transition' -and $Mutation -eq 'level-wall-base') { $wallDuration = 800000 }
        if ($domain -eq 'level_transition' -and $Mutation -eq 'level-wall-within') { $wallDuration = 950000 }
        if ($domain -eq 'level_transition' -and $Mutation -eq 'level-wall-over') { $wallDuration = 1010000 }
        $complete = "timing_domain_complete domain=$domain simulation_tick=$end wall_time_us=$(($tick * 1000) + $wallDuration) expected_events=1 observed_events=1 status=2 reason=none frame_limit=$Cap"
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

function Invoke-SampleSpreadFixture([string]$Name, [string]$Domain, [int]$Cap, [int]$Spread, [bool]$ShouldPass) {
    $dir = Join-Path $temp $Name
    [void](New-Item -ItemType Directory -Path $dir)
    $runs60 = [System.Collections.Generic.List[string]]::new()
    $runs120 = [System.Collections.Generic.List[string]]::new()
    for ($index = 1; $index -le 3; $index++) {
        $run60 = Join-Path $dir "60-$index.jsonl"
        $run120 = Join-Path $dir "120-$index.jsonl"
        Write-Run $run60 60 $(if ($index -eq 3 -and $Cap -eq 60) { "$Domain-spread-$Spread" } else { '' })
        Write-Run $run120 120 $(if ($index -eq 3 -and $Cap -eq 120) { "$Domain-spread-$Spread" } else { '' })
        $runs60.Add($run60); $runs120.Add($run120)
    }
    $savedPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $subject -FixtureOnly `
        -Input60 ($runs60 -join ';') -Input120 ($runs120 -join ';') `
        -OutputDir (Join-Path $dir 'out') *> (Join-Path $dir 'console.txt')
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
Invoke-Fixture 'wall-relative' 'wall-relative-base' 'wall-relative-over' $false
Invoke-Fixture 'wall-short-relative-floor' 'wall-short-base' 'wall-short-within' $true
Invoke-Fixture 'wall-absolute' 'wall-absolute-base' 'wall-absolute-over' $false
Invoke-Fixture 'media-wall-exception' 'media-wall-base' 'media-wall-within' $true
Invoke-Fixture 'level-wall-within' 'level-wall-base' 'level-wall-within' $true
Invoke-Fixture 'level-wall-over' 'level-wall-base' 'level-wall-over' $false
Invoke-SampleSpreadFixture 'sample-spread-60-boundary' 'animation' 60 18 $true
Invoke-SampleSpreadFixture 'sample-spread-60-over' 'animation' 60 19 $false
Invoke-SampleSpreadFixture 'sample-spread-120-boundary' 'animation' 120 10 $true
Invoke-SampleSpreadFixture 'sample-spread-120-over' 'animation' 120 11 $false
Invoke-SampleSpreadFixture 'media-spread-120-boundary' 'dialogue' 120 18 $true
Invoke-SampleSpreadFixture 'media-spread-120-over' 'dialogue' 120 19 $false
Write-Host 'PASS: timing-domain harness accepted seven valid fixtures and rejected eleven invalid fixtures.'
