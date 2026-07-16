# OpenJKDF2 Video Presentation Design

## Purpose

Create a short, 16:9 PDF presentation that George Karagioules can show in a
video to explain what was accomplished in OpenJKDF2 AMD Enhanced. The deck must
be technically honest: upstream OpenJKDF2 provides the modern cross-platform
engine foundation, while this fork adds an opinionated Windows 11 experience,
safety features, packaging, verification, and public release quality.

## Audience outcome

By the end, a general gaming and developer audience should understand why the
original 1997 game needed a modern engine, what upstream OpenJKDF2 already
solved, what this fork specifically added, and what evidence supports the
finished release.

## Deliverables

- A polished 16:9 PDF suitable for screen display in a video.
- A matching editable PowerPoint source.
- Approximately eight slides with low text density and large typography.

## Visual direction

Use a Jedi Knight menu-inspired presentation style:

- dark carved-stone or near-black surfaces;
- burnt-orange, gothic-style display headings;
- yellow energy accents inspired by the game's menu lightning;
- black framed content panels with restrained gold rules;
- real screenshots from the release as dominant evidence;
- modern, highly legible sans-serif body copy.

The design should reference the game's visual identity without copying or
redistributing proprietary assets beyond the user-provided and locally captured
screenshots used for portfolio commentary.

## Narrative

1. **Cover:** OpenJKDF2 AMD Enhanced, version 1.0, with the note that the work
   was completed through approximately five days of Codex-assisted engineering.
2. **Original problem:** The 1997 engine expected DirectX 5-era behavior,
   16-bit formats, old display switching, legacy driver behavior, and 1990s
   timing assumptions. Clearly state that upstream OpenJKDF2 already performs
   the fundamental modernization to SDL2, OpenAL, and OpenGL 3.3.
3. **Upstream versus fork:** Present the complete two-column comparison supplied
   by the user, covering platform focus, controls, display behavior, renderer
   validation, packaging, game-file discovery, recovery, frame pacing, and
   documentation.
4. **Modern controls:** Show the real Modern/Classic settings screenshot and the
   new default FPS mapping.
5. **Display behavior:** Show the real display-options screenshot and explain
   native borderless defaults, selectable modes, confirmation, and recovery.
6. **Renderer visibility:** Show the diagnostics screenshot and explain
   standards validation, capability-based fallbacks, logging, and safe mode.
7. **Release engineering:** Show verified figures: 46 Debug tests, 46 Release
   tests, a 4.4 MB engine-only package, SHA-256 verification, installer,
   Steam/GOG discovery, portable mode, uninstaller, and legal notices.
8. **Outcome:** Resolve the story with the positioning: OpenJKDF2 ready to
   install, not merely ready to compile, followed by portfolio and GitHub links.

## Content rules

- Do not claim this fork created OpenJKDF2 or uniquely made JKDF2 playable on
  AMD hardware.
- Describe RX 7900 XTX support as tested evidence, not universal AMD coverage.
- State that no original game assets ship in the release and that users provide
  a legally owned Steam or GOG installation.
- Attribute upstream OpenJKDF2 visibly.
- Use the user's phrase "approximately five days" rather than presenting the
  duration as a measured engineering benchmark.
- Keep body text large enough for video and avoid dense prose.

## Source material

- Local release screenshots under `docs/images/`.
- The user-provided comparison and original-engine problem statement.
- Upstream OpenJKDF2 README and repository.
- OpenJKDF2 AMD Enhanced README, release notes, test evidence, and release 1.0.

## Verification

- Render every slide to an image and inspect it at full size.
- Run slide-overflow and overlap checks.
- Export the PowerPoint to PDF, render every PDF page, and inspect the PDF
  montage and individual pages.
- Confirm there are no clipped headings, broken images, spelling underlines,
  placeholders, unsupported glyphs, or unreadably small text.

