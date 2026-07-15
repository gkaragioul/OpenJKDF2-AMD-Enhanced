# Windows package and clean-install verification — 2026-07-14

## Artifact

- Package: `OpenJKDF2-AMD-Enhanced-windows-x64.zip`
- SHA-256: `040b4735cc8dfc2b98628a6b8b6c319f998ccc66d3f63625d007335e7f176aa5`
- Configuration: Release x64
- Source commit: `4597a6a1`
- Source worktree recorded clean by `BUILD-PROVENANCE.json`

## Package audit

`scripts/verify-windows-package.ps1` verified all 36 manifest entries by path, size, and SHA-256. It found zero prohibited Jedi Knight asset files and no unlisted package files. The package contains the two executables, required open-source runtime DLLs, launcher/install/uninstall scripts, configuration example, Display Options documentation (including the legal optional-enhancement-pack guide), notices, and 13 dependency license files.

## Watchdog-integrated package re-audit - 2026-07-15

The clean worktree at commit `c5ed5c0c` produced a refreshed Release x64 ZIP
with SHA-256
`add291bbb9b9319fddb7a2abc8b8cd9510883bce82c3feb6b45863fb4ab6c253`.
The verifier checked all 37 manifest entries with zero proprietary findings and
a valid manifest. The added `OpenJKDF2-Display-Watchdog.exe` is 16,384 bytes
with SHA-256
`42fe49248d305e06b98fb6f132c7c4d0cf2e33a78a92c633eda3f3d1961ff19c`.
This re-audit covers package provenance and contents; the installed Steam
acceptance path below remains the separate 2026-07-14 run.

## Installed acceptance path

The final ZIP was extracted to an isolated acceptance directory and installed to an isolated per-user-style application directory. The test used the legitimate Steam installation at `D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight` read-only.

Measured results from `scripts/test-clean-install.ps1`:

- automatic Steam data discovery: passed
- installed packaged gameplay launch: passed
- save creation: passed (360,799 bytes)
- same-process save restore: passed
- fresh-process save restore: passed
- display invariant: passed (`2560x1440@165` before and after)
- original asset metadata invariant: passed
- application removal: passed
- desktop-shortcut removal: passed
- user-data preservation: passed

The refreshed acceptance run's machine-readable result is retained outside source control under `runtime-evidence/clean-install-display-options-2026-07-14-02/clean-install-result.json`.

## Scope and limitations

This proves the distributable package and Steam discovery path on the available Windows 11 machine. The GOG discovery implementation and interactive browse fallback are present but have not been exercised against a real GOG installation or through a human UI session. The run used an isolated install root on the development machine rather than a separate freshly provisioned Windows VM, so the broader “clean machine” criterion remains incomplete.
