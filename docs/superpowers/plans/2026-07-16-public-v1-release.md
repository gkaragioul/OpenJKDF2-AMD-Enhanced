# OpenJKDF2 AMD Enhanced Public v1.0 Release Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Publish a rights-safe OpenJKDF2 AMD Enhanced 1.0 source and Windows release on GitHub, then add a verified portfolio/download page with gameplay and settings screenshots.

**Architecture:** The enhanced OpenJKDF2 repository is the canonical source, license, build, package, checksum, and GitHub Release location. The existing Vite portfolio consumes that release through its established `/download/:tool` redirect and presents installation, proof, and legal-boundary information without hosting the game or its assets.

**Tech Stack:** C/CMake/Ninja, PowerShell packaging and validation, Git/GitHub CLI, React 18/Vite/Tailwind, Vercel, Puppeteer.

## Global Constraints

- Public repository: `gkaragioul/OpenJKDF2-AMD-Enhanced`.
- The final public repository exposes exactly one branch named `main`.
- The exact release tag is `1.0`; the release title is `OpenJKDF2 AMD Enhanced 1.0`.
- Preserve upstream `LICENSE.md` verbatim and license George Karagioules's modifications under the same terms.
- Third-party components retain their own licenses; do not label the whole project MIT, ISC, or GPL.
- No original game files, levels, textures, models, music, videos, dialogue, scripts, or proprietary executables enter the repository or release ZIP.
- The Windows ZIP requires a legally obtained Steam or GOG installation and reads it in place.
- Screenshots are limited technical case-study captures and are excluded from the Windows ZIP.
- Use only `main`; do not create an agent or feature branch.

---

### Task 1: Commit and re-verify the display-menu correction

**Files:**
- Modify: `scripts/check-video-menu.ps1`
- Modify: `scripts/test-display-options.ps1`
- Modify: `src/General/DefaultSettingsMigration.h`
- Modify: `src/Platform/SDL2/jkGUIDisplay.c`
- Modify: `src/Tests/test_default_settings_migration.c`
- Modify: `src/World/jkPlayer.c`

**Interfaces:**
- Consumes: existing `DisplayMode`, player profile, and window defaults APIs.
- Produces: interactive display controls and machine-wide display mode ownership with migration version `2`.

- [ ] **Step 1: Run the focused static checks against the current diff**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/check-video-menu.ps1
```

Expected: JSON/pass output with no missing contracts.

- [ ] **Step 2: Run the migration unit test in Debug and Release**

```powershell
ctest --test-dir build/msvc-debug -C Debug --output-on-failure
ctest --test-dir build/msvc-release -C Release --output-on-failure
```

Expected: `100% tests passed` for both configurations.

- [ ] **Step 3: Run the runtime display-options harness**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test-display-options.ps1 -DataDir "$env:OPENJKDF2_DATA_DIR" -EvidenceRoot "build/v1-display-options-$([Guid]::NewGuid().ToString('N'))"
```

Expected: Windowed and Borderless cases pass, preserve the desktop mode, and persist defaults version `2`.

- [ ] **Step 4: Commit only the six display-fix files**

```powershell
git add scripts/check-video-menu.ps1 scripts/test-display-options.ps1 src/General/DefaultSettingsMigration.h src/Platform/SDL2/jkGUIDisplay.c src/Tests/test_default_settings_migration.c src/World/jkPlayer.c
git commit -m "fix: keep display options interactive"
```

Expected: one focused commit and a clean source diff apart from later release work.

### Task 2: Make the license and package notices release-complete

**Files:**
- Create: `LICENSING.md`
- Create: `scripts/test-release-documentation.ps1`
- Modify: `packaging/windows/THIRD-PARTY-NOTICES.md`
- Modify: `packaging/windows/README-PACKAGE.md`
- Modify: `packaging/windows/CHANGELOG.md`
- Modify: `scripts/build-windows-package.ps1`
- Modify: `scripts/verify-windows-package.ps1`

**Interfaces:**
- Consumes: upstream `LICENSE.md`, pinned submodule revisions, bundled DrMinGW 0.9.3 files.
- Produces: `LICENSING.md` and release-package notices that identify license, source, and relinking access.

- [ ] **Step 1: Add a failing documentation contract test**

```powershell
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$checks = @{
  'LICENSING.md' = @('same permission and warranty terms', 'libsmacker', 'LGPL-2.1-or-later', 'OpenAL Soft', 'DrMinGW', '--recurse-submodules')
  'packaging/windows/THIRD-PARTY-NOTICES.md' = @('libsmacker', 'OpenAL Soft', 'DrMinGW 0.9.3', 'corresponding source')
  'packaging/windows/README-PACKAGE.md' = @('legally owned Steam or GOG', 'No game files', 'Borderless Fullscreen')
}
foreach ($entry in $checks.GetEnumerator()) {
  $text = Get-Content -Raw -LiteralPath (Join-Path $root $entry.Key)
  foreach ($required in $entry.Value) {
    if (-not $text.Contains($required)) { throw "$($entry.Key) missing: $required" }
  }
}
```

- [ ] **Step 2: Run the contract and verify it fails before the new guide exists**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test-release-documentation.ps1
```

Expected: failure naming missing `LICENSING.md`.

- [ ] **Step 3: Write the licensing guide and corrected notices**

`LICENSING.md` must state that upstream and George's changes use the existing custom permissive terms, while third-party components remain separately licensed. It must identify libsmacker as LGPL-2.1-or-later, OpenAL Soft as LGPL-2.0-or-later, DrMinGW 0.9.3 as LGPL-2.1, include source URLs/revisions, permit debugging/reverse engineering of LGPL modifications, and document `git clone --recurse-submodules`.

- [ ] **Step 4: Package and require the licensing guide**

Add `LICENSING.md` to the root-document copy list in `scripts/build-windows-package.ps1` and to the required file list in `scripts/verify-windows-package.ps1`.

- [ ] **Step 5: Run the documentation and package-tool tests**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test-release-documentation.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test-package-tools.ps1
```

Expected: both commands succeed and proprietary scan coverage remains enabled.

- [ ] **Step 6: Commit the licensing boundary**

```powershell
git add LICENSING.md scripts/test-release-documentation.ps1 packaging/windows/THIRD-PARTY-NOTICES.md packaging/windows/README-PACKAGE.md packaging/windows/CHANGELOG.md scripts/build-windows-package.ps1 scripts/verify-windows-package.ps1
git commit -m "docs: define v1 release licensing"
```

### Task 3: Replace the repository front page and prepare screenshots

**Files:**
- Modify: `README.md`
- Create: `docs/images/amd-enhanced-gameplay.jpg`
- Create: `docs/images/amd-enhanced-modern-controls.jpg`
- Create: `docs/images/amd-enhanced-display-options.jpg`
- Create: `docs/images/amd-enhanced-diagnostics.jpg`
- Create: `docs/RELEASE-1.0.md`

**Interfaces:**
- Consumes: verified runtime captures and the release/install contract.
- Produces: the GitHub repository landing page, release body source, and four web-only images.

- [ ] **Step 1: Capture or select clean technical screenshots**

Select a gameplay frame with no AMD overlay, desktop taskbar, credentials, or personal data. Capture the current `Setup > Controls > Control Options` screen showing Modern/Classic plus Apply, and `Setup > Display > Display Options` showing the display-mode choices and native monitor mode. Use the existing diagnostics capture that reports borderless native resolution.

- [ ] **Step 2: Crop and compress the captures**

```powershell
magick input.png -strip -resize '1920x1080>' -quality 84 docs/images/amd-enhanced-gameplay.jpg
```

Repeat for the three settings/proof images. Expected: each JPEG is below 700 KB, contains only the game window, and remains legible at 1280 CSS pixels.

- [ ] **Step 3: Replace `README.md` with the v1 user journey**

The README order is: independent-fork/BYO notice, screenshots, features, requirements, three installation steps, portable mode, uninstall, Modern controls, display modes, verification/checksum, troubleshooting, source/build, upstream credit, licensing, and full trademark/non-endorsement disclaimer. Link purchases only to official Steam and GOG pages.

- [ ] **Step 4: Write `docs/RELEASE-1.0.md`**

The body must identify the asset as an engine enhancement, list requirements and installation steps, summarize Modern controls and Borderless defaults, link source/license files, state that no game content is included, and reserve a checksum placeholder token `__ZIP_SHA256__` that the release script replaces before publication.

- [ ] **Step 5: Verify links and prohibited wording**

```powershell
rg -n 'download (the )?game|free game|ROM|crack|ISO' README.md docs/RELEASE-1.0.md
rg -n 'legally (owned|obtained)|Steam|GOG|LICENSE.md|LICENSING.md|OpenJKDF2' README.md docs/RELEASE-1.0.md
```

Expected: no prohibited distribution claim and all required boundary terms present.

- [ ] **Step 6: Commit repository presentation**

```powershell
git add README.md docs/RELEASE-1.0.md docs/images/amd-enhanced-*.jpg
git commit -m "docs: prepare OpenJKDF2 AMD Enhanced 1.0"
```

### Task 4: Produce the clean 1.0 Windows package

**Files:**
- Generated: `dist/OpenJKDF2-AMD-Enhanced-windows-x64-1.0.zip`
- Generated: `dist/SHA256SUMS.txt`
- Generated: package `BUILD-PROVENANCE.json`
- Generated: package `PACKAGE-MANIFEST.json`

**Interfaces:**
- Consumes: committed source and Release build outputs.
- Produces: asset-free Windows release artifacts tied to the exact source commit.

- [ ] **Step 1: Run all Debug and Release tests from the committed tree**

```powershell
ctest --test-dir build/msvc-debug -C Debug --output-on-failure
ctest --test-dir build/msvc-release -C Release --output-on-failure
```

Expected: `100% tests passed` in both trees.

- [ ] **Step 2: Rebuild Release binaries**

```powershell
cmake --build build/msvc-release --config Release
```

Expected: engine, renderer smoke tool, and display watchdog build successfully.

- [ ] **Step 3: Build and version the package**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-windows-package.ps1 -BuildDir build/msvc-release -OutputDir dist
Move-Item -LiteralPath dist/OpenJKDF2-AMD-Enhanced-windows-x64.zip -Destination dist/OpenJKDF2-AMD-Enhanced-windows-x64-1.0.zip
(Get-FileHash dist/OpenJKDF2-AMD-Enhanced-windows-x64-1.0.zip -Algorithm SHA256).Hash.ToLowerInvariant() + '  OpenJKDF2-AMD-Enhanced-windows-x64-1.0.zip' | Set-Content dist/SHA256SUMS.txt -Encoding ascii
```

- [ ] **Step 4: Verify package boundary, manifest, and clean provenance**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify-windows-package.ps1 -ZipPath dist/OpenJKDF2-AMD-Enhanced-windows-x64-1.0.zip
```

Expected: zero proprietary findings, valid manifest, all required licenses, and `source_dirty: false` for the release commit.

### Task 5: Publish the one-branch GitHub fork and release

**Files:**
- Git refs: `main`, annotated tag `1.0`
- GitHub Release assets: Windows ZIP and `SHA256SUMS.txt`

**Interfaces:**
- Consumes: clean verified source commit and release artifacts.
- Produces: public canonical repository and latest release URL.

- [ ] **Step 1: Rename the local enhanced branch and configure remotes**

```powershell
git branch -m main
git remote rename origin upstream
gh repo fork shinyquagsire23/OpenJKDF2 --fork-name OpenJKDF2-AMD-Enhanced --clone=false
git remote add origin https://github.com/gkaragioul/OpenJKDF2-AMD-Enhanced.git
```

Expected: `upstream` points to the original project and `origin` to George's public fork.

- [ ] **Step 2: Push only the enhanced main branch**

```powershell
git push -u origin main
gh api -X PATCH repos/gkaragioul/OpenJKDF2-AMD-Enhanced -f default_branch=main -f description='Modern Windows presentation, controls, display safety, diagnostics, and AMD Radeon validation for OpenJKDF2; requires legally owned game data.'
gh api -X DELETE repos/gkaragioul/OpenJKDF2-AMD-Enhanced/git/refs/heads/master
```

Expected: `gh api repos/gkaragioul/OpenJKDF2-AMD-Enhanced/branches --jq '.[].name'` prints only `main`.

- [ ] **Step 3: Create and push the exact annotated tag**

```powershell
git tag -a 1.0 -m 'OpenJKDF2 AMD Enhanced 1.0'
git push origin 1.0
```

- [ ] **Step 4: Render the release body with the real checksum**

```powershell
$hash = (Get-FileHash dist/OpenJKDF2-AMD-Enhanced-windows-x64-1.0.zip -Algorithm SHA256).Hash.ToLowerInvariant()
(Get-Content docs/RELEASE-1.0.md -Raw).Replace('__ZIP_SHA256__', $hash) | Set-Content build/release-1.0-body.md -Encoding utf8
```

- [ ] **Step 5: Publish GitHub Release 1.0**

```powershell
gh release create 1.0 dist/OpenJKDF2-AMD-Enhanced-windows-x64-1.0.zip dist/SHA256SUMS.txt --repo gkaragioul/OpenJKDF2-AMD-Enhanced --title 'OpenJKDF2 AMD Enhanced 1.0' --notes-file build/release-1.0-body.md --verify-tag
```

Expected: public release with exactly the ZIP and checksum assets.

### Task 6: Add the portfolio/download page

**Files:**
- Create: `G:/DevWork/WebSites/GeorgeKaragioules.com/src/pages/OpenJKDF2EnhancedPage.jsx`
- Create: `G:/DevWork/WebSites/GeorgeKaragioules.com/public/openjkdf2-enhanced-gameplay.jpg`
- Create: `G:/DevWork/WebSites/GeorgeKaragioules.com/public/openjkdf2-enhanced-modern-controls.jpg`
- Create: `G:/DevWork/WebSites/GeorgeKaragioules.com/public/openjkdf2-enhanced-display-options.jpg`
- Create: `G:/DevWork/WebSites/GeorgeKaragioules.com/public/openjkdf2-enhanced-diagnostics.jpg`
- Modify: `G:/DevWork/WebSites/GeorgeKaragioules.com/src/App.jsx`
- Modify: `G:/DevWork/WebSites/GeorgeKaragioules.com/api/download/[tool].js`
- Modify: `G:/DevWork/WebSites/GeorgeKaragioules.com/tools/prerender.js`
- Modify: `G:/DevWork/WebSites/GeorgeKaragioules.com/public/sitemap.xml`
- Modify: the existing tools/project discovery component that links to individual project pages.

**Interfaces:**
- Consumes: GitHub latest release asset `OpenJKDF2-AMD-Enhanced-windows-x64-1.0.zip` and four optimized captures.
- Produces: `/openjkdf2-enhanced` and `/download/openjkdf2-enhanced`.

- [ ] **Step 1: Add the exact latest-release mapping**

```javascript
'openjkdf2-enhanced': {
  repo: 'OpenJKDF2-AMD-Enhanced',
  preferred: [/^OpenJKDF2-AMD-Enhanced-windows-x64-\d+(?:\.\d+)*\.zip$/i]
}
```

Expected: the redirect selects the versioned Windows ZIP and never `SHA256SUMS.txt`.

- [ ] **Step 2: Build the project page**

Use the site's industrial diagnostic-lab palette and existing motion conventions. Include a primary download CTA, source CTA, BYO requirement, four-image gallery with modal viewing, three installation steps, Modern control map, display behavior, verification/checksum section, included/excluded content, upstream credit, and full independent-project disclaimer.

- [ ] **Step 3: Add route, discovery link, sitemap, and prerender entry**

Lazy-load `OpenJKDF2EnhancedPage`, register `/openjkdf2-enhanced`, add the project card/link, add the route to `tools/prerender.js`, and add `https://georgekaragioules.com/openjkdf2-enhanced` to the sitemap.

- [ ] **Step 4: Run lint and production build**

```powershell
npm run lint
npm run build:prerender
```

Expected: zero lint errors, Vite build success, and `dist/openjkdf2-enhanced/index.html` exists with the correct canonical URL and title.

- [ ] **Step 5: Commit and push the website on main**

```powershell
git add src/pages/OpenJKDF2EnhancedPage.jsx src/App.jsx 'api/download/[tool].js' tools/prerender.js public/sitemap.xml public/openjkdf2-enhanced-*.jpg
git commit -m "feat: add OpenJKDF2 Enhanced release page"
git push origin main
```

Do not stage existing `.agents/` or `.codex/` directories.

### Task 7: Deploy and perform full-story verification

**Files:**
- Vercel production deployment for `georgekaragioules.com`
- GitHub repository/release state

**Interfaces:**
- Consumes: published GitHub Release and production website commit.
- Produces: verified public download and portfolio experience.

- [ ] **Step 1: Deploy the existing Vercel project**

```powershell
vercel --prod --yes
```

Expected: successful production deployment on the linked project/domain.

- [ ] **Step 2: Verify GitHub state**

```powershell
gh repo view gkaragioul/OpenJKDF2-AMD-Enhanced --json nameWithOwner,visibility,defaultBranchRef,url
gh api repos/gkaragioul/OpenJKDF2-AMD-Enhanced/branches --jq '.[].name'
gh release view 1.0 --repo gkaragioul/OpenJKDF2-AMD-Enhanced --json name,tagName,isDraft,isPrerelease,url,assets
```

Expected: public, default `main`, only `main`, exact tag `1.0`, non-draft/non-prerelease, ZIP and checksum assets.

- [ ] **Step 3: Verify public pages and download redirect**

```powershell
curl.exe -I https://georgekaragioules.com/openjkdf2-enhanced
curl.exe -I https://georgekaragioules.com/download/openjkdf2-enhanced
```

Expected: page returns success and download redirects to the 1.0 ZIP asset.

- [ ] **Step 4: Verify desktop and mobile browser rendering**

Open the production page at 1440x900 and 390x844. Confirm no overflow, all screenshots open/close, CTAs work, instructions are readable, and the BYO/legal notice is visible above the fold.

- [ ] **Step 5: Record final hashes and URLs**

Return the repository URL, release URL, portfolio URL, ZIP filename, SHA-256, branch list, tests/build results, and any non-blocking warnings.
