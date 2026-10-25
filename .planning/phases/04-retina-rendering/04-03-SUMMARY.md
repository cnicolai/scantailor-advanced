---
phase: 04-retina-rendering
plan: 03
subsystem: ui
tags: [qt6, hidpi, retina, thumbnail-cache, devicePixelRatio, qt-test]

requires:
  - phase: 04-retina-rendering
    provides: "plan 04-01: hidpi::sanitizedRatio, TestHiDpiQt with makeNoisePage, gate-log.md"
provides:
  - "ThumbnailPixmapCache::setDevicePixelRatio / getDevicePixelRatio / effectiveThumbSize / MAX_EFFECTIVE_THUMB_SIDE"
  - "Thumbnail files named and built at Quality times the display ratio (q500 at the defaults on Retina), in-memory flush on a ratio change, size-generation guard against stale loads"
  - "Utils::createThumbnailCache(outputDir, dpr) and the MainWindow call that passes thumbView->devicePixelRatioF()"
  - "Quality default 250 and a Settings tooltip that explains the multiplication"
affects: [04-04, 04-05, 04-06, 04-07, 04-08]

plan_head_before: 812a25886768bb30180c6e7b9e794aaf6dcaa9c5
plan_head_after: 01c89be1ed4668b569f65276450e880c9b5c47ec

actuals:
  tokens: 5000
  tasks: 2
  commits: 4

tech-stack:
  added: []
  patterns:
    - "Disk cache keyed by effective width in the file name (q<Quality x ratio>), in-memory cache keyed by ImageId and flushed on a ratio change"
    - "Size generation counter under the cache mutex: a result carrying an older generation becomes REQUEST_EXPIRED so the view requests the thumbnail again"

key-files:
  created: []
  modified:
    - src/core/ThumbnailPixmapCache.h
    - src/core/ThumbnailPixmapCache.cpp
    - src/core/Utils.h
    - src/core/Utils.cpp
    - src/app/MainWindow.cpp
    - src/core/ApplicationSettings.cpp
    - src/app/SettingsDialog.ui
    - src/qt_tests/tst_hidpi_qt.h
    - src/qt_tests/tst_hidpi_qt.cpp
    - .planning/phases/04-retina-rendering/records/gate-log.md

key-decisions:
  - "Quality spin box range stays 100 to 1500 (a lower maximum would rewrite stored values when the dialog is confirmed); the cache clamps each effective side to 3000 instead"
  - "Items put into the cache by loadNow() are now counted as loaded, otherwise the ratio-change flush skipped them"
  - "makeThumbnail() left as it was: at ratio 1 a 1200x900 page still becomes a 249 px wide file in the q250 name (double fitting in QImage::scaled), so output at ratio 1 does not change"

patterns-established:
  - "A second gated commit per task, then its gate-log row as a docs commit (continues 04-01 and 04-02)"

requirements-completed: [HIDPI-03]

coverage:
  - id: D1
    description: "Thumbnail files are built at Quality times the ratio and named by that width (q500 at Quality 250 and ratio 2, q250 at ratio 1); the file of the other size is left byte-identical"
    requirement: HIDPI-03
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testThumbnailFilesKeyedByRatio"
        status: pass
    human_judgment: false
  - id: D2
    description: "effectiveThumbSize values, including the 3000 clamp, a ratio of 0 or NaN, and INT_MAX without overflow"
    requirement: HIDPI-03
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testThumbnailEffectiveSize"
        status: pass
    human_judgment: false
  - id: D3
    description: "An old q200 file is never deleted or stretched; a request at Quality 250 and ratio 2 is queued with a null pixmap and later loads a 500 px thumbnail built from the original"
    requirement: HIDPI-03
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testOldThumbnailLeftInPlace"
        status: pass
    human_judgment: false
  - id: D4
    description: "A background load that started before a ratio change is reported as REQUEST_EXPIRED and not stored as LOADED; a new request then builds the 500 px thumbnail"
    requirement: HIDPI-03
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testStaleLoadDiscarded"
        status: pass
    human_judgment: false
  - id: D5
    description: "Quality default is 250 with no stored value; the tooltip names the default and the multiplication; the range stays 100 to 1500"
    requirement: HIDPI-03
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#TestHiDpiQt::testThumbnailQualityDefault"
        status: pass
    human_judgment: false
  - id: D6
    description: "Both commits pass gate.sh with Release and Debug 100% out of 7, goldens unchanged, empty command diff"
    verification:
      - kind: other
        ref: "bash .planning/phases/03-qt-6-and-macos-only/gate.sh check r04-thumb-cache / r05-thumb-quality"
        status: pass
    human_judgment: false
  - id: D7
    description: "Thumbnails look sharp in the running app on the Retina display (the cache half of HIDPI-03; the paint half is plan 04-04)"
    requirement: HIDPI-03
    verification: []
    human_judgment: true
    rationale: "These tests prove the file size and cache behaviour; the sharpness seen in the thumbnail column needs the painting change of plan 04-04 and the live check of plan 04-07"

duration: 17min
completed: 2026-10-25
status: complete
---

# Phase 4 Plan 3: Thumbnail cache keyed by device pixel ratio Summary

**The thumbnail cache builds and names every file at Quality times the display's pixel ratio (500 px at the new default of 250 on a Retina display), flushes its in-memory pixmaps and discards stale background loads when the ratio changes, and leaves files of other sizes untouched**

## Performance

- **Duration:** about 17 min (start time not recorded at launch; estimated from the end of plan 04-02)
- **Completed:** 2026-10-25T14:05:00Z
- **Tasks:** 2
- **Files modified:** 10 (9 under src plus the gate log)

## Accomplishments

- `ThumbnailPixmapCache` gained `setDevicePixelRatio`, `getDevicePixelRatio`, `effectiveThumbSize` and `MAX_EFFECTIVE_THUMB_SIDE` (3000). Every file operation (`request` with `loadNow`, `ensureThumbnailExists`, `recreateThumbnail`, `backgroundProcessing`) works at `effectiveThumbSize(m_maxThumbSize, m_dpr)` computed under the mutex; the former unlocked read of the member in `ensureThumbnailExists` and `recreateThumbnail` is gone. `getMaxThumbSize()` still returns the Quality value, so `MainWindow::onSettingsChanged` is unchanged.
- A size generation counter (`m_sizeGeneration`, bumped by `setDevicePixelRatio` and `setMaxThumbSize`) travels with every `LoadResultEvent`. `processLoadResult` turns a LOADED result of an older generation into `REQUEST_EXPIRED`, which makes the thumbnail item repaint and request again; `recreateThumbnail` leaves the in-memory item alone when the generation changed during the write.
- `Utils::createThumbnailCache(outputDir, dpr)` sets the ratio before returning the cache; `MainWindow` passes `thumbView->devicePixelRatioF()`, so `LoadFileTask` writes the right size from the first page.
- `DEFAULT_THUMBNAIL_QUALITY` is 250; the Quality tooltip now reads "...built at this size multiplied by the display's pixel ratio, twice on the Studio Display. The default value is 250." The spin box range is unchanged.
- Five new `TestHiDpiQt` tests (`testThumbnailEffectiveSize`, `testThumbnailFilesKeyedByRatio`, `testOldThumbnailLeftInPlace`, `testStaleLoadDiscarded`, `testThumbnailQualityDefault`); qt_tests reports 15 passed, 0 failed, 0 skipped in Release, and five repeated runs of the stale-load test were stable.

## Task Commits

1. **Task 1: key the thumbnail cache by device pixel ratio** - `7ccaa186` (feat); gate `r04-thumb-cache` PASS (Release and Debug 100% out of 7, goldens unchanged, command diff 0 / 0), gate record `2002ec68`
2. **Task 2: raise the thumbnail quality default to 250** - `18371848` (feat); gate `r05-thumb-quality` PASS (same result), gate record `01c89be1`

**Plan metadata:** the commit that adds this summary (docs: complete plan)

## Files Created/Modified

- `src/core/ThumbnailPixmapCache.h`, `src/core/ThumbnailPixmapCache.cpp` - ratio-aware API, effective size on every file operation, generation guard
- `src/core/Utils.h`, `src/core/Utils.cpp`, `src/app/MainWindow.cpp` - factory takes the ratio; call site passes the thumbnail view's ratio
- `src/core/ApplicationSettings.cpp`, `src/app/SettingsDialog.ui` - default 250 and tooltip
- `src/qt_tests/tst_hidpi_qt.h`, `src/qt_tests/tst_hidpi_qt.cpp` - five new tests
- `.planning/phases/04-retina-rendering/records/gate-log.md` - rows `r04-thumb-cache` and `r05-thumb-quality`

## Decisions Made

- The Quality range stays 100 to 1500 and the cache clamps the effective side to 3000, so confirming the Settings dialog never rewrites a larger stored value (D-05).
- `makeThumbnail()` was not touched: at ratio 1 output stays as before.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Items cached by loadNow() were not counted as loaded**
- **Found during:** Task 1 (first run of `testThumbnailFilesKeyedByRatio`: after `setDevicePixelRatio(1.0)` the old pixmap was still returned as LOADED)
- **Issue:** `cachePixmapLocked()` inserted a new LOADED item without incrementing `m_numLoadedItems`, so `removeLoadedItemsLocked()` returned early (`m_numLoadedItems == 0`) and the ratio-change flush skipped the item. The path (`loadNow`) has no caller in production code, so the bug was latent.
- **Fix:** increment `m_numLoadedItems` for a new LOADED item.
- **Files modified:** `src/core/ThumbnailPixmapCache.cpp`
- **Verification:** `testThumbnailFilesKeyedByRatio` passes; gate `r04-thumb-cache` PASS
- **Committed in:** `7ccaa186`

**2. [Rule 1 - Test expectation] The ratio 1 thumbnail of a 1200x900 page is 249 px wide, not 250**
- **Found during:** Task 1
- **Issue:** `makeThumbnail()` fits the size with `QSize::scale()` (250x187) and then `QImage::scaled()` fits the image into that already fitted size once more (249x187). The plan expected width 250. At ratio 2 the result is exactly 500x375, so only the ratio 1 file differs from the plan's number.
- **Fix:** the test accepts 249 or 250 for the q250 file and compares the loaded pixmap against the file's width; `makeThumbnail()` is unchanged because output at ratio 1 must not change.
- **Files modified:** `src/qt_tests/tst_hidpi_qt.cpp`
- **Committed in:** `7ccaa186`

**3. [Rule 2 - Missing test for a stated truth] testStaleLoadDiscarded added**
- **Found during:** Task 1
- **Issue:** the plan's must-have about stale loads (RESEARCH Pitfall 7) had no test among the three listed.
- **Fix:** the new test waits for the worker to write the q250 file without running the event loop, changes the ratio, and checks the handler receives `REQUEST_EXPIRED`, that `loadFromCache` then reports `LOAD_FAILED`, and that a new request loads a 500 px pixmap. This is deterministic because the worker reads the generation before it writes the file and the result event is only delivered once the test spins the event loop.
- **Files modified:** `src/qt_tests/tst_hidpi_qt.h`, `src/qt_tests/tst_hidpi_qt.cpp`
- **Committed in:** `7ccaa186`

---

**Total deviations:** 3 auto-fixed (1 production bug, 1 test expectation, 1 added test)
**Impact on plan:** no scope creep; the only production change beyond the plan is the one-line counter fix in a path the flush depends on.

## Issues Encountered

None beyond the deviations. Uncommitted `.planning/config.json`, `.planning/milestone.lock` and `.planning/state.json` predate this plan and were left out of every commit.

## Known Stubs

None.

## Threat Flags

None. Thumbnail names keep the existing scheme apart from the width; no new file path, endpoint or trust boundary (T-04-09 to T-04-12 mitigated as planned: clamp tested with INT_MAX, old files hashed in the test, no migration of stored settings, size and generation copied under the mutex).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 04-04 can draw thumbnails through a temporary pixmap at the painter's ratio and key `QPixmapCache` by the ratio; the cache already hands it 500 px pixmaps on a Retina display.
- A project's existing `q200` thumbnails stay in `cache/thumbs`; they are no longer used and are not deleted (D-08).
- Sharpness of the thumbnail column in the running app is still to be seen after plan 04-04 (coverage D7).

## Self-Check: PASSED

- All ten modified files exist on disk; commits `7ccaa186`, `2002ec68`, `18371848`, `01c89be1` are ancestors of HEAD.
- `build/scratch/gate/r04-thumb-cache/summary.txt` and `build/scratch/gate/r05-thumb-quality/summary.txt` read `result: PASS` and `goldens: unchanged`; both command diffs are empty; `git diff a796c2ae -- src/pipeline_tests/golden` is empty.
- `build/scratch/qt-r05.txt` has PASS lines for all five new tests and `Totals: 15 passed, 0 failed, 0 skipped` for TestHiDpiQt.

---
*Phase: 04-retina-rendering*
*Completed: 2026-10-25*
