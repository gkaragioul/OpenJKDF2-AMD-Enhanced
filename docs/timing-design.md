# Rendering and simulation timing design

## Clock domains

The presentation cap controls when rendered frames are submitted. It does not
scale the game clock. Simulation consumes elapsed time through a bounded
fixed-step accumulator, limiting catch-up work after stalls and retaining a
remainder for the next frame. This prevents a 120 or 165 FPS presentation rate
from directly doubling doors, physics, AI, scripts, animation, particles,
weapons, or other simulation systems.

Menus remain capped independently to avoid unnecessary GPU load. Cutscene and
dialogue playback retain their media/audio clocks. Multiplayer continues to use
the engine's network update intervals rather than the presentation interval.

## Frame caps and VSync

The user-facing choices are 30, 40, 50, 60, 72, 90, 100, 120, 144, 165, 180,
200, 240, Desktop Refresh, and Unlimited. Direct numeric configuration is
normalized through the same policy. VSync Off uses cap-only pacing; On requests
the normal swap interval; Adaptive requests the platform's adaptive interval and
records the actual fallback if unsupported.

The limiter uses the high-resolution platform clock and sleeps for the coarse
portion of the remaining interval. A short final yield/spin avoids an entire
core's worth of busy waiting while reducing overshoot. Structured frame samples
record target, actual interval, limiter work, swap mode, median, and tail values.
The on-screen overlay can display FPS and frame-time history.

## Safety and evidence boundary

The engine warns about unstable timing and offers an explicit return to 60 FPS;
it does not silently overwrite the selected cap. Unit tests cover cap parsing,
desktop/unlimited choices, accumulator behavior, catch-up bounds, and timer
wraparound. Hardware evidence currently proves strict 60/120 presentation pacing
and first-door traversal invariance on the recorded RX 7900 XTX host. Broader
doors/elevators, weapon timing, AI, animation, cutscene, dialogue, script,
particle, physics, and multiplayer runtime coverage remains tracked explicitly
in `docs/evidence/requirements.csv` and must not be inferred from the design.
