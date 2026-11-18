---
phase: 04-retina-rendering
plan: 04
subsystem: ui
tags: [qt6, hidpi, retina, thumbnails, devicePixelRatio, qpixmapcache, qt-test]

requires:
  - phase: 04-retina-rendering
    provides: "plan 04-01: hidpi::sanitizedRatio, TestHiDpiQt helpers (placeOn, makeNoisePage), OneX/TwoX offscreen screens"
  - phase: 04-retina-rendering
    provides: "plan 04-03: ratio-aware ThumbnailPixmapCache (setDevicePixelRatio, q500 files, size-generation guard)"
provides:
  - "ThumbnailBase::paint renders through a temp pixmap at the painter's device ratio, QPixmapCache key ThumbnailBase::tempPixmap@<ratio>"
  - "ThumbnailDprTracker (src/core): event filter that pushes the view's devicePixelRatioF() into a weakly held thumbnail cache and repaints the viewport"
  - "MainWindow::m_thumbDprTracker created on thumbView in setupThumbView() and handed every new or reset cache in switchToNewProject()"
  - "Tests testThumbnailPaintAtRatio and testThumbnailsFollowScreenChange; gate rows r06-thumb-paint and r07-thumb-dpr-follow"
affects: [04-05, 04-06, 04-07, 04-08]

plan_head_before: b52766bcf7d4d452dab11a190c2c1d2475122196
plan_head_after: ce52ddbbbf16facda503192facb4e76f5eb7bcfd

actuals:
  tokens: 3900
  tasks: 2
  commits: 4

tech-stack:
  added: []
  patterns:
    - "Temp paint buffer keyed by device ratio in QPixmapCache so 1x and 2x views do not share one buffer"
    - "Core event filter owned by the view through the Qt parent, holding the cache as std::weak_ptr"

key-files:
  created:
    - src/core/ThumbnailDprTracker.h
    - src/core/ThumbnailDprTracker.cpp
  modified:
    - src/core/ThumbnailBase.cpp
    - src/core/CMakeLists.txt
    - src/app/MainWindow.h
    - src/app/MainWindow.cpp
    - src/qt_tests/tst_hidpi_qt.h
    - src/qt_tests/tst_hidpi_qt.cpp
    - .planning/phases/04-retina-rendering/records/gate-log.md

key-decisions:
  - "The loaded thumbnail pixmap keeps ratio 1: its pre-scale transform is computed from its pixel width (Pitfall 8); only the temp buffer carries the painter's ratio"
  - "The tracker never compares ratios itself: ThumbnailPixmapCache::setDevicePixelRatio is a no-op for an equal ratio, so the up-to-two DevicePixelRatioChange events per move are harmless"

patterns-established:
  - "Painting test into a QImage with setDevicePixelRatio(2) and a 2x2 block non-uniformity metric that a nearest-upscaled ratio 1 buffer cannot reach"

requirements-completed: [HIDPI-03, HIDPI-05]

coverage:
  - id: D1
    description: "A thumbnail painted into a ratio-2 target uses a temp pixmap of ratio 2 under the key tempPixmap@2.00 and shows device-resolution detail (at least 5 percent non-uniform 2x2 blocks at all four offsets); a ratio-1 target gets its own tempPixmap@1.00 entry"
    requirement: HIDPI-03
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testThumbnailPaintAtRatio"
        status: pass
    human_judgment: false
  - id: D2
    description: "A thumbnail view moved from OneX to TwoX and back makes the tracker set the cache ratio 2 then 1; q500 then q250 pixmaps are loaded and the 2x temp pixmap is painted, without a restart (simulated display move)"
    requirement: HIDPI-05
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testThumbnailsFollowScreenChange"
        status: pass
    human_judgment: true
    rationale: "The offscreen platform simulates the move and delivers the event to the view as a top-level window; delivery to thumbView as a child widget on a real Cocoa display drag is not observable on this one-display Mac and is covered by the live check of plan 04-07"
  - id: D3
    description: "Both commits pass gate.sh: Release and Debug 100% out of 7, goldens unchanged; the paint commit has an empty command diff, the tracker commit only the ThumbnailDprTracker.cpp compile line and the core archive line"
    verification:
      - kind: other
        ref: "bash .planning/phases/03-qt-6-and-macos-only/gate.sh check r06-thumb-paint / r07-thumb-dpr-follow"
        status: pass
    human_judgment: false
  - id: D4
    description: "Thumbnails look sharp in the thumbnail column on the Retina display"
    requirement: HIDPI-03
    verification: []
    human_judgment: true
    rationale: "The tests prove device-resolution rendering into a ratio-2 image; the look on the real display needs the live check of plan 04-07"

duration: 17min
completed: 2026-10-25
status: complete
---

# Phase 4 Plan 4: Thumbnails at device resolution and display-change tracking Summary

**ThumbnailBase now paints through a temp pixmap at the painter's device pixel ratio (ratio in the QPixmapCache key), and a small ThumbnailDprTracker installed by MainWindow keeps the thumbnail cache at the thumbnail view's ratio so a display change rebuilds q500/q250 thumbnails without a restart**

## Performance

- **Duration:** about 17 min
- **Started:** 2026-10-25T14:06:00Z (estimated)
- **Completed:** 2026-10-25T14:23:00Z
- **Tasks:** 2
- **Files modified:** 9 (2 created, 7 modified; 8 under src plus the gate log)

## Accomplishments

- `ThumbnailBase::paint`: the temp pixmap is `qCeil(displayRect * dpr)` device pixels plus 10 percent, carries `setDevicePixelRatio(dpr)` and is cached under `ThumbnailBase::tempPixmap@%1` (two decimals). The temp painter and the final `drawPixmap` stay in logical coordinates. At ratio 1 the arithmetic equals the old code and the loaded thumbnail pixmap stays at ratio 1.
- `ThumbnailDprTracker`: on `QEvent::DevicePixelRatioChange` for the view it sets the cache ratio from `devicePixelRatioF()` and calls `viewport()->update()`; it never consumes the event, owns the cache only as `std::weak_ptr`, and is owned by the view through the Qt parent.
- `MainWindow` creates one tracker in `setupThumbView()` and calls `m_thumbDprTracker->setCache(m_thumbnailCache)` right after the cache is created or reset in `switchToNewProject()` (a null cache is allowed).
- `testThumbnailPaintAtRatio` fails without the change (counterfactual run with the old `ThumbnailBase.cpp`: the `@2.00` entry does not exist). `testThumbnailsFollowScreenChange` passed in five consecutive runs; qt_tests reports 17 passed, 0 failed, 0 skipped for TestHiDpiQt in Release.

## Task Commits

1. **Task 1: ThumbnailBase renders through a temp pixmap at the painter's device ratio** - `a2396af5` (feat); gate `r06-thumb-paint` PASS (Release and Debug 100% out of 7, goldens unchanged, command diff 0 / 0), gate record `4d2b80e6`
2. **Task 2: ThumbnailDprTracker keeps the cache at the view's ratio, MainWindow installs it** - `489165ea` (feat); gate `r07-thumb-dpr-follow` PASS (same result, command diff 3 / 3 lines: the new compile line and the core archive line), gate record `ce52ddbb`

**Plan metadata:** the commit that adds this summary (docs: complete plan)

## Files Created/Modified

- `src/core/ThumbnailBase.cpp` - ratio-aware temp pixmap and key
- `src/core/ThumbnailDprTracker.h`, `src/core/ThumbnailDprTracker.cpp` - the event filter (new)
- `src/core/CMakeLists.txt` - tracker in `common_sources`
- `src/app/MainWindow.h`, `src/app/MainWindow.cpp` - `m_thumbDprTracker`, creation and hand-over of the cache
- `src/qt_tests/tst_hidpi_qt.h`, `src/qt_tests/tst_hidpi_qt.cpp` - helper `minBlockNonUniformFraction`, two tests
- `.planning/phases/04-retina-rendering/records/gate-log.md` - rows `r06-thumb-paint`, `r07-thumb-dpr-follow`

## Decisions Made

- Loaded thumbnail pixmap left at ratio 1 (its pre-scale transform divides by the pixel width), as the plan's prohibition requires.
- The ratio-1 thumbnail width in the follow test is accepted as 249 or 250 (the 04-03 `makeThumbnail()` rounding that is kept to leave ratio-1 output unchanged).

## Deviations from Plan

None - plan executed exactly as written. The only adjustment is the width tolerance (249 or 250) in the new test, which follows the 04-03 finding rather than the plan's "250".

## Issues Encountered

None. Including `ThumbnailBase.h` in the test brings in the header's existing unused-parameter warnings (virtual hooks with empty bodies); they are pre-existing and left alone. Uncommitted `.planning/config.json`, `.planning/milestone.lock` and `.planning/state.json` predate this plan and were left out of every commit.

## Known Stubs

None.

## Threat Flags

None. No new endpoint, file path or trust boundary; T-04-14 (weak cache pointer) and T-04-15 (tracker owned by the view, MainWindow keeps a non-owning pointer) are implemented as planned.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Thumbnails are drawn at device resolution and follow display changes in the offscreen simulation. Plan 04-07 (live check) still has to confirm that Cocoa delivers `DevicePixelRatioChange` to `thumbView` as a child widget and that the thumbnail column looks sharp on the real display (coverage D2, D4).
- Remaining Phase 4 plans can reuse `minBlockNonUniformFraction`, `placeOn` and the `OneX`/`TwoX` screens, and append rows to `records/gate-log.md`.

## Self-Check: PASSED

- Created files exist: `src/core/ThumbnailDprTracker.h`, `src/core/ThumbnailDprTracker.cpp`.
- Commits `a2396af5`, `4d2b80e6`, `489165ea`, `ce52ddbb` are ancestors of HEAD.
- `build/scratch/gate/r06-thumb-paint/summary.txt` and `build/scratch/gate/r07-thumb-dpr-follow/summary.txt` read `result: PASS` and `goldens: unchanged`; the r06 command diffs are empty.
- `build/scratch/qt-r06.txt` and `build/scratch/qt-r07.txt` have PASS lines for both new tests and `Totals: ... 0 failed, 0 skipped` for every class.
