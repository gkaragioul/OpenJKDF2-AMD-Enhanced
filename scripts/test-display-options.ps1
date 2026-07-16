[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$DataDir,
    [Parameter(Mandatory = $true)][string]$EvidenceRoot,
    [string]$Executable = "build/msvc-release/openjkdf2-64.exe",
    [int]$DurationMs = 4000,
    [int]$TimeoutSeconds = 40
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$assetRoot = (Resolve-Path -LiteralPath $DataDir).Path.TrimEnd("\", "/")
$exePath = (Resolve-Path -LiteralPath (Join-Path $repoRoot $Executable)).Path
$root = [IO.Path]::GetFullPath($EvidenceRoot)
if (Test-Path -LiteralPath $root) { throw "EvidenceRoot must be fresh: $root" }
New-Item -ItemType Directory -Path $root | Out-Null

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
public static class OpenJKDF2DisplayOptionsProbe {
 [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)] public struct D {
  [MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)] public string n; public short a,b,s,e;
  public int f,x,y,o,fo; public short c,du,yr,t,co;
  [MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)] public string fn; public short lp;
  public int bp,w,h,fl,hz,i1,i2,m,d,r1,r2,pw,ph;
 }
 [StructLayout(LayoutKind.Sequential)] public struct R { public int l,t,r,b; }
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern bool EnumDisplaySettings(string n,int m,ref D d);
 [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr h, out R r);
 public static string Current(){var d=new D();d.s=(short)Marshal.SizeOf(typeof(D));if(!EnumDisplaySettings(null,-1,ref d))throw new InvalidOperationException();return d.w+"x"+d.h+"@"+d.hz;}
 public static string Client(IntPtr h){R r;if(h==IntPtr.Zero||!GetClientRect(h,out r))return "";return (r.r-r.l)+"x"+(r.b-r.t);}
}
'@

function Get-AssetSnapshot([string]$Path) {
    @(Get-ChildItem -LiteralPath $Path -Recurse -File | Sort-Object FullName | ForEach-Object {
        "$($_.FullName.Substring($Path.Length))|$($_.Length)|$($_.LastWriteTimeUtc.Ticks)"
    })
}

function New-Registry([string]$Path, [int]$Mode, [int]$Monitor, [int]$Width, [int]$Height, [int]$Refresh) {
    $registry = [ordered]@{
        InstallType = 9
        Window_defaultsVersion = 2
        Window_displayMode = $Mode
        Window_isFullscreen = ($Mode -ne 0)
        Window_isHiDpi = $false
        Window_displayMonitor = $Monitor
        Window_windowWidth = $Width
        Window_windowHeight = $Height
        Window_refreshHz = $Refresh
        bIsSingleLevel = $true
        serverEpisodeGob = "JK1"
        serverMapJkl = "01narshadda.jkl"
    }
    $registry | ConvertTo-Json | Set-Content -LiteralPath $Path -Encoding utf8
}

function Invoke-DisplayCase([string]$Name, [int]$Mode, [int]$Monitor, [int]$Width, [int]$Height, [int]$ExpectedWidth, [int]$ExpectedHeight) {
    if ($Mode -eq 2) { throw "Exclusive mode is prohibited by this verifier" }
    $userRoot = Join-Path $root $Name
    $diagnostics = Join-Path $userRoot "diagnostics"
    New-Item -ItemType Directory -Path $userRoot | Out-Null
    New-Registry (Join-Path $userRoot "registry.json") $Mode $Monitor $Width $Height 0
    $displayBefore = [OpenJKDF2DisplayOptionsProbe]::Current()
    $start = New-Object Diagnostics.ProcessStartInfo
    $start.FileName = $exePath
    $start.WorkingDirectory = $repoRoot
    $start.UseShellExecute = $false
    [Environment]::SetEnvironmentVariable("OPENJKDF2_VALIDATE_PRESENTATION_MS", [string]$DurationMs, [EnvironmentVariableTarget]::Process)
    [Environment]::SetEnvironmentVariable("OPENJKDF2_VALIDATE_PRESENTATION_VSYNC", "Off", [EnvironmentVariableTarget]::Process)
    [Environment]::SetEnvironmentVariable("OPENJKDF2_VALIDATE_PRESENTATION_FRAME_CAP", "60", [EnvironmentVariableTarget]::Process)
    [Environment]::SetEnvironmentVariable("OPENJKDF2_VALIDATE_PRESENTATION_SCREENSHOT", (Join-Path $userRoot "presentation.bmp"), [EnvironmentVariableTarget]::Process)
    $start.Arguments = '--data-dir "' + $assetRoot + '" --user-dir "' + $userRoot +
        '" --diagnostics-dir diagnostics -autostart -sp -episode JK1 -map 01narshadda.jkl'
    $process = [Diagnostics.Process]::Start($start)
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    $client = ""
    while (-not $process.HasExited -and [DateTime]::UtcNow -lt $deadline) {
        $process.Refresh()
        $client = [OpenJKDF2DisplayOptionsProbe]::Client($process.MainWindowHandle)
        if ($client -eq "$($ExpectedWidth)x$($ExpectedHeight)") { break }
        Start-Sleep -Milliseconds 100
    }
    if (-not $client) {
        if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
        throw "$Name did not expose a measurable game window"
    }
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        $process.Kill(); $process.WaitForExit(); throw "$Name timed out"
    }
    $displayAfter = [OpenJKDF2DisplayOptionsProbe]::Current()
    $log = Join-Path $diagnostics "openjkdf2.jsonl"
    $state = Get-Content -Raw -LiteralPath (Join-Path $diagnostics "run-state.json") | ConvertFrom-Json
    $registryAfter = Get-Content -Raw -LiteralPath (Join-Path $userRoot "registry.json") | ConvertFrom-Json
    $screenshotPath = Join-Path $userRoot "presentation.bmp"
    $screenshotSize = ""
    if (Test-Path -LiteralPath $screenshotPath) {
        $image = [Drawing.Image]::FromFile($screenshotPath)
        try { $screenshotSize = "$($image.Width)x$($image.Height)" }
        finally { $image.Dispose() }
    }
    [ordered]@{
        name = $Name
        requested_mode = @("Windowed","Borderless")[$Mode]
        monitor_ordinal = $Monitor
        requested_client = "$($Width)x$($Height)"
        measured_client = $client
        expected_client = "$($ExpectedWidth)x$($ExpectedHeight)"
        client_size_pass = $client -eq "$($ExpectedWidth)x$($ExpectedHeight)"
        display_before = $displayBefore
        display_after = $displayAfter
        display_invariant = $displayBefore -eq $displayAfter
        exit_code = $process.ExitCode
        clean_state = $state.status -eq "clean"
        process_finished = [bool](Select-String -LiteralPath $log -Pattern "process_finished" -Quiet)
        shader_failure = [bool](Select-String -LiteralPath $log -Pattern '"severity":"error".*shader' -Quiet)
        persisted_mode = [int]$registryAfter.Window_displayMode
        persisted_defaults_version = [int]$registryAfter.Window_defaultsVersion
        persisted_monitor = [int]$registryAfter.Window_displayMonitor
        persisted_width = [int]$registryAfter.Window_windowWidth
        persisted_height = [int]$registryAfter.Window_windowHeight
        persisted_window_size_pass =
            [int]$registryAfter.Window_windowWidth -eq $Width -and
            [int]$registryAfter.Window_windowHeight -eq $Height
        screenshot = $screenshotPath
        screenshot_size = $screenshotSize
        screenshot_size_pass = $screenshotSize -eq "$($ExpectedWidth)x$($ExpectedHeight)"
    }
}

$assetsBefore = Get-AssetSnapshot $assetRoot
$desktop = [OpenJKDF2DisplayOptionsProbe]::Current()
$desktopParts = [regex]::Match($desktop, '^(\d+)x(\d+)@(\d+)$')
if (-not $desktopParts.Success) { throw "Unexpected desktop state: $desktop" }
$desktopWidth = [int]$desktopParts.Groups[1].Value
$desktopHeight = [int]$desktopParts.Groups[2].Value
$desktopRefresh = [int]$desktopParts.Groups[3].Value
$screens = @([Windows.Forms.Screen]::AllScreens)

$runs = @(
    (Invoke-DisplayCase "windowed-1920x1080" 0 0 1920 1080 1920 1080),
    (Invoke-DisplayCase "windowed-3840x2160" 0 0 3840 2160 3840 2160),
    (Invoke-DisplayCase "borderless-primary" 1 0 800 600 $desktopWidth $desktopHeight)
)
if ($screens.Count -gt 1) {
    $second = $screens[1].Bounds
    $runs += Invoke-DisplayCase "windowed-monitor-2" 0 1 1280 720 1280 720
}

$assetsAfter = Get-AssetSnapshot $assetRoot
$result = [ordered]@{
    schema = 1
    exclusive_runtime_tested = $false
    desktop = [ordered]@{ width=$desktopWidth; height=$desktopHeight; refresh_hz=$desktopRefresh }
    detected_monitor_count = $screens.Count
    multi_monitor_status = if ($screens.Count -gt 1) { "verified-windowed-selection" } else { "unverified-single-monitor-host" }
    tested_refresh_rates = @($desktopRefresh)
    unavailable_refresh_rates = @(60,120,144,165 | Where-Object { $_ -ne $desktopRefresh })
    asset_file_count_before = $assetsBefore.Count
    asset_file_count_after = $assetsAfter.Count
    asset_metadata_invariant = (Compare-Object $assetsBefore $assetsAfter).Count -eq 0
    runs = $runs
}
$resultPath = Join-Path $root "display-options-result.json"
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $resultPath -Encoding utf8
$result | ConvertTo-Json -Depth 8

$failed = @($runs | Where-Object {
    -not $_.client_size_pass -or -not $_.display_invariant -or $_.exit_code -ne 1 -or
    -not $_.clean_state -or -not $_.process_finished -or $_.shader_failure -or
    -not $_.persisted_window_size_pass -or -not $_.screenshot_size_pass -or
    $_.persisted_defaults_version -ne 2 -or
    $_.persisted_mode -ne (@{"Windowed"=0;"Borderless"=1}[$_.requested_mode]) -or
    $_.persisted_monitor -ne $_.monitor_ordinal
})
if (-not $result.asset_metadata_invariant -or $failed.Count -ne 0) {
    throw "Display options verification failed; inspect $resultPath"
}
