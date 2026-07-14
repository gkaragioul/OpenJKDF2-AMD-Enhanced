[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$DataDir,
    [Parameter(Mandatory = $true)][string]$UserDir,
    [string]$Executable = "build/msvc-release/openjkdf2-64.exe",
    [int]$TimeoutSeconds = 30
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
public static class OpenJKDF2InputProbe {
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

$before = Get-AssetSnapshot $assetRoot
$displayBefore = [OpenJKDF2InputProbe]::Current()
$env:OPENJKDF2_VALIDATE_INPUT_MS = "14000"
$env:OPENJKDF2_VALIDATE_INPUT_SCREENSHOT = "diagnostics\modern-input.png"
$keyUp = 0x0002
$mouseMove = 0x0001
$w = 0x57
$wDown = $false
$focusVerified = $false
try {
    $start = New-Object Diagnostics.ProcessStartInfo
    $start.FileName = $exePath
    $start.WorkingDirectory = $repoRoot
    $start.UseShellExecute = $false
    $start.Arguments = '--data-dir "' + $assetRoot + '" --user-dir "' + $userRoot + '" --diagnostics-dir diagnostics -autostart -sp -episode JK1 -map 01narshadda.jkl'
    $process = [Diagnostics.Process]::Start($start)
    $windowDeadline = [DateTime]::UtcNow.AddSeconds(10)
    do {
        Start-Sleep -Milliseconds 100
        $process.Refresh()
    } while ($process.MainWindowHandle -eq [IntPtr]::Zero -and -not $process.HasExited -and [DateTime]::UtcNow -lt $windowDeadline)
    if ($process.MainWindowHandle -eq [IntPtr]::Zero) { throw "Gameplay window was not created" }
    Start-Sleep -Seconds 6
    $process.Refresh()
    if (-not [OpenJKDF2InputProbe]::SetForegroundWindow($process.MainWindowHandle)) { throw "Could not focus gameplay window" }
    Start-Sleep -Seconds 2
    $focusVerified = [OpenJKDF2InputProbe]::GetForegroundWindow() -eq $process.MainWindowHandle
    if (-not $focusVerified) { throw "Gameplay window did not retain foreground focus" }
    [OpenJKDF2InputProbe]::keybd_event($w, 0, 0, [UIntPtr]::Zero)
    $wDown = $true
    for ($i = 0; $i -lt 8; ++$i) {
        [OpenJKDF2InputProbe]::mouse_event($mouseMove, 30, 0, 0, [UIntPtr]::Zero)
        Start-Sleep -Milliseconds 100
    }
    Start-Sleep -Milliseconds 1200
    [OpenJKDF2InputProbe]::keybd_event($w, 0, $keyUp, [UIntPtr]::Zero)
    $wDown = $false
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) { $process.Kill(); throw "Modern-input probe timed out" }
} finally {
    if ($wDown) { [OpenJKDF2InputProbe]::keybd_event($w, 0, $keyUp, [UIntPtr]::Zero) }
    Remove-Item Env:OPENJKDF2_VALIDATE_INPUT_MS -ErrorAction SilentlyContinue
    Remove-Item Env:OPENJKDF2_VALIDATE_INPUT_SCREENSHOT -ErrorAction SilentlyContinue
}
$displayAfter = [OpenJKDF2InputProbe]::Current()
$after = Get-AssetSnapshot $assetRoot
$diagnostics = Join-Path $userRoot "diagnostics"
$jsonl = Join-Path $diagnostics "openjkdf2.jsonl"
$state = Get-Content -Raw -LiteralPath (Join-Path $diagnostics "run-state.json") | ConvertFrom-Json
$event = Select-String -LiteralPath $jsonl -Pattern "input complete" | Select-Object -Last 1
$result = [ordered]@{
    schema = 1; startup_result = $process.ExitCode
    display_before = $displayBefore; display_after = $displayAfter; display_invariant = $displayBefore -eq $displayAfter
    asset_metadata_invariant = (Compare-Object $before $after).Count -eq 0
    focus_verified = $focusVerified
    moved = [bool](Select-String -LiteralPath $jsonl -Pattern "input complete moved=true" -Quiet)
    turned = [bool](Select-String -LiteralPath $jsonl -Pattern "input complete moved=true turned=true" -Quiet)
    input_event = if ($event) { $event.Line } else { $null }
    screenshot_exists = Test-Path -LiteralPath (Join-Path $diagnostics "modern-input.png")
    clean_state = $state.status -eq "clean"
    process_finished = [bool](Select-String -LiteralPath $jsonl -Pattern "process_finished" -Quiet)
}
$resultPath = Join-Path $userRoot "modern-input-result.json"
$result | ConvertTo-Json | Set-Content -LiteralPath $resultPath -Encoding utf8
$result | ConvertTo-Json
if ($result.startup_result -ne 1 -or -not $result.display_invariant -or -not $result.asset_metadata_invariant -or -not $result.focus_verified -or
    -not $result.moved -or -not $result.turned -or -not $result.screenshot_exists -or
    -not $result.clean_state -or -not $result.process_finished) {
    throw "Modern-input verification failed; inspect $resultPath"
}
