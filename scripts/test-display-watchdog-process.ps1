[CmdletBinding()]
param(
    [string]$WatchdogPath = 'build/msvc-release/openjkdf2-display-watchdog.exe',
    [string]$EvidenceRoot = ''
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$watchdogCandidate = if ([IO.Path]::IsPathRooted($WatchdogPath)) {
    $WatchdogPath
} else {
    Join-Path $repoRoot $WatchdogPath
}
$watchdog = (Resolve-Path -LiteralPath $watchdogCandidate).Path
$root = if ($EvidenceRoot) {
    [IO.Path]::GetFullPath($EvidenceRoot)
} else {
    Join-Path ([IO.Path]::GetTempPath()) ('openjkdf2-watchdog-' + [Guid]::NewGuid().ToString('N'))
}
if (Test-Path -LiteralPath $root) { throw "EvidenceRoot must be fresh: $root" }
[void](New-Item -ItemType Directory -Path $root)

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class OpenJKDF2WatchdogDisplayProbe {
 [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)] public struct D {
  [MarshalAs(UnmanagedType.ByValTStr, SizeConst=32)] public string n; public short a,b,s,e;
  public int f,x,y,o,fo; public short c,du,yr,t,co;
  [MarshalAs(UnmanagedType.ByValTStr, SizeConst=32)] public string fn; public short lp;
  public int bp,w,h,fl,hz,i1,i2,m,d,r1,r2,pw,ph;
 }
 [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern bool EnumDisplaySettings(string n, int m, ref D d);
 public static string Current() { var d=new D(); d.s=(short)Marshal.SizeOf(typeof(D)); if(!EnumDisplaySettings(null,-1,ref d)) throw new InvalidOperationException(); return d.w+"x"+d.h+"@"+d.hz; }
}
'@

function Invoke-Watchdog([string]$Arguments) {
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $watchdog
    $start.WorkingDirectory = $root
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.Arguments = $Arguments
    $process = [Diagnostics.Process]::Start($start)
    $process.WaitForExit()
    $process.ExitCode
}

$displayBefore = [OpenJKDF2WatchdogDisplayProbe]::Current()
$preflightState = Join-Path $root 'preflight-state.bin'
$preflightCaptureExit = Invoke-Watchdog ('--capture "' + $preflightState + '"')
if ($preflightCaptureExit -ne 0) { throw 'Watchdog could not capture the active display state.' }
$preflightExit = Invoke-Watchdog ('--restore "' + $preflightState + '"')
$displayAfterPreflight = [OpenJKDF2WatchdogDisplayProbe]::Current()
if ($preflightExit -ne 0) { [void](Invoke-Watchdog ('--disarm "' + $preflightState + '"')) }

$state = Join-Path $root 'watch-state.bin'
$proof = Join-Path $root 'watch-proof.json'
$ready = Join-Path $root 'watch-ready.json'
if ((Invoke-Watchdog ('--capture "' + $state + '"')) -ne 0) { throw 'Watchdog handshake state capture failed.' }

$parent = Start-Process -FilePath 'powershell.exe' -ArgumentList '-NoProfile','-Command','Start-Sleep -Seconds 30' -PassThru -WindowStyle Hidden
$watchStart = [Diagnostics.ProcessStartInfo]::new()
$watchStart.FileName = $watchdog
$watchStart.WorkingDirectory = $root
$watchStart.UseShellExecute = $false
$watchStart.CreateNoWindow = $true
$watchStart.Arguments = '--watch ' + $parent.Id + ' "' + $state + '" "' + $proof + '" "' + $ready + '"'
$helper = [Diagnostics.Process]::Start($watchStart)
try {
    $deadline = [DateTime]::UtcNow.AddSeconds(5)
    while (-not (Test-Path -LiteralPath $ready -PathType Leaf) -and [DateTime]::UtcNow -lt $deadline) {
        if ($helper.HasExited) { throw "Watchdog exited before ready handshake with code $($helper.ExitCode)." }
        Start-Sleep -Milliseconds 25
    }
    if (-not (Test-Path -LiteralPath $ready -PathType Leaf)) { throw 'Watchdog ready handshake timed out.' }
    $readyRecord = Get-Content -LiteralPath $ready -Raw | ConvertFrom-Json
    if ($readyRecord.status -ne 'ready' -or $readyRecord.parent_pid -ne $parent.Id) { throw 'Watchdog ready record is invalid.' }
    if ((Invoke-Watchdog ('--disarm "' + $state + '"')) -ne 0) { throw 'Watchdog state disarm failed.' }
    $parent.Kill()
    $parent.WaitForExit()
    if (-not $helper.WaitForExit(5000)) { throw 'Watchdog did not finish after parent exit.' }
    if ($helper.ExitCode -ne 0) { throw "Watchdog returned $($helper.ExitCode) after disarmed parent exit." }
} finally {
    if (-not $parent.HasExited) { $parent.Kill(); $parent.WaitForExit() }
    if (-not $helper.HasExited) { $helper.Kill(); $helper.WaitForExit() }
}

if (-not (Test-Path -LiteralPath $proof -PathType Leaf)) { throw 'Watchdog proof record is missing.' }
$proofRecord = Get-Content -LiteralPath $proof -Raw | ConvertFrom-Json

$disarmedState = Join-Path $root 'disarmed-state.bin'
$disarmedProof = Join-Path $root 'disarmed-proof.json'
$disarmedReady = Join-Path $root 'disarmed-ready.json'
if ((Invoke-Watchdog ('--capture "' + $disarmedState + '"')) -ne 0 -or
    (Invoke-Watchdog ('--disarm "' + $disarmedState + '"')) -ne 0) {
    throw 'Could not prepare a disarmed negative-test state.'
}
$disarmedParent = Start-Process -FilePath 'powershell.exe' -ArgumentList '-NoProfile','-Command','Start-Sleep -Seconds 30' -PassThru -WindowStyle Hidden
$disarmedStart = [Diagnostics.ProcessStartInfo]::new()
$disarmedStart.FileName = $watchdog
$disarmedStart.WorkingDirectory = $root
$disarmedStart.UseShellExecute = $false
$disarmedStart.CreateNoWindow = $true
$disarmedStart.Arguments = '--watch ' + $disarmedParent.Id + ' "' + $disarmedState + '" "' + $disarmedProof + '" "' + $disarmedReady + '"'
$disarmedHelper = [Diagnostics.Process]::Start($disarmedStart)
try {
    if (-not $disarmedHelper.WaitForExit(5000)) { throw 'Disarmed-state helper did not reject the snapshot.' }
    if ($disarmedHelper.ExitCode -ne 6) { throw "Disarmed-state helper returned $($disarmedHelper.ExitCode), expected 6." }
} finally {
    if (-not $disarmedParent.HasExited) { $disarmedParent.Kill(); $disarmedParent.WaitForExit() }
    if (-not $disarmedHelper.HasExited) { $disarmedHelper.Kill(); $disarmedHelper.WaitForExit() }
}
$disarmedProofRecord = Get-Content -LiteralPath $disarmedProof -Raw | ConvertFrom-Json
$disarmedRejected = -not (Test-Path -LiteralPath $disarmedReady) -and
    $disarmedProofRecord.wait -eq 'state_invalid' -and $disarmedProofRecord.restore -eq 'failed'

$displayAfter = [OpenJKDF2WatchdogDisplayProbe]::Current()
$result = [ordered]@{
    schema = 1
    exclusive_runtime_tested = $false
    display_before = $displayBefore
    display_after_preflight = $displayAfterPreflight
    display_after = $displayAfter
    display_invariant = $displayBefore -eq $displayAfterPreflight -and $displayBefore -eq $displayAfter
    preflight_capture_exit = $preflightCaptureExit
    preflight_exit = $preflightExit
    preflight_ready = $preflightExit -eq 0
    ready_status = $readyRecord.status
    ready_parent_pid_matched = $readyRecord.parent_pid -eq $parent.Id
    proof_wait = $proofRecord.wait
    proof_restore = $proofRecord.restore
    proof_display_count = $proofRecord.display_count
    disarmed_state_rejected = $disarmedRejected
}
$resultPath = Join-Path $root 'display-watchdog-process-result.json'
$result | ConvertTo-Json | Set-Content -LiteralPath $resultPath -Encoding utf8
$result | ConvertTo-Json
if (-not $result.display_invariant -or -not $result.ready_parent_pid_matched -or
    $result.proof_wait -ne 'parent_exit' -or $result.proof_restore -ne 'success' -or
    $result.proof_display_count -ne 0 -or -not $result.disarmed_state_rejected) {
    throw "Display watchdog process verification failed; inspect $resultPath"
}
