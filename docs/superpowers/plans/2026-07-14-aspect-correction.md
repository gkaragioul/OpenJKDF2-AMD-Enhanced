# Configurable Aspect Correction Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Provide independently configurable gameplay, menu, HUD, and video aspect correction through one tested geometry policy and the in-game Video menu.

**Architecture:** A pure `AspectPolicy` module returns the destination rectangle for full-output or aspect-preserved content. Existing camera, OpenGL presentation, software-overlay, cutscene/subtitle, and mouse-mapping paths consume that policy, while profile settings and an Aspect Options submenu expose four independent choices.

**Tech Stack:** C11, SDL/OpenGL presentation, existing JK GUI framework, JSON profile persistence, CMake/CTest, PowerShell runtime verification.

## Global Constraints

- Never change the physical monitor mode from aspect-policy code.
- Default gameplay and HUD to modern output aspect; default menus and videos to preserved authored aspect.
- Preserve the existing `originalaspect` profile key for gameplay compatibility.
- Use actual content dimensions for video fitting; do not assume every video is 4:3.
- Exclusive fullscreen remains gated.
- Use only `scripts/build-windows.ps1 -Configuration Debug|Release -Test` for C builds.

---

### Task 1: Pure aspect geometry policy

**Files:**
- Create: `src/General/AspectPolicy.h`
- Create: `src/General/AspectPolicy.c`
- Create: `src/Tests/test_aspect_policy.c`
- Modify: `cmake/OpenJKDF2Tests.cmake`

**Interfaces:**
- Consumes: `ResolutionLayoutRect`, `ResolutionLayout_FitAspect` from `General/ResolutionLayout.h`.
- Produces: `ResolutionLayoutRect AspectPolicy_Destination(int outputWidth, int outputHeight, int contentWidth, int contentHeight, int preserveAspect)`.

- [ ] **Step 1: Write the failing geometry test**

Cover full-output, 4:3 in 16:9/ultrawide/portrait, 16:9 content, and invalid dimensions in `test_aspect_policy.c`.

- [ ] **Step 2: Run the Debug suite and verify RED**

Run: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -Configuration Debug -Test`

Expected: compile or link failure because `AspectPolicy_Destination` is missing.

- [ ] **Step 3: Implement the minimal policy**

Return `{0,0,outputWidth,outputHeight}` when preservation is disabled; otherwise compute `(double)contentWidth / contentHeight` and call `ResolutionLayout_FitAspect`. Return an empty rectangle for non-positive dimensions.

- [ ] **Step 4: Run Debug and Release suites**

Expected: all unit tests pass in both configurations.

- [ ] **Step 5: Commit**

Commit message: `feat: add shared aspect geometry policy`

### Task 2: Persist four domain settings

**Files:**
- Modify: `src/World/jkPlayer.h`
- Modify: `src/World/jkPlayer.c`
- Modify: `src/General/VideoDefaults.h`
- Modify: `src/General/VideoDefaults.c`
- Modify: `src/Tests/test_video_defaults.c`

**Interfaces:**
- Produces globals `jkPlayer_preserveMenuAspect`, `jkPlayer_preserveHudAspect`, and `jkPlayer_preserveVideoAspect`; gameplay continues to use `jkPlayer_enableOrigAspect`.
- JSON keys: `preservemenuaspect`, `preservehudaspect`, `preservevideoaspect`.
- Cvars: `r_preserveMenuAspect`, `r_preserveHudAspect`, `r_preserveVideoAspect`.

- [ ] **Step 1: Extend default tests first**

Assert recommended defaults `{gameplay=0, menu=1, hud=0, video=1}` and safe defaults preserve menu/video.

- [ ] **Step 2: Run Debug and verify RED on missing fields**

- [ ] **Step 3: Add globals, cvars, reset values, JSON write/read, normalization, and default fields**

Use boolean reads/writes. Do not rename or remove `originalaspect`.

- [ ] **Step 4: Run both suites and commit**

Commit message: `feat: persist aspect settings by content domain`

### Task 3: Expose Aspect Options in the Video menu

**Files:**
- Modify: `src/Platform/SDL2/jkGUIDisplay.c`
- Modify: `resource/ui/openjkdf2.uni`

**Interfaces:**
- Consumes the four `jkPlayer_*Aspect` globals.
- Adds an Aspect Options submenu with four checkboxes, OK, and Cancel.

- [ ] **Step 1: Add a source-contract test to `scripts/check-c11-portability.ps1` or a focused PowerShell verifier**

Verify all four localization keys and all four menu bindings exist before implementation; run it and observe failure.

- [ ] **Step 2: Implement the submenu**

Replace the ambiguous `Use 1:1 aspect` checkbox with an Aspect Options button. Initialize checkbox state on open, copy values only on OK, persist using `jkPlayer_WriteConf`, and leave values unchanged on Cancel.

- [ ] **Step 3: Run Debug and Release suites and commit**

Commit message: `feat: add aspect options menu`

### Task 4: Integrate menu and mouse mapping

**Files:**
- Modify: `src/Platform/GL/std3D.c`
- Modify: `src/Platform/GL11/std3D.c`
- Modify: `src/Win95/Window.c`

**Interfaces:**
- Calls `AspectPolicy_Destination(Window_xSize, Window_ySize, 640, 480, jkPlayer_preserveMenuAspect)`.

- [ ] **Step 1: Extend aspect-policy tests for exact menu rectangles**

Assert 2560x1440 preserved menu rectangle `{320,0,1920,1440}` and stretch rectangle `{0,0,2560,1440}`; verify RED if a presentation adapter helper is needed.

- [ ] **Step 2: Replace unconditional menu pillarboxing in both GL paths**

Use the policy rectangle for destination vertices/subrects. Keep texture sampling at 640x480.

- [ ] **Step 3: Make mouse mapping use the identical policy rectangle**

No separate aspect calculation may remain in `Window.c`.

- [ ] **Step 4: Run both suites and commit**

Commit message: `fix: share menu aspect with mouse mapping`

### Task 5: Integrate HUD and software overlay composition

**Files:**
- Modify: `src/Platform/GL/std3D.c`
- Modify: `src/Platform/GL11/std3D.c`
- Modify: `src/Win95/Video.c`

**Interfaces:**
- Calls `AspectPolicy_Destination(outputWidth, outputHeight, 640, 480, jkPlayer_preserveHudAspect)`.

- [ ] **Step 1: Add preserved and full-output HUD rectangle assertions**
- [ ] **Step 2: Apply the destination to hardware HUD presentation**
- [ ] **Step 3: Apply the same destination to software keyed overlay composition**
- [ ] **Step 4: Run both suites and commit**

Commit message: `fix: make HUD aspect policy consistent`

### Task 6: Integrate videos and subtitle safe area

**Files:**
- Modify: `src/Platform/GL/std3D.c`
- Modify: `src/Platform/GL11/std3D.c`
- Modify: `src/Main/jkCutscene.c` only if decoded dimensions are not already exposed to presentation.

**Interfaces:**
- Calls `AspectPolicy_Destination(outputWidth, outputHeight, decodedWidth, decodedHeight, jkPlayer_preserveVideoAspect)`.
- Subtitle and pause-text placement uses the returned rectangle.

- [ ] **Step 1: Add tests for 640x360, 640x480, and ultrawide output rectangles**
- [ ] **Step 2: Replace fixed cutscene width limits and 4:3 assumptions with the policy rectangle**
- [ ] **Step 3: Anchor subtitle scaling and placement within that rectangle**
- [ ] **Step 4: Run both suites and commit**

Commit message: `fix: preserve authored video and subtitle aspect`

### Task 7: Runtime aspect verification and evidence

**Files:**
- Create: `scripts/test-aspect-correction.ps1`
- Create: `docs/evidence/aspect-correction-2026-07-14.md`
- Modify: `docs/evidence/requirements.csv`

**Interfaces:**
- Produces machine-readable rectangle/capture/display-invariance results under ignored `runtime-evidence`.

- [ ] **Step 1: Write the verifier before adding any runtime probe seam**

Require fresh user roots, 2560x1440 captures, structured policy events, and invariant display/Steam metadata.

- [ ] **Step 2: Add only the guarded runtime seam required by the verifier, observing RED first**
- [ ] **Step 3: Capture gameplay, menu, and an available cutscene/video path**
- [ ] **Step 4: Inspect images and logs, run both build suites, update evidence conservatively, and commit**

Commit message: `test: verify configurable aspect correction`
