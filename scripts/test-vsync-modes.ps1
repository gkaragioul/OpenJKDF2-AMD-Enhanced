[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$DataDir,
    [Parameter(Mandatory = $true)][string]$EvidenceRoot,
    [string]$Executable = "build/msvc-release/openjkdf2-64.exe",
    [int]$DurationMs = 10000,
    [int]$TimeoutSeconds = 45
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$assetRoot = (Resolve-Path -LiteralPath $DataDir).Path.TrimEnd("\", "/")
$exePath = (Resolve-Path -LiteralPath (Join-Path $repoRoot $Executable)).Path
$root = [IO.Path]::GetFullPath($EvidenceRoot)
if (Test-Path -LiteralPath $root) { throw "EvidenceRoot must be fresh: $root" }
New-Item -ItemType Directory -Path $root | Out-Null

Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
public static class OpenJKDF2VsyncDisplayProbe {
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
Add-Type -AssemblyName System.Drawing

function Get-AssetSnapshot([string]$Path) {
    @(Get-ChildItem -LiteralPath $Path -Recurse -File | Sort-Object FullName | ForEach-Object {
        "$($_.FullName.Substring($Path.Length))|$($_.Length)|$($_.LastWriteTimeUtc.Ticks)"
    })
}

function Invoke-PresentationRun([string]$Mode, [int]$FrameCap) {
    $userRoot = Join-Path $root $Mode.ToLowerInvariant()
    $start = New-Object Diagnostics.ProcessStartInfo
    $start.FileName = $exePath
    $start.WorkingDirectory = $repoRoot
    $start.UseShellExecute = $false
    $start.Environment["OPENJKDF2_VALIDATE_PRESENTATION_MS"] = [string]$DurationMs
    $start.Environment["OPENJKDF2_VALIDATE_PRESENTATION_VSYNC"] = $Mode
    $start.Environment["OPENJKDF2_VALIDATE_PRESENTATION_SCREENSHOT"] = "diagnostics\presentation.png"
    if ($FrameCap) { $start.Environment["OPENJKDF2_VALIDATE_PRESENTATION_FRAME_CAP"] = [string]$FrameCap }
    $start.Arguments = '--data-dir "' + $assetRoot + '" --user-dir "' + $userRoot +
        '" --diagnostics-dir diagnostics -autostart -sp -episode JK1 -map 01narshadda.jkl'
    $process = [Diagnostics.Process]::Start($start)
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        $process.Kill(); $process.WaitForExit(); throw "$Mode presentation run timed out"
    }
    $diagnostics = Join-Path $userRoot "diagnostics"
    $jsonl = Join-Path $diagnostics "openjkdf2.jsonl"
    $event = Select-String -LiteralPath $jsonl -Pattern "presentation complete" | Select-Object -Last 1
    if (-not $event -or $event.Line -notmatch 'requested=([A-Za-z]+) applied=([A-Za-z]+) frame_cap=([-0-9]+) samples=([0-9]+) median_ms=([0-9.]+) p95_ms=([0-9.]+) p99_ms=([0-9.]+) worst_ms=([0-9.]+)') {
        throw "$Mode structured presentation result missing"
    }
    $shot = Join-Path $diagnostics "presentation.png"
    $visibleSamples = 0
    if (Test-Path -LiteralPath $shot) {
        $image = [Drawing.Bitmap]::FromFile($shot)
        try {
            for ($y = 0; $y -lt $image.Height; $y += 32) {
                for ($x = 0; $x -lt $image.Width; $x += 32) {
                    $pixel = $image.GetPixel($x, $y)
                    if (($pixel.R + $pixel.G + $pixel.B) -gt 24) { $visibleSamples++ }
                }
            }
        } finally { $image.Dispose() }
    }
    [ordered]@{
        requested = $Matches[1]
        applied = $Matches[2]
        frame_cap = [int]$Matches[3]
        samples = [int]$Matches[4]
        median_ms = [double]::Parse($Matches[5], [Globalization.CultureInfo]::InvariantCulture)
        p95_ms = [double]::Parse($Matches[6], [Globalization.CultureInfo]::InvariantCulture)
        p99_ms = [double]::Parse($Matches[7], [Globalization.CultureInfo]::InvariantCulture)
        worst_ms = [double]::Parse($Matches[8], [Globalization.CultureInfo]::InvariantCulture)
        exit_code = $process.ExitCode
        screenshot_exists = Test-Path -LiteralPath $shot
        visible_pixel_samples = $visibleSamples
        clean_state = ((Get-Content -Raw -LiteralPath (Join-Path $diagnostics "run-state.json") | ConvertFrom-Json).status -eq "clean")
        process_finished = [bool](Select-String -LiteralPath $jsonl -Pattern "process_finished" -Quiet)
        fallback_logged = [bool](Select-String -LiteralPath $jsonl -Pattern "vsync=adaptive unsupported fallback=on" -Quiet)
    }
}

$assetsBefore = Get-AssetSnapshot $assetRoot
$displayBefore = [OpenJKDF2VsyncDisplayProbe]::Current()
$refreshHz = [int]($displayBefore -replace '^.*@','')
$runs = @(
    (Invoke-PresentationRun "Off" 120),
    (Invoke-PresentationRun "On" 60),
    (Invoke-PresentationRun "Adaptive" 60)
)
$displayAfter = [OpenJKDF2VsyncDisplayProbe]::Current()
$assetsAfter = Get-AssetSnapshot $assetRoot

$offBudget = 1000.0 / 120.0
$vsyncBudget = 1000.0 / 60.0
$off = $runs[0]; $on = $runs[1]; $adaptive = $runs[2]
$result = [ordered]@{
    schema = 1
    display_before = $displayBefore
    display_after = $displayAfter
    display_invariant = $displayBefore -eq $displayAfter
    asset_metadata_invariant = (Compare-Object $assetsBefore $assetsAfter).Count -eq 0
    refresh_hz = $refreshHz
    runs = $runs
    cap_only_pacing_pass = [Math]::Abs($off.median_ms - $offBudget) -le $offBudget * 0.05 -and $off.p95_ms -le $offBudget * 1.15
    vsync_on_pacing_pass = [Math]::Abs($on.median_ms - $vsyncBudget) -le $vsyncBudget * 0.05 -and $on.p95_ms -le $vsyncBudget * 1.15
    adaptive_pacing_pass = [Math]::Abs($adaptive.median_ms - $vsyncBudget) -le $vsyncBudget * 0.05 -and $adaptive.p95_ms -le $vsyncBudget * 1.15
}
$resultPath = Join-Path $root "vsync-modes-result.json"
$result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $resultPath -Encoding utf8
$result | ConvertTo-Json -Depth 6

$runInvariant = @($runs | Where-Object {
    $_.exit_code -ne 1 -or $_.samples -lt 30 -or -not $_.screenshot_exists -or $_.visible_pixel_samples -lt 10 -or
    -not $_.clean_state -or -not $_.process_finished
}).Count -eq 0
$modeInvariant = $off.requested -eq "Off" -and $off.applied -eq "Off" -and $off.frame_cap -eq 120 -and
    $on.requested -eq "On" -and $on.applied -eq "On" -and $on.frame_cap -eq 60 -and
    $adaptive.requested -eq "Adaptive" -and $adaptive.applied -in @("Adaptive", "On") -and $adaptive.frame_cap -eq 60
if (-not $result.display_invariant -or -not $result.asset_metadata_invariant -or -not $runInvariant -or
    -not $modeInvariant -or -not $result.cap_only_pacing_pass -or -not $result.vsync_on_pacing_pass -or
    -not $result.adaptive_pacing_pass) {
    throw "VSync mode verification failed; inspect $resultPath"
}
