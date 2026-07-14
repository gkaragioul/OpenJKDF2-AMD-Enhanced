# Baseline Environment — 2026-07-14

This record was captured before building or launching OpenJKDF2 AMD Enhanced. No display mode, driver setting, Steam file, save, or configuration was changed.

## Source

- Upstream: `https://github.com/shinyquagsire23/OpenJKDF2.git`
- Commit: `a189787a6180c3132c4a736da0835df046f40c77`
- Commit date: `2026-07-11T04:24:21-06:00`
- Development branch: `amd-enhanced/main`
- License: upstream permissive grant in `LICENSE.md`; bundled dependencies retain their own licenses
- Submodules: recorded by the upstream commit and not initialized at capture time

## Host

- OS: Microsoft Windows 11 Pro 64-bit
- Version/build: `10.0.26200` / `26200`
- CPU: AMD Ryzen 7 5700X3D, 8 cores / 16 logical processors
- GPU: AMD Radeon RX 7900 XTX (`PCI VEN_1002 DEV_744C`)
- Display state at capture: 2560x1440 at 165 Hz
- Display driver: `32.0.31021.5001`, dated 2026-06-28

`Win32_VideoController.AdapterRAM` reported approximately 4 GiB because that legacy WMI field is 32-bit-limited; it is not used as evidence of physical VRAM size.

## Toolchain discovered

- Git `2.54.0.windows.1`
- CMake `4.3.3`
- Ninja `1.13.2`
- Visual Studio 2022 Build Tools at `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`

## Safety gates

- The original Steam installation at `D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight` has not been modified or launched by this project.
- No game executable will be launched until structured diagnostics are available.
- No exclusive-fullscreen test will run until display restoration is implemented and independently verified.
- Initial renderer smoke tests must use generated data and windowed/hidden presentation only.

## Baseline gaps

The source snapshot previously present in the workspace had no Git metadata and cannot establish provenance. The new clone preserves upstream history. Debug/Release compilation, OpenGL strings, shader compiler logs, crash evidence, and frame-time evidence remain to be captured during implementation; they must not be inferred from this environment record.
