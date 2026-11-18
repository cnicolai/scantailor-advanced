---
phase: 04-retina-rendering
plan: 06
subsystem: ui
tags: [qt6, hidpi, retina, qpixmap, devicePixelRatio, icons, color-picker, qt-test, offscreen]

requires:
  - phase: 04-retina-rendering
    provides: "hidpi helpers (logicalSize, logicalRect, makePixmap, sanitizedRatio), TwoX/OneX offscreen screens, placeOn/waitForHq test helpers (plan 04-01)"
provides:
  - "Handles, trash icon and unremove button centred and hit-tested by logical pixmap size on Retina"
  - "Zone context-menu swatch built at the image view's device pixel ratio"
  - "ColorPickupInteraction::medianColor: pure median over the circle counted in logical pixels of a device-resolution grab"
  - "Four permanent TestHiDpiQt tests: icon geometry, page badge at device resolution, median invariance, real grab at ratio 2"
affects: [04-07, 04-08]

plan_head_before: ff528eb2f8a7e004f5427f7a64f23e82baf0c583
plan_head_after: 1aa4bab9bc061c73a24228a17bb3869fcc5e73af

actuals:
  tokens: 4600
  tasks: 2
  commits: 4

tech-stack:
  added: []
  patterns:
    - "Icon pixmaps are measured by hidpi::logicalSize/logicalRect wherever a view centres or hit-tests them; drawPixmap calls stay unchanged"
    - "Scratch icon pack in a QTemporaryDir (styled badge with a diagonal edge) to test the real SvgStyledIconEngine without linking the icon resources"
    - "Scope guard that resets the process-wide IconProvider pack, also on test failure"
    - "Median extracted into a static pure function so the sampling path is testable without driving the cursor"

key-files:
  created: []
  modified:
    - src/core/filters/deskew/ImageView.cpp
    - src/core/filters/page_split/ImageView.cpp
    - src/core/filters/page_split/Thumbnail.cpp
    - src/core/filters/page_split/UnremoveButton.cpp
    - src/core/zones/ZoneContextMenuInteraction.cpp
    - src/core/filters/output/ColorPickupInteraction.h
    - src/core/filters/output/ColorPickupInteraction.cpp
    - src/qt_tests/tst_hidpi_qt.h
    - src/qt_tests/tst_hidpi_qt.cpp
    - .planning/phases/04-retina-rendering/records/gate-log.md

key-decisions:
  - "src/app/ThumbnailSequence.cpp stays unchanged: page badges already come out of QIcon::pixmap at the screen ratio with a 23x17 logical bounding rect; a permanent test on the real styled icon engine proves it instead of taking it on trust"
  - "The colour picker keeps its semantics (median of the circle as drawn on screen): the circle predicate runs on logical pixel indices, the bit-mixing median is moved unchanged, the OpenGL branch is untouched"

patterns-established:
  - "Counterfactual runs: each new pixel/geometry test was run once against deliberately broken production code and had to fail"

requirements-completed: [HIDPI-04, HIDPI-06]

coverage:
  - id: D1
    description: "Deskew and page-split handles, the page-split trash icon and both unremove-button pixmaps are centred and hit-tested by logical size; the unremove button draws a 256x256 device-pixel block centred on its anchor and hovers within 64 logical pixels only"
    requirement: HIDPI-04
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testIconPixmapLogicalGeometry"
        status: pass
    human_judgment: true
    rationale: "The unremove button is pinned by pixel test; the deskew and page-split handle centring and the trash icon placement are edited by the same rule but have no pixel test of their own (checked by grep that no device-size read remains) and are judged in the live check of plan 04-07"
  - id: D2
    description: "A page badge built as ThumbnailSequence builds it has the view's device pixel ratio, 46x34 pixels, a 23x17 logical bounding rect, matches the 1:1 device reference in the view grab, and a ratio-1 badge is rejected by the same comparison"
    requirement: HIDPI-04
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testPageBadgeAtDeviceResolution"
        status: pass
    human_judgment: false
  - id: D3
    description: "The zone context-menu swatch is a 16x16-logical pixmap at the image view's ratio"
    requirement: HIDPI-04
    verification:
      - kind: other
        ref: "grep hidpi::makePixmap(QSize(16, 16), context.imageView().devicePixelRatioF()) in ZoneContextMenuInteraction.cpp"
        status: pass
    human_judgment: true
    rationale: "No test builds a context menu; the swatch sharpness in the real menu is judged in the live check of plan 04-07"
  - id: D4
    description: "medianColor gives the same colour for a ratio-1 grab and its 2x replication, the majority colour for a quadrant difference, the quadrant colour for the misread device-pixel crop, and an invalid colour for a null image"
    requirement: HIDPI-06
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testMedianColorRatioInvariant"
        status: pass
    human_judgment: false
  - id: D5
    description: "A real 15x15-logical grab of a page view is 30x30 device pixels with ratio 2 on TwoX and 15x15 on OneX, and medianColor returns the patch colour on both"
    requirement: HIDPI-06
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testColorPickerGrabAtRatio2"
        status: pass
    human_judgment: true
    rationale: "The cursor-to-rect mapping (mapFromGlobal, logical) is unchanged and not driven in the test; picking on the real display is part of the live check of plan 04-07"
  - id: D6
    description: "Both commits pass gate.sh: Release and Debug 100% tests passed out of 7, goldens unchanged, empty command diff"
    requirement: HIDPI-04
    verification:
      - kind: other
        ref: "bash .planning/phases/03-qt-6-and-macos-only/gate.sh check r10-icon-geometry / r11-color-picker"
        status: pass
    human_judgment: false

duration: 19min
completed: 2026-10-25
status: complete
---

# Phase 4 Plan 6: Icon geometry and colour picker sampling on Retina Summary

**Handles, trash icon and unremove button are centred and hit-tested by logical pixmap size, the zone swatch is built at the view's ratio, and the colour picker's median is a pure function over logical pixels of a device-resolution grab; four permanent TestHiDpiQt tests, including a proof that page badges already render at device resolution on the real styled icon engine**

## Performance

- **Duration:** 19 min
- **Started:** 2026-10-25T14:49:52Z
- **Completed:** 2026-10-25T15:09Z
- **Tasks:** 2 (both auto)
- **Files modified:** 10 (9 under src plus the gate log)

## Accomplishments

- 11 device-size reads of icon pixmaps (8 handle reads in the two image views, 1 trash, 2 unremove) are now `hidpi::logicalSize` / `hidpi::logicalRect`. The pixmaps themselves and the `drawPixmap` calls are unchanged: the icons were already sharp because `QIcon::pixmap` returns ratio-2 pixmaps, only their placement and hit area were wrong (8 logical pixels off for handles, 32 for the trash icon, 64 for the unremove button with a 256-logical hit area).
- `page_split::UnremoveButton` painted into a ratio-2 image draws a 256x256 device-pixel block centred within 1 device pixel of the anchor; a proximity update 100 logical pixels away does not mark it hovered, one 40 pixels away does (T-04-19).
- Page badges: `ThumbnailSequence.cpp` is unchanged since `03083f55`. `testPageBadgeAtDeviceResolution` repeats its four badge lines with the real `SvgStyledIconEngine` and a scratch styled icon with a diagonal edge. On `TwoX` both pixmaps have ratio 2 and 46x34 pixels, the item rect is 23x17 logical, the view grab matches the 1:1 device reference (at least 99 percent of pixels within 2 levels), and a ratio-1 badge upscaled 2x is rejected by the same comparison.
- `ColorPickupInteraction::medianColor(const QImage&)`: a device pixel takes part when its logical pixel lies in the circle of the old algorithm, so at ratio 1 the pixel set is identical and at ratio 2 each logical pixel contributes its 2x2 block. The bit-mixing, `nth_element` at `size() / 2` and unmixing code moved unchanged; `takeColor` keeps the grab and the OpenGL `sampleImageAtWidgetRect` branch and returns early on an invalid colour.
- Counterfactuals (production code deliberately broken, test run, code restored): the unremove button with `pixmap.rect()` fails the centring check (centred at 172,172 instead of 300,300); `medianColor` with `dpr = 1.0` fails the noise invariance check.

## Task Commits

1. **Task 1: place icon pixmaps by their logical size** - `3064968e` (fix); gate `r10-icon-geometry` PASS (empty command diff), gate record `89e11cac`
2. **Task 2: sample the colour picker by logical pixels** - `6dac9b1c` (feat); gate `r11-color-picker` PASS (empty command diff), gate record `1aa4bab9`

**Plan metadata:** the commits that add this summary and update state and roadmap.

## Files Created/Modified

- `src/core/filters/deskew/ImageView.cpp`, `src/core/filters/page_split/ImageView.cpp` - handle hit radius, centring rect and content-area margins by logical size
- `src/core/filters/page_split/Thumbnail.cpp`, `UnremoveButton.cpp` - trash icon and unremove button rects by logical size
- `src/core/zones/ZoneContextMenuInteraction.cpp` - swatch pixmap via `hidpi::makePixmap` at the view's ratio
- `src/core/filters/output/ColorPickupInteraction.h`, `.cpp` - `medianColor`, `takeColor` hands the device-resolution image to it
- `src/qt_tests/tst_hidpi_qt.h`, `.cpp` - four new tests, `IconPackReset` guard, scratch icon sheet writer, `replicate` helper
- `.planning/phases/04-retina-rendering/records/gate-log.md` - rows `r10-icon-geometry` and `r11-color-picker`

## Decisions Made

- `ThumbnailSequence.cpp` stays as is (badges are proven, not changed); building the badge at the view's own ratio would need a label-group rebuild on every display change, and a list that starts on a 1x screen would then keep ratio-1 badges on Retina, which is worse than the current `qApp->devicePixelRatio()` behaviour.
- The test replicates pixels itself (`replicate`) instead of `QImage::scaled(FastTransformation)`, so the 2x upscale is exact by construction.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- None. Both new tests of Task 1 and both of Task 2 passed on the first run. Uncommitted `.planning/config.json`, `.planning/milestone.lock` and `.planning/state.json` predate this plan and were left out of every commit.

## Known Stubs

None.

## Threat Flags

None. No new network, auth, file-access or schema surface. T-04-19 mitigated (hover inside 64 px, not at 100 px, tested). T-04-20: at ratio 1 the logical-pixel predicate is the old one and the median code is moved unchanged; `git diff a796c2ae -- src/pipeline_tests/golden` is empty and `output::Task`, `OutputGenerator`, `imageproc`, `dewarping` are untouched. T-04-21: both icon tests reset the pack through the `IconPackReset` scope guard; the temporary icon files live in a `QTemporaryDir` below `QDir::tempPath()`, which is the build tree's `test-tmp` under ctest.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 04-07 (live check) should look at: handle and cutter placement in the deskew and page-split views, the trash icon in split thumbnails, the unremove button size and hit area, the zone context-menu swatches, and a real colour pick on the display.
- The sampling test that Phase 6 (STRIP-07) relies on is in place: `ColorPickupInteraction::medianColor` is independent of the OpenGL branch, which Phase 6 removes.

## Self-Check: PASSED

- Modified files exist: the nine source files listed above and `records/gate-log.md` (checked below).
- Commits `3064968e`, `89e11cac`, `6dac9b1c`, `1aa4bab9` are ancestors of HEAD.
- Both gate summaries read `result: PASS`, `goldens: unchanged` and `diff_lines=0` for both presets; `qt_tests` reports `0 failed, 0 skipped` for all three classes (26 tests in `TestHiDpiQt`); the four new tests have PASS lines.
