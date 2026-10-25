# Phase 4 — UI Review

**Audited:** 2026-10-25
**Baseline:** Abstract 6-pillar standards (no UI-SPEC.md exists). The phase's own contract is 04-CONTEXT.md (D-00 to D-09, the "rule for every screen buffer").
**Screenshots:** not captured in this run (no dev server; this is a native Qt app and no browser or Playwright was used). Visual evidence is the recorded grabs in `records/live/` from the Studio Display run of plans 04-02 and 04-07, plus code and test-artifact review.
**Interaction captures:** off (`interaction_capture: false`)

---

## Pillar Scores

| Pillar | Score | Key Finding |
|--------|-------|-------------|
| 1. Copywriting | 3/4 | Quality tooltip is the only changed string; it names the user's own hardware ("twice on the Studio Display") |
| 2. Visuals | 3/4 | Recorded grabs are sharp (page, thumbnail, badge, handles, mask); trash, unremove and deskew-handle icons have no live grab |
| 3. Color | 3/4 | No new colour tokens; picker keeps its semantics and passes a colour check; not a colour-focused phase |
| 4. Typography | 3/4 | Phase did not touch type; scored on existing surfaces only |
| 5. Spacing | 3/4 | Overlay placement measured (handle offsets 0.000 logical px, stage ring centre 0.707 px, unremove 256x256 device block) |
| 6. Experience Design | 2/4 | Three icon pixmaps are built once and never rebuilt on DPR change (HIDPI-05 gap, code-derived) |

**Overall: 17/24**

---

## Top 3 Priority Fixes

1. **Three icon pixmaps keep the DPR they were built at** — on a window moved between a 2x and a 1x display they stay at the old ratio, so they are blurry (1x) or double-sized (2x) until the view is recreated — user impact: the trash, unremove and deskew handle icons look soft after a display move, which violates HIDPI-05 and the phase's "rule for every screen buffer" — concrete fix: rebuild them from the icon when `devicePixelRatioF()` changes (`UnremoveButton` handles `QEvent::DevicePixelRatioChange`, or build the pixmap in `paintEvent` via `QIcon::pixmap(size, dpr)` with a per-DPR cache). Sites: `src/core/filters/page_split/UnremoveButton.cpp:17-18`, `src/core/filters/page_split/Thumbnail.cpp:26` (`m_trashPixmap`), `src/core/filters/deskew/ImageView.cpp:27` (`m_handlePixmap`).
2. **Trash and unremove icons have no live measurement** — `records/live-overlays.md` lists handles, badges, masks, stage frames and decorations but not the page-split trash or unremove icons, and `testIconPixmapLogicalGeometry` checks geometry only — user impact: nothing in the record shows these icons are sharp on the Studio Display, so the "sharp overlays" success criterion is only partly evidenced — concrete fix: add a `retina_check` section that grabs `UnremoveButton` and the page-split `Thumbnail` trash icon at DPR 2 and applies the same device-detail measure used for the handles.
3. **Settings tooltip names the author's hardware** — `src/app/SettingsDialog.ui:110` reads "...multiplied by the display's pixel ratio, twice on the Studio Display." Other users will see a claim about a display they may not have, and the number is already implied by "pixel ratio" — user impact: misleading copy in a shipped dialog — concrete fix: change to "...multiplied by the display's pixel ratio (2 on Retina displays). The default value is 250." and keep the wording in D-06's "multiplied on Retina" phrasing.

---

## Detailed Findings

### Pillar 1: Copywriting (3/4)

- Changed user-facing text is the Quality tooltip only (`src/app/SettingsDialog.ui:110`). It states the default (250) correctly and says the value is multiplied by the display's pixel ratio, which matches D-06.
- **WARNING:** the tooltip's final clause ("twice on the Studio Display") is hardware-specific. The phase's logic is `devicePixelRatioF()`, not a fixed 2, and D-05 says "never a hard-coded 2". The copy should describe the rule, not one machine.
- The Size tooltip (`SettingsDialog.ui:153`) is unchanged and fine.
- Pre-existing, not this phase: "sensivity" (sic) appears three times in the deviation tooltips (`SettingsDialog.ui:333, 356, 404`). Noted only; not scored against this phase.
- The phase added no empty, error or CTA copy. Those are out of scope for this phase and were not audited further.

### Pillar 2: Visuals (3/4)

Measured on the real display (records, `retina_check` run on Cocoa, screen 2560x1440 logical, ratio 2):

- Page view at fit, 100% and 200%: HQ ratio 2, equal fraction 1.000, max channel difference 0 (`records/live-page-view.md`). The crops `records/live/page-100-grab.png` show crisp device-pixel glyph edges.
- Thumbnail: ratio 2, file `q500`, detail 0.1824 (`records/live/thumb-grab.png` is a sharp, uniform grab of the page content).
- Page badge 46x34 device pixels, 99%+ match with the 1:1 reference (`badge-grab.png`).
- Handles: 4 blobs, offsets 0.000 logical px (the pre-phase bug of an 8 px offset is gone) (`handles-grab.png`).
- Zone mask: 747x560 device px, detail 0.3844 (`mask-grab.png`).

Gaps:
- **WARNING:** the trash icon (`page_split/Thumbnail.cpp:26`, 64x64) and the unremove button (`UnremoveButton.cpp:17-18`, 128x128) are not in any live grab. The unremove button has a pixel test on a synthetic icon only; the trash icon has no pixel test at all (04-06 SUMMARY states this).
- The live thumbnail and overlay checks are one-display runs. The `DevicePixelRatioChange` delivery to the child `thumbView` on a real display drag was not observed (records, "What this does not show").

### Pillar 3: Color (3/4)

- The phase introduced no new colour values. The only colour literal in the touched output-view code is the mask fill `QColor(maskColor)` (`PictureZoneEditor.cpp:221`), which comes from existing settings.
- The colour picker keeps its semantics (median of the circle as drawn). The live check picked `#dc781e` from a `#dc781e` patch at ratio 2 (`records/live-overlays.md`, colour picker section). Picked colour values in existing projects are unchanged, as D-00 requires.
- Not scored lower because no colour accent or 60/30/10 surface was changed by this phase. This is a scope note, not a colour audit of the whole app.

### Pillar 4: Typography (3/4)

- The phase did not change any font size, weight or family. The thumbnail's rendered text in `thumb-grab.png` is sharp at device resolution, which is the only typographic change the phase produced (text is now rasterised at 2x in thumbnails).
- Not audited beyond the touched surfaces. Score is carried from the existing surfaces, not a font-scale audit.

### Pillar 5: Spacing (3/4)

- Overlay positions are measured against logical-pixel targets:
  - Handles: horizontal and vertical offsets 0.000 logical px (`ImageView.cpp:146-150` draws at `logicalRect`).
  - Stage-list ring: centre offset 0.707 logical px against a 1.5 px threshold (`records/live-overlays.md`).
  - Unremove button: 256x256 device block centred on its anchor, hover within 64 logical px (`UnremoveButton.cpp:35`, `tst_hidpi_qt.cpp`).
- The stage-list animation frame is a 13x13 logical box at ratio 2 (`StageListView.cpp:294`).
- Not 4/4: the 0.707 px stage offset is only measured as a union over 12 grabs, not a single frame (records note this), so the centring is a measurement-recipe result rather than a single-frame check.

### Pillar 6: Experience Design (2/4)

- **WARNING (HIDPI-05 gap):** the pixmaps `m_defaultPixmap`/`m_hoveredPixmap` (`UnremoveButton.cpp:17-18`), `m_trashPixmap` (`page_split/Thumbnail.cpp:26`) and `m_handlePixmap` (`deskew/ImageView.cpp:27`) are built once in the constructor from `QIcon::pixmap(w, h)`. No DPR-change handler exists for them: `DevicePixelRatioChange` is handled only in `ImageViewBase`, `StageListView`, `ThumbnailDprTracker` and the test file (`grep`). When a view moves between a 2x and a 1x screen these icons keep the old ratio. This is code-derived, not measured; the offscreen tests do not move the screen for these widgets.
- **WARNING:** `ImageViewBase::event` (`ImageViewBase.cpp:411-424`) and `validateHqPixmap` (`:946`) are correct and idempotent; the defect is limited to the three icon sites above.
- Positive: the HQ pixmap, thumbnail cache, zone mask and picker paths all respond to DPR changes (permanent offscreen tests `testThumbnailsFollowScreenChange`, `testZoneMaskAtDeviceResolution`, and the picker test).
- Not applicable (phase did not add): loading, empty and error states, confirmation for destructive actions.
- Evidence limit: the only live DPR move was simulated offscreen (CONTEXT D-03); the real Cocoa display move is not observed on this one-display Mac. That is recorded in `records/live-overlays.md` and is a limit of the measurement, not a UI defect.

---

## Files Audited

- `.planning/phases/04-retina-rendering/04-CONTEXT.md`
- `.planning/phases/04-retina-rendering/04-01-SUMMARY.md` through `04-08-SUMMARY.md` (key-files and coverage sections)
- `.planning/phases/04-retina-rendering/records/live-page-view.md`, `records/live-overlays.md`
- `.planning/phases/04-retina-rendering/records/live/page-100-grab.png`, `records/live/thumb-grab.png` (visually inspected)
- `src/app/SettingsDialog.ui` (Quality tooltip, Size tooltip, deviation tooltips)
- `src/core/ApplicationSettings.cpp` (Quality default 250, line 25)
- `src/core/ImageViewBase.cpp` (DPR change handling 411-424, smoothing note 425-430, HQ pixmap 946-1000)
- `src/core/filters/page_split/UnremoveButton.cpp`, `src/core/filters/page_split/Thumbnail.cpp`, `src/core/filters/deskew/ImageView.cpp` (icon pixmap construction)
- `src/core/filters/output/PictureZoneEditor.cpp`, `src/core/filters/output/ColorPickupInteraction.cpp`, `src/core/StageListView.cpp`, `src/core/ThumbnailBase.cpp`, `src/core/zones/ZoneContextMenuInteraction.cpp`, `src/core/HiDpi.cpp` (grep for raster sites and DPR use)
- Not read: `src/qt_tests/tst_hidpi_qt.cpp` beyond grep hits, `src/app/retina_check/main.cpp`. Test results are taken from the SUMMARY and records, not re-run (no build or ctest was run in this audit).
