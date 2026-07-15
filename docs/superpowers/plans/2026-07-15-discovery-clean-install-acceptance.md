# Discovery and Clean-Install Acceptance Plan

Date: 2026-07-15

Objective: prove first-run Steam/GOG discovery, required-asset validation, browse fallback, and the strongest clean-install path available on this Windows host without copying or modifying proprietary game data.

## Task 1: Discovery contract (RED)

Files:
- Modify: `scripts/test-package-tools.ps1`
- Modify: `cmake/OpenJKDF2Tests.cmake`

- [ ] Add isolated Steam primary-library, Steam secondary-library, GOG common-location, invalid-asset, browse-success, and browse-rejection fixtures.
- [ ] Register the asset-free discovery test in CTest.
- [ ] Run it against the current module and observe the missing resolution/browse contract fail.

## Task 2: Deterministic discovery and resolution (GREEN)

Files:
- Modify: `packaging/windows/PackageTools.psm1`
- Modify: `packaging/windows/Launch-OpenJKDF2.ps1`

- [ ] Separate candidate enumeration from path resolution so explicit, saved, automatic, and browse sources are validated consistently.
- [ ] Parse Steam library folders and app manifests, retain common Steam/GOG locations and registry discovery, and deduplicate canonical paths.
- [ ] Add an injectable browse provider for asset-free tests while retaining the real Windows folder picker for users.
- [ ] Add a discovery-only launcher path that persists only a validated directory and never starts the game.
- [ ] Return actionable missing-file details without downloading or copying proprietary content.

## Task 3: Packaged first-run and isolated install acceptance

Files:
- Modify: `scripts/test-clean-install.ps1`
- Modify: package documentation as needed

- [ ] Make the clean-install harness invoke the installed launcher in discovery-only mode before gameplay.
- [ ] Verify the saved launcher configuration identifies the legitimate read-only Steam directory.
- [ ] Re-run installed launch, save/restore, display invariance, asset invariance, uninstall, shortcut removal, and user-data preservation against the watchdog-integrated package.
- [ ] Exercise the packaged module with the Steam/GOG/browse fixture contract.

## Task 4: Evidence, gates, and final audit

Files:
- Modify: `docs/evidence/windows-package-2026-07-14.md`
- Modify: `docs/evidence/requirements.csv`
- Modify: `FINAL-REPORT.md`
- Create: `docs/evidence/discovery-clean-install-2026-07-15.json`

- [ ] Record real Steam and synthetic GOG/browse evidence separately; do not claim a real GOG installation.
- [ ] Record that Windows Sandbox, Hyper-V, and a second Windows host are unavailable if that remains true.
- [ ] Classify `M10-DISCOVERY` from tested behavior and `AC-CLEAN` from the exact available-host boundary.
- [ ] Rebuild and verify the legal x64 package and checksums.
- [ ] Run exact Debug and Release build/test gates.
- [ ] Audit raw-to-curated evidence, privacy, package contents, staged files, and `git diff --check`.
- [ ] Commit implementation and evidence separately.
