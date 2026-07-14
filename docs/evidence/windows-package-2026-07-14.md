# Windows package and clean-install verification — 2026-07-14

## Artifact

- Package: `OpenJKDF2-AMD-Enhanced-windows-x64.zip`
- SHA-256: `2dabdeba6c16ba1298a3a2dbe534449e82e4b60e3db2000498dba1f273f732d6`
- Configuration: Release x64
- Source commit: `58959a1229018ce0f0781655b12f2b7926a98a44`
- Source worktree recorded clean by `BUILD-PROVENANCE.json`

## Package audit

`scripts/verify-windows-package.ps1` verified all 36 manifest entries by path, size, and SHA-256. It found zero prohibited Jedi Knight asset files and no unlisted package files. The package contains the two executables, required open-source runtime DLLs, launcher/install/uninstall scripts, configuration example, documentation (including the legal optional-enhancement-pack guide), notices, and 13 dependency license files.

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

The acceptance run's machine-readable result is retained outside source control under `runtime-evidence/clean-install-2026-07-14-03/clean-install-result.json`.

## Scope and limitations

This proves the distributable package and Steam discovery path on the available Windows 11 machine. The GOG discovery implementation and interactive browse fallback are present but have not been exercised against a real GOG installation or through a human UI session. The run used an isolated install root on the development machine rather than a separate freshly provisioned Windows VM, so the broader “clean machine” criterion remains incomplete.
