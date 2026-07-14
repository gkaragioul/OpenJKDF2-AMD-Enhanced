# Building on Windows

OpenJKDF2 AMD Enhanced uses Visual Studio 2022 Build Tools, CMake 3.20 or newer,
Ninja, Git, and Python 3 with `cogapp`. All dependencies are pinned Git
submodules. Generated files stay under `build/`; the original game installation
is never used as a build directory.

Initialize the source tree once:

```powershell
git submodule update --init --recursive
python -m pip install --target build/python-packages cogapp
```

Build without launching the game:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/build-windows.ps1 -Configuration Debug -Test
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/build-windows.ps1 -Configuration Release -Test
```

The driver creates isolated `build/msvc-debug` and `build/msvc-release`
directories, builds only `openjkdf2-64`, and optionally runs CTest tests labeled
`unit`. Renderer hardware smoke tests are deliberately excluded from ordinary
unit runs.
