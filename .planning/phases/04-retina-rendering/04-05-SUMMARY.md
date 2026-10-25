---
phase: 04-retina-rendering
plan: 05
subsystem: ui
tags: [qt6, hidpi, retina, qpixmap, devicePixelRatio, zone-mask, stage-list, qt-test, offscreen]

requires:
  - phase: 04-retina-rendering
    provides: "hidpi helpers, ImageViewBase DevicePixelRatioChange handling, TwoX offscreen screen and placeOn/waitForHq test helpers (plan 04-01)"
provides:
  - "PictureZoneEditor screen mask resampled at the view's device pixel ratio, logical origin, ratio kept in the animated per-frame copy, ratio checked at paint time"
  - "StageListView::createBatchAnimationFrames: batch animation frames at device resolution with an explicit logical rect, rebuilt on DevicePixelRatioChange, logical centring in RightColDelegate"
  - "Five permanent TestHiDpiQt tests for the zone mask and the stage-list animation"
affects: [04-06, 04-07, 04-08]

plan_head_before: ae25c4f3661e07abff2093cfd3a357ad453b9a1f
plan_head_after: 80becd6cf107d4c73d6fc48ae36979b9f305a911

actuals:
  tokens: 6900
  tasks: 2
  commits: 4

tech-stack:
  added: []
  patterns:
    - "Paint-time ratio comparison backs the event: validateScreenPictureMask compares the stored mask ratio with the viewport's, so a display change rebuilds the mask without a new event handler"
    - "Static frame renderer taking the ratio as a parameter, so the frame geometry is unit-testable without a window"
    - "Counterfactual runs: each new pixel test was run once against the deliberately broken code and had to fail"

key-files:
  created: []
  modified:
    - src/core/filters/output/PictureZoneEditor.h
    - src/core/filters/output/PictureZoneEditor.cpp
    - src/core/StageListView.h
    - src/core/StageListView.cpp
    - src/qt_tests/tst_hidpi_qt.h
    - src/qt_tests/tst_hidpi_qt.cpp
    - src/qt_tests/CMakeLists.txt
    - .planning/phases/04-retina-rendering/records/gate-log.md

key-decisions:
  - "qt_tests links dewarping: output/Settings.h includes <DistortionModel.h>, which only the dewarping include directory provides, and the zone-mask tests need a real output::Settings"
  - "The explicit logical rect in createBatchAnimationFrames stays (RESEARCH Pitfall 2 is real: without it the ring is centred at the frame corner), pinned by a quadrant-balance check"
  - "The picture mask in the zone tests is built from a Format_RGB32 image: BinaryImage(const QImage&) does not accept Format_Grayscale8"

patterns-established:
  - "Tint probes for overlay placement: compare a grab with the overlay against a grab of the same view with an empty mask at the point where the right and the wrong placement differ"

requirements-completed: [HIDPI-04, HIDPI-05]

coverage:
  - id: D1
    description: "The picture-zone editor builds its screen mask at the viewport device pixel ratio (size and ratio on TwoX, pre-phase size on OneX)"
    requirement: HIDPI-04
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testZoneMaskAtDeviceResolution"
        status: pass
    human_judgment: false
  - id: D2
    description: "The animated overlay tints the white page at the picture square's centre and nowhere near the place a double-size copy would put it"
    requirement: HIDPI-04
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testZoneMaskOverlayConfined"
        status: pass
    human_judgment: true
    rationale: "The tint probe fails for a mask without the ratio, but not for a per-frame copy without the ratio (same position, only lower resolution); the sharpness of the animated overlay on the real display is judged in the live check of plan 04-07"
  - id: D3
    description: "The zone mask is rebuilt at ratio 2 and 1 when the editor moves between the screens"
    requirement: HIDPI-05
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testZoneMaskRebuildsOnScreenChange"
        status: pass
    human_judgment: true
    rationale: "The offscreen platform simulates the move; delivery of the event by Cocoa on a real display drag is covered by the live check of plan 04-07"
  - id: D4
    description: "Stage-list batch animation frames have the device size, the ratio, a centred ring, and survive a 1 pixel square"
    requirement: HIDPI-04
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testStageAnimationFrames"
        status: pass
    human_judgment: false
  - id: D5
    description: "The stage list rebuilds its frames when moved between screens; repeated ratio events change nothing"
    requirement: HIDPI-05
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testStageAnimationFollowsScreenChange"
        status: pass
    human_judgment: true
    rationale: "The delegate's logical centring (RightColDelegate::paint) has no pixel test of its own; it is judged in the live check of plan 04-07"
  - id: D6
    description: "Both commits pass gate.sh: Release and Debug 100% tests passed out of 7, goldens unchanged"
    requirement: HIDPI-04
    verification:
      - kind: other
        ref: "bash .planning/phases/03-qt-6-and-macos-only/gate.sh check r08-zone-mask / r09-stage-animation"
        status: pass
    human_judgment: false

duration: 23min
completed: 2026-10-25
status: complete
---

# Phase 4 Plan 5: Zone mask and stage-list animation at device resolution Summary

**The picture-zone mask of the output stage and the stage-list batch animation are built at the view's device pixel ratio, drawn at logical positions and sizes, and rebuilt when the ratio changes; five permanent TestHiDpiQt tests prove it on the simulated 2x screen**

## Performance

- **Duration:** 23 min
- **Started:** 2026-10-25T14:25:07Z
- **Completed:** 2026-10-25T14:48Z
- **Tasks:** 2 (both auto)
- **Files modified:** 8 (7 under src plus the gate log)

## Accomplishments

- `PictureZoneEditor`: `MaskTransformTask` takes the ratio, computes its target rect with `hidpi::targetRect`, hands `transformToGray` a `hidpi::toDevice` matrix (imageproc untouched) and tags the ARGB mask with the ratio. The mask origin is a logical `QPointF`, the animated per-frame copy keeps the mask's device size and ratio, and `validateScreenPictureMask` also compares the stored ratio with the viewport's, which rebuilds the mask after a display change.
- `StageListView`: `createBatchAnimationFrames(squareSide, dpr, head, tail)` renders 8 frames with `hidpi::makePixmap` and an explicit logical rect; `event()` rebuilds them on a real ratio change only; `RightColDelegate::paint` centres by logical size.
- Measured on `TwoX`: zone mask is 2.0-ratio and exactly `hidpi::targetRect(...).size()` in device pixels; the overlay tint at the square centre is above 8 levels and 0 to 2 levels at the place a double-size copy would put it. Stage frames are 40x40 at ratio 2 (20x20 logical), ring centred within 1 pixel, and the 1 pixel square gives 2x2 frames.
- Counterfactuals (each broken code run against its test, then restored): a mask without `setDevicePixelRatio(m_dpr)` fails all three zone-mask tests; stage frames without the explicit logical rect fail the quadrant-balance check with 0.028 (limit 0.75).

## Task Commits

1. **Task 1: the picture-zone mask at device resolution** - `6e7a4eff` (feat); gate `r08-zone-mask` PASS, gate record `583af182`
2. **Task 2: the stage-list batch animation at device resolution** - `3899d951` (feat); gate `r09-stage-animation` PASS (empty command diff), gate record `80becd6c`

**Plan metadata:** the commits that add this summary and update state and roadmap.

## Files Created/Modified

- `src/core/filters/output/PictureZoneEditor.h`, `.cpp` - mask at device resolution, logical origin, ratio in the copy, paint-time ratio check, diagnostics `screenPictureMask()` and `screenPictureMaskIsCurrent()`
- `src/core/StageListView.h`, `.cpp` - static frame renderer, `batchAnimationFrames()`, `event()` override, logical centring in the delegate
- `src/qt_tests/tst_hidpi_qt.h`, `.cpp` - five new tests and helpers (`makeZoneMask`, `makeZoneEditor`, `waitForZoneMask`, `EmptySelectionProvider`, `drawnBounds`, `quadrantBalance`)
- `src/qt_tests/CMakeLists.txt` - `dewarping` added to the qt_tests link libraries
- `.planning/phases/04-retina-rendering/records/gate-log.md` - rows `r08-zone-mask` and `r09-stage-animation`

## Decisions Made

- `qt_tests` links `dewarping`, see deviation 1.
- The explicit logical rect for `BubbleAnimation::nextFrame` stays; on this Qt (6.12) `QPaintDevice::width()` of a ratio-2 pixmap is its device width (40 for a 20 logical square), so the default rect really is twice too large.
- The 5 s settle is not needed for the new tests: `waitForHq` plus `QTRY_VERIFY_WITH_TIMEOUT(screenPictureMaskIsCurrent(), 10000)` is enough, as in plan 04-01.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] qt_tests could not include output/Settings.h**
- **Found during:** Task 1 (first build of the zone-mask tests)
- **Issue:** the tests need a real `output::Settings`; its header includes `<DistortionModel.h>`, which only the `dewarping` include directory provides, and `output` links dewarping PRIVATE, so `qt_tests` did not see it.
- **Fix:** `dewarping` added to `target_link_libraries(qt_tests PRIVATE ...)`.
- **Consequence:** the gate command diff of `r08-zone-mask` is not empty as the plan expected (12 lines per preset): only `-I src/dewarping` on the five qt_tests compile lines and `libdewarping.a` on the qt_tests link line, no automoc change. The plan's third automated check of Task 1 (`cmds-*.diff` empty) therefore fails by design; every other check passes, and the gate-log row says so. `r09-stage-animation` has an empty diff.
- **Files modified:** `src/qt_tests/CMakeLists.txt`
- **Verification:** gate `r08-zone-mask` `result: PASS`, Release and Debug 100% passed out of 7.
- **Committed in:** `6e7a4eff`

**2. [Rule 1 - Bug in the plan's test recipe] Picture mask image format**
- **Found during:** Task 1 (first test run: `std::runtime_error: Unsupported QImage format`)
- **Issue:** `imageproc::BinaryImage(const QImage&)` does not accept `Format_Grayscale8`, which the plan named for the mask image.
- **Fix:** the test draws the mask into a `Format_RGB32` image (black background, white square); semantics unchanged.
- **Committed in:** `6e7a4eff`

**3. [Rule 1 - Weak test] Frame centring check did not discriminate**
- **Found during:** Task 2 (counterfactual run without the explicit logical rect still passed)
- **Issue:** the plan's "bubbles centred within 1 device pixel" check passes for a ring that is clipped at the frame corner, because the visible arc is symmetric about the same point.
- **Fix:** added `quadrantBalance(frame) >= 0.75` (smallest over largest drawn-pixel count of the four quadrants); the broken code scores 0.028, the correct code at least 0.75.
- **Committed in:** `3899d951`

---

**Total deviations:** 3 auto-fixed (1 blocking, 2 test-recipe corrections)
**Impact on plan:** no production-code scope change. The only structural effect is the qt_tests link line and the non-empty command diff of gate `r08-zone-mask`.

## Issues Encountered

- `testStageAnimationFollowsScreenChange` prints about 15 `qt.svg: Cannot open file ':/icons/...svg'` warnings: constructing the real `StageSequence` creates the filters' icon users and the icon resources are not linked into `qt_tests`. The test passes; the warnings are noise, not failures.
- `testZoneMaskOverlayConfined` does not fail when only the per-frame copy loses its ratio (the inner draw compensates, so the overlay keeps its place but loses resolution). That line is covered by a source grep in the plan's verify step and by the live check of plan 04-07.
- Uncommitted `.planning/config.json`, `.planning/milestone.lock` and `.planning/state.json` predate this plan and were left out of every commit.

## Known Stubs

None.

## Threat Flags

None. No new network, auth, file-access or schema surface. T-04-17 is mitigated as planned: both handlers compare the stored ratio with `qFuzzyCompare` before rebuilding, and the stage-list test sends the event twice and checks the frame cache keys. T-04-18: `git diff 03083f55 -- src/imageproc src/pipeline_tests/golden src/core/filters/output/Task.cpp` is empty; goldens unchanged in both gates.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 04-06 can rely on the same `TwoX`/`OneX` screens, `placeOn()` and `waitForHq()`; `tst_hidpi_qt.cpp` now also holds `waitForZoneMask()`.
- The live check (plan 04-07) should look at the animated zone overlay on the real display, the stage-list animation during batch processing, and the centring of the animation square.

## Self-Check: PASSED

- Modified files exist: `src/core/filters/output/PictureZoneEditor.{h,cpp}`, `src/core/StageListView.{h,cpp}`, `src/qt_tests/tst_hidpi_qt.{h,cpp}`, `src/qt_tests/CMakeLists.txt`, `records/gate-log.md`.
- Commits `6e7a4eff`, `583af182`, `3899d951`, `80becd6c` are ancestors of HEAD.
- Both gate summaries read `result: PASS` and `goldens: unchanged`; `qt_tests` reports `0 failed, 0 skipped` for all three classes; the five new tests have PASS lines.
