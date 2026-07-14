[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$DataDir,
    [Parameter(Mandatory = $true)][string]$UserDir,
    [string]$Executable = "build/msvc-release/openjkdf2-64.exe",
    [int]$TimeoutSeconds = 180
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$assetRoot = (Resolve-Path -LiteralPath $DataDir).Path.TrimEnd("\", "/")
$exePath = (Resolve-Path -LiteralPath (Join-Path $repoRoot $Executable)).Path
$userRoot = [IO.Path]::GetFullPath($UserDir).TrimEnd("\", "/")
if (Test-Path -LiteralPath $userRoot) { throw "UserDir must be a fresh path: $userRoot" }
if ($userRoot.Equals($assetRoot, [StringComparison]::OrdinalIgnoreCase) -or
    $userRoot.StartsWith($assetRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) -or
    $assetRoot.StartsWith($userRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw "UserDir and DataDir must be separate trees"
}
if ($assetRoot.Contains('"') -or $userRoot.Contains('"')) { throw "Quoted path characters are unsupported" }

Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
public static class OpenJKDF2DoorProbe {
 [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)] public struct D {
  [MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)] public string n; public short a,b,s,e;
  public int f,x,y,o,fo; public short c,du,yr,t,co;
  [MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)] public string fn; public short lp;
  public int bp,w,h,fl,hz,i1,i2,m,d,r1,r2,pw,ph;
 }
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern bool EnumDisplaySettings(string n,int m,ref D d);
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
 [DllImport("user32.dll")] public static extern void keybd_event(byte key,byte scan,uint flags,UIntPtr extra);
 [DllImport("user32.dll")] public static extern void mouse_event(uint flags,int dx,int dy,uint data,UIntPtr extra);
 public static string Current(){var d=new D();d.s=(short)Marshal.SizeOf(typeof(D));if(!EnumDisplaySettings(null,-1,ref d))throw new InvalidOperationException();return d.w+"x"+d.h+"@"+d.hz;}
}
'@
function Get-AssetSnapshot([string]$Root) {
    @(Get-ChildItem -LiteralPath $Root -Recurse -File | Sort-Object FullName | ForEach-Object {
        "$($_.FullName.Substring($Root.Length))|$($_.Length)|$($_.LastWriteTimeUtc.Ticks)"
    })
}
function Set-Key([byte]$Key, [bool]$Down) {
    [OpenJKDF2DoorProbe]::keybd_event($Key, 0, $(if ($Down) { 0 } else { 0x0002 }), [UIntPtr]::Zero)
}
function Move-Mouse([int]$TotalX) {
    $step = if ($TotalX -lt 0) { -20 } else { 20 }
    $count = [Math]::Abs([int]($TotalX / $step))
    for ($i = 0; $i -lt $count; ++$i) {
        [OpenJKDF2DoorProbe]::mouse_event(0x0001, $step, 0, 0, [UIntPtr]::Zero)
        Start-Sleep -Milliseconds 50
    }
}
function Get-DoorSample {
    if (-not (Test-Path -LiteralPath $script:doorJsonl)) { return $null }
    $match = Select-String -LiteralPath $script:doorJsonl -Pattern 'first_door sample' | Select-Object -Last 1
    if (-not $match -or $match.Line -notmatch 'player=\(([-0-9.]+),([-0-9.]+),([-0-9.]+)\) yaw=([-0-9.]+) sector=([0-9]+) door_a=([-0-9.]+) door_b=([-0-9.]+)') { return $null }
    [pscustomobject]@{
        X = [double]::Parse($Matches[1], [Globalization.CultureInfo]::InvariantCulture)
        Y = [double]::Parse($Matches[2], [Globalization.CultureInfo]::InvariantCulture)
        Z = [double]::Parse($Matches[3], [Globalization.CultureInfo]::InvariantCulture)
        Yaw = [double]::Parse($Matches[4], [Globalization.CultureInfo]::InvariantCulture)
        Sector = [int]$Matches[5]
        DoorA = [double]::Parse($Matches[6], [Globalization.CultureInfo]::InvariantCulture)
        DoorB = [double]::Parse($Matches[7], [Globalization.CultureInfo]::InvariantCulture)
    }
}
function Hold-KeyUntil([byte]$Key, [scriptblock]$Reached, [string]$Name, [int]$Seconds = 20) {
    $deadline = [DateTime]::UtcNow.AddSeconds($Seconds)
    Set-Key $Key $true
    try {
        do {
            Start-Sleep -Milliseconds 200
            $sample = Get-DoorSample
            if ($sample -and (& $Reached $sample)) { return $sample }
        } while ([DateTime]::UtcNow -lt $deadline)
        $last = Get-DoorSample
        throw "Waypoint '$Name' timed out; last sample: $($last | ConvertTo-Json -Compress)"
    } finally {
        Set-Key $Key $false
    }
}
function Hold-KeyUntilWithPulses([byte]$Key, [byte]$PulseKey, [scriptblock]$Reached, [string]$Name, [int]$Seconds = 40) {
    $deadline = [DateTime]::UtcNow.AddSeconds($Seconds)
    $nextPulse = [DateTime]::UtcNow
    Set-Key $Key $true
    try {
        do {
            if ([DateTime]::UtcNow -ge $nextPulse) {
                Set-Key $PulseKey $true
                Start-Sleep -Milliseconds 150
                Set-Key $PulseKey $false
                $nextPulse = [DateTime]::UtcNow.AddMilliseconds(850)
            }
            Start-Sleep -Milliseconds 100
            $sample = Get-DoorSample
            if ($sample -and (& $Reached $sample)) { return $sample }
        } while ([DateTime]::UtcNow -lt $deadline)
        throw "Waypoint '$Name' timed out; last sample: $((Get-DoorSample) | ConvertTo-Json -Compress)"
    } finally {
        Set-Key $PulseKey $false
        Set-Key $Key $false
    }
}
function Turn-ToYaw([double]$Target, [int]$Seconds = 20) {
    $deadline = [DateTime]::UtcNow.AddSeconds($Seconds)
    do {
        $sample = Get-DoorSample
        if ($sample) {
            $delta = $Target - $sample.Yaw
            while ($delta -gt 180.0) { $delta -= 360.0 }
            while ($delta -lt -180.0) { $delta += 360.0 }
            if ([Math]::Abs($delta) -le 5.0) { return $sample }
            $dx = if ($delta -gt 0.0) { -5 } else { 5 }
            [OpenJKDF2DoorProbe]::mouse_event(0x0001, $dx, 0, 0, [UIntPtr]::Zero)
        }
        Start-Sleep -Milliseconds 300
    } while ([DateTime]::UtcNow -lt $deadline)
    throw "Yaw turn timed out; last sample: $((Get-DoorSample) | ConvertTo-Json -Compress)"
}
function Activate-DoorWithPitchSweep {
    foreach ($dy in (@(0) + @(1..12 | ForEach-Object { 8 }) + @(-96) + @(1..12 | ForEach-Object { -8 }))) {
        if ($dy -ne 0) {
            [OpenJKDF2DoorProbe]::mouse_event(0x0001, 0, $dy, 0, [UIntPtr]::Zero)
            Start-Sleep -Milliseconds 250
        }
        Set-Key $script:eKey $true
        Start-Sleep -Milliseconds 200
        Set-Key $script:eKey $false
        Start-Sleep -Milliseconds 550
        $sample = Get-DoorSample
        if ($sample -and $sample.DoorA -ge 0.01 -and $sample.DoorB -ge 0.01) { return $sample }
    }
    throw "Door activation pitch sweep failed; last sample: $((Get-DoorSample) | ConvertTo-Json -Compress)"
}
function Reach-DoorApproach([byte]$Forward, [byte]$Left, [byte]$Right, [byte]$Jump) {
    for ($attempt = 0; $attempt -lt 5; ++$attempt) {
        try {
            return Hold-KeyUntilWithPulses $Forward $Jump { param($p) $p.Sector -eq 104 -and $p.X -ge -4.50 } "door approach" 10
        } catch {
            if ($attempt -eq 4) { throw }
            if (($attempt % 2) -eq 0) {
                try { [void](Hold-KeyUntil $Right { param($p) $p.Y -le -4.15 } "door approach south detour" 3) } catch {}
            } else {
                try { [void](Hold-KeyUntil $Left { param($p) $p.Y -ge -4.00 } "door approach north detour" 3) } catch {}
            }
        }
    }
}

$before = Get-AssetSnapshot $assetRoot
$displayBefore = [OpenJKDF2DoorProbe]::Current()
$env:OPENJKDF2_VALIDATE_FIRST_DOOR_MS = "120000"
$env:OPENJKDF2_VALIDATE_FIRST_DOOR_SCREENSHOT = "diagnostics\first-door.png"
$env:OPENJKDF2_VALIDATE_FIRST_DOOR_YAW = "-90"
$env:OPENJKDF2_VALIDATE_FIRST_DOOR_WARP_APPROACH = "1"
$w = 0x57; $a = 0x41; $s = 0x53; $d = 0x44; $e = 0x45; $space = 0x20
$wDown = $false; $aDown = $false; $sDown = $false; $dDown = $false; $eDown = $false; $spaceDown = $false; $focusVerified = $false
$script:doorJsonl = Join-Path $userRoot "diagnostics\openjkdf2.jsonl"
$script:eKey = $e
try {
    $start = New-Object Diagnostics.ProcessStartInfo
    $start.FileName = $exePath
    $start.WorkingDirectory = $repoRoot
    $start.UseShellExecute = $false
    $start.Arguments = '--data-dir "' + $assetRoot + '" --user-dir "' + $userRoot + '" --diagnostics-dir diagnostics -autostart -sp -episode JK1 -map 01narshadda.jkl'
    $process = [Diagnostics.Process]::Start($start)
    $windowDeadline = [DateTime]::UtcNow.AddSeconds(10)
    do { Start-Sleep -Milliseconds 100; $process.Refresh() }
    while ($process.MainWindowHandle -eq [IntPtr]::Zero -and -not $process.HasExited -and [DateTime]::UtcNow -lt $windowDeadline)
    if ($process.MainWindowHandle -eq [IntPtr]::Zero) { throw "Gameplay window was not created" }
    Start-Sleep -Seconds 6
    $process.Refresh()
    if (-not [OpenJKDF2DoorProbe]::SetForegroundWindow($process.MainWindowHandle)) { throw "Could not focus gameplay window" }
    Start-Sleep -Seconds 4
    $focusVerified = [OpenJKDF2DoorProbe]::GetForegroundWindow() -eq $process.MainWindowHandle
    if (-not $focusVerified) { throw "Gameplay window did not retain foreground focus" }

    [void](Hold-KeyUntil $a { param($p) $p.Sector -eq 349 -and $p.Y -ge -4.85 } "spawn north")
    [void](Hold-KeyUntil $w { param($p) $p.Sector -eq 361 -and $p.X -ge -9.50 } "sector 361 east")
    [void](Hold-KeyUntil $d { param($p) $p.Y -le -5.35 } "sector 361 south")
    [void](Hold-KeyUntil $w { param($p) $p.Sector -in 371,19 -or ($p.Sector -eq 370 -and $p.X -ge -8.35) } "sector 370 east")
    [void](Hold-KeyUntil $d { param($p) $p.Sector -eq 19 -or $p.Y -le -5.45 } "sector 370 south")
    [void](Hold-KeyUntil $w { param($p) $p.Sector -eq 19 } "sector 19")
    [void](Hold-KeyUntil $a { param($p) $p.Sector -eq 22 -and $p.Y -ge -3.90 } "sector 22 north" 30)
    [void](Hold-KeyUntil $s { param($p) $p.Sector -eq 28 -and $p.X -le -9.60 } "sector 28 west" 30)
    [void](Hold-KeyUntil $a { param($p) $p.Y -ge -3.55 } "sector 28 north")
    [void](Hold-KeyUntil $w { param($p) $p.Sector -in 33,32,104 -or ($p.Sector -eq 30 -and $p.X -ge -8.10) } "sector 30 east" 30)
    [void](Hold-KeyUntil $s { param($p) $p.Sector -in 32,104 -or $p.X -le -8.20 } "sector 30 backoff")
    [void](Hold-KeyUntil $d { param($p) $p.Sector -in 32,104 -or $p.Y -le -3.25 } "sector 30 center")
    Set-Key $space $true; $spaceDown = $true
    try {
        [void](Hold-KeyUntil $w { param($p) $p.Sector -in 32,104 } "sector 32" 30)
    } finally {
        Set-Key $space $false; $spaceDown = $false
    }
    [void](Hold-KeyUntil $s { param($p) $p.Sector -eq 104 -or $p.X -le -7.60 } "sector 32 backoff")
    [void](Hold-KeyUntil $d { param($p) $p.Sector -eq 104 -or $p.Y -le -3.95 } "first switch south" 30)
    [void](Hold-KeyUntilWithPulses $w $space { param($p) $p.Sector -in 31,104 } "sector 31 entry" 30)
    [void](Hold-KeyUntil $a { param($p) $p.Y -ge -4.05 } "sector 31 north lane")
    [void](Reach-DoorApproach $w $a $d $space)
    [void](Hold-KeyUntil $a { param($p) $p.Y -ge -3.90 } "south switch alignment")
    [void](Turn-ToYaw -90.0)
    [void](Activate-DoorWithPitchSweep)
    [void](Hold-KeyUntilWithPulses $a $space { param($p) $p.Y -gt -3.65 } "cross opened door" 30)
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) { $process.Kill(); throw "First-door probe timed out" }
} finally {
    if ($wDown) { Set-Key $w $false }
    if ($aDown) { Set-Key $a $false }
    if ($sDown) { Set-Key $s $false }
    if ($dDown) { Set-Key $d $false }
    if ($eDown) { Set-Key $e $false }
    if ($spaceDown) { Set-Key $space $false }
    if ($process -and -not $process.HasExited) {
        $process.Kill()
        $process.WaitForExit()
    }
    Remove-Item Env:OPENJKDF2_VALIDATE_FIRST_DOOR_MS -ErrorAction SilentlyContinue
    Remove-Item Env:OPENJKDF2_VALIDATE_FIRST_DOOR_SCREENSHOT -ErrorAction SilentlyContinue
    Remove-Item Env:OPENJKDF2_VALIDATE_FIRST_DOOR_YAW -ErrorAction SilentlyContinue
    Remove-Item Env:OPENJKDF2_VALIDATE_FIRST_DOOR_WARP_APPROACH -ErrorAction SilentlyContinue
}
$displayAfter = [OpenJKDF2DoorProbe]::Current()
$after = Get-AssetSnapshot $assetRoot
$diagnostics = Join-Path $userRoot "diagnostics"
$jsonl = Join-Path $diagnostics "openjkdf2.jsonl"
$state = Get-Content -Raw -LiteralPath (Join-Path $diagnostics "run-state.json") | ConvertFrom-Json
$event = Select-String -LiteralPath $jsonl -Pattern "first_door complete" | Select-Object -Last 1
$result = [ordered]@{
    schema = 1; startup_result = $process.ExitCode
    display_before = $displayBefore; display_after = $displayAfter; display_invariant = $displayBefore -eq $displayAfter
    asset_metadata_invariant = (Compare-Object $before $after).Count -eq 0; focus_verified = $focusVerified
    door_moved = [bool](Select-String -LiteralPath $jsonl -Pattern "first_door complete door_moved=true" -Quiet)
    crossed = [bool](Select-String -LiteralPath $jsonl -Pattern "first_door complete door_moved=true crossed=true" -Quiet)
    door_event = if ($event) { $event.Line } else { $null }
    screenshot_exists = Test-Path -LiteralPath (Join-Path $diagnostics "first-door.png")
    clean_state = $state.status -eq "clean"
    process_finished = [bool](Select-String -LiteralPath $jsonl -Pattern "process_finished" -Quiet)
}
$resultPath = Join-Path $userRoot "first-door-result.json"
$result | ConvertTo-Json | Set-Content -LiteralPath $resultPath -Encoding utf8
$result | ConvertTo-Json
if ($result.startup_result -ne 1 -or -not $result.display_invariant -or -not $result.asset_metadata_invariant -or
    -not $result.focus_verified -or -not $result.door_moved -or -not $result.crossed -or
    -not $result.screenshot_exists -or -not $result.clean_state -or -not $result.process_finished) {
    throw "First-door verification failed; inspect $resultPath and the sampled diagnostics"
}
