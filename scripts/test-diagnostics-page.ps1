[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$DataDir,
    [Parameter(Mandatory = $true)][string]$EvidenceRoot,
    [string]$Executable = 'build/msvc-release/openjkdf2-64.exe',
    [int]$TimeoutSeconds = 45
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$assetRoot = (Resolve-Path -LiteralPath $DataDir).Path.TrimEnd('\', '/')
$exePath = (Resolve-Path -LiteralPath (Join-Path $repoRoot $Executable)).Path
$root = [IO.Path]::GetFullPath($EvidenceRoot)
if (Test-Path -LiteralPath $root) { throw "EvidenceRoot must be fresh: $root" }
$userRoot = Join-Path $root 'UserData'
$diagnostics = Join-Path $userRoot 'diagnostics'
New-Item -ItemType Directory -Path $userRoot -Force | Out-Null

Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System; using System.Drawing; using System.Drawing.Imaging; using System.Runtime.InteropServices;
public static class OpenJKDF2DiagnosticsCapture {
 [StructLayout(LayoutKind.Sequential)] struct RECT { public int Left,Top,Right,Bottom; }
 [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] static extern bool PostMessage(IntPtr h, int m, IntPtr w, IntPtr l);
 public static void Skip(IntPtr h) { if(h!=IntPtr.Zero) PostMessage(h,0x100,(IntPtr)0x1B,IntPtr.Zero); }
 public static string Capture(IntPtr h, string path) {
  RECT r; if (!GetWindowRect(h, out r)) throw new InvalidOperationException("GetWindowRect failed");
  int w=r.Right-r.Left, ht=r.Bottom-r.Top; using(var b=new Bitmap(w,ht)) using(var g=Graphics.FromImage(b)) {
   g.CopyFromScreen(r.Left,r.Top,0,0,new Size(w,ht),CopyPixelOperation.SourceCopy); b.Save(path,ImageFormat.Png);
  } return w+"x"+ht;
 }
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
    bIsSingleLevel = $true; serverEpisodeGob = 'JK1'; serverMapJkl = '01narshadda.jkl'
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $userRoot 'registry.json') -Encoding utf8

$assetsBefore = Get-AssetSnapshot $assetRoot
$start = [Diagnostics.ProcessStartInfo]::new()
$start.FileName = $exePath
$start.WorkingDirectory = $repoRoot
$start.UseShellExecute = $false
[Environment]::SetEnvironmentVariable('OPENJKDF2_VALIDATE_DIAGNOSTICS_PAGE', '1', [EnvironmentVariableTarget]::Process)
$start.Arguments = '--data-dir "' + $assetRoot + '" --user-dir "' + $userRoot +
    '" --diagnostics-dir diagnostics'
$process = [Diagnostics.Process]::Start($start)
$log = Join-Path $diagnostics 'openjkdf2.jsonl'
$deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
$displayed = $false
$nextSkip = [DateTime]::UtcNow.AddSeconds(2)
while ([DateTime]::UtcNow -lt $deadline -and -not $process.HasExited) {
    if ((Test-Path -LiteralPath $log) -and
        (Select-String -LiteralPath $log -Pattern 'diagnostics_page displayed=true' -Quiet)) {
        $displayed = $true; break
    }
    if ([DateTime]::UtcNow -ge $nextSkip) {
        $process.Refresh(); [OpenJKDF2DiagnosticsCapture]::Skip($process.MainWindowHandle)
        $nextSkip = [DateTime]::UtcNow.AddSeconds(2)
    }
    Start-Sleep -Milliseconds 100
}
if (-not $displayed) { if (-not $process.HasExited) { $process.Kill() }; throw 'Diagnostics modal did not appear' }
Start-Sleep -Milliseconds 750
$handleDeadline = [DateTime]::UtcNow.AddSeconds(5)
do { $process.Refresh(); Start-Sleep -Milliseconds 100 } while ($process.MainWindowHandle -eq 0 -and [DateTime]::UtcNow -lt $handleDeadline)
if ($process.MainWindowHandle -eq 0) { $process.Kill(); throw 'Game window handle unavailable' }
$shot = Join-Path $root 'diagnostics-page.png'
$captureDimensions = [OpenJKDF2DiagnosticsCapture]::Capture($process.MainWindowHandle, $shot)
$shell = New-Object -ComObject WScript.Shell
$activated = $shell.AppActivate($process.Id)
if ($activated) { Start-Sleep -Milliseconds 200; $shell.SendKeys('{ENTER}') }
if (-not $process.WaitForExit($TimeoutSeconds * 1000)) { $process.Kill(); $process.WaitForExit(); throw 'Diagnostics run timed out' }

$image = [Drawing.Bitmap]::FromFile($shot)
try {
    $visible = 0
    $diagnosticRed = 0
    for ($y=0; $y -lt $image.Height; $y+=16) { for ($x=0; $x -lt $image.Width; $x+=16) {
        $p=$image.GetPixel($x,$y); if (($p.R+$p.G+$p.B) -gt 30) { $visible++ }
        if ($p.R -gt 90 -and $p.R -gt ($p.G * 1.5) -and $p.R -gt ($p.B * 1.5)) { $diagnosticRed++ }
    }}
} finally { $image.Dispose() }
$assetsAfter = Get-AssetSnapshot $assetRoot
$result = [ordered]@{
    schema = 1; exclusive_runtime_tested = $false; exit_code = $process.ExitCode
    displayed = $displayed; dismissed = [bool](Select-String -LiteralPath $log -Pattern 'diagnostics_page dismissed=true' -Quiet)
    window_activated = [bool]$activated; capture = $shot; capture_dimensions = $captureDimensions
    visible_pixel_samples = $visible; diagnostic_red_samples = $diagnosticRed
    asset_metadata_invariant = (Compare-Object $assetsBefore $assetsAfter).Count -eq 0
    clean_state = ((Get-Content -Raw -LiteralPath (Join-Path $diagnostics 'run-state.json') | ConvertFrom-Json).status -eq 'clean')
}
$result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $root 'diagnostics-page-result.json') -Encoding utf8
$result | ConvertTo-Json
if ($result.exit_code -ne 1 -or -not $result.displayed -or -not $result.dismissed -or
    -not $result.window_activated -or $result.visible_pixel_samples -lt 100 -or $result.diagnostic_red_samples -lt 20 -or
    -not $result.asset_metadata_invariant -or -not $result.clean_state) { throw 'Diagnostics page verification failed' }
