# Phase 06 — UI Review

**Audited:** 2026-11-17
**Baseline:** Abstract 6-pillar standards (no UI-SPEC.md for this phase), scoped to the Phase 6 UI changes listed in the audit brief
**Screenshots:** not captured (no dev server; this is a Qt Widgets desktop app. Offscreen render was not run, so all visual findings are code-derived and unverified in rendered form)
**Interaction captures:** off (`workflow.ui_interaction_capture` is false)

---

## Pillar Scores

| Pillar | Score | Key Finding |
|--------|-------|-------------|
| 1. Copywriting | 3/4 | The limit text states "537 megapixels", but the real cap is 536,870,911 px (2^29 − 1). Two Settings strings are US spellings in a dialog this phase edited. |
| 2. Visuals | 3/4 | An oversized page shows its specific "too large" message only in the page view. Thumbnails show the generic failed state with no reason. |
| 3. Color | 3/4 | Phase 6 adds no colour. Too-large files reuse the failed-file red with no distinct cue beyond the text. |
| 4. Typography | 3/4 | No type changes in scope. New messages use the existing ErrorWidget and QMessageBox. Nothing was rendered to confirm line wrapping in the error label. |
| 5. Spacing | 3/4 | Removing the OpenGL and Language rows left no orphan spacer in the code. The fixed dialog geometry (337 × 442) was not re-checked against the shorter content. |
| 6. Experience Design | 3/4 | Refusal, the cap reset and the cancelled-save guard are all implemented. The save-failure message gives no reason and no path, and a refused thumbnail gives no reason. |

**Overall: 18/24**

No pillar scores 1, so there are no BLOCKERs. Every finding below is a WARNING.

---

## Top 3 Priority Fixes

1. **The "limit" copy overstates the cap by about 0.1%.** `ImageSizeLimit::limitText()` prints `qRound64(kMaxPixels / 1e6)`, which gives "537 megapixels". The real limit is 536,870,911 px, so a file of 537,000,000 px is refused while the text implies it is allowed. This text appears in the page-view error (`LoadFileTask.cpp:141`) and in the file-list box (`ProjectFilesDialog.cpp:481`). — **User impact:** a user who sizes a scan to "537 MP" will be refused without understanding why. — **Fix:** print the exact value, for example `tr("%1 px per side and %2 px in total")` with `kMaxPixels` in grouped digits, or round down with `qFloor` and say "536 megapixels". Update the header comment at `ImageSizeLimit.h:52-53`, which says "65535 px per side and 537 megapixels in total".

2. **A refused thumbnail gives no reason.** `ThumbnailPixmapCache.cpp:695-698` returns a null `QImage` when `ImageLoader::loadSource` refuses the file, and the thumbnail then shows the generic failed state. Only the page view names the size (`LoadFileTask::ErrorResult`). — **User impact:** in the thumbnail strip a user sees a broken tile and cannot tell whether the file is corrupt or simply above the limit. — **Fix:** when the thumbnail load fails with an oversized source, set the thumbnail's tooltip to `ImageSizeLimit::tooLargeText(claimedSize)`, using `ImageSizeLimit::claimedSizeIfTooLarge`. That reuses the existing copy and needs no new string.

3. **The unified save-failure message is generic.** `ProjectSaveFlow::showSaveError` shows only "Error saving the project file!" (`ProjectSaveFlow.cpp`), with no file path and no cause. This one message is now shared by the main Save, the Save As flow and the Out-of-Memory dialog, so the gap grows with each use. — **User impact:** after a failed save while closing (WR-06) the user is stuck with a message that does not say whether the folder is read-only, the disk is full or the name is invalid. — **Fix:** append the target path, for example `tr("Error saving the project file:\n%1").arg(projectFile)`, and pass the path into `showSaveError`. Keep the title "Error".

---

## Detailed Findings

### Pillar 1: Copywriting (3/4)

Scope strings checked: `LoadFileTask.cpp:141` ("The following file is too large to load: … Its image size is … The limit is …"), `ImageSizeLimit.cpp:23-31` (`sizeText`, `limitText`, `tooLargeText`, `oversizedPagesText`), `ProjectFilesDialog.cpp:481`, `SystemLoadWidget.cpp:58` (", limited to %1 by memory"), `ProjectSaveFlow.cpp` (`showSaveError`), `SettingsDialog.ui` tooltips.

- **Numeric inaccuracy (WARNING):** `ImageSizeLimit.cpp:26-28` prints `qRound64(kMaxPixels / 1e6)`. That is `round(536.87)` = "537 megapixels". See Priority Fix 1.
- **Unit notation is inconsistent (WARNING):** `ImageSizeLimit::sizeText` uses the multiplication sign "×" (U+00D7) in "%1 × %2 px". `FixDpiDialog.cpp:720` uses the ASCII "x" for the same kind of size ("%1 x %2 px"). The header comment at `ImageSizeLimit.h:62` says "W x H", so the code and its own documentation disagree. Pick one. The brief asks for consistency with existing wording, which means "x".
- **US spellings in a dialog this phase edited (WARNING):** `SettingsDialog.ui:48` "Color Scheme:", `:205` "Color Compression:" and `:177` "Show question on canceling multi page selection". The brief asks for British English. These strings predate Phase 6, but the phase edited this dialog, so they are in scope to mention. Fixing them is a copy change only and does not need a translation update (D-00d).
- **Hardware-specific tooltip (WARNING):** `SettingsDialog.ui` tooltip on `thumbnailQualitySB` reads "…On a Retina display the image is built at this size multiplied by the display's pixel ratio, twice on the Studio Display…". This is a tooltip, not a placeholder. It names one user's monitor and gives a rule that does not describe how the setting behaves for other users. Shorten it to the behaviour, for example "On a high-DPI display the image is built at this size multiplied by the pixel ratio."
- **Save-failure text has no cause or path (WARNING):** see Priority Fix 3.
- **Positive:** the new messages carry the specific size and the limit, the box titles stay "Error" (matching the rest of the app), and the new text contains no contractions.

### Pillar 2: Visuals (3/4)

- **Thumbnail refusal is silent (WARNING):** see Priority Fix 2. The page view shows a clear message, but the thumbnail strip, which is where most users first see a bad file, shows only the generic failed state.
- **Focal point of the page-view error is clear (positive):** `ErrWidget` puts the message in the image area with `Qt::PlainText`, so the path and the limit are shown as plain text with no markup risk.
- **Settings dialog (not verified rendered):** the OpenGL checkbox and device label and the Language row were removed. The "User Interface" group now holds only the auto-save checkbox and the colour-scheme row. The hierarchy is intact in the source.

### Pillar 3: Color (3/4)

- Phase 6 introduces no new colour and no new accent. The too-large entries in `ProjectFilesDialog` use the same red as failed files (`ProjectFilesDialog.cpp:546-557`).
- **Colour-only distinction is avoided, but the distinction is text-only (WARNING, minor):** a too-large entry and a failed entry look the same apart from the text "(too large: …)". That is acceptable, since the text carries the meaning, but no icon or weight differs. Consider a distinct marker if the list grows.
- No hardcoded colour values were added in the scoped files (`grep` for `#` and `rgb(` in the scoped files found none).

### Pillar 4: Typography (3/4)

- No font size, weight or family changes are in scope. The phase reuses `ErrorWidget` and `QMessageBox`, so the typography follows the existing app.
- **Not verified in rendered form (WARNING, minor):** the page-view error is a multi-line string with `\n` and a long path (`LoadFileTask.cpp:141`). Long paths in `ErrorWidget` may wrap poorly. This was not rendered, so it is a risk, not a confirmed defect.

### Pillar 5: Spacing (3/4)

- **Settings dialog layout (code-verified, not rendered):** the diff removes `enableOpenglCb`, `openglDeviceLabel` and the whole `horizontalLayout_4` language row, and the matching `<zorder>` entries go with them. No orphan spacer remains in the "User Interface" group. Nothing in the `.ui` diff leaves an empty item.
- **Fixed geometry (WARNING, minor):** `SettingsDialog.ui` still declares `width=337, height=442`. With two rows removed, the first tab may now show extra empty space at the bottom. This cannot be confirmed without a render.
- No arbitrary spacing values were introduced in the scoped code.

### Pillar 6: Experience Design (3/4)

Implemented and sound:
- **Specific refusal in the page view:** `LoadFileTask::ErrorResult::updateUI` shows the claimed size and the limit (`LoadFileTask.cpp:139-143`). No allocation happens first (per the SAFE-01 summary and the footprint tests).
- **Project file dialog:** oversized files are listed, marked and not accepted, and the warning box carries one limit line (`ProjectFilesDialog.cpp:474-484`).
- **Cancelled or failed Save while closing (WR-06):** `closeProjectInteractive` returns `false` on a failed or cancelled save, so the project stays open (`MainWindowProject.cpp:325-326`, `349-350`).
- **Batch memory tooltip (WR-02/WR-03):** "limited to N by memory" appears only when the cap binds (`SystemLoadWidget.cpp:55-59`), and the cap resets when the batch stops or the project switches. The UAT confirmed the first half in the UI. The reset is only confirmed by code reading.

Gaps:
- **Refused thumbnail gives no reason (WARNING):** see Priority Fix 2.
- **Save failure gives no reason or path (WARNING):** see Priority Fix 3.
- **Memory tooltip is visible only during a drag (WARNING, minor):** `showHideToolTip` shows the tooltip while the user drags. A user who sets the slider and then looks away sees no reason the count is lower than the setting. The UAT covers this with a dragged check, so this is a discoverability note, not a defect.
- **Settings dialog "Save As" and OOM save share one code path (positive):** `ProjectSaveFlow::writeWithFeedback` is the single entry point, so a future fix to the save message reaches all three callers.

---

## Files Audited

- `src/core/ImageSizeLimit.h`, `src/core/ImageSizeLimit.cpp`
- `src/core/LoadFileTask.cpp`
- `src/core/ThumbnailPixmapCache.cpp`
- `src/app/ProjectFilesDialog.cpp`
- `src/app/SystemLoadWidget.cpp`
- `src/app/SettingsDialog.ui` (diff from the Phase 6 base)
- `src/app/ProjectSaveFlow.cpp`
- `src/app/OutOfMemoryDialog.cpp`
- `src/app/MainWindowProject.cpp` (`closeProjectInteractive`, `saveProjectTriggered`)
- `src/app/FixDpiDialog.cpp` (line 720, for unit-notation comparison)
- Phase 6 SUMMARY, CONTEXT, REVIEW-FIX and UAT files

Not audited: an offscreen `ui_snapshot` render (not run), any Chrome or browser tooling (not applicable to a Qt desktop app, and forbidden by the user), and any live launch of the app.
