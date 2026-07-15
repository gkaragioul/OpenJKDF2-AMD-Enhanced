#include "jkGame.h"

#include "General/stdPalEffects.h"
#include "Main/sithMain.h"
#include "Engine/rdroid.h"
#include "Raster/rdCache.h"
#include "Raster/rdZRaster.h"
#include "Engine/sithRender.h"
#include "Engine/sithPhysics.h"
#include "Engine/sithPuppet.h"
#include "World/sithWorld.h"
#include "World/jkPlayer.h"
#include "World/sithSector.h"
#include "World/sithThing.h"
#include "World/sithTemplate.h"
#include "World/sithWeapon.h"
#include "Win95/Video.h"
#include "Win95/stdComm.h"
#include "Platform/std3D.h"
#include "Win95/stdDisplay.h"
#include "Win95/Window.h"
#include "Main/jkHud.h"
#include "Main/jkHudInv.h"
#include "Main/jkHudScope.h"
#include "Main/jkHudCameraView.h"
#include "Main/jkDev.h"
#include "Main/jkQuakeConsole.h"
#include "Engine/rdColormap.h"
#include "Engine/sithCamera.h"
#include "Devices/sithControl.h"
#include "Devices/sithSound.h"
#include "Devices/sithSoundMixer.h"
#include "Primitives/rdMatrix.h"
#include "General/stdString.h"
#include "General/DiagnosticLog.h"
#include "General/FrameTelemetry.h"
#include "General/FrameRate.h"
#include "General/ControlPreset.h"
#include "General/PresentationMode.h"
#include "General/QualityPreset.h"
#include "General/RuntimeProbe.h"
#include "General/TimingDomainsRuntime.h"
#include "Gameplay/sithTime.h"
#include "Gameplay/sithEvent.h"
#include "Main/jkCutscene.h"
#include "Main/jkMain.h"

#include "stdPlatform.h"
#include "jk.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(SDL2_RENDER) && !defined(TARGET_RETRO_HOMEBREW)
#define TIMING_SCRIPT_TASK_ID 5
#define TIMING_SCRIPT_TOKEN 0x54494D45

static int jkGame_TimingScriptTask(int32_t unused, SithEventParams* params)
{
    (void)unused;
    return params && params->idx == TIMING_SCRIPT_TOKEN;
}
#endif

#if defined(TARGET_TWL)
#include <nds.h>
#endif

int jkGame_Startup()
{
    stdPlatform_Printf("OpenJKDF2: %s\n", __func__);
    
    sithWorld_RegisterTextSectionParser("jk", jkGame_ParseSection);
    jkGame_bInitted = 1;
    return 1;
}

int jkGame_ParseSection(SithWorld* a1, int a2)
{
    return a2 == 0;
}

void jkGame_ForceRefresh()
{
    sithCamera_Close();
    rdCanvas_Free(Video_pCanvas);
#ifdef SDL2_RENDER
    rdCanvas_Free(Video_pCanvasOverlayMap);
#endif
}

void jkGame_Shutdown()
{
    stdPlatform_Printf("OpenJKDF2: %s\n", __func__);
    
    jkGame_bInitted = 0;
}

void jkGame_ScreensizeIncrease()
{
    if ( Video_modeStruct.viewSizeIdx < 0xAu )
    {
        // MOTS added
        if (Main_bMotsCompat) {
            jkHudScope_Close();
            jkHudCameraView_Close();
        }

#ifndef LINUX_TMP
        sithCamera_Close();
        rdCanvas_Free(Video_pCanvas);
#ifdef SDL2_RENDER
        rdCanvas_Free(Video_pCanvasOverlayMap);
#endif
        ++Video_modeStruct.viewSizeIdx;
        Video_camera_related();
#endif
        // MOTS added
        if (Main_bMotsCompat) {
            jkHudScope_Open();
            jkHudCameraView_Open();
        }
    }
}

void jkGame_ScreensizeDecrease()
{
    if ( Video_modeStruct.viewSizeIdx )
    {
        // MOTS added
        if (Main_bMotsCompat) {
            jkHudScope_Close();
            jkHudCameraView_Close();
        }

#ifndef LINUX_TMP
        sithCamera_Close();
        rdCanvas_Free(Video_pCanvas);
#ifdef SDL2_RENDER
        rdCanvas_Free(Video_pCanvasOverlayMap);
#endif
        --Video_modeStruct.viewSizeIdx;
        Video_camera_related();
#endif
        // MOTS added
        if (Main_bMotsCompat) {
            jkHudScope_Open();
            jkHudCameraView_Open();
        }
    }
}

void jkGame_SetDefaultSettings()
{
#ifdef TARGET_DREAMCAST
    // Added: subtitles default on (cutscene voice audio can be unreliable, and
    // the opening cutscene expects them). Overridden by a saved player config.
    jkPlayer_setFullSubtitles = 1;
#else
    jkPlayer_setFullSubtitles = 0;
#endif
    jkPlayer_setDisableCutscenes = 0;
    jkPlayer_setRotateOverlayMap = 1;
    jkPlayer_setDrawStatus = 1;
    jkPlayer_setCrosshair = 0;
    jkPlayer_setSaberCam = 0;
}

int jkGame_Update()
{
    int64_t v0; // rcx
    SithThing *v2; // esi
    int v3; // eax
    flex_d_t v4; // st7
    int result; // eax
    int v6; // [esp+1Ch] [ebp-1Ch]

    static int jkGame_Update_Start = 0;
    static int jkGame_Update_ClearScreen = 0;
    static int jkGame_Update_AdvanceFrame = 0;
    static int jkGame_Update_UpdateCamera = 0;
    static int jkGame_Update_DrawPov = 0;
    static int jkGame_Update_HudDrawn = 0;
    static int jkGame_Update_End = 0;

    jkGame_Update_Start = stdPlatform_GetTimeMsec();

    // HACK HACK HACK: Adjust nearPlane depending on if we're using the scope/camera views
#if defined(SDL2_RENDER) || defined(TARGET_RETRO_HOMEBREW)
    if (sithCamera_g_aCameras[0].rdCamera.pClipFrustum) {
        sithCamera_g_aCameras[0].rdCamera.pClipFrustum->nearPlane = SITHCAMERA_ZNEAR_FIRSTPERSON;

        if (Main_bMotsCompat) {
            if (playerThings[playerThingIdx].actorThing->actorParams.flags & SITH_AF_SCOPEHUD) {
                sithCamera_g_aCameras[0].rdCamera.pClipFrustum->nearPlane = SITHCAMERA_ZNEAR;
            }
            if ((playerThings[playerThingIdx].actorThing->actorParams.flags & SITH_AF_ARACHNID) != 0) {
                sithCamera_g_aCameras[0].rdCamera.pClipFrustum->nearPlane = SITHCAMERA_ZNEAR;
            }
        }
    }
    
#endif

#if defined(SDL2_RENDER) || defined(TARGET_RETRO_HOMEBREW)
    // HACK
    Video_modeStruct.b3DAccel = 1;
#endif

#if !defined(SDL2_RENDER) && !defined(TARGET_RETRO_HOMEBREW)
    if ( Video_modeStruct.Video_8606C0 || Video_modeStruct.geoMode <= 2 )
#endif
#if !defined(TARGET_TWL)
        // DC (like SDL2) composites the whole menu buffer as an overlay, so it must
        // be cleared to the color key each frame or the HUD overlay shows stale
        // content over the 3D scene. TWL skips this (it reads only HUD sub-regions).
        stdDisplay_VBufferFill(Video_pMenuBuffer, Video_fillColor, 0); // Significant delay on TWL
#endif
    jkDev_DrawLog();
    jkHudInv_ClearRects();
    jkHud_ClearRects(0);
    jkGame_Update_ClearScreen = stdPlatform_GetTimeMsec();

    stdPalEffects_UpdatePalette(stdDisplay_GetPalette());
#if !defined(SDL2_RENDER) && !defined(TARGET_RETRO_HOMEBREW)
    if ( Video_modeStruct.b3DAccel )
#endif
        rdSetColorEffects(&stdPalEffects_state.effect);

#if defined(SDL2_RENDER) || defined(TARGET_RETRO_HOMEBREW)
    _memcpy(stdDisplay_masterPalette, sithWorld_g_pCurrentWorld->colormaps->colors, 0x300);
#endif
    rdAdvanceFrame();
    jkGame_Update_AdvanceFrame = stdPlatform_GetTimeMsec();
#ifdef RDRASTER_SOFTWARE_RENDERER
    // Added: render the world 3D through the software (CPU) rasterizer when the r_softwareRenderer
    // cvar is on. Acceleration is set to 0 so rdCache_Flush takes its software branch; the world +
    // weapon draw is redirected into a dedicated full-resolution buffer (presented full-screen by
    // std3D_DrawMenu), keeping Video_menuBuffer as the 640x480-logical HUD overlay composited on top.
    // When the cvar is OFF, none of this runs and the normal hardware (GL) path renders the frame.
    int rdsw_bActive = rdroid_bSoftwareRenderer;
    int rdsw_savedAccel = rdroid_curAcceleration;
    tVBuffer* rdsw_pWorldBuf = NULL;
    tVBuffer* rdsw_pSavedVBuf = NULL;
    tVBuffer* rdsw_pRenderBuf = NULL;
    // On a hardware->software transition, free the material GL textures the hardware path uploaded:
    // the software rasterizer samples texels from the system-RAM SDL surfaces and never touches VRAM,
    // so those textures are dead weight while SW is active. They re-upload lazily (texture_loaded is
    // reset) if the user switches back to hardware. (UI/HUD textures are a separate cache, untouched.)
    static int rdsw_bWasActive = 0;
    if (rdsw_bActive && !rdsw_bWasActive)
        std3D_PurgeEntireTextureCache();
    rdsw_bWasActive = rdsw_bActive;
    if (rdsw_bActive)
    {
        rdroid_curAcceleration = 0;
        // The world buffer matches the menu buffer dims, so redirecting the canvas at it leaves the
        // canvas geometry unchanged. (std3D_DrawMenu samples only a 640x480 sub-rect of the menu
        // buffer, which is why rendering the world there put it in a corner.)
        rdsw_pWorldBuf = Video_swEnsureWorldBuffer();
        rdsw_pRenderBuf = rdsw_pWorldBuf;
        if (rdsw_pWorldBuf && Video_pCanvas)
        {
            rdsw_pSavedVBuf = Video_pCanvas->pVBuffer;
            Video_pCanvas->pVBuffer = rdsw_pWorldBuf;
            // Clear to fill color (index 0) so untouched pixels present transparent (menu shader
            // discards index 0), matching the per-frame Video_pMenuBuffer fill for the world.
            stdDisplay_VBufferLock(rdsw_pWorldBuf);
            stdDisplay_VBufferFill(rdsw_pWorldBuf, Video_fillColor, 0);
        }
        else
        {
            // No world buffer yet — fall back to the menu buffer (renders into the corner, as before).
            rdsw_pRenderBuf = Video_pMenuBuffer;
            // The software rasterizer writes pixels directly, so the canvas surface must be
            // locked (surface_lock_alloc is NULL otherwise on the accelerated present path).
            stdDisplay_VBufferLock(Video_pMenuBuffer);
        }
#ifdef RDRASTER_SW_ZBUFFER
        // Clear the software depth buffer for the frame BEFORE the world is drawn. This must happen
        // here (not only via std3D_ClearZBuffer) because rdCamera_AdvanceFrame clears JK's software
        // z-buffer by filling canvas->d3d_vbuf on the accel<=0 path, so the std3D hook never fires at
        // scene start — leaving the depth buffer unallocated until DrawPov clears it (hence the world
        // only appeared once a POV weapon existed).
        rdZRaster_BeginFrame(rdsw_pRenderBuf);
#endif
    }
#endif
#if !defined(SDL2_RENDER) && !defined(TARGET_RETRO_HOMEBREW)
    if ( Video_modeStruct.b3DAccel )
#endif
    {
        sithDrawScene();
    }
#if !defined(SDL2_RENDER) && !defined(TARGET_RETRO_HOMEBREW)
    else
    {
        stdDisplay_VBufferLock(Video_pMenuBuffer);
        stdDisplay_VBufferLock(Video_pVbufIdk);
        sithDrawScene();
        stdDisplay_VBufferUnlock(Video_pVbufIdk);
        stdDisplay_VBufferUnlock(Video_pMenuBuffer);
    }
#endif
    jkGame_Update_UpdateCamera = stdPlatform_GetTimeMsec();
#ifdef RDRASTER_SOFTWARE_RENDERER
    // Added: keep acceleration off + the surface locked across DrawPov so the first-person weapon
    // renders through the software path into the menu buffer, exactly as the original did (JK ran
    // the whole frame in software). DrawPov itself calls std3D_ClearZBuffer + RD_ZBUFFER_READ_WRITE
    // (which now also clears the software depth buffer), so the weapon draws in front of the world.
    jkPlayer_DrawPov();
    if (rdsw_bActive)
    {
        if (rdsw_pWorldBuf && Video_pCanvas)
        {
            stdDisplay_VBufferUnlock(rdsw_pWorldBuf);
            Video_pCanvas->pVBuffer = rdsw_pSavedVBuf;   // restore the menu-buffer canvas for the HUD
            Video_swWorldPresentPending = 1;             // world rendered this frame → present it
        }
        else
        {
            stdDisplay_VBufferUnlock(Video_pMenuBuffer);
        }
        rdroid_curAcceleration = rdsw_savedAccel;
    }
#else
    jkPlayer_DrawPov();
#endif
    jkGame_Update_DrawPov = stdPlatform_GetTimeMsec();

#if 1
    //if (Main_bMotsCompat)
    ++Video_dword_5528A0; // MOTS added
    if ( Main_bDispStats )
    {
        v2 = sithWorld_g_pCurrentWorld->pLocalPlayer;
        //++Video_dword_5528A0; // MOTS removed
        v3 = stdPlatform_GetTimeMsec();
        v0 = v3 - Video_lastTimeMsec;
        Video_dword_5528A8 = v3;
        if ( (unsigned int)(v3 - Video_lastTimeMsec) > 0x3E8 )
        {
            if ( Main_bDispStats )
            {
                v6 = v2->sector->id;
                Video_flt_55289C = (flex_d_t)(Video_dword_5528A0 - Video_dword_5528A4) * 1000.0 / (flex_d_t)v0;
                _sprintf(
                    std_g_genBuffer,
                    "%02.3f (%02d%%)f %3ds %3da %3dz %4dp %3d curSector %3d fo",
                    Video_flt_55289C,
                    (unsigned int)(__int64)((flex_d_t)(unsigned int)jkGame_updateMsecsTotal / (flex_d_t)(int)v0 * 100.0),
                    sithRender_numRenderedSectors,
                    sithRender_geoThingsDrawn,
                    sithRender_nongeoThingsDrawn,
                    rdCache_drawnFaces,
                    v6,
                    sithNet_thingsIdx);
                if ( sithNet_isMulti )
                    _sprintf(&std_g_genBuffer[_strlen(std_g_genBuffer)], " %d m %d b", stdComm_dword_8321F4, stdComm_dword_8321F0);
                jkDev_sub_41FC40(100, std_g_genBuffer);
                v3 = Video_dword_5528A8;
            }
            Video_lastTimeMsec = v3;
            Video_dword_5528A4 = Video_dword_5528A0;
            jkGame_dword_552B5C = 0;
            jkGame_updateMsecsTotal = 0;
            stdComm_dword_8321F0 = 0;
            stdComm_dword_8321F4 = 0;
        }
    }
    else if ( Main_bFrameRate )
    {
        //++Video_dword_5528A0; // MOTS removed
        Video_dword_5528A8 = stdPlatform_GetTimeMsec();
        if ( (unsigned int)(Video_dword_5528A8 - Video_lastTimeMsec) > 1000 )
        {
            v4 = (flex_d_t)(Video_dword_5528A0 - Video_dword_5528A4) * 1000.0 / (flex_d_t)(unsigned int)(Video_dword_5528A8 - Video_lastTimeMsec);
            Video_flt_55289C = v4;
            _sprintf(std_g_genBuffer, "%02.3f", v4);
            jkDev_sub_41FC40(100, std_g_genBuffer);
            Video_lastTimeMsec = Video_dword_5528A8;
            Video_dword_5528A4 = Video_dword_5528A0;
        }
    }
#endif

#if defined(SDL2_RENDER)
    tVBuffer* pOverlayBuffer = Video_pCanvasOverlayMap->pVBuffer;
    stdDisplay_VBufferLock(pOverlayBuffer);
    stdDisplay_VBufferFill(pOverlayBuffer, Video_fillColor, 0);
    stdDisplay_VBufferUnlock(pOverlayBuffer);
#endif

    // MOTS added: scope/security cam overlays
    if (!Main_bMotsCompat) {
        if ( (playerThings[playerThingIdx].actorThing->actorParams.flags & SITH_AF_NOIDLECAMERA) == 0 ) {
            jkHud_Draw();
        }
    }
    else {
        if (playerThings[playerThingIdx].actorThing->actorParams.flags & SITH_AF_SCOPEHUD) {
            jkHudScope_Draw();
        }
        if ((playerThings[playerThingIdx].actorThing->actorParams.flags & SITH_AF_ARACHNID) == 0) {
            if ((playerThings[playerThingIdx].actorThing->actorParams.flags & SITH_AF_NOIDLECAMERA) == 0) {
                jkHud_Draw();
            }
        }
        else {
            jkHudCameraView_Draw();
        }
    }

    jkGame_Update_HudDrawn = stdPlatform_GetTimeMsec();

    jkDev_BlitLogToScreen();
    jkHudInv_Draw();
#if !defined(SDL2_RENDER) && !defined(TARGET_RETRO_HOMEBREW)
    if ( Video_modeStruct.b3DAccel )
        std3D_DrawOverlay();
#endif

    // MOTS added
    /*
    if (Main_bRecord != 0) {
        jkGame_Screenshot();
    }
    */

#if defined(SDL2_RENDER)
    jkQuakeConsole_Render();
#endif

#ifdef RDRASTER_SOFTWARE_RENDERER
    // Software renderer: fold the 2D overlays (HUD + map) into the world buffer so the whole frame is
    // one software image; std3D_DrawMenu then presents just that buffer and skips its menu-overlay quad.
    Video_swCompositeOverlaysIntoWorld();
#endif
#if defined(SDL2_RENDER) || defined(TARGET_RETRO_HOMEBREW)
    std3D_DrawMenu();
    rdFinishFrame();
#endif

#if defined(SDL2_RENDER) && !defined(TARGET_RETRO_HOMEBREW)
    if (TimingDomainsRuntime_IsEnabled())
    {
        static int timingFrameCapApplied = 0;
        if (!timingFrameCapApplied)
        {
            sithControl_ApplyModernPreset();
            jkHudInv_InputInit();
            jkPlayer_fpslimit = TimingDomainsRuntime_FrameLimit();
            jkPlayer_enableVsync = 0;
            FrameTelemetry_Reset();
            timingFrameCapApplied = 1;
        }
        TimingDomainsRuntime_Tick((uint64_t)sithTime_g_msecGameTime, Linux_TimeUs());
        if (TimingDomainsRuntime_ShouldActivateWeapon())
        {
            SithWorld* pTimingWorld = sithWorld_g_pCurrentWorld;
            SithThing* pTimingPlayer = pTimingWorld ? pTimingWorld->pLocalPlayer : NULL;
            int timingWeaponResult = 0;
            if (pTimingPlayer)
                timingWeaponResult = sithWeapon_ValidationFirePrimary(pTimingPlayer);
            if (timingWeaponResult != 1)
            {
                char timingWeaponEvent[192];
                snprintf(timingWeaponEvent, sizeof(timingWeaponEvent),
                         "weapon_activation_attempt result=%d game_time=%.3f mount_wait=%.3f selection=%d pressed=%d",
                         timingWeaponResult, (double)sithTime_g_secGameTime,
                         (double)sithWeapon_secMountWait, sithWeapon_8BD024,
                         sithWeapon_a8BD030[0]);
                diag_log_event(DIAG_SEVERITY_INFO, "timing_validation", timingWeaponEvent);
            }
        }
        if (TimingDomainsRuntime_ShouldStartAI())
        {
            SithWorld* pTimingWorld = sithWorld_g_pCurrentWorld;
            if (pTimingWorld)
            {
                uint32_t i;
                for (i = 0; i < pTimingWorld->numThings; ++i)
                {
                    SithThing* pTimingActor = &pTimingWorld->aThings[i];
                    if (pTimingActor->type == SITH_THING_ACTOR && pTimingActor->controlType == SITH_CT_AI &&
                        pTimingActor->actor && pTimingActor->actorParams.health > 0.0 &&
                        !(pTimingActor->flags & (SITH_TF_DEAD | SITH_TF_DESTROYED)))
                    {
                        TimingDomainsRuntime_RegisterTarget(TIMING_DOMAIN_AI, pTimingActor, 0);
                        break;
                    }
                }
            }
        }
        if (TimingDomainsRuntime_ShouldStartPhysics())
        {
            SithWorld* pTimingWorld = sithWorld_g_pCurrentWorld;
            SithThing* pTimingPlayer = pTimingWorld ? pTimingWorld->pLocalPlayer : NULL;
            if (pTimingPlayer && pTimingPlayer->moveType == SITH_MT_PHYSICS)
            {
                rdVector3 force = pTimingPlayer->orient.lvec;
                force.z = 0.0f;
                force.x *= pTimingPlayer->physicsParams.mass * 0.5f;
                force.y *= pTimingPlayer->physicsParams.mass * 0.5f;
                sithPhysics_ApplyForce(pTimingPlayer, &force);
                TimingDomainsRuntime_RegisterPhysicsTarget(pTimingPlayer,
                    pTimingPlayer->position.x, pTimingPlayer->position.y, pTimingPlayer->position.z);
            }
        }
        if (TimingDomainsRuntime_ShouldStartAnimation())
        {
            SithWorld* pTimingWorld = sithWorld_g_pCurrentWorld;
            SithThing* pTimingPlayer = pTimingWorld ? pTimingWorld->pLocalPlayer : NULL;
            if (pTimingPlayer && pTimingPlayer->renderData.puppet)
            {
                int timingTrack = sithPuppet_PlayMode(pTimingPlayer, SITH_ANIM_ACTIVATE, NULL);
                if (timingTrack >= 0)
                    TimingDomainsRuntime_RegisterTarget(TIMING_DOMAIN_ANIMATION,
                        pTimingPlayer->renderData.puppet, timingTrack);
            }
        }
        if (TimingDomainsRuntime_ShouldSpawnParticle())
        {
            SithWorld* pTimingWorld = sithWorld_g_pCurrentWorld;
            SithThing* pTimingPlayer = pTimingWorld ? pTimingWorld->pLocalPlayer : NULL;
            SithThing* pTimingTemplate = sithTemplate_GetTemplate("+rpt_sparks");
            if (pTimingPlayer && pTimingTemplate)
            {
                SithThing* pTimingParticle = sithThing_CreateThing(pTimingTemplate, pTimingPlayer);
                if (pTimingParticle)
                    TimingDomainsRuntime_RegisterTarget(TIMING_DOMAIN_PARTICLE, pTimingParticle, 0);
            }
        }
        if (TimingDomainsRuntime_ShouldStartScript())
        {
            SithEventParams timingParams;
            memset(&timingParams, 0, sizeof(timingParams));
            timingParams.idx = TIMING_SCRIPT_TOKEN;
            sithEvent_RegisterTask(TIMING_SCRIPT_TASK_ID, jkGame_TimingScriptTask, 0, SITHEVENT_TASKONDEMAND);
            if (sithEvent_CreateEvent(TIMING_SCRIPT_TASK_ID, &timingParams, 500))
                TimingDomainsRuntime_RegisterScriptTarget(TIMING_SCRIPT_TASK_ID, TIMING_SCRIPT_TOKEN);
        }
        if (TimingDomainsRuntime_ShouldStartDialogue())
        {
            sithSound* pTimingVoice = sithSound_Load("i00ky01z.wav", 1);
            if (pTimingVoice)
            {
                sithPlayingSound* pTimingChannel = sithSoundMixer_PlaySound(
                    pTimingVoice, 1.0f, 0.0f, SITHSOUNDFLAG_VOICE | SITHSOUNDFLAG_HIGHPRIO);
                if (pTimingChannel)
                    TimingDomainsRuntime_RegisterTarget(TIMING_DOMAIN_DIALOGUE, pTimingChannel, 0);
            }
        }
        if (TimingDomainsRuntime_ShouldStartCutscene())
        {
            char cutsceneEvent[160];
            int cutsceneResult = jkCutscene_sub_421310("resource\\video\\41DA.SMK");
            snprintf(cutsceneEvent, sizeof(cutsceneEvent),
                     "cutscene_request result=%d rendering=%s",
                     cutsceneResult, jkCutscene_isRendering ? "true" : "false");
            diag_log_event(DIAG_SEVERITY_INFO, "timing_validation", cutsceneEvent);
        }
        if (jkCutscene_isRendering && jkCutscene_smack_related_loops())
            jkCutscene_sub_421410();
        if (TimingDomainsRuntime_IsFinished())
            g_should_exit = 1;
    }

    // First-level opening-door observer for guarded acceptance testing.
    {
        static RuntimeProbe doorProbe = { 0 };
        static bool doorPresetApplied = false;
        static bool doorYawApplied = false;
        static bool doorFrameCapApplied = false;
        static bool doorApproachWarped = false;
        static bool doorCrossingPositioned = false;
        static rdVector3 doorStartPositions[2] = { 0 };
        static int doorStartSector = -1;
        static float doorMaxDisplacement[2] = { 0 };
        static uint32_t doorNextSampleMs = 250u;
        static uint32_t doorMovementStartMs = 0u;
        static uint32_t doorMovementFullMs = 0u;
        const char* pDoorMs = getenv("OPENJKDF2_VALIDATE_FIRST_DOOR_MS");
        SithWorld* pWorld = sithWorld_g_pCurrentWorld;
        SithThing* pPlayer = pWorld ? pWorld->pLocalPlayer : NULL;
        if (pDoorMs && pWorld && pPlayer && pPlayer->sector && pWorld->numThingsLoaded > 57)
        {
            SithThing* pDoorA = &pWorld->aThings[56];
            SithThing* pDoorB = &pWorld->aThings[57];
            rdVector3 playerAngles;
            uint32_t nowMs = stdPlatform_GetTimeMsec();
            uint32_t delayMs = (uint32_t)strtoul(pDoorMs, NULL, 10);
            if (!doorPresetApplied)
            {
                sithControl_ApplyModernPreset();
                jkHudInv_InputInit();
                doorPresetApplied = true;
                diag_log_event(DIAG_SEVERITY_INFO, "validation", "first_door modern_preset_applied=true");
            }
            if (!doorYawApplied)
            {
                const char* pDoorYaw = getenv("OPENJKDF2_VALIDATE_FIRST_DOOR_YAW");
                float yaw;
                if (pDoorYaw && runtime_probe_parse_degrees(pDoorYaw, &yaw))
                {
                    rdMatrix_ExtractAngles34(&pPlayer->orient, &playerAngles);
                    playerAngles.y = yaw;
                    rdMatrix_BuildRotate34(&pPlayer->orient, &playerAngles);
                    diag_log_event(DIAG_SEVERITY_INFO, "validation", "first_door yaw_override_applied=true");
                }
                else if (pDoorYaw)
                {
                    diag_log_event(DIAG_SEVERITY_ERROR, "validation", "first_door yaw_override_rejected=true");
                }
                doorYawApplied = true;
            }
            if (!doorFrameCapApplied)
            {
                const char* pDoorFrameCap = getenv("OPENJKDF2_VALIDATE_FIRST_DOOR_FRAME_CAP");
                int frameCap;
                if (pDoorFrameCap && runtime_probe_parse_rate(pDoorFrameCap, &frameCap))
                {
                    char frameCapEvent[128];
                    jkPlayer_fpslimit = frameCap;
                    jkPlayer_enableVsync = 0;
                    FrameTelemetry_Reset();
                    snprintf(frameCapEvent, sizeof(frameCapEvent),
                             "first_door frame_cap_applied=%d vsync=off", frameCap);
                    diag_log_event(DIAG_SEVERITY_INFO, "validation", frameCapEvent);
                }
                else if (pDoorFrameCap)
                {
                    diag_log_event(DIAG_SEVERITY_ERROR, "validation", "first_door frame_cap_rejected=true");
                }
                doorFrameCapApplied = true;
            }
            if (!doorApproachWarped && getenv("OPENJKDF2_VALIDATE_FIRST_DOOR_WARP_APPROACH") &&
                (pPlayer->sector->id == 31 || pPlayer->sector->id == 32 ||
                 (pPlayer->sector->id == 30 && pPlayer->position.x >= -8.30f)) &&
                pWorld->numSectors > 104)
            {
                rdVector3 doorApproach = { -4.45f, -4.05f, -1.18f };
                rdVector_Zero3(&pPlayer->physicsParams.vel);
                sithThing_SetPositionAndOrient(pPlayer, &doorApproach, &pPlayer->orient);
                sithThing_EnterSector(pPlayer, &pWorld->aSectors[104], 1, 0);
                doorApproachWarped = true;
                diag_log_event(DIAG_SEVERITY_INFO, "validation", "first_door approach_repositioned=true");
            }
            if (getenv("OPENJKDF2_VALIDATE_FIRST_DOOR_WARP_APPROACH"))
            {
                pPlayer->actorParams.flags |= SITH_AF_INVULNERABLE;
                if (doorApproachWarped && !doorCrossingPositioned &&
                    doorMaxDisplacement[0] < 0.01f && doorMaxDisplacement[1] < 0.01f)
                {
                    rdVector3 doorSwitchPosition = { -4.45f, -3.875f, -1.18f };
                    rdVector_Zero3(&pPlayer->physicsParams.vel);
                    sithThing_SetPositionAndOrient(pPlayer, &doorSwitchPosition, &pPlayer->orient);
                    sithThing_EnterSector(pPlayer, &pWorld->aSectors[104], 1, 0);
                }
            }
            rdMatrix_ExtractAngles34(&pPlayer->orient, &playerAngles);
            if (!doorProbe.started)
            {
                doorStartPositions[0] = pDoorA->position;
                doorStartPositions[1] = pDoorB->position;
                doorStartSector = pPlayer->sector->id;
            }
            {
                float displacementA = sqrtf(runtime_probe_distance_squared(
                    doorStartPositions[0].x, doorStartPositions[0].y, doorStartPositions[0].z,
                    pDoorA->position.x, pDoorA->position.y, pDoorA->position.z));
                float displacementB = sqrtf(runtime_probe_distance_squared(
                    doorStartPositions[1].x, doorStartPositions[1].y, doorStartPositions[1].z,
                    pDoorB->position.x, pDoorB->position.y, pDoorB->position.z));
                if (displacementA > doorMaxDisplacement[0]) doorMaxDisplacement[0] = displacementA;
                if (displacementB > doorMaxDisplacement[1]) doorMaxDisplacement[1] = displacementB;
                if (!doorMovementStartMs && displacementA >= 0.001f && displacementB >= 0.001f)
                    doorMovementStartMs = nowMs;
                if (!doorMovementFullMs && displacementA >= 0.19f && displacementB >= 0.19f)
                    doorMovementFullMs = nowMs;
                if (!doorCrossingPositioned && getenv("OPENJKDF2_VALIDATE_FIRST_DOOR_WARP_APPROACH") &&
                    displacementA >= 0.01f && displacementB >= 0.01f && pWorld->numSectors > 104)
                {
                    rdVector3 doorCrossing = { -4.60f, -4.05f, -1.18f };
                    rdVector_Zero3(&pPlayer->physicsParams.vel);
                    sithThing_SetPositionAndOrient(pPlayer, &doorCrossing, &pPlayer->orient);
                    sithThing_EnterSector(pPlayer, &pWorld->aSectors[104], 1, 0);
                    doorCrossingPositioned = true;
                    diag_log_event(DIAG_SEVERITY_INFO, "validation", "first_door crossing_repositioned=true");
                }
            }
            if (doorProbe.started)
            {
                uint32_t elapsedMs = (uint32_t)(nowMs - doorProbe.start_ms);
                if (elapsedMs >= doorNextSampleMs && elapsedMs < delayMs)
                {
                    char sampleEvent[512];
                    snprintf(sampleEvent, sizeof(sampleEvent),
                             "first_door sample elapsed_ms=%u player=(%.4f,%.4f,%.4f) yaw=%.3f sector=%d door_a=%.4f door_b=%.4f",
                             elapsedMs, pPlayer->position.x, pPlayer->position.y, pPlayer->position.z,
                             playerAngles.y, pPlayer->sector->id, doorMaxDisplacement[0], doorMaxDisplacement[1]);
                    diag_log_event(DIAG_SEVERITY_INFO, "validation", sampleEvent);
                    doorNextSampleMs += 250u;
                }
            }
            {
                const bool doorMoved = doorMaxDisplacement[0] >= 0.05f && doorMaxDisplacement[1] >= 0.05f;
                const bool crossed = pPlayer->sector->id != doorStartSector && pPlayer->position.y > -3.75f;
                const bool stableCaptureComplete = doorMovementFullMs && crossed &&
                    (uint32_t)(nowMs - doorMovementFullMs) >= 3000u;
                const bool observerExpired = runtime_probe_due(&doorProbe, nowMs, delayMs);
                if (stableCaptureComplete || observerExpired)
                {
                    FrameTelemetrySnapshot frameSnapshot = FrameTelemetry_GetSnapshot();
                    FrameTelemetryStatistics frameStatistics = { 0 };
                    const int haveFrameStatistics =
                        FrameTelemetry_CalculateStatistics(&frameSnapshot, &frameStatistics);
                    const uint32_t doorMovementMs = doorMovementStartMs && doorMovementFullMs ?
                        (uint32_t)(doorMovementFullMs - doorMovementStartMs) : 0u;
                    char doorEvent[768];
                    snprintf(doorEvent, sizeof(doorEvent),
                             "first_door complete door_moved=%s crossed=%s player=(%.4f,%.4f,%.4f) yaw=%.3f start_sector=%d end_sector=%d door_a=%.4f door_b=%.4f frame_cap=%d door_movement_ms=%u frame_samples=%u frame_median_ms=%.4f frame_p95_ms=%.4f frame_p99_ms=%.4f frame_worst_ms=%.4f clean_exit=true",
                             doorMoved ? "true" : "false", crossed ? "true" : "false",
                             pPlayer->position.x, pPlayer->position.y, pPlayer->position.z, playerAngles.y,
                             doorStartSector, pPlayer->sector->id, doorMaxDisplacement[0], doorMaxDisplacement[1],
                             jkPlayer_fpslimit, doorMovementMs,
                             haveFrameStatistics ? frameStatistics.sampleCount : 0u,
                             haveFrameStatistics ? frameStatistics.medianMilliseconds : 0.0,
                             haveFrameStatistics ? frameStatistics.p95Milliseconds : 0.0,
                             haveFrameStatistics ? frameStatistics.p99Milliseconds : 0.0,
                             haveFrameStatistics ? frameStatistics.worstMilliseconds : 0.0);
                    diag_log_event((doorMoved && crossed) ? DIAG_SEVERITY_INFO : DIAG_SEVERITY_ERROR,
                                   "validation", doorEvent);
                    {
                        const char* pDoorShotPath = getenv("OPENJKDF2_VALIDATE_FIRST_DOOR_SCREENSHOT");
                        if (pDoorShotPath)
                            std3D_Screenshot(pDoorShotPath);
                    }
                    g_should_exit = 1;
                }
            }
        }
    }

    // Opt-in crash/restoration validation. This is inert unless explicitly
    // requested by the test harness after gameplay and mouse capture are live.
    {
        static RuntimeProbe crashProbe = { 0 };
        const char* pCrashMs = getenv("OPENJKDF2_VALIDATE_CRASH_MS");
        SithWorld* pWorld = sithWorld_g_pCurrentWorld;
        SithThing* pPlayer = pWorld ? pWorld->pLocalPlayer : NULL;
        if (pCrashMs && pPlayer && pPlayer->sector)
        {
            const uint32_t nowMs = stdPlatform_GetTimeMsec();
            const uint32_t delayMs = (uint32_t)strtoul(pCrashMs, NULL, 10);
            if (runtime_probe_due(&crashProbe, nowMs, delayMs))
            {
                diag_log_event(DIAG_SEVERITY_INFO, "validation",
                               "gameplay_crash requested=true");
                fflush(NULL);
#if defined(_WIN32)
                RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 0, NULL);
#else
                abort();
#endif
            }
        }
    }

    // Opt-in production enhancement observer. It applies the complete Ultra
    // preset to the live renderer, captures sustained frame telemetry, and
    // exits cleanly after the requested observation interval.
    {
        static RuntimeProbe enhancementProbe = { 0 };
        static bool enhancementApplied = false;
        static bool enhancementCaptureRequested = false;
        const char* pEnhancementMs = getenv("OPENJKDF2_VALIDATE_ENHANCEMENTS_MS");
        SithWorld* pWorld = sithWorld_g_pCurrentWorld;
        SithThing* pPlayer = pWorld ? pWorld->pLocalPlayer : NULL;
        if (pEnhancementMs && pPlayer && pPlayer->sector)
        {
            const uint32_t nowMs = stdPlatform_GetTimeMsec();
            const uint32_t delayMs = (uint32_t)strtoul(pEnhancementMs, NULL, 10);
            if (!enhancementApplied)
            {
                QualityPresetSettings settings = QualityPreset_Get(QUALITY_PRESET_ULTRA);
                jkPlayer_enableTextureFilter = settings.textureFiltering;
                jkPlayer_anisotropy = settings.anisotropy;
                jkPlayer_mipmapBias = settings.mipmapBias;
                jkPlayer_enableBloom = settings.bloom;
                jkPlayer_enableSSAO = settings.ssao;
                jkPlayer_ssaaMultiple = settings.ssaaMultiple;
                jkPlayer_bEnableTexturePrecache = settings.texturePrecache;
                jkPlayer_bEnableJkgm = settings.assetEnhancements;
                jkPlayer_enableVsync = PRESENTATION_VSYNC_OFF;
                jkPlayer_fpslimit = 60;
                FrameTelemetry_Reset();
                diag_log_event(DIAG_SEVERITY_INFO, "validation",
                               "enhancements applied preset=Ultra filtering=true anisotropy=16 mipmap_bias=0.5 bloom=true ssao=true ssaa=1.5 precache=true replacements=true");
                enhancementApplied = true;
            }
            if (!enhancementCaptureRequested &&
                runtime_probe_due(&enhancementProbe, nowMs, delayMs))
            {
                const char* pShotPath = getenv("OPENJKDF2_VALIDATE_ENHANCEMENTS_SCREENSHOT");
                if (pShotPath)
                    std3D_RequestWindowScreenshot(pShotPath);
                else
                {
                    diag_log_event(DIAG_SEVERITY_ERROR, "validation",
                                   "enhancements screenshot_path_missing=true");
                    g_should_exit = 1;
                }
                enhancementCaptureRequested = true;
            }
            if (enhancementCaptureRequested && std3D_IsWindowScreenshotComplete())
            {
                FrameTelemetrySnapshot snapshot = FrameTelemetry_GetSnapshot();
                FrameTelemetryStatistics statistics = { 0 };
                const int haveStatistics = FrameTelemetry_CalculateStatistics(&snapshot, &statistics);
                char completeEvent[512];
                snprintf(completeEvent, sizeof(completeEvent),
                         "enhancements complete preset=Ultra total_frames=%llu samples=%u median_ms=%.4f p95_ms=%.4f p99_ms=%.4f worst_ms=%.4f clean_exit=true",
                         (unsigned long long)snapshot.totalRecordedSamples,
                         haveStatistics ? statistics.sampleCount : 0u,
                         haveStatistics ? statistics.medianMilliseconds : 0.0,
                         haveStatistics ? statistics.p95Milliseconds : 0.0,
                         haveStatistics ? statistics.p99Milliseconds : 0.0,
                         haveStatistics ? statistics.worstMilliseconds : 0.0);
                diag_log_event(haveStatistics ? DIAG_SEVERITY_INFO : DIAG_SEVERITY_ERROR,
                               "validation", completeEvent);
                g_should_exit = 1;
            }
        }
    }

    // Opt-in production presentation observer. It applies the requested VSync
    // mode and frame cap, measures the real swap/pacing loop, then exits cleanly.
    {
        static RuntimeProbe presentationProbe = { 0 };
        static bool presentationApplied = false;
        static bool presentationCaptureRequested = false;
        const char* pPresentationMs = getenv("OPENJKDF2_VALIDATE_PRESENTATION_MS");
        SithWorld* pWorld = sithWorld_g_pCurrentWorld;
        SithThing* pPlayer = pWorld ? pWorld->pLocalPlayer : NULL;
        if (pPresentationMs && pPlayer && pPlayer->sector)
        {
            const uint32_t nowMs = stdPlatform_GetTimeMsec();
            const uint32_t delayMs = (uint32_t)strtoul(pPresentationMs, NULL, 10);
            if (!presentationApplied)
            {
                const char* pMode = getenv("OPENJKDF2_VALIDATE_PRESENTATION_VSYNC");
                const char* pFrameCap = getenv("OPENJKDF2_VALIDATE_PRESENTATION_FRAME_CAP");
                int requestedMode;
                int frameCap = FRAME_RATE_UNLIMITED;
                char applyEvent[160];
                if (!runtime_probe_parse_vsync(pMode, &requestedMode) ||
                    (pFrameCap && !runtime_probe_parse_rate(pFrameCap, &frameCap)))
                {
                    diag_log_event(DIAG_SEVERITY_ERROR, "validation",
                                   "presentation request_rejected=true");
                    g_should_exit = 1;
                }
                else
                {
                    jkPlayer_enableVsync = requestedMode;
                    jkPlayer_fpslimit = frameCap;
                    FrameTelemetry_Reset();
                    snprintf(applyEvent, sizeof(applyEvent),
                             "presentation requested=%s frame_cap=%d",
                             PresentationMode_VsyncName(requestedMode), frameCap);
                    diag_log_event(DIAG_SEVERITY_INFO, "validation", applyEvent);
                }
                presentationApplied = true;
            }
            if (!presentationCaptureRequested && runtime_probe_due(&presentationProbe, nowMs, delayMs))
            {
                const char* pShotPath = getenv("OPENJKDF2_VALIDATE_PRESENTATION_SCREENSHOT");
                if (pShotPath)
                    std3D_RequestWindowScreenshot(pShotPath);
                else
                {
                    diag_log_event(DIAG_SEVERITY_ERROR, "validation",
                                   "presentation screenshot_path_missing=true");
                    g_should_exit = 1;
                }
                presentationCaptureRequested = true;
            }
            if (presentationCaptureRequested && std3D_IsWindowScreenshotComplete())
            {
                FrameTelemetrySnapshot snapshot = FrameTelemetry_GetSnapshot();
                FrameTelemetryStatistics statistics = { 0 };
                const int haveStatistics = FrameTelemetry_CalculateStatistics(&snapshot, &statistics);
                const int appliedMode = Window_GetAppliedVsyncMode();
                char completeEvent[512];
                snprintf(completeEvent, sizeof(completeEvent),
                         "presentation complete requested=%s applied=%s frame_cap=%d samples=%u median_ms=%.4f p95_ms=%.4f p99_ms=%.4f worst_ms=%.4f clean_exit=true",
                         PresentationMode_VsyncName(jkPlayer_enableVsync),
                         PresentationMode_VsyncName(appliedMode), jkPlayer_fpslimit,
                         haveStatistics ? statistics.sampleCount : 0u,
                         haveStatistics ? statistics.medianMilliseconds : 0.0,
                         haveStatistics ? statistics.p95Milliseconds : 0.0,
                         haveStatistics ? statistics.p99Milliseconds : 0.0,
                         haveStatistics ? statistics.worstMilliseconds : 0.0);
                diag_log_event(haveStatistics ? DIAG_SEVERITY_INFO : DIAG_SEVERITY_ERROR,
                               "validation", completeEvent);
                g_should_exit = 1;
            }
        }
    }

    // Opt-in end-to-end input observer. It records only player deltas and sector IDs,
    // never key contents or desktop input, and is inert during ordinary play.
    {
        static RuntimeProbe inputProbe = { 0 };
        static bool inputPresetApplied = false;
        static rdVector3 inputStartPosition = { 0 };
        static float inputStartYaw = 0.0f;
        static int inputStartSector = -1;
        const char* pInputMs = getenv("OPENJKDF2_VALIDATE_INPUT_MS");
        SithThing* pPlayer = sithWorld_g_pCurrentWorld ? sithWorld_g_pCurrentWorld->pLocalPlayer : NULL;
        if (pInputMs && pPlayer && pPlayer->sector)
        {
            if (!inputPresetApplied)
            {
                const char* requestedPreset = getenv("OPENJKDF2_VALIDATE_INPUT_PRESET");
                const int preset = requestedPreset && !__strcmpi(requestedPreset, "Classic")
                    ? CONTROL_PRESET_CLASSIC : CONTROL_PRESET_MODERN;
                int bindingsValid;
                char presetEvent[160];
                if (preset == CONTROL_PRESET_CLASSIC)
                    sithControl_ApplyClassicPreset();
                else
                    sithControl_ApplyModernPreset();
                jkHudInv_InputInit();
                inputPresetApplied = true;
                bindingsValid = sithControl_ValidatePresetBindings(preset);
                snprintf(presetEvent, sizeof(presetEvent),
                         "input preset=%s applied=true bindings_valid=%s",
                         ControlPreset_Name(preset),
                         bindingsValid ? "true" : "false");
                diag_log_event(bindingsValid ? DIAG_SEVERITY_INFO : DIAG_SEVERITY_ERROR,
                               "validation", presetEvent);
            }
            rdVector3 inputAngles;
            uint32_t nowMs = stdPlatform_GetTimeMsec();
            uint32_t delayMs = (uint32_t)strtoul(pInputMs, NULL, 10);
            rdMatrix_ExtractAngles34(&pPlayer->orient, &inputAngles);
            if (!inputProbe.started)
            {
                inputStartPosition = pPlayer->position;
                inputStartYaw = inputAngles.y;
                inputStartSector = pPlayer->sector->id;
            }
            if (runtime_probe_due(&inputProbe, nowMs, delayMs))
            {
                const float distanceSquared = runtime_probe_distance_squared(
                    inputStartPosition.x, inputStartPosition.y, inputStartPosition.z,
                    pPlayer->position.x, pPlayer->position.y, pPlayer->position.z);
                const float yawDelta = runtime_probe_angle_delta_degrees(inputStartYaw, inputAngles.y);
                const bool moved = runtime_probe_moved(
                    inputStartPosition.x, inputStartPosition.y, inputStartPosition.z,
                    pPlayer->position.x, pPlayer->position.y, pPlayer->position.z, 0.05f);
                const bool turned = runtime_probe_turned(inputStartYaw, inputAngles.y, 1.0f);
                char inputEvent[512];
                snprintf(inputEvent, sizeof(inputEvent),
                         "input complete moved=%s turned=%s distance=%.4f yaw_delta=%.3f start_sector=%d end_sector=%d clean_exit=true",
                         moved ? "true" : "false", turned ? "true" : "false", sqrtf(distanceSquared),
                         yawDelta, inputStartSector, pPlayer->sector->id);
                diag_log_event((moved && turned) ? DIAG_SEVERITY_INFO : DIAG_SEVERITY_ERROR,
                               "validation", inputEvent);
                {
                    const char* pInputShotPath = getenv("OPENJKDF2_VALIDATE_INPUT_SCREENSHOT");
                    if (pInputShotPath)
                        std3D_Screenshot(pInputShotPath);
                }
                g_should_exit = 1;
            }
        }
    }

    // Automated validation hook. Disabled unless OPENJKDF2_AUTOSHOT_MS is set.
    // Capture once, then use the ordinary shutdown path so diagnostics close cleanly.
    {
        static RuntimeProbe runtimeProbe = { 0 };
        const char* pShotMs = getenv("OPENJKDF2_AUTOSHOT_MS");
        if (pShotMs)
        {
            uint32_t nowMs = stdPlatform_GetTimeMsec();
            uint32_t delayMs = (uint32_t)strtoul(pShotMs, NULL, 10);
            if (runtime_probe_due(&runtimeProbe, nowMs, delayMs))
            {
                const char* pShotPath = getenv("OPENJKDF2_AUTOSHOT_PATH");
#ifdef RDRASTER_SOFTWARE_RENDERER
                std3D_ScreenshotWindow(pShotPath ? pShotPath : "sw_autoshot.png");
#else
                std3D_Screenshot(pShotPath ? pShotPath : "gameplay_autoshot.png");
#endif
                diag_log_event(DIAG_SEVERITY_INFO, "validation", "gameplay_screenshot_requested clean_exit=true");
                g_should_exit = 1;
            }
        }
    }
#endif

    // MOTS removed
    if ( Video_modeStruct.b3DAccel )
        result = stdDisplay_DDrawGdiSurfaceFlip();
    else
        result = stdDisplay_VBufferCopy(Video_pOtherBuf, Video_pMenuBuffer, 0, 0, 0, 0);
    // end MOTS removed

    // MOTS added
    /*
    if ((Video_modeStruct.Video_motsNew1 != 0) && (Video_modeStruct.b3DAccel == 0)) {
        result = stdDisplay_VBufferCopy(Video_pOtherBuf,Video_pMenuBuffer,0,0,NULL,0);
        return result;
    }
    result = stdDisplay_DDrawGdiSurfaceFlip();
    */

    jkGame_Update_End = stdPlatform_GetTimeMsec();

#if defined(TARGET_TWL)
    int jkGame_Delta_Start_ClearScreen = jkGame_Update_ClearScreen - jkGame_Update_Start;
    int jkGame_Delta_ClearScreen_AdvanceFrame = jkGame_Update_AdvanceFrame - jkGame_Update_ClearScreen;
    int jkGame_Delta_AdvanceFrame_UpdateCamera = jkGame_Update_UpdateCamera - jkGame_Update_AdvanceFrame;
    int jkGame_Delta_UpdateCamera_DrawPov = jkGame_Update_DrawPov - jkGame_Update_UpdateCamera;
    int jkGame_Delta_DrawPov_HudDrawn = jkGame_Update_HudDrawn - jkGame_Update_DrawPov;
    int jkGame_Delta_HudDrawn_End = jkGame_Update_End - jkGame_Update_HudDrawn;
    
    static int last_time_ms = 0;
    int now_ms = stdPlatform_GetTimeMsec();
    int total_delta = now_ms - last_time_ms;
    last_time_ms = now_ms;
    extern int std3D_timeWastedWaitingAround;
    extern int32_t sithRender_g_numVisibleSectors;

    int healthNum = 0;
    int shieldsNum = 0;
    int forceNum = 0;
    int ammoNum = 0;
    int currentItemBin = 0;
    int currentForceBin = 0;
    int bHasSuperShields = 0;
    int bHasSuperWeapon = 0;
    int bHasForceSurge = 0;
    int bHasFieldLight = 0;

    if (sithWorld_g_pCurrentWorld) {
        SithThing* pPlayer = sithWorld_g_pCurrentWorld->pLocalPlayer;
        if ( pPlayer->type == SITH_THING_PLAYER ) {
            healthNum = pPlayer->actorParams.health;
            shieldsNum = (int32_t)sithInventory_GetInventory(pPlayer, SITHBIN_SHIELDS);
            forceNum = (int32_t)sithInventory_GetInventory(pPlayer, SITHBIN_FORCEMANA);
            ammoNum = jkHud_GetWeaponAmmo(pPlayer);

            bHasSuperShields = playerThings[playerThingIdx].bHasSuperShields;
            bHasSuperWeapon = playerThings[playerThingIdx].bHasSuperWeapon;
            bHasForceSurge = playerThings[playerThingIdx].bHasForceSurge;
            bHasFieldLight = sithInventory_IsInventoryActivated(pPlayer, SITHBIN_FIELDLIGHT);
        }
    }

    char resetConsole[16];
    int consoleX, consoleY;
    consoleGetCursor(NULL, &consoleX, &consoleY);
    snprintf(resetConsole, sizeof(resetConsole)-1, "\x1b[%d;%dH\x1b[97m", consoleY, consoleX);
    stdPlatform_Printf("\x1b[0;0H                                \r\x1b[0;0H\x1b[%d;1m%cHLTH %03d \x1b[%d;1m%cSHLD %03d \x1b[39;0m%c\n                               \n", (bHasSuperShields ? 33 : 31), (bHasSuperShields ? '*' : ' '), healthNum, (bHasSuperShields ? 33 : 32), (bHasSuperShields ? '*' : ' '), shieldsNum, (bHasFieldLight ? '*' : ' '));
    stdPlatform_Printf(resetConsole);
    if (ammoNum < 0) {
        stdPlatform_Printf("\x1b[1;0H                                \r\x1b[1;0H\x1b[%d;1m%cAMMO --- \x1b[%d;1m%cMANA %03d   \n                               \n\x1b[39;0m", (bHasSuperWeapon ? 33 : 39), (bHasSuperWeapon ? '*' : ' '), (bHasForceSurge ? 33 : 39), (bHasForceSurge ? '*' : ' '), forceNum);
    }
    else {
        stdPlatform_Printf("\x1b[1;0H                                \r\x1b[1;0H\x1b[33;%dm%cAMMO %03d \x1b[%d;1m%cMANA %03d   \n                               \n\x1b[39;0m", (bHasSuperWeapon ? 1 : 0), (bHasSuperWeapon ? '*' : ' '), ammoNum, (bHasForceSurge ? 33 : 36), (bHasForceSurge ? '*' : ' '), forceNum);
    }
    stdPlatform_Printf("\x1b[6;0H                                \r");
    stdPlatform_Printf("\x1b[5;0H                                \r");
    stdPlatform_Printf("\x1b[4;0H                                \r");
    stdPlatform_Printf("\x1b[3;0H                                \r");
    stdPlatform_Printf("\x1b[2;0H                                \r\x1b[2;0H");
    
    jkDev_UpdateEntries();
    jkDev_PrintfLog();
    stdPlatform_Printf("\x1b[7;0H                                \r");
    stdPlatform_Printf(resetConsole);
    stdPlatform_Printf("\x1b[10;0H                               \rdlt all=%d mn=%d %d wrld=%d\n                               \r pov=%d hud=%d drw=%d wst=%d %d \n                               \n                               \n", total_delta-std3D_timeWastedWaitingAround, sithMain_tickEndMs-sithMain_tickStartMs, jkGame_Delta_ClearScreen_AdvanceFrame, jkGame_Delta_AdvanceFrame_UpdateCamera, jkGame_Delta_UpdateCamera_DrawPov, jkGame_Delta_DrawPov_HudDrawn, jkGame_Delta_HudDrawn_End - std3D_timeWastedWaitingAround, std3D_timeWastedWaitingAround, sithRender_g_numVisibleSectors);
    stdPlatform_Printf(resetConsole);
    stdPlatform_Printf("\x1b[13;0H                               \r");
    stdPlatform_PrintHeapStats();
    stdPlatform_Printf(resetConsole);
    //world=28 drw=15 emu
    //world=48 drw=33 dsi, 33 down to 25 with jank phys?
#endif

    return result;
}

#ifdef SDL2_RENDER
void jkGame_Screenshot()
{
    //stdPlatform_Printf("TODO: Implement screenshots\n");
    char local_80[128];
    int bVar2 = 0;
    do {
        stdString_snprintf(local_80, sizeof(local_80), "SHOT%04d.PNG", Video_dword_5528B0);
        stdFile_t fp = pHS->fileOpen(local_80, "r");
        if (fp == 0) {
            bVar2 = 1;
        }
        else {
            pHS->fileClose(fp);
        }
        Video_dword_5528B0++;
        if (Video_dword_5528B0 > 9999) {
            bVar2 = 1;
        }
    } while (!bVar2);

    std3D_Screenshot(local_80);
}
#endif

void jkGame_Gamma()
{
    int v0; // eax
    char *v1; // eax

    v0 = ++Video_modeStruct.Video_8606A4;
    if ( Video_modeStruct.Video_8606A4 >= 0xAu )
    {
        v0 = 0;
        Video_modeStruct.Video_8606A4 = 0;
    }
    stdDisplay_GammaCorrect3(v0);
#if !defined(SDL2_RENDER) && !defined(TARGET_RETRO_HOMEBREW)
    stdPalEffects_RefreshPalette();
    if ( Video_modeStruct.b3DAccel )
    {
        v1 = stdDisplay_GetPalette();
        sithRender_SetPalette(v1);
    }
#endif
}

void jkGame_PrecalcViewSizes(int width, int height, jkViewSize *aOut)
{
    flex_d_t v5; // st7
    flex_d_t v6; // st6
    flex_t v7; // [esp+4h] [ebp-Ch]
    flex_t v8;
    flex_t widtha; // [esp+14h] [ebp+4h]
    flex_t widthb; // [esp+14h] [ebp+4h]
    flex_t heighta; // [esp+18h] [ebp+8h]

    v5 = (flex_d_t)width;

    widtha = (flex_t)height;
    heighta = widtha;
    v6 = widtha * 0.5;
    widthb = v5 * 0.5;
    v8 = v6;
    v7 = heighta * 0.36000001;
    aOut[10].xMax = widthb;
    aOut[10].yMax = v6;
    aOut[9].xMax = widthb;
    aOut[9].yMax = v8;
    aOut[10].xMin = width;
    aOut[10].yMin = height;
    aOut[9].xMin = width;
    aOut[9].yMin = height;
    aOut[8].xMin = (__int64)(v5 * 0.9375 - -0.5);
    aOut[8].xMax = widthb;
    aOut[8].yMax = v8;
    aOut[8].yMin = (__int64)(heighta * 0.9375 - -0.5);
    aOut[7].xMin = (__int64)(v5 * 0.875 - -0.5);
    aOut[7].xMax = widthb;
    aOut[7].yMax = v8;
    aOut[7].yMin = (__int64)(heighta * 0.875 - -0.5);
    aOut[6].xMin = (__int64)(v5 * 0.8125 - -0.5);
    aOut[6].xMax = widthb;
    aOut[6].yMax = v8;
    aOut[6].yMin = (__int64)(heighta * 0.8125 - -0.5);
    aOut[5].xMin = (__int64)(v5 * 0.71875 - -0.5);
    aOut[5].xMax = widthb;
    aOut[5].yMax = v7;
    aOut[5].yMin = (__int64)(heighta * 0.71875 - -0.5);
    aOut[4].xMin = (__int64)(v5 * 0.625 - -0.5);
    aOut[4].xMax = widthb;
    aOut[4].yMax = v7;
    aOut[4].yMin = (__int64)(heighta * 0.625 - -0.5);
    aOut[3].xMin = (__int64)(v5 * 0.53125 - -0.5);
    aOut[3].xMax = widthb;
    aOut[3].yMax = v7;
    aOut[3].yMin = (__int64)(heighta * 0.53125 - -0.5);
    aOut[2].xMin = (__int64)(v5 * 0.4375 - -0.5);
    aOut[2].xMax = widthb;
    aOut[2].yMax = v7;
    aOut[2].yMin = (__int64)(heighta * 0.4375 - -0.5);
    aOut[1].xMin = (__int64)(v5 * 0.34375 - -0.5);
    aOut[1].xMax = widthb;
    aOut[1].yMax = v7;
    aOut[1].yMin = (__int64)(heighta * 0.34375 - -0.5);
    aOut->xMin = (__int64)(v5 * 0.25 - -0.5);
    aOut->yMin = (__int64)(heighta * 0.25 - -0.5);
    aOut->xMax = widthb;
    aOut->yMax = v7;
}

void jkGame_ddraw_idk_palettes()
{
    if ( Video_bOpened )
    {
        stdDisplay_VBufferFill(Video_pMenuBuffer, Video_fillColor, 0);
        stdDisplay_DDrawGdiSurfaceFlip();
        stdDisplay_ddraw_surface_flip2();
        stdDisplay_VBufferFill(Video_pMenuBuffer, Video_fillColor, 0);
        sithRender_SetPalette(stdDisplay_GetPalette());
    }
}

void jkGame_nullsub_36()
{
    ;
}
