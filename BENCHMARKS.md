# Measured performance results

All figures are Release-build measurements from the single available Windows
11 / RX 7900 XTX host. They are not projections for other RDNA generations.

| Scenario | Target | Median | p95 | Result |
| --- | ---: | ---: | ---: | --- |
| Frame-cap-only | 120 FPS | 8.3232 ms | 8.5754 ms | Pass |
| First-door timing | 60 vs 120 FPS | — | — | Door traversal differed by 7 ms (1.48%) |

The VSync On and Adaptive 60 FPS measurements and raw JSONL capture locations
are documented in `docs/evidence/vsync-modes-2026-07-14.md`. Gameplay timing
methodology and first-door event evidence are in
`docs/evidence/timing-invariance-2026-07-14.md`.

Display-option validation was functional rather than a throughput benchmark:
exact live client and capture sizes passed at 1920x1080, 2560x1440, 3840x2160,
and 1280x720 on a second monitor while the desktop stayed 2560x1440@165.

No result is claimed for RDNA 1, RDNA 2, NVIDIA, Intel, physical 60/120/144 Hz
display modes, or sustained full-campaign performance because those measurements
were unavailable.
