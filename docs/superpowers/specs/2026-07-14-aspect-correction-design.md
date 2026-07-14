# Configurable Aspect Correction Design

## Objective

Replace scattered hard-coded aspect behavior with one policy shared by gameplay, menus, HUD presentation, videos, subtitles, and mouse-coordinate mapping. Users must be able to configure each content domain independently without changing the physical display mode.

## User-facing behavior

The Video settings screen gains an Aspect Options submenu with four independent checkboxes:

- Preserve original gameplay aspect
- Preserve menu aspect
- Preserve HUD aspect
- Preserve video aspect

Defaults retain the current intended experience: gameplay and HUD use the modern output aspect, while menus and videos preserve their authored aspect. The existing `originalaspect` profile value remains the compatibility source for the gameplay choice. New menu, HUD, and video values persist beside it and have configuration-variable equivalents.

## Geometry policy

A small pure `AspectPolicy` module accepts output dimensions, content dimensions, and a preserve flag. When preservation is disabled it returns the complete output rectangle. When enabled it delegates to the existing aspect-fit geometry and returns a centered pillarboxed or letterboxed rectangle. Invalid dimensions return an empty rectangle and callers retain their conservative existing fallback.

The policy uses actual content dimensions. Menus and the classic HUD use their 640x480 authored space. Cutscenes use the decoded frame dimensions. This avoids assuming every video is 4:3.

## Integration

- Gameplay camera: the existing original-aspect behavior remains the gameplay policy implementation.
- Menu presentation: OpenGL core and compatibility presentation use the menu policy instead of unconditional 4:3 fitting.
- HUD presentation: the overlay destination uses the HUD policy.
- Videos and subtitles: the decoded video frame is fitted using the video policy; subtitles remain anchored within the resulting video-safe rectangle.
- Mouse coordinates: window-to-logical mapping uses the same menu destination rectangle that presentation uses.
- Software renderer: overlay composition consumes the same HUD rectangle so it matches the hardware path.

## Compatibility and failure handling

Existing profiles keep their gameplay setting and receive safe defaults for the three new settings. Unknown values normalize to boolean choices. No policy changes display resolution, refresh rate, gamma, HDR, color profile, monitor topology, or window mode. Exclusive fullscreen remains gated.

## Verification

Test-driven unit coverage proves full-output and fitted geometry at 1920x1080, 2560x1440, 3840x2160, ultrawide, and portrait dimensions, plus invalid-input behavior and defaults. Debug and Release x64 suites must pass. Runtime verification captures actual 2560x1440 gameplay/menu/video frames where deterministic automation is available, checks expected fitted bounds, and samples display state before and after.
