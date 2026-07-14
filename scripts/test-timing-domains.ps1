[CmdletBinding()]
param(
    [string] $Executable = 'build/msvc-release/openjkdf2-64.exe',
    [string] $DataDir,
    [Parameter(Mandatory = $true)][string] $OutputDir,
    [switch] $FixtureOnly,
    [string] $Input60,
    [string] $Input120,
    [int] $TimeoutSeconds = 180
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$domainNames = @('weapon','ai','physics','animation','particle','script','dialogue','cutscene','level_transition')
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

function Compare-TimingRuns($Run60, $Run120) {
    if ($Run60.frame_limit -ne 60 -or $Run120.frame_limit -ne 120) { throw 'Fixture/run frame caps must be 60 and 120' }
    $comparisons = [ordered]@{}; $passed = $true
    foreach ($domain in $domainNames) {
        $a = $Run60.domains[$domain]; $b = $Run120.domains[$domain]
        $simulationDelta = [Math]::Abs([double]$a.simulation_duration - [double]$b.simulation_duration)
        $wallDelta = [Math]::Abs([double]$a.wall_duration_us - [double]$b.wall_duration_us)
        $wallLimit = [Math]::Max(100000.0, [Math]::Max([double]$a.wall_duration_us, [double]$b.wall_duration_us) * 0.20)
        $domainPassed = $a.observed_events -eq $b.observed_events -and $simulationDelta -le 17.0 -and $wallDelta -le $wallLimit
        if (-not $domainPassed) { $passed = $false }
        $comparisons[$domain] = [ordered]@{
            cap_60 = $a; cap_120 = $b
            simulation_delta = $simulationDelta; simulation_limit = 17.0
            wall_delta_us = $wallDelta; wall_limit_us = $wallLimit
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
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")] public static extern void mouse_event(uint f,int dx,int dy,uint data,UIntPtr extra);
 [DllImport("user32.dll")] public static extern void keybd_event(byte key,byte scan,uint flags,UIntPtr extra);
 public static string Current(){var d=new D();d.s=(short)Marshal.SizeOf(typeof(D));if(!EnumDisplaySettings(null,-1,ref d))throw new InvalidOperationException();return d.w+"x"+d.h+"@"+d.hz;}
}
'@
}

function Invoke-TimingRun([int] $Cap, [string] $ExePath, [string] $AssetRoot, [string] $Root) {
    $userRoot = Join-Path $Root "user-$Cap"; $diagnostics = Join-Path $Root "diagnostics-$Cap"
    [void](New-Item -ItemType Directory -Path $userRoot); [void](New-Item -ItemType Directory -Path $diagnostics)
    $displayBefore = [TimingDomainsNativeProbe]::Current(); $eventStart = Get-Date
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $ExePath; $start.WorkingDirectory = $repoRoot; $start.UseShellExecute = $false
    $start.Arguments = '--validation-observer=timing-domains --frame-limit ' + $Cap + ' --data-dir "' + $AssetRoot + '" --user-dir "' + $userRoot + '" --diagnostics-dir "' + $diagnostics + '" -autostart -sp -episode JK1 -map 01narshadda.jkl'
    $start.Environment['OPENJKDF2_VALIDATE_FIRST_DOOR_MS'] = '150000'
    $start.Environment['OPENJKDF2_VALIDATE_FIRST_DOOR_WARP_APPROACH'] = '1'
    $process = [Diagnostics.Process]::Start($start)
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    try {
        $windowDeadline = [DateTime]::UtcNow.AddSeconds(20)
        do { Start-Sleep -Milliseconds 100; $process.Refresh() }
        while ($process.MainWindowHandle -eq [IntPtr]::Zero -and -not $process.HasExited -and [DateTime]::UtcNow -lt $windowDeadline)
        if (-not $process.HasExited -and $process.MainWindowHandle -ne [IntPtr]::Zero) {
            [void][TimingDomainsNativeProbe]::SetForegroundWindow($process.MainWindowHandle)
        }
        while (-not $process.HasExited -and [DateTime]::UtcNow -lt $deadline) {
            [TimingDomainsNativeProbe]::mouse_event(0x0002,0,0,0,[UIntPtr]::Zero)
            Start-Sleep -Milliseconds 80
            [TimingDomainsNativeProbe]::mouse_event(0x0004,0,0,0,[UIntPtr]::Zero)
            [TimingDomainsNativeProbe]::keybd_event(0x45,0,0,[UIntPtr]::Zero)
            Start-Sleep -Milliseconds 80
            [TimingDomainsNativeProbe]::keybd_event(0x45,0,2,[UIntPtr]::Zero)
            Start-Sleep -Seconds 1
            $process.Refresh()
        }
        if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit(); throw "Timing-domain run at $Cap FPS timed out" }
    } finally {
        if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
    }
    $displayAfter = [TimingDomainsNativeProbe]::Current()
    $errors = @(Get-WinEvent -FilterHashtable @{LogName='Application'; StartTime=$eventStart; Id=1000} -ErrorAction SilentlyContinue |
        Where-Object { $_.Message -match 'openjkdf2-64|atio6axx' } | Select-Object TimeCreated,Id,Message)
    $meta = [ordered]@{ exit_code=$process.ExitCode; display_before=$displayBefore; display_after=$displayAfter; display_invariant=$displayBefore -eq $displayAfter; application_errors=$errors }
    $meta | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $Root "run-$Cap-meta.json") -Encoding utf8
    if ($process.ExitCode -ne 1) { throw "Timing-domain run at $Cap FPS exited with $($process.ExitCode)" }
    if ($displayBefore -ne $displayAfter) { throw "Display state changed during $Cap FPS run" }
    if ($errors.Count) { throw "Application Error was recorded during $Cap FPS run" }
    Join-Path $diagnostics 'openjkdf2.jsonl'
}

$outputPath = [IO.Path]::GetFullPath($OutputDir)
if (Test-Path -LiteralPath $outputPath) { throw "OutputDir must be fresh: $outputPath" }
[void](New-Item -ItemType Directory -Path $outputPath)

if ($FixtureOnly) {
    if (-not $Input60 -or -not $Input120) { throw 'FixtureOnly requires Input60 and Input120' }
    $log60 = $Input60; $log120 = $Input120
} else {
    if (-not $DataDir) { throw 'DataDir is required for runtime validation' }
    Initialize-NativeProbe
    $assetRoot = (Resolve-Path -LiteralPath $DataDir).Path.TrimEnd('\','/')
    $exePath = (Resolve-Path -LiteralPath (Join-Path $repoRoot $Executable)).Path
    if ($outputPath.StartsWith($assetRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'OutputDir must be outside DataDir' }
    $assetBefore = Get-AssetSnapshot $assetRoot
    $log60 = Invoke-TimingRun 60 $exePath $assetRoot $outputPath
    $log120 = Invoke-TimingRun 120 $exePath $assetRoot $outputPath
    $assetAfter = Get-AssetSnapshot $assetRoot
    if ((Compare-Object $assetBefore $assetAfter).Count -ne 0) { throw 'Steam asset metadata changed during validation' }
}

$run60 = Convert-TimingLog $log60; $run120 = Convert-TimingLog $log120
$comparison = Compare-TimingRuns $run60 $run120
$result = [ordered]@{ schema=1; passed=$comparison.passed; cap_60=$run60; cap_120=$run120; comparisons=$comparison.domains }
$resultPath = Join-Path $outputPath 'timing-domains-comparison.json'
$result | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $resultPath -Encoding utf8
$result | ConvertTo-Json -Depth 12
if (-not $comparison.passed) { throw "Timing-domain comparison failed; inspect $resultPath" }
