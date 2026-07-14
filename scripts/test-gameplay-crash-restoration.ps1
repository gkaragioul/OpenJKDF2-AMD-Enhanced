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
$userRoot = Join-Path $root "UserData"
$diagnostics = Join-Path $userRoot "diagnostics"
New-Item -ItemType Directory -Path $userRoot | Out-Null

Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
public static class OpenJKDF2CrashDisplayProbe {
 [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)] public struct D {
  [MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)] public string n; public short a,b,s,e;
  public int f,x,y,o,fo; public short c,du,yr,t,co;
  [MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)] public string fn; public short lp;
  public int bp,w,h,fl,hz,i1,i2,m,d,r1,r2,pw,ph;
 }
 [StructLayout(LayoutKind.Sequential)] public struct R { public int l,t,r,b; public override string ToString(){return l+","+t+","+r+","+b;} }
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern bool EnumDisplaySettings(string n,int m,ref D d);
 [DllImport("user32.dll")] static extern bool GetClipCursor(out R r);
 public static string Current(){var d=new D();d.s=(short)Marshal.SizeOf(typeof(D));if(!EnumDisplaySettings(null,-1,ref d))throw new InvalidOperationException();return d.w+"x"+d.h+"@"+d.hz;}
 public static string Clip(){R r;if(!GetClipCursor(out r))throw new InvalidOperationException();return r.ToString();}
}
'@

function Get-AssetSnapshot([string]$Path) {
    @(Get-ChildItem -LiteralPath $Path -Recurse -File | Sort-Object FullName | ForEach-Object {
        "$($_.FullName.Substring($Path.Length))|$($_.Length)|$($_.LastWriteTimeUtc.Ticks)"
    })
}

[ordered]@{
    InstallType = 9; Window_displayMode = 1; Window_isFullscreen = $true
    Window_isHiDpi = $false; Window_displayMonitor = 0
    Window_windowWidth = 1280; Window_windowHeight = 720; Window_refreshHz = 0
    bIsSingleLevel = $true; serverEpisodeGob = "JK1"; serverMapJkl = "01narshadda.jkl"
    inputRawMouse = $true
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $userRoot "registry.json") -Encoding utf8

function Start-Game([bool]$Crash) {
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $exePath
    $start.WorkingDirectory = $repoRoot
    $start.UseShellExecute = $false
    if ($Crash) {
        $start.Environment["OPENJKDF2_VALIDATE_CRASH_MS"] = "3000"
    } else {
        $start.Environment["OPENJKDF2_VALIDATE_PRESENTATION_MS"] = "3000"
        $start.Environment["OPENJKDF2_VALIDATE_PRESENTATION_VSYNC"] = "Off"
        $start.Environment["OPENJKDF2_VALIDATE_PRESENTATION_FRAME_CAP"] = "60"
        $start.Environment["OPENJKDF2_VALIDATE_PRESENTATION_SCREENSHOT"] = (Join-Path $root "recovery.bmp")
    }
    $start.Arguments = '--data-dir "' + $assetRoot + '" --user-dir "' + $userRoot +
        '" --diagnostics-dir diagnostics -autostart -sp -episode JK1 -map 01narshadda.jkl'
    [Diagnostics.Process]::Start($start)
}

$assetsBefore = Get-AssetSnapshot $assetRoot
$displayBefore = [OpenJKDF2CrashDisplayProbe]::Current()
$clipBefore = [OpenJKDF2CrashDisplayProbe]::Clip()
$crashProcess = Start-Game $true
if (-not $crashProcess.WaitForExit($TimeoutSeconds * 1000)) {
    $crashProcess.Kill(); $crashProcess.WaitForExit(); throw "Gameplay crash trigger timed out"
}
Start-Sleep -Milliseconds 750
$reportPath = Join-Path $diagnostics "OpenJKDF2-crash.RPT"
$reportExists = Test-Path -LiteralPath $reportPath -PathType Leaf
$reportBytes = if ($reportExists) { (Get-Item -LiteralPath $reportPath).Length } else { 0 }
$reportRecognized = $reportExists -and
    (Get-Content -Raw -LiteralPath $reportPath) -match '(?i)exception|access violation|stack'
$crashLog = Join-Path $diagnostics "openjkdf2.jsonl"
$crashRequested = (Test-Path -LiteralPath $crashLog -PathType Leaf) -and
    (Select-String -LiteralPath $crashLog -Pattern 'gameplay_crash requested=true' -Quiet)
$displayAfterCrash = [OpenJKDF2CrashDisplayProbe]::Current()
$clipAfterCrash = [OpenJKDF2CrashDisplayProbe]::Clip()

# Prove the same last-known-good profile can relaunch and shut down cleanly.
$recoveryProcess = Start-Game $false
$shell = New-Object -ComObject WScript.Shell
$promptDeadline = [DateTime]::UtcNow.AddSeconds(15)
$recoveryPromptAccepted = $false
while ([DateTime]::UtcNow -lt $promptDeadline -and -not $recoveryProcess.HasExited) {
    if ($shell.AppActivate("OpenJKDF2 AMD Enhanced - Recovery")) {
        Start-Sleep -Milliseconds 250
        $shell.SendKeys("{ENTER}")
        $recoveryPromptAccepted = $true
        break
    }
    Start-Sleep -Milliseconds 100
}
if (-not $recoveryProcess.WaitForExit($TimeoutSeconds * 1000)) {
    $recoveryProcess.Kill(); $recoveryProcess.WaitForExit(); throw "Post-crash recovery run timed out"
}
$state = Get-Content -Raw -LiteralPath (Join-Path $diagnostics "run-state.json") | ConvertFrom-Json
$displayAfterRecovery = [OpenJKDF2CrashDisplayProbe]::Current()
$clipAfterRecovery = [OpenJKDF2CrashDisplayProbe]::Clip()
$assetsAfter = Get-AssetSnapshot $assetRoot

$result = [ordered]@{
    schema = 1
    exclusive_runtime_tested = $false
    crash_requested = [bool]$crashRequested
    crash_exit_code = $crashProcess.ExitCode
    crash_was_abnormal = $crashProcess.ExitCode -ne 1
    crash_report_created = [bool]$reportExists
    crash_report_bytes = $reportBytes
    crash_report_recognized = [bool]$reportRecognized
    display_before = $displayBefore
    display_after_crash = $displayAfterCrash
    display_after_recovery = $displayAfterRecovery
    display_invariant = $displayBefore -eq $displayAfterCrash -and $displayBefore -eq $displayAfterRecovery
    cursor_clip_before = $clipBefore
    cursor_clip_after_crash = $clipAfterCrash
    cursor_clip_after_recovery = $clipAfterRecovery
    cursor_clip_restored = $clipBefore -eq $clipAfterCrash -and $clipBefore -eq $clipAfterRecovery
    recovery_exit_code = $recoveryProcess.ExitCode
    recovery_prompt_accepted = $recoveryPromptAccepted
    last_known_good_restored = [bool](Select-String -LiteralPath $crashLog -Pattern 'last_known_good_restored safe_mode=true' -Quiet)
    recovery_clean_state = $state.status -eq "clean"
    recovery_screenshot = Test-Path -LiteralPath (Join-Path $root "recovery.bmp")
    asset_metadata_invariant = (Compare-Object $assetsBefore $assetsAfter).Count -eq 0
}
$resultPath = Join-Path $root "gameplay-crash-restoration-result.json"
$result | ConvertTo-Json | Set-Content -LiteralPath $resultPath -Encoding utf8
$result | ConvertTo-Json
if (-not $result.crash_requested -or -not $result.crash_was_abnormal -or
    -not $result.crash_report_created -or $result.crash_report_bytes -lt 256 -or
    -not $result.crash_report_recognized -or -not $result.display_invariant -or
    -not $result.cursor_clip_restored -or $result.recovery_exit_code -ne 1 -or
    -not $result.recovery_prompt_accepted -or -not $result.last_known_good_restored -or
    -not $result.recovery_clean_state -or -not $result.recovery_screenshot -or
    -not $result.asset_metadata_invariant) {
    throw "Gameplay crash restoration verification failed; inspect $resultPath"
}
