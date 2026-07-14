# Compatibility matrix

Results apply only to the exact tested hardware and settings. Capability-based
renderer paths contain no GPU-name allowlist. “Unverified” means no suitable
physical or remote host was available; it is not a compatibility claim.

| GPU | Architecture | Driver / OS | Backend | Tested settings | Result / limitation |
| --- | --- | --- | --- | --- | --- |
| AMD Radeon RX 7900 XTX | RDNA 3 | Recorded in privacy-safe runtime diagnostics / Windows 11 | Hardware OpenGL | 2560x1440 Borderless @ desktop 165 Hz; 1920x1080 and 3840x2160 Windowed; 1280x720 second monitor; capped 60/120 | Pass for recorded probes: no shader error, driver crash, corruption observed in captures, or software fallback. Sustained full campaign remains unverified. |
| AMD representative | RDNA 1 | Unavailable | — | — | Unverified hardware |
| AMD representative | RDNA 2 | Unavailable | — | — | Unverified hardware |
| NVIDIA representative | Unavailable | — | — | — | Unverified hardware |
| Intel representative | Unavailable | — | — | — | Unverified hardware |

Refresh/display-mode coverage on the tested host was 165 Hz desktop. 60, 120,
and 144 Hz physical refresh switching was unavailable and was not simulated.
Exclusive Fullscreen is safety-gated and was not tested.

Evidence: `docs/evidence/rdna3-gameplay-probe-2026-07-14.md`,
`docs/evidence/display-options-2026-07-14.md`, and
`docs/evidence/vsync-modes-2026-07-14.md`.
