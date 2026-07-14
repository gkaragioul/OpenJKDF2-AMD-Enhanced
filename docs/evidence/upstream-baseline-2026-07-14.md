# Unmodified upstream AMD baseline — 2026-07-14

## Result

The unmodified official OpenJKDF2 v0.9.9 Windows artifact reproduced the
reported AMD failure on the RX 7900 XTX. It launched against the user's
licensed Steam data in a window, then terminated naturally with Windows
exception `0xC00000FD` (stack overflow) inside AMD OpenGL driver module
`atio6axx.dll` version `32.0.31021.5001`.

This is direct reproduction evidence. It does not infer the fault from the GPU
name, and it does not attribute the engine defect to every AMD configuration.

## Provenance

- Authoritative release: OpenJKDF2 `v0.9.9`, published 2026-07-05.
- Release commit: `814b684c0d06bbfc6e1121941fc8a9e88e3aede7`.
- Official asset: `win64-debug.zip`.
- GitHub-published and locally verified SHA-256:
  `842d2ece45f52cd533df341107a4b415d7cd35c2ccff44100664195bc95e8c28`.
- Extracted `openjkdf2-64.exe` SHA-256:
  `90b8d957c5e3bb4f3bd9961e86f34b7769620e2c67955e099d3e340f2df007ba`.
- Release page: <https://github.com/shinyquagsire23/OpenJKDF2/releases/tag/v0.9.9>.

No upstream source or binary was patched. The runtime directory contained the
official three-file Windows package, a private copy of `JK.EXE`, private
writable `Controls` and `player` directories, and directory junctions used
read-only to expose `Episode`, `Resource`, and `MUSIC` from the licensed Steam
installation. No proprietary asset is tracked or redistributed by this fork.

## Reproduction

The exact command line was:

```text
openjkdf2-64.exe -autostart -sp -episode JK1 -map 01narshadda.jkl
```

Observed sequence:

1. The process started and created a 993×519 window.
2. It remained observable for 18.666 seconds.
3. It exited naturally with signed code `-1073741571` (`0xC00000FD`).
4. Windows Application Error event 1000 identified `atio6axx.dll` as the
   faulting module and `0xC00000FD` as the exception.
5. The captured upstream window was black at the sampled instant; no playable
   hardware-rendered frame was reached.

The official build emitted no stdout or stderr bytes, so Windows Application
Error is the authoritative crash record. The local, ignored runtime-evidence
directory retains the raw event record, hash-verified archive, extracted
package, stdout/stderr captures, and screenshot. The adjacent JSON file is the
privacy-safe tracked summary.

## Safety invariants

- Display before: AMD Radeon RX 7900 XTX, driver `32.0.31021.5001`,
  2560×1440 at 165 Hz.
- Display after: 2560×1440 at 165 Hz.
- Display invariant: passed.
- Steam tree metadata SHA-256 before and after:
  `d67c6f84ece52b6aa6f48f0cde2dae715a50c320775223e62f7e34b416b85744`.
- Steam metadata invariant: passed.
- Exclusive fullscreen was not used.

## Source-build observations

The current authoritative `origin/master` commit
`a189787a6180c3132c4a736da0835df046f40c77` was also checked out without source
changes and tested through both documented Windows routes:

- Native MSVC configuration succeeded, but compilation failed at
  `src/Win95/Window.c:812` because the C source uses the GNU case-range syntax
  `case SDL_EVENT_WINDOW_FIRST ... SDL_EVENT_WINDOW_LAST`, which MSVC rejects.
- Upstream's unchanged `build_win64.sh` configured successfully in a clean,
  LF-only, no-space WSL checkout. On Ubuntu 26.04's MinGW-w64 13 toolchain,
  bundled OpenAL and FreeGLUT failed at link time with unresolved MinGW CRT math
  symbols such as `__mingw_raise_matherr`, `__sinl_internal`, and `log2l`.

Those toolchain failures are recorded as upstream build compatibility findings;
they are not substituted for the successful runtime reproduction above.
