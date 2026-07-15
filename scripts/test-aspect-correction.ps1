[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$DataDir,
    [Parameter(Mandatory = $true)][string]$EvidenceRoot,
    [string]$Executable = "build/msvc-release/openjkdf2-64.exe",
    [int]$TimeoutSeconds = 40
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
public static class OpenJKDF2AspectDisplayProbe {
 [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)] public struct D {
  [MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)] public string n; public short a,b,s,e;
  public int f,x,y,o,fo; public short c,du,yr,t,co;
  [MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)] public string fn; public short lp;
  public int bp,w,h,fl,hz,i1,i2,m,d,r1,r2,pw,ph;
 }
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern bool EnumDisplaySettings(string n,int m,ref D d);
 [DllImport("user32.dll")] static extern bool PostMessage(IntPtr h,int m,IntPtr w,IntPtr l);
 public static string Current(){var d=new D();d.s=(short)Marshal.SizeOf(typeof(D));if(!EnumDisplaySettings(null,-1,ref d))throw new InvalidOperationException();return d.w+"x"+d.h+"@"+d.hz;}
 public static void Skip(IntPtr h){if(h!=IntPtr.Zero)PostMessage(h,0x100,(IntPtr)0x1B,IntPtr.Zero);}
}
'@
Add-Type -AssemblyName System.Drawing

function Get-AssetSnapshot([string]$Path) {
    @(Get-ChildItem -LiteralPath $Path -Recurse -File | Sort-Object FullName | ForEach-Object {
        "$($_.FullName.Substring($Path.Length))|$($_.Length)|$($_.LastWriteTimeUtc.Ticks)"
    })
}

function Invoke-AspectRun([string]$Domain, [string]$Arguments) {
    $userRoot = Join-Path $root $Domain
    $diagnostics = Join-Path $userRoot "diagnostics"
    $shotRelative = "diagnostics\aspect-$Domain.png"
    $start = New-Object Diagnostics.ProcessStartInfo
    $start.FileName = $exePath
    $start.WorkingDirectory = $repoRoot
    $start.UseShellExecute = $false
    [Environment]::SetEnvironmentVariable("OPENJKDF2_ASPECT_CAPTURE", $Domain, [EnvironmentVariableTarget]::Process)
    [Environment]::SetEnvironmentVariable("OPENJKDF2_ASPECT_CAPTURE_PATH", $shotRelative, [EnvironmentVariableTarget]::Process)
    $start.Arguments = '--data-dir "' + $assetRoot + '" --user-dir "' + $userRoot +
        '" --diagnostics-dir diagnostics ' + $Arguments
    $process = [Diagnostics.Process]::Start($start)
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    $nextSkip = [DateTime]::MinValue
    while (-not $process.HasExited -and [DateTime]::UtcNow -lt $deadline) {
        if ($Domain -eq "menu" -and [DateTime]::UtcNow -ge $nextSkip) {
            $process.Refresh()
            if ($process.MainWindowHandle -ne [IntPtr]::Zero) {
                [OpenJKDF2AspectDisplayProbe]::Skip($process.MainWindowHandle)
                $nextSkip = [DateTime]::UtcNow.AddSeconds(5)
            }
        }
        Start-Sleep -Milliseconds 250
    }
    if (-not $process.HasExited) {
        $process.Kill()
        $process.WaitForExit()
        throw "$Domain aspect run timed out"
    }
    $shot = Join-Path $userRoot $shotRelative
    $log = Join-Path $diagnostics "openjkdf2.jsonl"
    if (-not (Test-Path -LiteralPath $shot)) { throw "$Domain capture missing: $shot" }
    $image = [Drawing.Bitmap]::FromFile($shot)
    try {
        $width = $image.Width; $height = $image.Height
        $visibleSamples = 0
        for ($y = 0; $y -lt $height; $y += 32) {
            for ($x = 0; $x -lt $width; $x += 32) {
                $pixel = $image.GetPixel($x, $y)
                if (($pixel.R + $pixel.G + $pixel.B) -gt 24) { $visibleSamples++ }
            }
        }
    } finally { $image.Dispose() }
    $event = Select-String -LiteralPath $log -Pattern "aspect_capture domain=$Domain " | Select-Object -Last 1
    if (-not $event) { throw "$Domain structured aspect event missing" }
    [ordered]@{
        domain = $Domain
        exit_code = $process.ExitCode
        width = $width
        height = $height
        visible_pixel_samples = $visibleSamples
        capture = $shot.Substring($root.Length).TrimStart("\", "/")
        structured_event = $event.Line
        clean_state = ((Get-Content -Raw -LiteralPath (Join-Path $diagnostics "run-state.json") | ConvertFrom-Json).status -eq "clean")
        process_finished = [bool](Select-String -LiteralPath $log -Pattern "process_finished" -Quiet)
    }
}

$assetsBefore = Get-AssetSnapshot $assetRoot
$displayBefore = [OpenJKDF2AspectDisplayProbe]::Current()
$runs = @(
    (Invoke-AspectRun "video" ""),
    (Invoke-AspectRun "menu" ""),
    (Invoke-AspectRun "hud" "-autostart -sp -episode JK1 -map 01narshadda.jkl")
)
$displayAfter = [OpenJKDF2AspectDisplayProbe]::Current()
$assetsAfter = Get-AssetSnapshot $assetRoot

$result = [ordered]@{
    schema = 1
    display_before = $displayBefore
    display_after = $displayAfter
    display_invariant = $displayBefore -eq $displayAfter
    asset_file_count_before = $assetsBefore.Count
    asset_file_count_after = $assetsAfter.Count
    asset_metadata_invariant = (Compare-Object $assetsBefore $assetsAfter).Count -eq 0
    runs = $runs
}
$resultPath = Join-Path $root "aspect-correction-result.json"
$result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $resultPath -Encoding utf8
$result | ConvertTo-Json -Depth 6

if (-not $result.display_invariant -or -not $result.asset_metadata_invariant -or
    @($runs | Where-Object { $_.exit_code -ne 1 -or $_.width -ne 2560 -or $_.height -ne 1440 -or $_.visible_pixel_samples -lt 10 -or
        -not $_.clean_state -or -not $_.process_finished }).Count -ne 0) {
    throw "Aspect correction verification failed; inspect $resultPath"
}
