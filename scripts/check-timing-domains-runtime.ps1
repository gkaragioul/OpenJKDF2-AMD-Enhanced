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

function Require-RawPattern([string] $Path, [string] $Pattern, [string] $Description) {
    $full = Join-Path $root $Path
    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) { $failures.Add("missing file: $Path"); return }
    $contents = Get-Content -LiteralPath $full -Raw
    if ($contents -notmatch $Pattern) { $failures.Add("$Path missing $Description") }
}

function Reject-RawPattern([string] $Path, [string] $Pattern, [string] $Description) {
    $full = Join-Path $root $Path
    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) { $failures.Add("missing file: $Path"); return }
    $contents = Get-Content -LiteralPath $full -Raw
    if ($contents -match $Pattern) { $failures.Add("$Path contains prohibited $Description") }
}

Require-Text 'src/main.c' 'TimingDomainsRuntime_Configure' 'runtime configuration'
Require-Text 'src/Main/jkGame.c' 'TimingDomainsRuntime_Tick' 'simulation tick bridge'
Require-RawPattern 'src/Main/sithMain.c' '(?s)sithTime_Advance\(\);.*?TimingDomainsRuntime_RefreshClock\(\(uint64_t\)sithTime_g_msecGameTime, Linux_TimeUs\(\)\)' 'current simulation clock refresh before production subsystem updates'
Require-Text 'src/Main/jkGame.c' 'jkPlayer_fpslimit = TimingDomainsRuntime_FrameLimit' 'validated frame-cap application'
Require-RawPattern 'src/World/sithWeapon.c' '(?s)if \(v9\s*&&.*?TimingDomainsRuntime_NotifyWeaponFire\(pShooter,.*?sithWeapon_fireWait.*?\)' 'weapon activation notification with shooter and production cooldown deadline'
Require-RawPattern 'src/World/sithWeapon.c' 'TimingDomainsRuntime_NotifyWeaponReady\(pThing\)' 'weapon cooldown-ready notification with shooter identity'
Require-RawPattern 'src/AI/sithAI.c' 'TimingDomainsRuntime_NotifyAI\(thing\)' 'AI notification with actor identity'
Require-RawPattern 'src/Engine/sithPhysics.c' 'TimingDomainsRuntime_NotifyPhysics\(pThing,\s*pThing->position\.x,\s*pThing->position\.y,\s*pThing->position\.z\)' 'physics notification with thing identity and measured position'
Require-RawPattern 'src/Engine/rdPuppet.c' 'TimingDomainsRuntime_NotifyAnimation\(pPuppet,\s*track\)' 'animation notification with puppet and track identity'
Require-RawPattern 'src/World/sithThing.c' 'TimingDomainsRuntime_NotifyParticle\(pThing\)' 'particle notification with thing identity'
Require-Text 'src/General/TimingDomainsRuntime.h' 'TimingDomainsRuntime_ShouldSpawnParticle' 'particle scenario request API'
Require-RawPattern 'src/Main/jkGame.c' '(?s)TimingDomainsRuntime_ShouldStartAI\(\).*?TimingDomainsRuntime_RegisterTarget\(TIMING_DOMAIN_AI' 'validation-owned AI actor selection'
Require-RawPattern 'src/Main/jkGame.c' '(?s)TimingDomainsRuntime_ShouldStartPhysics\(\).*?sithPhysics_ApplyForce\(.*?TimingDomainsRuntime_RegisterPhysicsTarget' 'validation-owned production physics action'
Require-RawPattern 'src/Main/jkGame.c' '(?s)TimingDomainsRuntime_ShouldStartAnimation\(\).*?sithPuppet_PlayMode\(.*?TimingDomainsRuntime_RegisterTarget\(TIMING_DOMAIN_ANIMATION' 'validation-owned production animation action'
Require-RawPattern 'src/Main/jkGame.c' '(?s)TimingDomainsRuntime_ShouldSpawnParticle\(\).*?sithTemplate_GetTemplate\("\+rpt_sparks"\).*?TimingDomainsRuntime_RegisterTarget\(TIMING_DOMAIN_PARTICLE' 'validation-owned real fixed-lifetime particle action'
Reject-RawPattern 'src/Main/jkGame.c' '(?s)pTimingParticle->(?:particleParams\.flags|msecLifeLeft)\s*(?:&=|=)' 'validation scenario mutation of particle flags or configured lifetime'
Require-RawPattern 'src/Main/jkGame.c' '(?s)TimingDomainsRuntime_ShouldStartScript\(\).*?sithEvent_CreateEvent\(TIMING_SCRIPT_TASK_ID,.*?TimingDomainsRuntime_RegisterScriptTarget' 'validation-owned queued script action'
Require-RawPattern 'src/Main/jkGame.c' '(?s)TimingDomainsRuntime_ShouldStartDialogue\(\).*?sithSound_Load\("i00ky01z\.wav".*?sithSoundMixer_PlaySound\(.*?TimingDomainsRuntime_RegisterTarget\(TIMING_DOMAIN_DIALOGUE' 'validation-owned real voice playback action'
Require-RawPattern 'src/Main/jkGame.c' '(?s)TimingDomainsRuntime_ShouldStartCutscene\(\).*?resource\\\\video\\\\41DA\.SMK' 'bounded real cutscene scenario asset'
Require-RawPattern 'src/Main/jkGame.c' '(?s)jkCutscene_isRendering\s*&&\s*jkCutscene_smack_related_loops\(\).*?jkCutscene_sub_421410\(\)' 'real cutscene decoder service and completion path'
Require-RawPattern 'src/Main/jkMain.c' '(?s)void jkMain_GameplayTick\(.*?TimingDomainsRuntime_ShouldRequestLevelTransition\(\).*?jkMain_LoadLevelSingleplayer\("JK1",\s*"02narshadda\.jkl"\)' 'level-transition request to the episode-defined next level from the production gameplay state tick'
Require-Text 'src/Main/jkMain.c' 'level_transition_dispatch' 'transition dispatch boundary telemetry'
Require-Text 'src/Main/jkMain.c' 'level_transition_queued' 'transition queue boundary telemetry'
Require-Text 'src/Main/jkMain.c' 'level_transition_state_change' 'GUI state-change boundary telemetry'
Require-Text 'src/Main/jkMain.c' 'level_transition_post_load' 'post-load boundary telemetry'
Require-RawPattern 'src/Gameplay/sithEvent.c' 'TimingDomainsRuntime_NotifyScript\(i->taskNum,\s*i->params\.idx\)' 'script notification with task and validation token'
Require-RawPattern 'src/Devices/sithSoundMixer.c' 'TimingDomainsRuntime_NotifyDialogue\(hChannel\)' 'dialogue notification with channel identity'
Require-Text 'src/Main/jkCutscene.c' 'TimingDomainsRuntime_NotifyCutscene' 'cutscene notification'
Require-RawPattern 'src/Main/jkMain.c' '(?s)level_loaded\s*=\s*v3;.*?TimingDomainsRuntime_NotifyLevelTransition\(jkMain_aLevelJklFname,\s*level_loaded\)' 'post-load level-transition notification with target identity and load result'
Require-Text 'src/General/StartupOptions.c' '--validation-observer=' 'literal observer flag'
Require-RawPattern 'src/World/sithWeapon.c' '(?s)sithWeapon_ValidationFirePrimary.*?SITH_MESSAGE_ACTIVATE.*?sithWeapon_ValidationReleasePrimary.*?SITH_MESSAGE_DEACTIVATED' 'primary fire held until a validation-owned production release'
Require-RawPattern 'src/World/sithWeapon.c' '(?s)int sithWeapon_ValidationFirePrimary\(SithThing\* pThing\)\s*\{.*?sithWeapon_a8BD030\[0\]\s*=\s*1;.*?sithWeapon_validationPrimaryActive\s*=\s*1;.*?sithCog_SendMessage\(weapon->cog,\s*SITH_MESSAGE_ACTIVATE' 'primary fire state committed before synchronous activation dispatch'
Require-RawPattern 'src/World/sithWeapon.c' '(?s)static void sithWeapon_ValidationReleasePrimary\(SithThing\* pThing\)\s*\{.*?sithWeapon_a8BD030\[0\]\s*=\s*0;.*?sithWeapon_validationPrimaryActive\s*=\s*0;.*?sithCog_SendMessage\(weapon->cog,\s*SITH_MESSAGE_DEACTIVATED' 'primary fire state cleared before synchronous deactivation dispatch'
Require-RawPattern 'src/World/sithWeapon.c' '(?s)if \(v9\s*&&.*?TimingDomainsRuntime_NotifyWeaponFire\(pShooter,.*?sithWeapon_fireWait.*?sithWeapon_ValidationReleasePrimary\(pShooter\)' 'validation-owned primary release after the real projectile spawn'
Require-RawPattern 'src/World/sithWeapon.c' '(?s)sithWeapon_validationPrimaryThing\s*=\s*pThing;.*?sithWeapon_validationPrimaryActive\s*=\s*1;.*?SITH_MESSAGE_ACTIVATE' 'validation shooter identity committed before synchronous activation dispatch'
Require-RawPattern 'src/World/sithWeapon.c' '(?s)if \(v9\s*&&\s*sithWeapon_validationPrimaryActive\s*&&\s*pShooter\s*==\s*sithWeapon_validationPrimaryThing\).*?TimingDomainsRuntime_NotifyWeaponFire.*?sithWeapon_ValidationReleasePrimary.*?sithWeapon_validationPrimaryThing\s*=\s*NULL;.*?SITH_MESSAGE_DEACTIVATED' 'projectile notification and release scoped to the armed validation shooter'
Require-Text 'src/General/TimingDomainsRuntime.c' 'timing_domain_start' 'start JSON event'
Require-Text 'src/General/TimingDomainsRuntime.c' 'timing_domain_complete' 'completion JSON event'
Require-Text 'src/General/TimingDomainsRuntime.c' 'timing_domains_summary' 'summary JSON event'
Require-RawPattern 'src/General/TimingDomainsRuntime.c' '(?s)TimingDomainsObserver_Complete\(.*?TIMING_DOMAIN_LEVEL_TRANSITION.*?TimingDomainsScenario_Update\(.*?TIMING_ACTION_FINISH.*?timing_runtime_write_summary\(\)' 'synchronous final summary after the level-transition notification'
Require-RawPattern 'src/Main/jkGame.c' '(?s)TimingDomainsRuntime_ShouldActivateWeapon\(\).*?sithWeapon_ValidationFirePrimary\(pTimingPlayer\)' 'validation-owned primary weapon activation through the production weapon route'
Require-RawPattern 'src/World/sithWeapon.c' '(?s)int sithWeapon_ValidationFirePrimary\(.*?sithTime_g_secGameTime\s*<\s*sithWeapon_secMountWait.*?sithWeapon_UpdateActorWeaponState\(pThing\).*?SITH_MESSAGE_ACTIVATE.*?SITH_MESSAGE_DEACTIVATED' 'production weapon activation and release helper with normal mount and selection readiness'
Require-RawPattern 'scripts/test-timing-domains.ps1' '(?s)\$runFailure\s*=\s*\$null.*?try\s*\{.*?\$process\s*=\s*\[Diagnostics\.Process\]::Start\(\$start\).*?catch\s*\{\s*\$runFailure\s*=\s*\$_\s*\}.*?\$displayAfter\s*=.*?Set-Content.*?if\s*\(\$displayBefore\s*-ne\s*\$displayAfter\).*?if\s*\(\$errors\.Count\).*?if\s*\(\$runFailure\)\s*\{\s*throw\s+\$runFailure\s*\}' 'child failure deferred until after display and event-log safety assertions'
Require-RawPattern 'scripts/test-timing-domains.ps1' '(?s)function Wait-TimingObserverReady\(\[string\] \$DiagnosticsRoot, \[Diagnostics\.Process\] \$Process, \[DateTime\] \$Deadline\).*?while \(-not \$Process\.HasExited -and \[DateTime\]::UtcNow -lt \$Deadline\)' 'condition-based readiness wait bounded by the overall run deadline'
Reject-RawPattern 'scripts/test-timing-domains.ps1' 'TimingDomainsNativeProbe\]::mouse_event' 'desktop mouse injection for the timing observer'
Require-Text 'scripts/test-timing-domains.ps1' 'function Merge-TimingRuns' 'multi-sample median aggregation'
Require-Text 'scripts/test-timing-domains.ps1' '\$runtimeSampleCount = 3' 'three fresh runtime samples per frame cap'
Require-RawPattern 'scripts/test-timing-domains.ps1' '(?s)\$warmupLog\s*=\s*Invoke-TimingRun\s+60\s+''warmup-60''.*?Convert-TimingLog\s+\$warmupLog.*?\$runOrder' 'explicit validated cold-start warm-up before measured samples'
Require-RawPattern 'scripts/test-timing-domains.ps1' '(?s)\$runOrder\s*=.*?60,120,120,60,60,120.*?Merge-TimingRuns' 'interleaved reversible runtime order followed by median comparison'
Require-RawPattern 'scripts/test-timing-domains.ps1' 'if \(@\(Compare-Object \$assetBefore \$assetAfter\)\.Count -ne 0\)' 'strict-mode-safe empty asset comparison'
Require-RawPattern 'scripts/test-timing-domains.ps1' '(?s)\$assetBefore\s*=.*?\$runtimeFailure\s*=\s*\$null.*?try\s*\{.*?Invoke-TimingRun.*?catch\s*\{\s*\$runtimeFailure\s*=\s*\$_\s*\}.*?finally\s*\{.*?\$assetAfter\s*=.*?Compare-Object\s+\$assetBefore\s+\$assetAfter.*?\}.*?if\s*\(\$runtimeFailure\)\s*\{\s*throw\s+\$runtimeFailure\s*\}' 'runtime batch failure deferred until after Steam asset invariant assertion'

if ($failures.Count) {
    $failures | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Host 'PASS: timing-domain runtime bridge is connected to all nine production domains.'
