[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$DataDir,
    [Parameter(Mandatory = $true)][string]$EvidenceRoot,
    [string]$Executable = 'build/msvc-release/openjkdf2-64.exe',
    [int]$TargetFps = 120,
    [int]$TimeoutSeconds = 45
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$assetRoot = (Resolve-Path -LiteralPath $DataDir).Path.TrimEnd('\', '/')
$exePath = (Resolve-Path -LiteralPath (Join-Path $repoRoot $Executable)).Path
$root = [IO.Path]::GetFullPath($EvidenceRoot)
if (Test-Path -LiteralPath $root) { throw "EvidenceRoot must be fresh: $root" }
$userRoot = Join-Path $root 'UserData'
New-Item -ItemType Directory -Path $userRoot -Force | Out-Null

Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System; using System.Drawing; using System.Drawing.Imaging; using System.Runtime.InteropServices;
public static class OpenJKDF2FrameRateUi {
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left,Top,Right,Bottom; }
 [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] static extern bool SetCursorPos(int x,int y);
 [DllImport("user32.dll")] static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")] static extern bool BringWindowToTop(IntPtr h);
 [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr h,int c);
 [DllImport("user32.dll")] static extern IntPtr GetForegroundWindow();
 [DllImport("user32.dll")] static extern void mouse_event(uint f,uint dx,uint dy,uint d,UIntPtr e);
 [DllImport("user32.dll")] static extern bool PostMessage(IntPtr h,int m,IntPtr w,IntPtr l);
 public static void Skip(IntPtr h){if(h!=IntPtr.Zero)PostMessage(h,0x100,(IntPtr)0x1B,IntPtr.Zero);}
 public static void Escape(IntPtr h){if(h!=IntPtr.Zero){PostMessage(h,0x100,(IntPtr)0x1B,IntPtr.Zero);PostMessage(h,0x101,(IntPtr)0x1B,IntPtr.Zero);}}
 public static void Key(IntPtr h,int key,int count){for(int i=0;i<count;i++){PostMessage(h,0x100,(IntPtr)key,IntPtr.Zero);PostMessage(h,0x101,(IntPtr)key,IntPtr.Zero);}}
 public static string Rect(IntPtr h){RECT r;if(!GetWindowRect(h,out r))throw new InvalidOperationException();return r.Left+","+r.Top+","+(r.Right-r.Left)+","+(r.Bottom-r.Top);}
 public static void Click(int x,int y){SetCursorPos(x,y);mouse_event(2,0,0,0,UIntPtr.Zero);mouse_event(4,0,0,0,UIntPtr.Zero);}
 public static bool Activate(IntPtr h){ShowWindow(h,9);BringWindowToTop(h);SetForegroundWindow(h);return GetForegroundWindow()==h;}
 public static void Capture(IntPtr h,string p){RECT r;if(!GetWindowRect(h,out r))throw new InvalidOperationException();int w=r.Right-r.Left,ht=r.Bottom-r.Top;using(var b=new Bitmap(w,ht))using(var g=Graphics.FromImage(b)){g.CopyFromScreen(r.Left,r.Top,0,0,new Size(w,ht));b.Save(p,ImageFormat.Png);}}
}
'@

function Get-AssetSnapshot([string]$Path) {
    @(Get-ChildItem -LiteralPath $Path -Recurse -File | Sort-Object FullName | ForEach-Object { "$($_.FullName.Substring($Path.Length))|$($_.Length)|$($_.LastWriteTimeUtc.Ticks)" })
}

[ordered]@{
    InstallType=9; Window_displayMode=1; Window_isFullscreen=$true; Window_isHiDpi=$false
    Window_displayMonitor=0; Window_windowWidth=1280; Window_windowHeight=720; Window_refreshHz=0
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $userRoot 'registry.json') -Encoding utf8

function Start-DisplayRun([string]$Name) {
    $log=Join-Path $userRoot 'diagnostics\openjkdf2.jsonl'
    $baselineLine=if(Test-Path $log){(Select-String -LiteralPath $log -Pattern 'display_page displayed=true'|Select-Object -Last 1).Line}else{$null}
    $start=[Diagnostics.ProcessStartInfo]::new(); $start.FileName=$exePath; $start.WorkingDirectory=$repoRoot; $start.UseShellExecute=$false
    $start.Environment['OPENJKDF2_VALIDATE_DISPLAY_PAGE']='1'
    $start.Arguments='--data-dir "'+$assetRoot+'" --user-dir "'+$userRoot+'" --diagnostics-dir diagnostics'
    $p=[Diagnostics.Process]::Start($start)
    $deadline=[DateTime]::UtcNow.AddSeconds($TimeoutSeconds); $skipAt=[DateTime]::UtcNow.AddSeconds(2); $skipSent=$false; $line=$null
    while([DateTime]::UtcNow -lt $deadline -and -not $p.HasExited){
        if(Test-Path $log){$line=Select-String -LiteralPath $log -Pattern 'display_page displayed=true'|Select-Object -Last 1;if($line -and $line.Line -ne $baselineLine){break}else{$line=$null}}
        if(-not $skipSent -and [DateTime]::UtcNow -ge $skipAt){
            $p.Refresh(); if($p.MainWindowHandle -ne 0){[OpenJKDF2FrameRateUi]::Skip($p.MainWindowHandle);$skipSent=$true}
        }
        Start-Sleep -Milliseconds 100
    }
    if(-not $line){if(-not $p.HasExited){$p.Kill()};throw "$Name display page did not appear"}
    Start-Sleep -Milliseconds 750; $p.Refresh()
    [pscustomobject]@{Process=$p;Log=$log;DisplayedLine=$line.Line;Handle=$p.MainWindowHandle}
}

$assetsBefore=Get-AssetSnapshot $assetRoot
$first=Start-DisplayRun 'first'
$shell=New-Object -ComObject WScript.Shell; $shell.AppActivate($first.Process.Id)|Out-Null; if(-not [OpenJKDF2FrameRateUi]::Activate($first.Handle)){throw 'Could not activate first display page'}
$parts=[OpenJKDF2FrameRateUi]::Rect($first.Handle).Split(',')|ForEach-Object{[int]$_}; $left=$parts[0];$top=$parts[1];$w=$parts[2];$h=$parts[3]
$scale=[Math]::Min($w/640.0,$h/480.0);$ox=$left+($w-640*$scale)/2;$oy=$top+($h-480*$scale)/2
$internalSliderX=[double]($TargetFps-2)
[OpenJKDF2FrameRateUi]::Click([int]($ox+$internalSliderX*$scale),[int]($oy+325*$scale));Start-Sleep -Milliseconds 200
Start-Sleep -Milliseconds 3000
$shell.AppActivate($first.Process.Id)|Out-Null; [OpenJKDF2FrameRateUi]::Activate($first.Handle)|Out-Null
$firstShot=Join-Path $root 'frame-rate-selected.png';[OpenJKDF2FrameRateUi]::Capture($first.Handle,$firstShot)
$shell.AppActivate($first.Process.Id)|Out-Null; Start-Sleep -Milliseconds 200; $shell.SendKeys('{ENTER}')
if(-not $first.Process.WaitForExit($TimeoutSeconds*1000)){$first.Process.Kill();throw 'First display run timed out'}
$firstDismiss=Select-String -LiteralPath $first.Log -Pattern 'display_page dismissed=true'|Select-Object -Last 1

if(-not $firstDismiss){throw "First Apply did not return; exit code $($first.Process.ExitCode)"}
$second=Start-DisplayRun 'restart'; $loaded=if($second.DisplayedLine -match 'fps_limit=([-0-9]+)'){[int]$Matches[1]}else{9999}
$shell.AppActivate($second.Process.Id)|Out-Null;if(-not [OpenJKDF2FrameRateUi]::Activate($second.Handle)){throw 'Could not activate restart display page'};Start-Sleep -Milliseconds 300
$secondShot=Join-Path $root 'frame-rate-reloaded.png';[OpenJKDF2FrameRateUi]::Capture($second.Handle,$secondShot)
[OpenJKDF2FrameRateUi]::Escape($second.Handle)
if(-not $second.Process.WaitForExit($TimeoutSeconds*1000)){$second.Process.Kill();throw 'Restart display run timed out'}
$assetsAfter=Get-AssetSnapshot $assetRoot
$persistedFiles=@(Get-ChildItem -LiteralPath $userRoot -Recurse -File|Where-Object{(Get-Content -Raw $_.FullName -ErrorAction SilentlyContinue) -match '"fpslimit"\s*:\s*'+$TargetFps}|ForEach-Object FullName)
$applied=$firstDismiss -and $firstDismiss.Line -match "result=1 fps_limit=$TargetFps"
$result=[ordered]@{schema=1;target_fps=$TargetFps;interactive_apply=[bool]$applied;restart_loaded_fps=$loaded;persisted_file_count=$persistedFiles.Count;persisted_files=$persistedFiles;first_capture=$firstShot;restart_capture=$secondShot;first_exit=$first.Process.ExitCode;restart_exit=$second.Process.ExitCode;asset_metadata_invariant=(Compare-Object $assetsBefore $assetsAfter).Count -eq 0;exclusive_runtime_tested=$false}
$result|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $root 'frame-rate-ui-result.json') -Encoding utf8;$result|ConvertTo-Json -Depth 4
if(-not $result.interactive_apply -or $loaded -ne $TargetFps -or $persistedFiles.Count -lt 1 -or $result.first_exit -ne 1 -or $result.restart_exit -ne 1 -or -not $result.asset_metadata_invariant){throw 'Frame-rate UI persistence verification failed'}
