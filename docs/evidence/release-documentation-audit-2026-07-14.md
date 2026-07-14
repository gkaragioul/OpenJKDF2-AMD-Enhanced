# Release documentation and upstream-quality audit — 2026-07-14

## Scope

This audit verifies Milestone 11's locally controllable deliverables: Windows 11
build instructions, architecture and renderer fallback decisions, timing design,
known limitations, measured results, troubleshooting, licensing and attribution,
fork identity, focused history, formatting policy, static checks, unit tests,
native Release CI, and preparation of generally useful change groups for future
upstream review.

## Authoritative documents

| Requirement | Evidence |
|---|---|
| Windows 11 toolchain and exact Debug/Release commands | `docs/building-windows.md` |
| Renderer, display, storage, packaging, diagnostics decisions | `docs/architecture.md` |
| Render/simulation clocks, limiter, VSync, evidence boundary | `docs/timing-design.md` |
| GPU/driver/backend/settings matrix | `COMPATIBILITY.md` |
| Measured frame pacing | `BENCHMARKS.md` |
| Completed work, limitations, reproduction links | `FINAL-REPORT.md` |
| Build/package/runtime troubleshooting | `packaging/windows/TROUBLESHOOTING.md` |
| Source and dependency attribution | `LICENSE.md`, `packaging/windows/THIRD-PARTY-NOTICES.md` |
| Candidate upstream patch groups and fork-only exclusions | `docs/upstreaming.md` |
| Community-fork and non-endorsement disclosure | `README.md` |

The README explicitly states that the fork contains no game assets and is not
affiliated with LucasArts, Lucasfilm, Disney, AMD, or upstream OpenJKDF2. The
package notices separately preserve upstream and third-party attribution.

## Automation

- `.editorconfig` defines UTF-8, LF, final-newline, trailing-whitespace, and
  indentation policy without mechanically reformatting unrelated legacy code.
- `c11_portability` performs the practical static source/portability audit used
  by this C codebase.
- `release_documentation_contract` fails when required documents, disclosures,
  report sections, or native Release-CI tokens are absent or stale.
- `.github/workflows/win64.yml` uses `windows-latest`, recursive submodules, the
  exact MSVC Release build/test command, and uploads the resulting executable.
  It is configured for the fork branch, pull requests, and manual dispatch.
- The workflow parsed as valid YAML during this audit.

## Verification results

The following exact commands both passed all 33 tests with zero failures:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -Configuration Release -Test
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -Configuration Debug -Test
```

`git diff --check` also passed. Existing warnings originate in retained
third-party libsmacker formatting and the legacy GL compatibility header; they
do not change the zero-exit build/test result and are not hidden by the docs.

Recent `git log` entries remain focused by feature/evidence group. This audit
does not claim that an upstream pull request was published: that external action
requires upstream coordination and credentials. It proves the generally useful
change groups are identified and separable for review.
