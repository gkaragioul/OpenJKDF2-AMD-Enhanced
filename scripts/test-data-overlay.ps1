[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$DataDir,
    [Parameter(Mandatory = $true)][string]$UserDir,
    [string]$Executable = "build/msvc-release/openjkdf2-64.exe",
    [int]$TimeoutSeconds = 30,
    [switch]$EmptyEnhancementPack
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
public static class OpenJKDF2OverlayDisplayProbe {
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

if ($EmptyEnhancementPack) {
    $packRoot = Join-Path $userRoot "jkgm\materials\empty-contract-pack"
    [void](New-Item -ItemType Directory -Path $packRoot -Force)
    '{"materials":[]}' | Set-Content -LiteralPath (Join-Path $packRoot "metadata.json") -Encoding utf8
}

$before = Get-AssetSnapshot $assetRoot
$displayBefore = [OpenJKDF2OverlayDisplayProbe]::Current()
$env:OPENJKDF2_AUTOSHOT_MS = "5000"
$env:OPENJKDF2_AUTOSHOT_PATH = "diagnostics\first-level.png"
$start = New-Object Diagnostics.ProcessStartInfo
$start.FileName = $exePath
$start.WorkingDirectory = $repoRoot
$start.UseShellExecute = $false
$start.Arguments = '--data-dir "' + $assetRoot + '" --user-dir "' + $userRoot + '" --diagnostics-dir diagnostics -autostart -sp -episode JK1 -map 01narshadda.jkl'
$process = [Diagnostics.Process]::Start($start)
if (-not $process.WaitForExit($TimeoutSeconds * 1000)) { $process.Kill(); throw "Overlay probe timed out" }
$displayAfter = [OpenJKDF2OverlayDisplayProbe]::Current()
$after = Get-AssetSnapshot $assetRoot
$diagnostics = Join-Path $userRoot "diagnostics"
$png = Join-Path $diagnostics "first-level.png"
$jsonl = Join-Path $diagnostics "openjkdf2.jsonl"
$state = Get-Content -Raw -LiteralPath (Join-Path $diagnostics "run-state.json") | ConvertFrom-Json
Add-Type -AssemblyName System.Drawing
$image = [Drawing.Image]::FromFile($png)
try {
    $width = $image.Width; $height = $image.Height
    $bitmap = [Drawing.Bitmap]$image
    $luminanceSum = 0.0; $litSamples = 0; $sampleCount = 0
    for ($y = 0; $y -lt $height; $y += 16) {
        for ($x = 0; $x -lt $width; $x += 16) {
            $pixel = $bitmap.GetPixel($x, $y)
            $luminance = ($pixel.R + $pixel.G + $pixel.B) / 3.0
            $luminanceSum += $luminance
            if ($luminance -gt 12) { ++$litSamples }
            ++$sampleCount
        }
    }
    $meanLuminance = $luminanceSum / $sampleCount
    $litFraction = $litSamples / $sampleCount
} finally { $image.Dispose() }
$result = [ordered]@{
    schema = 1; startup_result = $process.ExitCode
    display_before = $displayBefore; display_after = $displayAfter; display_invariant = $displayBefore -eq $displayAfter
    asset_file_count_before = $before.Count; asset_file_count_after = $after.Count
    asset_metadata_invariant = (Compare-Object $before $after).Count -eq 0
    screenshot_width = $width; screenshot_height = $height
    screenshot_mean_luminance = [Math]::Round($meanLuminance, 2)
    screenshot_lit_fraction = [Math]::Round($litFraction, 4)
    clean_state = $state.status -eq "clean"
    storage_event = [bool](Select-String -LiteralPath $jsonl -Pattern "path_overlay active=true writable=user" -Quiet)
    validation_event = [bool](Select-String -LiteralPath $jsonl -Pattern "gameplay_screenshot_requested" -Quiet)
    enhancement_pack_seeded = [bool]$EmptyEnhancementPack
    original_asset_fallback = [bool](Select-String -LiteralPath $jsonl -Pattern "original_asset_fallback reason=no_matching_override" -Quiet)
    process_finished = [bool](Select-String -LiteralPath $jsonl -Pattern "process_finished" -Quiet)
    user_files = @(Get-ChildItem -LiteralPath $userRoot -Recurse -File | ForEach-Object { $_.FullName.Substring($userRoot.Length).TrimStart("\", "/") })
}
$resultPath = Join-Path $userRoot "overlay-result.json"
$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $resultPath -Encoding utf8
$result | ConvertTo-Json -Depth 4
if ($result.startup_result -ne 1 -or -not $result.display_invariant -or -not $result.asset_metadata_invariant -or
    $width -ne 2560 -or $height -ne 1440 -or $meanLuminance -lt 20 -or $litFraction -lt 0.5 -or
    -not $result.clean_state -or -not $result.storage_event -or
    ($EmptyEnhancementPack -and -not $result.original_asset_fallback) -or
    -not $result.validation_event -or -not $result.process_finished) {
    throw "Data-overlay verification failed; inspect $resultPath"
}
