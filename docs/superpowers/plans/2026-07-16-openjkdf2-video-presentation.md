# OpenJKDF2 Video Presentation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce a video-ready Jedi Knight-inspired PDF presentation and matching editable PowerPoint that accurately explains the work completed in OpenJKDF2 AMD Enhanced.

**Architecture:** A plain JavaScript ES module in the external presentation scratch workspace will build an eight-slide 1280x720 deck with `@oai/artifact-tool`. The generator embeds the four release screenshots, uses reusable text/image/frame helpers, exports PPTX plus slide PNGs and layout JSON, and then LibreOffice converts the verified PPTX to PDF for independent Poppler inspection.

**Tech Stack:** `@oai/artifact-tool`, bundled Node.js runtime, LibreOffice headless conversion, Poppler `pdfinfo`/`pdftoppm`, presentation container QA scripts.

## Global Constraints

- Use the approved Jedi Knight menu-inspired art direction: dark stone surfaces, burnt-orange gothic headings, yellow energy accents, black panels, and real release screenshots.
- Keep the deck at 1280x720 and approximately eight slides.
- Use at least 50pt-equivalent deck titles, 35pt-equivalent slide titles, 24pt-equivalent callout headings, and 16pt-equivalent body text.
- State that upstream OpenJKDF2 provides the modern cross-platform engine foundation.
- Do not claim this fork uniquely made JKDF2 playable on AMD hardware.
- Describe the RX 7900 XTX as the tested configuration, not universal AMD coverage.
- State that the release contains no original game assets and requires a legally owned Steam or GOG installation.
- Include the user-provided phrase "approximately five days of Codex-assisted engineering."
- Deliver both PDF and editable PowerPoint.

---

### Task 1: Author the artifact-tool presentation

**Files:**
- Create in external scratch: `tmp/create-openjkdf2-presentation.mjs`
- Create in external scratch: `tmp/source-notes.txt`
- Read: `docs/images/amd-enhanced-gameplay.png`
- Read: `docs/images/amd-enhanced-modern-controls.png`
- Read: `docs/images/amd-enhanced-display-options.png`
- Read: `docs/images/amd-enhanced-diagnostics.png`

**Interfaces:**
- Consumes: four PNG paths, final PPTX path, scratch preview/layout paths.
- Produces: `main(): Promise<void>` that writes eight slide PNGs, eight layout JSON files, one montage, and the final PPTX.

- [ ] **Step 1: Initialize the external artifact-tool workspace**

Run with `HOME=C:\Users\georg` so the setup helper resolves the bundled runtime:

```powershell
$env:HOME = 'C:\Users\georg'
& $node $setupScript --workspace $tmpDir
```

Expected: `$tmpDir\node_modules\@oai\artifact-tool\package.json` exists and reports version 2.7.3 or newer.

- [ ] **Step 2: Define the fixed theme and helper interfaces**

Implement these exact helpers in `create-openjkdf2-presentation.mjs`:

```js
const COLORS = {
  black: "#080808", stone: "#17191D", stone2: "#252932",
  orange: "#D87512", orangeBright: "#F39A24", energy: "#FFE36A",
  cream: "#F4E7C5", muted: "#B7B2A7", line: "#6A4A1B",
};
const TITLE_FONT = "Copperplate Gothic Bold";
const BODY_FONT = "Bahnschrift";
let objectCounter = 0;
const nextName = (prefix) => `${prefix}-${String(++objectCounter).padStart(3, "0")}`;

function addText(slide, value, position, style = {}) {
  const box = slide.shapes.add({
    geometry: "textbox",
    name: nextName(style.name || "text"),
    position,
    fill: "none",
    line: { style: "solid", fill: "none", width: 0 },
  });
  box.text = value;
  box.text.style = {
    fontSize: style.fontSize || 24,
    typeface: style.typeface || BODY_FONT,
    color: style.color || COLORS.cream,
    bold: Boolean(style.bold),
    alignment: style.alignment || "left",
    verticalAlignment: style.verticalAlignment || "top",
    autoFit: style.autoFit || "shrinkText",
  };
  return box;
}

function addPanel(slide, position, fill = COLORS.stone2, line = COLORS.line) {
  return slide.shapes.add({
    geometry: "rect",
    name: nextName("panel"),
    position,
    fill,
    line: { style: "solid", fill: line, width: 2 },
  });
}

function addImage(slide, bytes, alt, position, fit = "cover") {
  return slide.images.add({
    blob: bytes,
    contentType: "image/png",
    name: nextName("image"),
    alt,
    fit,
    position,
    geometry: "rect",
  });
}

function addFooter(slide, page, source = "") {
  if (source) {
    addText(slide, source, { left: 42, top: 680, width: 1040, height: 20 }, {
      fontSize: 12, color: COLORS.muted, autoFit: "none", name: "source",
    });
  }
  addText(slide, String(page).padStart(2, "0"), { left: 1178, top: 676, width: 60, height: 24 }, {
    fontSize: 14, color: COLORS.orangeBright, alignment: "right", autoFit: "none", name: "page",
  });
}
```

Each helper must assign a stable `name`, use exact pixel positions, and keep all objects within the 1280x720 canvas.

- [ ] **Step 3: Build the eight approved slides**

Create the slides in this exact order:

1. Cover with gameplay screenshot and "approximately five days of Codex-assisted engineering."
2. Original 1997 graphics assumptions with an explicit note that upstream OpenJKDF2 performs the SDL2/OpenAL/OpenGL 3.3 modernization.
3. Full upstream-versus-fork comparison table using all eight supplied comparison rows.
4. Modern controls with the real Modern/Classic screenshot and key mappings.
5. Borderless display behavior with the real display-options screenshot and default/choice/safety callouts.
6. Renderer visibility with the diagnostics screenshot and validation/fallback/logging/safe-mode explanation.
7. Release engineering with `92`, `4.4 MB`, and `SHA-256` evidence callouts plus package contents.
8. Closing outcome: "OpenJKDF2, ready to install - not just ready to compile," followed by the portfolio and GitHub URLs.

Do not add an agenda slide, invented benchmark, AMD-only code claim, or copyrighted game asset beyond the supplied technical screenshots.

- [ ] **Step 4: Export presentation evidence**

For each slide, export a PNG at scale 1 and layout JSON. Export the montage as WebP and the deck with:

```js
const pptx = await PresentationFile.exportPptx(presentation);
await pptx.save(FINAL_PPTX);
```

Expected: eight PNGs, eight layout JSON files, one montage, and a non-empty PPTX.

### Task 2: Verify and refine the PowerPoint

**Files:**
- Test: external scratch `tmp/slides/*.png`
- Test: external scratch `tmp/layout/*.json`
- Test: external scratch `tmp/qa/pptx-overflow.txt`
- Test: external scratch `tmp/qa/pptx-content.txt`
- Produce: `output/presentations/OpenJKDF2-AMD-Enhanced-Video-Presentation.pptx`

**Interfaces:**
- Consumes: generated PPTX, slide PNGs, and layout JSON.
- Produces: visually approved PPTX with zero unintended overlap or overflow defects.

- [ ] **Step 1: Run automated slide bounds checks**

```powershell
& $python $slidesTest $finalPptx | Tee-Object $qaDir\pptx-overflow.txt
```

Expected: exit code 0 and no content outside the original slide canvas.

- [ ] **Step 2: Inspect the deck montage and every slide at full size**

Review all eight rendered PNGs for title wrapping, clipped text, image distortion, unsupported glyphs, low contrast, and accidental overlaps. Treat the comparison table as the highest-density slide and keep its body type at or above the approved minimum.

- [ ] **Step 3: Verify presentation copy**

Extract or inspect the PPTX and confirm the presence of these exact strings:

```text
approximately five days of Codex-assisted engineering
Upstream OpenJKDF2
Modern FPS controls
Borderless Fullscreen
RX 7900 XTX
92
SHA-256
No original game assets
```

Expected: every required phrase appears and no `Lorem ipsum`, `TBD`, or `TODO` remains.

- [ ] **Step 4: Regenerate after any correction**

Modify only the scratch `.mjs`, rerun it, rerun the bounds check, and repeat full-size inspection until all eight slides pass.

### Task 3: Export and verify the PDF

**Files:**
- Produce: `output/pdf/OpenJKDF2-AMD-Enhanced-Video-Presentation.pdf`
- Test: external scratch `tmp/qa/pdf-pages/*.png`
- Test: external scratch `tmp/qa/pdfinfo.txt`
- Test: external scratch `tmp/qa/pdf-text.txt`

**Interfaces:**
- Consumes: verified PPTX.
- Produces: eight-page, 16:9 PDF that visually matches the approved PowerPoint.

- [ ] **Step 1: Convert the PPTX with LibreOffice**

```powershell
& 'C:\Program Files\LibreOffice\program\soffice.exe' --headless --convert-to pdf --outdir $pdfOutputDir $finalPptx
```

Expected: exit code 0 and a non-empty PDF.

- [ ] **Step 2: Verify PDF structure and text**

```powershell
pdfinfo $finalPdf | Tee-Object $qaDir\pdfinfo.txt
pdftotext $finalPdf $qaDir\pdf-text.txt
```

Expected: `Pages: 8`, landscape 16:9 page geometry, and every required phrase from Task 2.

- [ ] **Step 3: Render every PDF page and inspect it**

```powershell
pdftoppm -png -r 150 $finalPdf $qaDir\pdf-pages\page
```

Review the PDF montage and every page at full size. Expected: no clipping, font substitution damage, black boxes, broken images, or layout drift.

- [ ] **Step 4: Record final verification and commit deliverables**

Run `git diff --check`, inspect `git status --short`, stage only the approved plan and final presentation outputs, and commit:

```powershell
git add docs/superpowers/plans/2026-07-16-openjkdf2-video-presentation.md output/presentations/OpenJKDF2-AMD-Enhanced-Video-Presentation.pptx output/pdf/OpenJKDF2-AMD-Enhanced-Video-Presentation.pdf
git commit -m "docs: add OpenJKDF2 video presentation"
```

Expected: one commit containing the plan, PPTX, and PDF, with scratch and QA artifacts remaining outside the repository.
