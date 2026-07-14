[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$DataDir,
    [Parameter(Mandatory = $true)][string]$UserDir,
    [string]$Executable = "build/msvc-release/openjkdf2-64.exe",
    [int]$DurationMilliseconds = 12000,
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
if ($assetRoot.Contains('"') -or $userRoot.Contains('"')) { throw "Quoted path characters are unsupported" }

Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
public static class OpenJKDF2EnhancementDisplayProbe {
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
function Get-AssetSnapshot([string]$Root) {
    @(Get-ChildItem -LiteralPath $Root -Recurse -File | Sort-Object FullName | ForEach-Object {
        "$($_.FullName.Substring($Root.Length))|$($_.Length)|$($_.LastWriteTimeUtc.Ticks)"
    })
}

$packRoot = Join-Path $userRoot "jkgm\materials\empty-performance-pack"
[void](New-Item -ItemType Directory -Path $packRoot -Force)
'{"materials":[]}' | Set-Content -LiteralPath (Join-Path $packRoot "metadata.json") -Encoding utf8
$before = Get-AssetSnapshot $assetRoot
$displayBefore = [OpenJKDF2EnhancementDisplayProbe]::Current()
$start = New-Object Diagnostics.ProcessStartInfo
$start.FileName = $exePath
$start.WorkingDirectory = $repoRoot
$start.UseShellExecute = $false
$start.Environment["OPENJKDF2_VALIDATE_ENHANCEMENTS_MS"] = [string]$DurationMilliseconds
$start.Environment["OPENJKDF2_VALIDATE_ENHANCEMENTS_SCREENSHOT"] = "diagnostics\ultra-performance.png"
$start.Arguments = '--data-dir "' + $assetRoot + '" --user-dir "' + $userRoot + '" --diagnostics-dir diagnostics -autostart -sp -episode JK1 -map 01narshadda.jkl'
$process = [Diagnostics.Process]::Start($start)
if (-not $process.WaitForExit($TimeoutSeconds * 1000)) { $process.Kill(); throw "Enhancement performance probe timed out" }
$displayAfter = [OpenJKDF2EnhancementDisplayProbe]::Current()
$after = Get-AssetSnapshot $assetRoot
$diagnostics = Join-Path $userRoot "diagnostics"
$jsonl = Join-Path $diagnostics "openjkdf2.jsonl"
$png = Join-Path $diagnostics "ultra-performance.png"
$state = Get-Content -Raw -LiteralPath (Join-Path $diagnostics "run-state.json") | ConvertFrom-Json
$event = Select-String -LiteralPath $jsonl -Pattern "enhancements complete preset=Ultra" | Select-Object -Last 1
if (-not $event -or $event.Line -notmatch 'total_frames=([0-9]+) samples=([0-9]+) median_ms=([0-9.]+) p95_ms=([0-9.]+) p99_ms=([0-9.]+) worst_ms=([0-9.]+)') {
    throw "Enhancement completion telemetry missing or malformed"
}
$totalFrames = [long]$Matches[1]
$samples = [int]$Matches[2]
$medianMs = [double]::Parse($Matches[3], [Globalization.CultureInfo]::InvariantCulture)
$p95Ms = [double]::Parse($Matches[4], [Globalization.CultureInfo]::InvariantCulture)
$p99Ms = [double]::Parse($Matches[5], [Globalization.CultureInfo]::InvariantCulture)
$worstMs = [double]::Parse($Matches[6], [Globalization.CultureInfo]::InvariantCulture)
Add-Type -AssemblyName System.Drawing
$image = [Drawing.Image]::FromFile($png)
try { $width = $image.Width; $height = $image.Height } finally { $image.Dispose() }
$frameBudgetMs = 1000.0 / 60.0
$result = [ordered]@{
    schema = 1; startup_result = $process.ExitCode
    display_before = $displayBefore; display_after = $displayAfter; display_invariant = $displayBefore -eq $displayAfter
    asset_metadata_invariant = (Compare-Object $before $after).Count -eq 0
    ultra_applied = [bool](Select-String -LiteralPath $jsonl -Pattern "enhancements applied preset=Ultra.*bloom=true ssao=true ssaa=1.5.*replacements=true" -Quiet)
    original_asset_fallback = [bool](Select-String -LiteralPath $jsonl -Pattern "original_asset_fallback reason=no_matching_override" -Quiet)
    total_frames = $totalFrames; samples = $samples; median_ms = $medianMs; p95_ms = $p95Ms; p99_ms = $p99Ms; worst_ms = $worstMs
    pacing_pass = $totalFrames -ge 600 -and $samples -eq 60 -and [Math]::Abs($medianMs - $frameBudgetMs) -le $frameBudgetMs * 0.05 -and $p95Ms -le $frameBudgetMs * 1.15
    screenshot_width = $width; screenshot_height = $height
    clean_state = $state.status -eq "clean"
    process_finished = [bool](Select-String -LiteralPath $jsonl -Pattern "process_finished" -Quiet)
}
$resultPath = Join-Path $userRoot "enhancement-performance-result.json"
$result | ConvertTo-Json | Set-Content -LiteralPath $resultPath -Encoding utf8
$result | ConvertTo-Json
if ($result.startup_result -ne 1 -or -not $result.display_invariant -or -not $result.asset_metadata_invariant -or
    -not $result.ultra_applied -or -not $result.original_asset_fallback -or -not $result.pacing_pass -or
    $width -ne 2560 -or $height -ne 1440 -or -not $result.clean_state -or -not $result.process_finished) {
    throw "Enhancement performance verification failed; inspect $resultPath"
}
