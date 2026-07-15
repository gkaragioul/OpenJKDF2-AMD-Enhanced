[CmdletBinding()]
param(
    [string] $Executable = 'build/msvc-release/openjkdf2-64.exe',
    [string] $DataDir,
    [Parameter(Mandatory = $true)][string] $OutputDir,
    [switch] $FixtureOnly,
    [string] $Input60,
    [string] $Input120,
    [switch] $Run120First,
    [int] $TimeoutSeconds = 180
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$domainNames = @('weapon','ai','physics','animation','particle','script','dialogue','cutscene','level_transition')
$runtimeSampleCount = 3
$invariant = [Globalization.CultureInfo]::InvariantCulture

function Convert-TimingLog([string] $Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Missing timing log: $Path" }
    $starts = @{}; $completes = @{}; $summary = $null
    foreach ($line in Get-Content -LiteralPath $Path) {
        try { $entry = $line | ConvertFrom-Json -ErrorAction Stop } catch { throw "Malformed JSON in ${Path}: $line" }
        if ($entry.schema -ne 1) { throw "Unsupported schema in $Path" }
        if ($entry.subsystem -ne 'timing_validation') { continue }
        $event = [string]$entry.event
        if ($event -match '^timing_domain_(start|complete) domain=([a-z_]+) simulation_tick=([0-9]+) wall_time_us=([0-9]+) expected_events=([0-9]+) observed_events=([0-9]+) status=([0-9]+) reason=([a-z_]+) frame_limit=([0-9]+)$') {
            $kind = $Matches[1]; $domain = $Matches[2]
            if ($domain -notin $domainNames) { throw "Unknown timing domain '$domain'" }
            $record = [pscustomobject]@{
                domain = $domain
                simulation_tick = [uint64]::Parse($Matches[3], $invariant)
                wall_time_us = [uint64]::Parse($Matches[4], $invariant)
                expected_events = [int]$Matches[5]
                observed_events = [int]$Matches[6]
                status = [int]$Matches[7]
                reason = $Matches[8]
                frame_limit = [int]$Matches[9]
            }
            $table = if ($kind -eq 'start') { $starts } else { $completes }
            if ($table.ContainsKey($domain)) { throw "Duplicate $kind record for '$domain'" }
            $table[$domain] = $record
        } elseif ($event -match '^timing_domains_summary passed=(true|false) domains=([0-9]+) frame_limit=([0-9]+) simulation_tick=([0-9]+) wall_time_us=([0-9]+)$') {
            if ($null -ne $summary) { throw 'Duplicate timing summary' }
            $summary = [pscustomobject]@{ passed = $Matches[1] -eq 'true'; domains = [int]$Matches[2]; frame_limit = [int]$Matches[3] }
        }
    }
    if ($null -eq $summary -or -not $summary.passed -or $summary.domains -ne 9) { throw "Missing or failed timing summary in $Path" }
    $results = [ordered]@{}
    foreach ($domain in $domainNames) {
        if (-not $starts.ContainsKey($domain) -or -not $completes.ContainsKey($domain)) { throw "Missing timing records for '$domain'" }
        $start = $starts[$domain]; $complete = $completes[$domain]
        if ($start.status -ne 1 -or $complete.status -ne 2 -or $complete.reason -ne 'none') { throw "Domain '$domain' did not pass" }
        if ($complete.expected_events -ne $complete.observed_events) { throw "Domain '$domain' event count mismatch" }
        if ($start.frame_limit -ne $summary.frame_limit -or $complete.frame_limit -ne $summary.frame_limit) { throw "Domain '$domain' frame cap mismatch" }
        $results[$domain] = [ordered]@{
            simulation_duration = [uint64]($complete.simulation_tick - $start.simulation_tick)
            wall_duration_us = [uint64]($complete.wall_time_us - $start.wall_time_us)
            expected_events = $complete.expected_events
            observed_events = $complete.observed_events
        }
    }
    [pscustomobject]@{ frame_limit = $summary.frame_limit; domains = $results }
}

function Get-Median([double[]] $Values) {
    $sorted = @($Values | Sort-Object)
    if (-not $sorted.Count) { throw 'Cannot calculate a median without samples' }
    $middle = [int][Math]::Floor($sorted.Count / 2.0)
    if (($sorted.Count % 2) -eq 1) { return [double]$sorted[$middle] }
    ([double]$sorted[$middle - 1] + [double]$sorted[$middle]) / 2.0
}

function Merge-TimingRuns([object[]] $Runs, [int] $FrameLimit) {
    $samples = @($Runs)
    if (-not $samples.Count) { throw "No timing samples were supplied for $FrameLimit FPS" }
    if (@($samples | Where-Object { $_.frame_limit -ne $FrameLimit }).Count) {
        throw "Timing sample frame-cap mismatch for $FrameLimit FPS"
    }
    $simulationSampleRangeLimit = [Math]::Ceiling(1000.0 / $FrameLimit) + 1.0
    $mediaSimulationSampleRangeLimit = [Math]::Max($simulationSampleRangeLimit, 18.0)
    $results = [ordered]@{}
    foreach ($domain in $domainNames) {
        $records = @($samples | ForEach-Object { $_.domains[$domain] })
        $simulationSamples = @($records | ForEach-Object { [double]$_.simulation_duration })
        $wallSamples = @($records | ForEach-Object { [double]$_.wall_duration_us })
        $orderedSimulationSamples = @($simulationSamples | Sort-Object)
        $simulationSampleRange = $orderedSimulationSamples[-1] - $orderedSimulationSamples[0]
        $domainSampleRangeLimit = if ($domain -in @('dialogue','cutscene')) { $mediaSimulationSampleRangeLimit } else { $simulationSampleRangeLimit }
        if ($simulationSampleRange -gt $domainSampleRangeLimit) {
            throw "Timing simulation sample spread for '$domain' at $FrameLimit FPS was $simulationSampleRange ms (limit $domainSampleRangeLimit ms)"
        }
        $expectedCounts = @($records | ForEach-Object { [int]$_.expected_events } | Sort-Object -Unique)
        $observedCounts = @($records | ForEach-Object { [int]$_.observed_events } | Sort-Object -Unique)
        if ($expectedCounts.Count -ne 1 -or $observedCounts.Count -ne 1 -or
            $expectedCounts[0] -ne $observedCounts[0]) {
            throw "Timing sample event-count mismatch for '$domain' at $FrameLimit FPS"
        }
        $results[$domain] = [ordered]@{
            simulation_duration = Get-Median $simulationSamples
            wall_duration_us = Get-Median $wallSamples
            expected_events = $expectedCounts[0]
            observed_events = $observedCounts[0]
            sample_count = $samples.Count
            simulation_sample_range = $simulationSampleRange
            simulation_samples = $simulationSamples
            wall_samples_us = $wallSamples
        }
    }
    [pscustomobject]@{ frame_limit = $FrameLimit; sample_count = $samples.Count; domains = $results }
}

function Compare-TimingRuns($Run60, $Run120) {
    if ($Run60.frame_limit -ne 60 -or $Run120.frame_limit -ne 120) { throw 'Fixture/run frame caps must be 60 and 120' }
    $comparisons = [ordered]@{}; $passed = $true
    foreach ($domain in $domainNames) {
        $a = $Run60.domains[$domain]; $b = $Run120.domains[$domain]
        $simulationDelta = [Math]::Abs([double]$a.simulation_duration - [double]$b.simulation_duration)
        $wallDelta = [Math]::Abs([double]$a.wall_duration_us - [double]$b.wall_duration_us)
        $longerWallDuration = [Math]::Max([double]$a.wall_duration_us, [double]$b.wall_duration_us)
        $wallRelativeFloor = 100000.0
        $wallRelativeDenominator = [Math]::Max($longerWallDuration, $wallRelativeFloor)
        $wallRelativeDelta = $wallDelta / $wallRelativeDenominator
        $isLevelTransition = $domain -eq 'level_transition'
        $wallAbsoluteLimit = if ($domain -in @('dialogue','cutscene','level_transition')) { 100000.0 } else { 50000.0 }
        $wallRelativeLimit = if ($isLevelTransition) { 0.20 } else { 0.05 }
        $wallEffectiveLimit = if ($isLevelTransition) { [Math]::Max($wallAbsoluteLimit, $longerWallDuration * $wallRelativeLimit) } else { [Math]::Min($wallAbsoluteLimit, $wallRelativeDenominator * $wallRelativeLimit) }
        $wallPassed = $wallDelta -le $wallEffectiveLimit
        $domainPassed = $a.observed_events -eq $b.observed_events -and
            $simulationDelta -le 17.0 -and
            $wallPassed
        if (-not $domainPassed) { $passed = $false }
        $comparisons[$domain] = [ordered]@{
            cap_60 = $a; cap_120 = $b
            simulation_delta = $simulationDelta; simulation_limit = 17.0
            wall_delta_us = $wallDelta; wall_absolute_limit_us = $wallAbsoluteLimit; wall_effective_limit_us = $wallEffectiveLimit
            wall_relative_delta = $wallRelativeDelta; wall_relative_limit = $wallRelativeLimit; wall_relative_floor_us = $wallRelativeFloor; wall_limit_policy = if ($isLevelTransition) { 'maximum' } else { 'minimum' }
            passed = $domainPassed
        }
    }
    [pscustomobject]@{ passed = $passed; domains = $comparisons }
}

function Get-AssetSnapshot([string] $Root) {
    @(Get-ChildItem -LiteralPath $Root -Recurse -File | Sort-Object FullName | ForEach-Object {
        "$($_.FullName.Substring($Root.Length))|$($_.Length)|$($_.LastWriteTimeUtc.Ticks)"
    })
}

function Initialize-NativeProbe {
    if ('TimingDomainsNativeProbe' -as [type]) { return }
    Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
public static class TimingDomainsNativeProbe {
 [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)] public struct D {
  [MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)] public string n; public short a,b,s,e;
  public int f,x,y,o,fo; public short c,du,yr,t,co;
  [MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)] public string fn; public short lp;
  public int bp,w,h,fl,hz,i1,i2,m,d,r1,r2,pw,ph;
 }
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern bool EnumDisplaySettings(string n,int m,ref D d);
 public static string Current(){var d=new D();d.s=(short)Marshal.SizeOf(typeof(D));if(!EnumDisplaySettings(null,-1,ref d))throw new InvalidOperationException();return d.w+"x"+d.h+"@"+d.hz;}
}
'@
}

function Wait-TimingObserverReady([string] $DiagnosticsRoot, [Diagnostics.Process] $Process, [DateTime] $Deadline) {
    $logPath = Join-Path $DiagnosticsRoot 'openjkdf2.jsonl'
    do {
        if (Test-Path -LiteralPath $logPath -PathType Leaf) {
            $ready = Select-String -LiteralPath $logPath -SimpleMatch '"subsystem":"timing_validation","event":"timing_domain_armed domain=weapon' -Quiet
            if ($ready) { return }
        }
        Start-Sleep -Milliseconds 100
        $Process.Refresh()
    } while (-not $Process.HasExited -and [DateTime]::UtcNow -lt $Deadline)

    if ($Process.HasExited) {
        throw "Timing-domain process exited before gameplay readiness with $($Process.ExitCode)"
    }
    throw 'Timing-domain observer did not reach gameplay readiness before the overall run deadline'
}

function Invoke-TimingRun([int] $Cap, [string] $SampleId, [string] $ExePath, [string] $AssetRoot, [string] $Root) {
    $userRoot = Join-Path $Root "user-$SampleId"; $diagnostics = Join-Path $Root "diagnostics-$SampleId"
    [void](New-Item -ItemType Directory -Path $userRoot); [void](New-Item -ItemType Directory -Path $diagnostics)
    $displayBefore = [TimingDomainsNativeProbe]::Current(); $eventStart = Get-Date
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $ExePath; $start.WorkingDirectory = $repoRoot; $start.UseShellExecute = $false
    $start.Arguments = '--validation-observer=timing-domains --frame-limit ' + $Cap + ' --data-dir "' + $AssetRoot + '" --user-dir "' + $userRoot + '" --diagnostics-dir "' + $diagnostics + '" -autostart -sp -episode JK1 -map 01narshadda.jkl'
    $process = $null
    $runFailure = $null
    try {
        $process = [Diagnostics.Process]::Start($start)
        $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
        $windowDeadline = [DateTime]::UtcNow.AddSeconds(20)
        do { Start-Sleep -Milliseconds 100; $process.Refresh() }
        while ($process.MainWindowHandle -eq [IntPtr]::Zero -and -not $process.HasExited -and [DateTime]::UtcNow -lt $windowDeadline)
        if ($process.HasExited -or $process.MainWindowHandle -eq [IntPtr]::Zero) { throw 'Timing-domain gameplay window did not appear' }
        Wait-TimingObserverReady $diagnostics $process $deadline
        while (-not $process.HasExited -and [DateTime]::UtcNow -lt $deadline) {
            Start-Sleep -Milliseconds 250
            $process.Refresh()
        }
        if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit(); throw "Timing-domain run at $Cap FPS timed out" }
    } catch {
        $runFailure = $_
    } finally {
        if ($process -and -not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
    }
    $displayAfter = [TimingDomainsNativeProbe]::Current()
    $errors = @(Get-WinEvent -FilterHashtable @{LogName='Application'; StartTime=$eventStart; Id=1000} -ErrorAction SilentlyContinue |
        Where-Object { $_.Message -match 'openjkdf2-64|atio6axx' } | Select-Object TimeCreated,Id,Message)
    $exitCode = if ($process -and $process.HasExited) { $process.ExitCode } else { $null }
    $meta = [ordered]@{ exit_code=$exitCode; display_before=$displayBefore; display_after=$displayAfter; display_invariant=$displayBefore -eq $displayAfter; application_errors=$errors; run_failure=if ($runFailure) { [string]$runFailure.Exception.Message } else { $null } }
    $meta | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $Root "run-$SampleId-meta.json") -Encoding utf8
    if ($displayBefore -ne $displayAfter) { throw "Display state changed during $Cap FPS run" }
    if ($errors.Count) { throw "Application Error was recorded during $Cap FPS run" }
    if ($runFailure) { throw $runFailure }
    $process.Refresh()
    if ($process.ExitCode -ne 1) { throw "Timing-domain run at $Cap FPS exited with $($process.ExitCode)" }
    Join-Path $diagnostics 'openjkdf2.jsonl'
}

$outputPath = [IO.Path]::GetFullPath($OutputDir)
if (Test-Path -LiteralPath $outputPath) { throw "OutputDir must be fresh: $outputPath" }
[void](New-Item -ItemType Directory -Path $outputPath)
$warmupSample = $null

if ($FixtureOnly) {
    if (-not $Input60 -or -not $Input120) { throw 'FixtureOnly requires Input60 and Input120' }
    $logs60 = @($Input60 -split ';'); $logs120 = @($Input120 -split ';')
} else {
    if (-not $DataDir) { throw 'DataDir is required for runtime validation' }
    Initialize-NativeProbe
    $assetRoot = (Resolve-Path -LiteralPath $DataDir).Path.TrimEnd('\','/')
    $exePath = (Resolve-Path -LiteralPath (Join-Path $repoRoot $Executable)).Path
    if ($outputPath.StartsWith($assetRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'OutputDir must be outside DataDir' }
    $assetBefore = Get-AssetSnapshot $assetRoot
    $runtimeFailure = $null
    try {
        $warmupLog = Invoke-TimingRun 60 'warmup-60' $exePath $assetRoot $outputPath
        $warmupSample = Convert-TimingLog $warmupLog
        $logs60 = [System.Collections.Generic.List[string]]::new()
        $logs120 = [System.Collections.Generic.List[string]]::new()
        $sampleIndices = @{ 60 = 0; 120 = 0 }
        $runOrder = if ($Run120First) { @(120,60,60,120,120,60) } else { @(60,120,120,60,60,120) }
        foreach ($cap in $runOrder) {
            $sampleIndices[$cap]++
            $sampleId = "$cap-$($sampleIndices[$cap])"
            $log = Invoke-TimingRun $cap $sampleId $exePath $assetRoot $outputPath
            if ($cap -eq 60) { $logs60.Add($log) } else { $logs120.Add($log) }
        }
        if ($logs60.Count -ne $runtimeSampleCount -or $logs120.Count -ne $runtimeSampleCount) {
            throw 'Runtime sample order did not produce three samples per frame cap'
        }
    } catch {
        $runtimeFailure = $_
    } finally {
        $assetAfter = Get-AssetSnapshot $assetRoot
        if (@(Compare-Object $assetBefore $assetAfter).Count -ne 0) { throw 'Steam asset metadata changed during validation' }
    }
    if ($runtimeFailure) { throw $runtimeFailure }
}

$runSamples60 = @($logs60 | ForEach-Object { Convert-TimingLog $_ })
$runSamples120 = @($logs120 | ForEach-Object { Convert-TimingLog $_ })
$run60 = Merge-TimingRuns $runSamples60 60
$run120 = Merge-TimingRuns $runSamples120 120
$comparison = Compare-TimingRuns $run60 $run120
$result = [ordered]@{
    schema=1; passed=$comparison.passed
    warmup_sample=$warmupSample
    cap_60=$run60; cap_120=$run120
    cap_60_samples=$runSamples60; cap_120_samples=$runSamples120
    comparisons=$comparison.domains
}
$resultPath = Join-Path $outputPath 'timing-domains-comparison.json'
$result | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $resultPath -Encoding utf8
$result | ConvertTo-Json -Depth 12
if (-not $comparison.passed) { throw "Timing-domain comparison failed; inspect $resultPath" }
