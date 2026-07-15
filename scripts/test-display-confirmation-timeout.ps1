[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$DataDir,
    [Parameter(Mandatory = $true)][string]$UserDir,
    [string]$Executable = "build/msvc-release/openjkdf2-64.exe",
    [int]$TimeoutSeconds = 40
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
New-Item -ItemType Directory -Path $userRoot -Force | Out-Null
[ordered]@{
    InstallType = 9; Window_displayMode = 1; Window_isFullscreen = $true
    Window_isHiDpi = $false; Window_displayMonitor = 0
    Window_windowWidth = 1280; Window_windowHeight = 720; Window_refreshHz = 0
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $userRoot "registry.json") -Encoding utf8

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
public static class OpenJKDF2DisplayConfirmationProbe {
 [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)] public struct D {
  [MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)] public string n; public short a,b,s,e;
  public int f,x,y,o,fo; public short c,du,yr,t,co;
  [MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)] public string fn; public short lp;
  public int bp,w,h,fl,hz,i1,i2,m,d,r1,r2,pw,ph;
 }
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern bool EnumDisplaySettings(string n,int m,ref D d);
 [DllImport("user32.dll")] static extern bool PostMessage(IntPtr h,int m,IntPtr w,IntPtr l);
 public static void Skip(IntPtr h){if(h!=IntPtr.Zero)PostMessage(h,0x100,(IntPtr)0x1B,IntPtr.Zero);}
 public static string Current(){var d=new D();d.s=(short)Marshal.SizeOf(typeof(D));if(!EnumDisplaySettings(null,-1,ref d))throw new InvalidOperationException();return d.w+"x"+d.h+"@"+d.hz;}
}
'@
function Get-AssetSnapshot([string]$Root) {
    @(Get-ChildItem -LiteralPath $Root -Recurse -File | Sort-Object FullName | ForEach-Object {
        "$($_.FullName.Substring($Root.Length))|$($_.Length)|$($_.LastWriteTimeUtc.Ticks)"
    })
}
function Save-VirtualDesktopScreenshot([string]$Path) {
    $bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen
    $bitmap = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($bounds.Left, $bounds.Top, 0, 0, $bitmap.Size)
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

$before = Get-AssetSnapshot $assetRoot
$displayBefore = [OpenJKDF2DisplayConfirmationProbe]::Current()
$start = New-Object Diagnostics.ProcessStartInfo
$start.FileName = $exePath
$start.WorkingDirectory = $repoRoot
$start.UseShellExecute = $false
[Environment]::SetEnvironmentVariable("OPENJKDF2_VALIDATE_DISPLAY_REVERT", "1", [EnvironmentVariableTarget]::Process)
$start.Arguments = '--data-dir "' + $assetRoot + '" --user-dir "' + $userRoot + '" --diagnostics-dir diagnostics'
$stopwatch = [Diagnostics.Stopwatch]::StartNew()
$process = [Diagnostics.Process]::Start($start)
$jsonl = Join-Path $userRoot "diagnostics\openjkdf2.jsonl"
$startDeadline = [DateTime]::UtcNow.AddSeconds(15)
$nextSkip = [DateTime]::UtcNow.AddSeconds(2)
$confirmationStarted = $false
while ([DateTime]::UtcNow -lt $startDeadline -and -not $process.HasExited) {
    if ((Test-Path -LiteralPath $jsonl) -and
        (Select-String -LiteralPath $jsonl -Pattern 'display_confirmation_validation started=true' -Quiet)) {
        $confirmationStarted = $true
        break
    }
    if ([DateTime]::UtcNow -ge $nextSkip) {
        $process.Refresh()
        [OpenJKDF2DisplayConfirmationProbe]::Skip($process.MainWindowHandle)
        $nextSkip = [DateTime]::UtcNow.AddSeconds(2)
    }
    Start-Sleep -Milliseconds 100
}
if (-not $confirmationStarted) {
    if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
    throw "Timed display-confirmation route did not start"
}
$confirmationWatch = [Diagnostics.Stopwatch]::StartNew()
Start-Sleep -Seconds 2
$screenshot = Join-Path $userRoot "display-confirmation-dialog.png"
if (-not $process.HasExited) { Save-VirtualDesktopScreenshot $screenshot }
if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
    $process.Kill()
    throw "Timed display-confirmation probe did not exit"
}
$confirmationWatch.Stop()
$stopwatch.Stop()
$displayAfter = [OpenJKDF2DisplayConfirmationProbe]::Current()
$after = Get-AssetSnapshot $assetRoot
$diagnostics = Join-Path $userRoot "diagnostics"
$state = Get-Content -Raw -LiteralPath (Join-Path $diagnostics "run-state.json") | ConvertFrom-Json
$result = [ordered]@{
    schema = 1
    startup_result = $process.ExitCode
    elapsed_ms = $stopwatch.ElapsedMilliseconds
    confirmation_elapsed_ms = $confirmationWatch.ElapsedMilliseconds
    timeout_waited = $confirmationWatch.ElapsedMilliseconds -ge 15000
    dialog_screenshot_exists = Test-Path -LiteralPath $screenshot
    revert_event = [bool](Select-String -LiteralPath $jsonl -Pattern 'display_settings_reverted' -Quiet)
    restored_event = [bool](Select-String -LiteralPath $jsonl -Pattern 'display_confirmation_validation complete=true confirmed=false restored=true' -Quiet)
    validation_pass = [bool](Select-String -LiteralPath $jsonl -Pattern 'display_confirmation_validation result=pass' -Quiet)
    display_before = $displayBefore
    display_after = $displayAfter
    display_invariant = $displayBefore -eq $displayAfter
    asset_metadata_invariant = (Compare-Object $before $after).Count -eq 0
    clean_state = $state.status -eq "clean"
    process_finished = [bool](Select-String -LiteralPath $jsonl -Pattern 'process_finished' -Quiet)
}
$resultPath = Join-Path $userRoot "display-confirmation-result.json"
$result | ConvertTo-Json | Set-Content -LiteralPath $resultPath -Encoding utf8
$result | ConvertTo-Json
if ($result.startup_result -ne 1 -or -not $result.timeout_waited -or -not $result.dialog_screenshot_exists -or
    -not $result.revert_event -or -not $result.restored_event -or -not $result.validation_pass -or
    -not $result.display_invariant -or -not $result.asset_metadata_invariant -or
    -not $result.clean_state -or -not $result.process_finished) {
    throw "Timed display-confirmation verification failed; inspect $resultPath"
}
