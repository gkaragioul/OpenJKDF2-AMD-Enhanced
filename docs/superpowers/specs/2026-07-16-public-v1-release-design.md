# OpenJKDF2 AMD Enhanced Public v1.0 Release Design

## Objective

Publish OpenJKDF2 AMD Enhanced as a transparent, rights-safe OpenJKDF2 fork that
Windows users can download and run against their own legally obtained Steam or
GOG installation of *Star Wars: Jedi Knight - Dark Forces II*. The public
release must make George Karagioules's engineering contribution easy to inspect
without distributing Lucasfilm game data or implying endorsement by Lucasfilm,
Disney, AMD, Valve, GOG, or the upstream OpenJKDF2 maintainers.

## Chosen approach

Create `gkaragioul/OpenJKDF2-AMD-Enhanced` as a public GitHub fork of
`shinyquagsire23/OpenJKDF2`. Rename the enhanced development branch to `main`,
make it the default branch, and publish no other branches. Publish the first
GitHub Release from the exact annotated tag `1.0`. George's portfolio will link
to that release through its existing latest-release download redirect pattern.

This is preferred over an unrelated imported repository because GitHub will
show the upstream relationship and commit ancestry. It is preferred over a
binary-only repository because the linked and statically compiled open-source
components need clear notices and equivalent access to their corresponding
source.

## Repository contract

- Public repository: `https://github.com/gkaragioul/OpenJKDF2-AMD-Enhanced`
- Only public branch: `main`
- First tag: `1.0`
- Release title: `OpenJKDF2 AMD Enhanced 1.0`
- Windows asset: `OpenJKDF2-AMD-Enhanced-windows-x64-1.0.zip`
- Integrity asset: `SHA256SUMS.txt`
- Default branch protections must not create or expose an additional branch.
- The upstream remote remains documented as
  `https://github.com/shinyquagsire23/OpenJKDF2`.
- Generated build folders, runtime evidence, local game data, user saves,
  credentials, crash dumps, and machine-specific configuration remain ignored.

## Licensing decision

Do not replace the upstream top-level license with MIT, GPL, or a guessed SPDX
identifier. GitHub reports the upstream license as `Other`/`NOASSERTION`, while
the actual `LICENSE.md` grants permission to use, copy, modify, and distribute
OpenJKDF2 without fee. Preserve that text verbatim.

Add an explicit licensing guide that states:

- OpenJKDF2 remains copyright its contributors under `LICENSE.md`.
- George Karagioules's OpenJKDF2 AMD Enhanced modifications are copyright 2026
  George Karagioules and are offered under the same permission and warranty
  terms in `LICENSE.md`.
- This does not relicense third-party components. Every dependency remains
  governed by its own license.
- `libsmacker` is LGPL-2.1-or-later and is compiled into the application. The
  tagged repository, build scripts, and included source provide the material
  needed to rebuild and relink it.
- OpenAL Soft is shipped as a replaceable DLL under LGPL-2.0-or-later, and its
  exact source revision and license are identified.
- DrMinGW runtime DLLs are shipped under LGPL-2.1 with their license, source
  location, and version identified.
- SDL, SDL_mixer, FreeGLUT, GLEW, zlib, libpng, libsmusher,
  nativefiledialog-extended, and their enabled codec dependencies retain their
  individual notices.

The release package must include the top-level license, a corrected
third-party notice, and the relevant license texts. Its notice must prominently
permit modification and reverse engineering for debugging modifications where
required by the LGPL components. The GitHub Release must link to the `1.0`
source tree and explain cloning with `--recurse-submodules`.

This is a licensing and release-boundary engineering decision, not a substitute
for advice from a qualified lawyer.

## Rights-safe distribution boundary

The source repository and downloadable ZIP may contain the enhanced engine,
launcher, installer, watchdog, diagnostic tools, open-source runtime libraries,
configuration examples, reports, documentation, manifests, and checksums.

They must not contain original game levels, textures, models, music, video,
dialogue, scripts, proprietary executables, registry exports, Steam/GOG
credentials, or files copied from a user's installation. The launcher only
discovers, validates, and reads a user-selected legal installation in place.
The package scanner must fail on known game-asset extensions before the ZIP is
created, and release verification must report zero findings.

## GitHub README and release instructions

The repository home page will lead with a precise description and a visible
notice that the download is an engine enhancement, not the game. Its primary
sections are:

1. What this fork changes: modern controls, borderless native-resolution
   startup, display options, frame pacing, renderer diagnostics, and AMD Radeon
   validation.
2. Requirements: Windows x64 and a legally obtained Steam or GOG copy.
3. Three-step installation: download and extract `1.0`, run `Install.ps1`, then
   launch the desktop shortcut and allow automatic discovery or select the game
   folder.
4. Portable mode and uninstall instructions.
5. Default Modern control map and the in-game path for switching Modern versus
   Classic controls.
6. Display mode instructions, with Borderless Fullscreen at the monitor's native
   pixel dimensions as the default.
7. Troubleshooting, package verification, source build instructions, issue
   reporting, upstream attribution, licensing, and non-endorsement disclaimer.

The `1.0` release description repeats the requirements and installation steps,
summarizes the verified enhancements, links to source and licenses, publishes
the SHA-256 checksum, and states exactly what is not included.

## Screenshot policy and gallery

Use a small case-study gallery rather than shipping screenshots inside the
Windows package. The selected images are captures made while testing with a
legally owned copy and are used only to identify and explain the modified
software:

1. One representative gameplay screenshot without desktop overlays,
   notifications, or personal information.
2. One Modern Controls settings screenshot showing the control-style selector
   and application action.
3. One Display Options screenshot showing Windowed, Borderless Fullscreen, and
   Fullscreen choices plus native monitor resolution behavior.
4. Optionally, one diagnostics screenshot showing the renderer, refresh rate,
   and borderless state when it adds technical proof.

Images will be cropped to the game window, compressed for the web, captioned,
and attributed to *Star Wars: Jedi Knight - Dark Forces II* / Lucasfilm where
appropriate. They are not reusable asset packs and are not included in the
release ZIP. The page will use only the number and resolution needed for
technical commentary and review.

## Portfolio page

Add `/openjkdf2-enhanced` to `georgekaragioules.com` using the site's existing
industrial, diagnostic-lab visual language. The first viewport contains:

- `Legacy Game Engine Modernization` as the portfolio framing;
- `OpenJKDF2 AMD Enhanced` as the project name;
- a direct `Download Windows x64` action routed to the latest GitHub release;
- `View source` as the secondary action; and
- an immediate `Requires your own Steam or GOG copy` notice.

Below it, show the screenshot gallery, the three installation steps, verified
technical improvements, Modern control defaults, display behavior, package
contents versus excluded game content, checksum/source trust information, and
the complete independent-project disclaimer. Add the route to the sitemap and
the existing tools/project discovery surface. Do not use Lucasfilm, Star Wars,
or AMD logos as project branding.

## Error handling and user safety

- If Steam/GOG discovery fails, the launcher opens a folder picker and explains
  the required directory without copying files.
- If validation fails, it lists missing required files and does not start.
- Installation and uninstallation remain per-user and never delete the original
  game or normal user-data directory.
- The download endpoint returns a clear error if GitHub has no matching ZIP,
  rather than selecting an arbitrary asset.
- The README provides a manual GitHub Release link if the website redirect is
  unavailable.

## Verification and publication sequence

1. Commit the completed display-menu fix and release documentation to the
   enhanced branch, then rename it to `main`.
2. Run Debug and Release test suites, package-tool tests, package verification,
   proprietary-content scanning, and `git diff --check`.
3. Rebuild the Release binaries and package from a clean committed source state
   so `BUILD-PROVENANCE.json` records `source_dirty: false` and the tag commit.
4. Verify every manifest hash, the ZIP checksum, and the absence of game data.
5. Create the public GitHub fork, push only `main`, set it as default, and verify
   that no other public branches exist.
6. Create annotated tag `1.0`, push it, and publish the GitHub Release with the
   Windows ZIP and `SHA256SUMS.txt`.
7. Add the portfolio page and download mapping, then run lint, production build,
   local desktop/mobile browser checks, metadata checks, and link checks.
8. Deploy the portfolio from its existing Vercel project only after the public
   release URL succeeds.
9. Re-query GitHub to verify repository visibility, default branch, branch list,
   tag, release text, release assets, and checksums.

## Acceptance criteria

- The repository is public and accurately identified as an OpenJKDF2 fork.
- GitHub exposes exactly one branch named `main` and an exact `1.0` tag.
- The `1.0` release has complete instructions, source/license links, Windows ZIP,
  and checksum.
- The release ZIP contains zero proprietary game-data findings and launches only
  when the user supplies a valid legal installation.
- The repository and package preserve the upstream license and all applicable
  third-party notices without claiming an inaccurate standard SPDX license.
- README and portfolio instructions are sufficient for a first-time user.
- The portfolio includes the requested gameplay and settings screenshots without
  placing them in the downloadable package.
- The website production build and all release verification checks pass.
