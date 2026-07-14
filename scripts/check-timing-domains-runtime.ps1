[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$failures = [System.Collections.Generic.List[string]]::new()

function Require-Text([string] $Path, [string] $Pattern, [string] $Description) {
    $full = Join-Path $root $Path
    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) {
        $failures.Add("missing file: $Path")
        return
    }
    if (-not (Select-String -LiteralPath $full -Pattern $Pattern -Quiet)) {
        $failures.Add("$Path missing $Description")
    }
}

Require-Text 'src/main.c' 'TimingDomainsRuntime_Configure' 'runtime configuration'
Require-Text 'src/Main/jkGame.c' 'TimingDomainsRuntime_Tick' 'simulation tick bridge'
Require-Text 'src/Main/jkGame.c' 'jkPlayer_fpslimit = TimingDomainsRuntime_FrameLimit' 'validated frame-cap application'
Require-Text 'src/World/sithWeapon.c' 'TimingDomainsRuntime_NotifyWeapon' 'weapon lifecycle notification'
Require-Text 'src/AI/sithAI.c' 'TimingDomainsRuntime_NotifyAI' 'AI notification'
Require-Text 'src/Engine/sithPhysics.c' 'TimingDomainsRuntime_NotifyPhysics' 'physics notification'
Require-Text 'src/Engine/rdPuppet.c' 'TimingDomainsRuntime_NotifyAnimation' 'animation notification'
Require-Text 'src/World/sithThing.c' 'TimingDomainsRuntime_NotifyParticle' 'particle notification'
Require-Text 'src/Gameplay/sithEvent.c' 'TimingDomainsRuntime_NotifyScript' 'script notification'
Require-Text 'src/Devices/sithSoundMixer.c' 'TimingDomainsRuntime_NotifyDialogue' 'dialogue notification'
Require-Text 'src/Main/jkCutscene.c' 'TimingDomainsRuntime_NotifyCutscene' 'cutscene notification'
Require-Text 'src/Main/jkMain.c' 'TimingDomainsRuntime_NotifyLevelTransition' 'level-transition notification'
Require-Text 'src/General/StartupOptions.c' '--validation-observer=' 'literal observer flag'
Require-Text 'src/General/TimingDomainsRuntime.c' 'timing_domain_start' 'start JSON event'
Require-Text 'src/General/TimingDomainsRuntime.c' 'timing_domain_complete' 'completion JSON event'
Require-Text 'src/General/TimingDomainsRuntime.c' 'timing_domains_summary' 'summary JSON event'

if ($failures.Count) {
    $failures | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Host 'PASS: timing-domain runtime bridge is connected to all nine production domains.'
