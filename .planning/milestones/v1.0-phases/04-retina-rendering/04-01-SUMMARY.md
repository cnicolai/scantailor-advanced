---
phase: 04-retina-rendering
plan: 01
subsystem: ui
tags: [qt6, hidpi, retina, qpixmap, devicePixelRatio, qt-test, offscreen]

requires:
  - phase: 03-qt-6-and-macos-only
    provides: Qt 6 only build, gate.sh, qt_tests with the offscreen platform
provides:
  - "src/core/HiDpi.{h,cpp}: the buffer rule stated once and the eight pure hidpi helpers"
  - "ImageViewBase HQ pixmap built at the viewport device pixel ratio, rebuilt on DevicePixelRatioChange"
  - "qt_tests two-screen offscreen platform (OneX ratio 1, TwoX ratio 2) and TestHiDpiQt with eight permanent tests"
  - "records/gate-log.md for the Phase 4 gated commits"
affects: [04-02, 04-03, 04-04, 04-05, 04-06, 04-07, 04-08]

plan_head_before: 059f48000351c105bcafc2a33ed1634d43c26ea4
plan_head_after: 8f855a0f37c09906968ab3803902197db6010592

actuals:
  tokens: 8100
  tasks: 2
  commits: 4

tech-stack:
  added: []
  patterns:
    - "hidpi helper namespace: logical view coordinates, device-pixel buffers carrying setDevicePixelRatio, ratio read from the widget never a constant"
    - "Two-screen offscreen test platform: offscreen:configfile plus QT_SCREEN_SCALE_FACTORS, written by qt_tests main() before QApplication"
    - "Idempotent DevicePixelRatioChange handler backed by a paint-time ratio comparison (validateHqPixmap)"

key-files:
  created:
    - src/core/HiDpi.h
    - src/core/HiDpi.cpp
    - src/qt_tests/tst_hidpi_qt.h
    - src/qt_tests/tst_hidpi_qt.cpp
    - .planning/phases/04-retina-rendering/records/gate-log.md
  modified:
    - src/core/CMakeLists.txt
    - src/core/ImageViewBase.h
    - src/core/ImageViewBase.cpp
    - src/qt_tests/main.cpp
    - src/qt_tests/CMakeLists.txt

key-decisions:
  - "Test windows are moved by position as well as by setScreen(): the offscreen platform re-derives the screen from the window position on every geometry change, so setScreen() alone flips back to OneX on the next resize"
  - "Tests wait for the viewport child to report the new ratio before waiting for the HQ pixmap: the child learns the ratio after its top-level parent, and an HQ pixmap built at the old ratio is briefly still 'current'"

patterns-established:
  - "Each gated commit is followed by a docs commit appending its row to records/gate-log.md"
  - "Pixel-exact grab test: viewport grab compared with imageproc::transform at the device transform, with a counterfactual that a ratio 1 buffer fails the same comparison"

requirements-completed: [HIDPI-02, HIDPI-05, TESTX-03]

coverage:
  - id: D1
    description: "Page view HQ pixmap is built at the viewport device pixel ratio (size, ratio and logical position) on a simulated 2x screen"
    requirement: HIDPI-02
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testHqPixmapAtDeviceResolution"
        status: pass
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testHqGrabMatchesDeviceReference"
        status: pass
    human_judgment: false
  - id: D2
    description: "Ratio 1 rendering is the pre-phase arithmetic, and the helpers are pinned at ratios 1, 1.5 and 2 including empty and non-finite inputs"
    requirement: HIDPI-02
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testHqPixmapAtOneXUnchanged"
        status: pass
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testHelpersMatchLegacyAtRatio1"
        status: pass
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testHelpersAtFractionalRatio"
        status: pass
    human_judgment: false
  - id: D3
    description: "The page view rebuilds its HQ pixmap when it moves between a 1x and a 2x screen; repeated ratio events are harmless"
    requirement: HIDPI-05
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testHqRebuildsOnScreenChange"
        status: pass
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testRepeatedRatioEventKeepsHqPixmap"
        status: pass
    human_judgment: true
    rationale: "The offscreen platform simulates the move; delivery of the event by Cocoa on a real display drag is not observable on this Mac (one display) and is covered by the live check of plan 04-02"
  - id: D4
    description: "The 0.5 mm smoothing switch measures physical millimetres at ratio 1 and 2"
    requirement: HIDPI-02
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testSmoothingSwitchPhysicalSize"
        status: pass
    human_judgment: false
  - id: D5
    description: "Two-screen offscreen test platform in qt_tests; both commits pass gate.sh (Release and Debug, goldens unchanged)"
    requirement: TESTX-03
    verification:
      - kind: other
        ref: "bash .planning/phases/03-qt-6-and-macos-only/gate.sh check r01-hq-tracer / r02-hq-dpr-change"
        status: pass
    human_judgment: false

duration: 19min
completed: 2026-10-25
status: complete
---

# Phase 4 Plan 1: HQ page pixmap at device resolution Summary

**The page view builds its high-quality pixmap at the viewport's device pixel ratio (hidpi helpers, logical positioning, rebuild on display change), proven pixel-exact on a simulated 2x offscreen screen by eight permanent TestHiDpiQt tests**

## Performance

- **Duration:** 19 min
- **Started:** 2026-10-25T13:15:25Z
- **Completed:** 2026-10-25T13:34:13Z
- **Tasks:** 2 (1 tracer, 1 auto)
- **Files modified:** 10 (5 created, 5 modified; 9 under src plus the gate log)

## Accomplishments

- `src/core/HiDpi.{h,cpp}` states the buffer rule once and provides `sanitizedRatio`, `deviceSize`, `toDevice`, `targetRect`, `toLogical`, `makePixmap`, `logicalSize`, `logicalRect`; at ratio 1 each returns exactly the pre-phase arithmetic.
- `ImageViewBase` builds the HQ image with a device-scaled transform (`imageproc::transform` itself untouched), tags it with the ratio, keeps its position as a logical `QPointF`, rejects a pixmap whose stored ratio differs from the viewport's, and reacts to `QEvent::DevicePixelRatioChange` idempotently.
- Measured on `TwoX` (ratio 2): the viewport grab equals the device-resolution reference in 100 percent of compared pixels with maximum channel difference 0 at fit, 1:1 and 2:1 zoom; a ratio 1 buffer scaled up differs from the reference in 94 percent of pixels at fit (floor 5 percent).
- Smoothing switch `displayedSourcePixelWidthMM()` equals the zoom times millimetres per logical pixel within 0.6 percent on `OneX` (0.08417 vs 0.08467 mm) and 0.1 percent on `TwoX` (0.16917 vs 0.16933 mm); no recalibration needed.
- `qt_tests` runs on two named offscreen screens (`OneX` ratio 1, `TwoX` ratio 2) configured before `QApplication` from `<build>/test-tmp/offscreen-screens.json`; still 7 ctest suites.

## Task Commits

1. **Task 1 (tracer): HQ pixmap at the viewport's device pixel ratio on a 2x offscreen screen** - `7c730ab2` (feat); gate `r01-hq-tracer` PASS, gate record `ec0be406`
2. **Task 2: helpers pinned, grab versus device reference, rebuild on display change, smoothing in millimetres** - `288ea84b` (feat); gate `r02-hq-dpr-change` PASS (empty command diff), gate record `8f855a0f`

**Plan metadata:** the commit that adds this summary (docs: complete plan)

## Files Created/Modified

- `src/core/HiDpi.h`, `src/core/HiDpi.cpp` - buffer rule and pure helpers (new)
- `src/core/CMakeLists.txt` - HiDpi in `common_sources`
- `src/core/ImageViewBase.h`, `src/core/ImageViewBase.cpp` - HQ path at device resolution, `m_hqDpr`, `event()`, diagnostics accessors, `displayedSourcePixelWidthMM()`
- `src/qt_tests/main.cpp`, `src/qt_tests/CMakeLists.txt` - two-screen offscreen platform, `TestHiDpiQt` registration
- `src/qt_tests/tst_hidpi_qt.h`, `src/qt_tests/tst_hidpi_qt.cpp` - `TestHiDpiQt` (new)
- `.planning/phases/04-retina-rendering/records/gate-log.md` - one row per gated commit (new)

## Decisions Made

- Placing a test window on a screen uses `setScreen()` plus a `move()` into the screen's rectangle. The offscreen plugin re-picks the screen from the window position on every geometry change, so `setScreen()` alone (as in the plan and RESEARCH) reverted to `OneX` at the following `resize()`.
- Tests wait for `viewport()->devicePixelRatioF()` to reach the target before waiting for the HQ pixmap, because the viewport child reports a new ratio after its top-level parent.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Offscreen window fell back to the 1x screen after resize()**
- **Found during:** Task 1 (first run of `testHqPixmapAtDeviceResolution`)
- **Issue:** after `setScreen(TwoX)` and `resize()`, the window reported ratio 1 and a logical size of 800x600; the offscreen platform re-derives the screen from the window position, which stayed inside `OneX`.
- **Fix:** `placeOn()` also moves the window to `screen->geometry().topLeft() + QPoint(20, 20)` (the alternative named in RESEARCH Code Examples), and the tests wait for the viewport ratio before the HQ pixmap.
- **Files modified:** `src/qt_tests/tst_hidpi_qt.cpp`
- **Verification:** all eight TestHiDpiQt tests pass in Release and Debug via the gate.
- **Committed in:** `7c730ab2`

---

**Total deviations:** 1 auto-fixed (1 blocking, test-platform quirk)
**Impact on plan:** test helper only; no production-code change and no scope creep. Tests 1 and 2 were also switched to the shared `waitForHq()` helper in Task 2 (refactor, same assertions).

## Issues Encountered

None beyond the deviation above. Uncommitted `.planning/STATE.md`, `.planning/config.json`, `.planning/milestone.lock` and `.planning/state.json` predate this plan and were left out of every commit.

## Known Stubs

None.

## Threat Flags

None. The only new file write (the offscreen screens JSON) is inside the build tree's `test-tmp` directory and fails loudly (`qt_tests` returns 2), as planned in T-04-03.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 04-02 (live Retina check on the real display) can use `hqPixmap()`, `hqPixmapPos()`, `hqPixmapIsCurrent()` and `displayedSourcePixelWidthMM()`.
- Later plans reuse the `hidpi` helpers, the `OneX`/`TwoX` screens and `placeOn()`/`waitForHq()` in `tst_hidpi_qt.cpp`, and append rows to `records/gate-log.md`.
- Offscreen rebuild-on-move is simulated; the real Cocoa event delivery is still to be confirmed live (plan 04-02).

## Self-Check: PASSED

- Created files exist: `src/core/HiDpi.h`, `src/core/HiDpi.cpp`, `src/qt_tests/tst_hidpi_qt.h`, `src/qt_tests/tst_hidpi_qt.cpp`, `records/gate-log.md` (checked below).
- Commits `7c730ab2`, `ec0be406`, `288ea84b`, `8f855a0f` are ancestors of HEAD.
- Both gate summaries read `result: PASS` and `goldens: unchanged`; qt_tests reports `0 failed, 0 skipped` for all three classes.

---
*Phase: 04-retina-rendering*
*Completed: 2026-10-25*
