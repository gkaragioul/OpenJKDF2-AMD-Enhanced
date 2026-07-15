[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$DataDir,
    [Parameter(Mandatory = $true)][string]$EvidenceRoot,
    [string]$Executable = "build/msvc-release/openjkdf2-64.exe",
    [int]$TimeoutSeconds = 45
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$assetRoot = (Resolve-Path -LiteralPath $DataDir).Path.TrimEnd("\", "/")
$exePath = (Resolve-Path -LiteralPath (Join-Path $repoRoot $Executable)).Path
$root = [IO.Path]::GetFullPath($EvidenceRoot)
if (Test-Path -LiteralPath $root) { throw "EvidenceRoot must be fresh: $root" }
New-Item -ItemType Directory -Path $root | Out-Null

Add-Type -AssemblyName System.Windows.Forms
Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
public static class OpenJKDF2LifecycleProbe {
 [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)] public struct D {
  [MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)] public string n; public short a,b,s,e;
  public int f,x,y,o,fo; public short c,du,yr,t,co;
  [MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)] public string fn; public short lp;
  public int bp,w,h,fl,hz,i1,i2,m,d,r1,r2,pw,ph;
 }
 [StructLayout(LayoutKind.Sequential)] public struct R { public int l,t,r,b; public override string ToString(){return l+","+t+","+r+","+b;} }
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern bool EnumDisplaySettings(string n,int m,ref D d);
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")] public static extern bool GetClipCursor(out R r);
 public static string Current(){var d=new D();d.s=(short)Marshal.SizeOf(typeof(D));if(!EnumDisplaySettings(null,-1,ref d))throw new InvalidOperationException();return d.w+"x"+d.h+"@"+d.hz;}
 public static string Clip(){R r;if(!GetClipCursor(out r))throw new InvalidOperationException();return r.ToString();}
}
'@

function Get-AssetSnapshot([string]$Path) {
    @(Get-ChildItem -LiteralPath $Path -Recurse -File | Sort-Object FullName | ForEach-Object {
        "$($_.FullName.Substring($Path.Length))|$($_.Length)|$($_.LastWriteTimeUtc.Ticks)"
    })
}

function New-BorderlessRegistry([string]$Path) {
    [ordered]@{
        InstallType = 9; Window_displayMode = 1; Window_isFullscreen = $true
        Window_isHiDpi = $false; Window_displayMonitor = 0
        Window_windowWidth = 1280; Window_windowHeight = 720; Window_refreshHz = 0
        bIsSingleLevel = $true; serverEpisodeGob = "JK1"; serverMapJkl = "01narshadda.jkl"
        inputRawMouse = $true
    } | ConvertTo-Json | Set-Content -LiteralPath $Path -Encoding utf8
}

function Start-ValidationGame([string]$Case, [int]$DurationMs) {
    $userRoot = Join-Path $root $Case
    New-Item -ItemType Directory -Path $userRoot | Out-Null
    New-BorderlessRegistry (Join-Path $userRoot "registry.json")
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $exePath
    $start.WorkingDirectory = $repoRoot
    $start.UseShellExecute = $false
    [Environment]::SetEnvironmentVariable("OPENJKDF2_VALIDATE_PRESENTATION_MS", [string]$DurationMs, [EnvironmentVariableTarget]::Process)
    [Environment]::SetEnvironmentVariable("OPENJKDF2_VALIDATE_PRESENTATION_VSYNC", "Off", [EnvironmentVariableTarget]::Process)
    [Environment]::SetEnvironmentVariable("OPENJKDF2_VALIDATE_PRESENTATION_FRAME_CAP", "60", [EnvironmentVariableTarget]::Process)
    $start.Arguments = '--data-dir "' + $assetRoot + '" --user-dir "' + $userRoot +
        '" --diagnostics-dir diagnostics -autostart -sp -episode JK1 -map 01narshadda.jkl'
    [pscustomobject]@{ Process = [Diagnostics.Process]::Start($start); UserRoot = $userRoot }
}

function Wait-GameWindow($Run) {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    $log = Join-Path $Run.UserRoot "diagnostics\openjkdf2.jsonl"
    do {
        $Process = $Run.Process
        if ($Process.HasExited) { throw "Game exited before exposing its final window" }
        $Process.Refresh()
        $presentationReady = (Test-Path -LiteralPath $log -PathType Leaf) -and
            (Select-String -LiteralPath $log -Pattern 'presentation requested=' -Quiet)
        if ($presentationReady -and $Process.MainWindowHandle -ne [IntPtr]::Zero) { return $Process.MainWindowHandle }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $deadline)
    throw "Timed out waiting for game window"
}

$assetsBefore = Get-AssetSnapshot $assetRoot
$displayBefore = [OpenJKDF2LifecycleProbe]::Current()
$clipBefore = [OpenJKDF2LifecycleProbe]::Clip()

$focusRun = Start-ValidationGame "focus-loops" 18000
$gameHandle = Wait-GameWindow $focusRun
$shell = New-Object -ComObject WScript.Shell
$focusForm = [Windows.Forms.Form]::new()
$focusForm.Text = "OpenJKDF2 lifecycle focus probe"
$focusForm.Width = 320; $focusForm.Height = 160; $focusForm.TopMost = $true
$focusForm.Show(); [Windows.Forms.Application]::DoEvents()
$focusTransfers = 0
try {
    for ($i = 0; $i -lt 3; $i++) {
        if (-not $shell.AppActivate($focusRun.Process.Id)) { throw "Could not focus game on loop $i" }
        Start-Sleep -Milliseconds 500
        if (-not [OpenJKDF2LifecycleProbe]::SetForegroundWindow($focusForm.Handle)) { throw "Could not transfer focus on loop $i" }
        [Windows.Forms.Application]::DoEvents(); Start-Sleep -Milliseconds 500
        $focusTransfers++
    }
    [void][OpenJKDF2LifecycleProbe]::SetForegroundWindow($gameHandle)
    if (-not $focusRun.Process.WaitForExit($TimeoutSeconds * 1000)) { throw "Focus-loop run timed out" }
}
finally {
    $focusForm.Close(); $focusForm.Dispose()
    if (-not $focusRun.Process.HasExited) { $focusRun.Process.Kill(); $focusRun.Process.WaitForExit() }
}
$focusLog = Join-Path $focusRun.UserRoot "diagnostics\openjkdf2.jsonl"
$lostCount = @(Select-String -LiteralPath $focusLog -Pattern 'window_focus_lost').Count
$gainedCount = @(Select-String -LiteralPath $focusLog -Pattern 'window_focus_gained').Count
$releasedCount = @(Select-String -LiteralPath $focusLog -Pattern 'mouse_capture=released').Count

$forcedRun = Start-ValidationGame "forced-termination" 60000
$forcedHandle = Wait-GameWindow $forcedRun
[void]$shell.AppActivate($forcedRun.Process.Id)
Start-Sleep -Seconds 3
$forcedRun.Process.Kill(); $forcedRun.Process.WaitForExit()
Start-Sleep -Milliseconds 500

$displayAfter = [OpenJKDF2LifecycleProbe]::Current()
$clipAfter = [OpenJKDF2LifecycleProbe]::Clip()
$assetsAfter = Get-AssetSnapshot $assetRoot
$result = [ordered]@{
    schema = 1
    exclusive_runtime_tested = $false
    focus_transfers = $focusTransfers
    focus_lost_events = $lostCount
    focus_gained_events = $gainedCount
    mouse_capture_release_events = $releasedCount
    focus_run_clean_exit = $focusRun.Process.ExitCode -eq 1
    forced_process_terminated = $forcedRun.Process.HasExited
    display_before = $displayBefore
    display_after = $displayAfter
    display_invariant = $displayBefore -eq $displayAfter
    cursor_clip_before = $clipBefore
    cursor_clip_after = $clipAfter
    cursor_clip_restored = $clipBefore -eq $clipAfter
    asset_metadata_invariant = (Compare-Object $assetsBefore $assetsAfter).Count -eq 0
}
$resultPath = Join-Path $root "display-lifecycle-result.json"
$result | ConvertTo-Json | Set-Content -LiteralPath $resultPath -Encoding utf8
$result | ConvertTo-Json
if ($focusTransfers -ne 3 -or $lostCount -lt 3 -or $gainedCount -lt 3 -or $releasedCount -lt 3 -or
    -not $result.focus_run_clean_exit -or -not $result.forced_process_terminated -or
    -not $result.display_invariant -or -not $result.cursor_clip_restored -or
    -not $result.asset_metadata_invariant) {
    throw "Display lifecycle verification failed; inspect $resultPath"
}
