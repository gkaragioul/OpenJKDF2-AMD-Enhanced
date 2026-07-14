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

Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
public static class OpenJKDF2SaveLoadDisplayProbe {
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

$before = Get-AssetSnapshot $assetRoot
$displayBefore = [OpenJKDF2SaveLoadDisplayProbe]::Current()
function Invoke-ValidationProcess([string]$ModeVariable) {
    Set-Item -Path ("Env:" + $ModeVariable) -Value "1"
    try {
        $start = New-Object Diagnostics.ProcessStartInfo
        $start.FileName = $exePath
        $start.WorkingDirectory = $repoRoot
        $start.UseShellExecute = $false
        $start.Arguments = '--data-dir "' + $assetRoot + '" --user-dir "' + $userRoot + '" --diagnostics-dir diagnostics -autostart -sp -episode JK1 -map 01narshadda.jkl'
        $process = [Diagnostics.Process]::Start($start)
        if (-not $process.WaitForExit($TimeoutSeconds * 1000)) { $process.Kill(); throw "Save/load probe timed out" }
        return $process.ExitCode
    } finally {
        Remove-Item -Path ("Env:" + $ModeVariable) -ErrorAction SilentlyContinue
    }
}
$sameProcessExit = Invoke-ValidationProcess "OPENJKDF2_VALIDATE_SAVE_LOAD"
$diagnostics = Join-Path $userRoot "diagnostics"
$jsonl = Join-Path $diagnostics "openjkdf2.jsonl"
$sameProcessLog = Join-Path $diagnostics "same-process.jsonl"
Copy-Item -LiteralPath $jsonl -Destination $sameProcessLog
$freshProcessExit = Invoke-ValidationProcess "OPENJKDF2_VALIDATE_RESTORE_ONLY"
$displayAfter = [OpenJKDF2SaveLoadDisplayProbe]::Current()
$after = Get-AssetSnapshot $assetRoot
$state = Get-Content -Raw -LiteralPath (Join-Path $diagnostics "run-state.json") | ConvertFrom-Json
$validationSave = Get-ChildItem -LiteralPath $userRoot -Recurse -File -Filter "_JKVALIDATE_SAVE_LOAD.jks" | Select-Object -First 1
$result = [ordered]@{
    schema = 1; same_process_exit = $sameProcessExit; fresh_process_exit = $freshProcessExit
    display_before = $displayBefore; display_after = $displayAfter; display_invariant = $displayBefore -eq $displayAfter
    asset_metadata_invariant = (Compare-Object $before $after).Count -eq 0
    validation_save_exists = $null -ne $validationSave
    validation_save_bytes = if ($validationSave) { $validationSave.Length } else { 0 }
    save_requested = [bool](Select-String -LiteralPath $sameProcessLog -Pattern "save_load save_requested" -Quiet)
    state_perturbed = [bool](Select-String -LiteralPath $sameProcessLog -Pattern "save_load state_perturbed" -Quiet)
    same_process_restore_requested = [bool](Select-String -LiteralPath $sameProcessLog -Pattern "save_load restore_requested" -Quiet)
    same_process_restored = [bool](Select-String -LiteralPath $sameProcessLog -Pattern "save_load complete same_process=true state_restored=true" -Quiet)
    fresh_process_restore_requested = [bool](Select-String -LiteralPath $jsonl -Pattern "save_load restore_requested fresh_process=true" -Quiet)
    fresh_process_restored = [bool](Select-String -LiteralPath $jsonl -Pattern "save_load complete fresh_process=true state_restored=true" -Quiet)
    clean_state = $state.status -eq "clean"
    process_finished = [bool](Select-String -LiteralPath $jsonl -Pattern "process_finished" -Quiet)
}
$resultPath = Join-Path $userRoot "save-load-result.json"
$result | ConvertTo-Json | Set-Content -LiteralPath $resultPath -Encoding utf8
$result | ConvertTo-Json
if ($result.same_process_exit -ne 1 -or $result.fresh_process_exit -ne 1 -or
    -not $result.display_invariant -or -not $result.asset_metadata_invariant -or
    -not $result.validation_save_exists -or $result.validation_save_bytes -le 0 -or -not $result.save_requested -or
    -not $result.state_perturbed -or -not $result.same_process_restore_requested -or -not $result.same_process_restored -or
    -not $result.fresh_process_restore_requested -or -not $result.fresh_process_restored -or
    -not $result.clean_state -or -not $result.process_finished) {
    throw "Save/load lifecycle verification failed; inspect $resultPath"
}
