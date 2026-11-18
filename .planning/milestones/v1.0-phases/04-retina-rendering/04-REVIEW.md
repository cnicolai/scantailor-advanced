---
phase: 04-retina-rendering
reviewed: 2026-10-25T00:00:00Z
depth: standard
files_reviewed: 34
files_reviewed_list:
  - src/app/CMakeLists.txt
  - src/app/MainWindow.cpp
  - src/app/MainWindow.h
  - src/app/retina_check/main.cpp
  - src/app/SettingsDialog.ui
  - src/core/ApplicationSettings.cpp
  - src/core/CMakeLists.txt
  - src/core/filters/deskew/ImageView.cpp
  - src/core/filters/output/ColorPickupInteraction.cpp
  - src/core/filters/output/ColorPickupInteraction.h
  - src/core/filters/output/PictureZoneEditor.cpp
  - src/core/filters/output/PictureZoneEditor.h
  - src/core/filters/page_split/ImageView.cpp
  - src/core/filters/page_split/Thumbnail.cpp
  - src/core/filters/page_split/UnremoveButton.cpp
  - src/core/HiDpi.cpp
  - src/core/HiDpi.h
  - src/core/ImageViewBase.cpp
  - src/core/ImageViewBase.h
  - src/core/StageListView.cpp
  - src/core/StageListView.h
  - src/core/ThumbnailBase.cpp
  - src/core/ThumbnailDprTracker.cpp
  - src/core/ThumbnailDprTracker.h
  - src/core/ThumbnailPixmapCache.cpp
  - src/core/ThumbnailPixmapCache.h
  - src/core/Utils.cpp
  - src/core/Utils.h
  - src/core/zones/ZoneContextMenuInteraction.cpp
  - src/qt_tests/CMakeLists.txt
  - src/qt_tests/main.cpp
  - src/qt_tests/tst_hidpi_qt.cpp
  - src/qt_tests/tst_hidpi_qt.h
  - TESTING.md
findings:
  critical: 0
  warning: 3
  info: 5
  total: 8
status: issues_found
---

# Phase 4: Code Review Report

**Reviewed:** 2026-10-25
**Depth:** standard
**Files Reviewed:** 34
**Status:** issues_found

## Summary

The phase changes were reviewed against `git diff 51e77637`. The view-side work is sound. The HQ pixmap, zone mask, stage-list frames, icon geometry and colour picker keep view coordinates logical and buffers in device pixels. The `hidpi::` helpers reduce to the old arithmetic at ratio 1. No view DPR reaches output generation: the only images that get `setDevicePixelRatio()` are display-only results (`hqImage`, the zone mask, `makePixmap` pixmaps). `sampleImageAtWidgetRect()` returns a copy of `m_image`, which has ratio 1. `ThumbnailDprTracker` ownership is correct. It is a child of the view and holds the cache through a `weak_ptr`. It is created in the `MainWindow` constructor (`setupThumbView()` runs at line 138, before any `switchToNewProject()` call), so the `m_thumbDprTracker->setCache()` call cannot hit a null pointer.

The weak points are in `ThumbnailPixmapCache`. The new per-ratio thumbnail files expose two stale-content paths, and a display change now reaches an existing queue-bookkeeping bug that only a quality change could reach before.

No security issues were found. The `retina_check` output-path guard is lexical and does not resolve symlinks (IN-05).

## Warnings

### WR-01: `removeLoadedItemsLocked()` resets `m_endOfLoadedItems` to `end()` while non-LOADED items remain, and a display change now reaches it

**File:** `src/core/ThumbnailPixmapCache.cpp:835-850` (called from the new `setDevicePixelRatio()`, ~line 940)
**Issue:** The invariant (documented at lines 183-199) is that every LOADED item precedes all other items in the remove queue, and `m_endOfLoadedItems` marks the first non-LOADED item. The flush erases the LOADED items and then sets the marker to `m_removeQueue.end()`. That is only correct if the queue is empty afterwards. QUEUED, IN_PROGRESS and LOAD_FAILED items stay in the queue, so the marker should be `m_removeQueue.begin()`. Consequences, reproducible with `[L1 L2 Q3 Q4]`:
- After the flush the queue is `[Q3 Q4]` with the marker at `end()`.
- When Q3 finishes, `processLoadResult` calls `relocate(m_endOfLoadedItems, rqIt)`, which moves a LOADED item behind non-LOADED ones.
- `removeExcessLocked()` assumes `m_removeQueue.front()` is LOADED (it only `assert`s that). With a stale front it erases whichever item is first, such as a LOAD_FAILED item, or a QUEUED or IN_PROGRESS item whose `lqIt` a pending `LoadResultEvent` still holds, which is a dangling iterator. If it erases a LOAD_FAILED item, `m_numLoadedItems` is not decremented and the 40-pixmap bound stops working.
- `request()` (line 463) re-synchronises the marker only when a new item arrives, which hides the problem in the common case.

Before this phase, only `setMaxThumbSize()` (a settings dialog action) could trigger this. Now every window move between displays with different ratios does, normally while thumbnails are loading, which is when the queue is non-empty.

**Fix:**
```cpp
void ThumbnailPixmapCache::Impl::removeLoadedItemsLocked() {
  if (m_numLoadedItems == 0) {
    return;
  }
  for (auto it = m_removeQueue.begin(); it != m_removeQueue.end();) {
    if (it->status == Item::LOADED) {
      it = m_removeQueue.erase(it);
    } else {
      ++it;
    }
  }
  m_numLoadedItems = 0;
  // Everything left is non-LOADED, so the first remaining item starts the non-LOADED range.
  m_endOfLoadedItems = m_removeQueue.begin();
}
```
Check that `request()`'s `m_endOfLoadedItems == m_removeQueue.end()` test still holds. `begin() == end()` for an empty queue, so it does. Add a `qt_tests` case that flushes with a queued item and a LOAD_FAILED item present and then loads more than `maxCachedPixmaps` pixmaps.

### WR-02: `recreateThumbnail()` refreshes only the thumbnail file of the current ratio, and drops its in-memory invalidation when the generation changed

**File:** `src/core/ThumbnailPixmapCache.cpp:514-548`, also `ensureThumbnailExists()` at 488-507
**Issue:** The phase turned one thumbnail file per image into one per effective width (`q250`, `q500`). Two stale-content paths follow.
1. `recreateThumbnail()` is the "the image changed, replace the thumbnail" operation (called after the output stage rewrites a page). It now rewrites only the file for the ratio in force at that moment. Files of the other ratio (`q250` after working on a 1x display, `q500` after working on a 2x one) keep the old content. When the window later moves to the other display, `loadSaveThumbnail()` finds the stale file (`QFile::exists`) and shows the pre-reprocessing thumbnail. `ensureThumbnailExists()` likewise returns early on the existing stale file. The header states "Files on disk are never touched (files of other sizes stay where they are)", so this is by design, but it contradicts what `recreateThumbnail()` promises for the image.
2. The new early return at line 543 (`generation != m_sizeGeneration`) skips the invalidation of the in-memory item. If the ratio flips while the new file is being written, the file is written under the old ratio's name. The in-memory item and the on-disk file for the new ratio are both left holding the old content, so the thumbnail does not update.

Effect: a thumbnail that shows output from before the last reprocessing. This is visible, wrong content, with no data loss.

**Fix:** In `recreateThumbnail()`, write the thumbnail for the current effective size and, when the generation changed in between, repeat the write for the new size (loop until the generation is stable) before the item lookup. To cover the other-ratio files, either delete the sibling `*_q<width>.png` files of the image when recreating, or remember a per-image "recreated" marker so a size-mismatched file is rebuilt. Deleting the siblings is the smallest change: `QDir(thumbDir).entryList({prefix + "_q*.png"})`, minus the one just written.

### WR-03: `ImageViewBase::event()` reads the viewport's ratio inside the scroll area's own `DevicePixelRatioChange`, which the tests show can lag behind

**File:** `src/core/ImageViewBase.cpp:412-424`
**Issue:** The handler compares `m_hqDpr` with `viewport()->devicePixelRatioF()`. The test helper comment in `src/qt_tests/tst_hidpi_qt.cpp` (`waitForHq`) says "the child learns about a new ratio a little after its top-level parent". The event is delivered to the scroll area first, so at that moment the viewport can still report the old ratio. In that case `qFuzzyCompare` is true, the handler does nothing, and the cancellation of the in-flight task and the pixmap drop never happen from this path. The code stays correct only because `validateHqPixmap()` re-checks the ratio at paint time and a screen change repaints everything, so the failure is a stale, partly dead branch and not a visible bug. The code comment and the `testRepeatedRatioEventKeepsHqPixmap` and `testHqRebuildsOnScreenChange` tests give the impression that the event path does the work. The in-flight task for the old ratio is also not cancelled promptly (`scheduleHqVersionRebuild()` cancels it only when the timer is inactive or the transform differs).

**Fix:** Use the ratio source that matches the event's recipient (`devicePixelRatioF()` of the scroll area, as `StageListView::event()` and `ThumbnailDprTracker` do), or drop the handler and cancel a task with a stale `m_hqDpr` in `scheduleHqVersionRebuild()` / `initiateBuildingHqVersion()`. If the handler stays, add a test that fails when the viewport lags.

## Info

### IN-01: Test-only accessors added to production classes

**File:** `src/core/ImageViewBase.h:83-98`, `src/core/filters/output/PictureZoneEditor.h:49-52`, `src/core/StageListView.h:38-39`
**Issue:** `hqPixmap()`, `hqPixmapPos()`, `hqPixmapIsCurrent()`, `displayedSourcePixelWidthMM()`, `screenPictureMask()`, `screenPictureMaskIsCurrent()` and `batchAnimationFrames()` are public API whose comments say "the application does not call them". The same state is used by `retina_check` and `qt_tests`, so they could be exposed through a `friend` test accessor instead.
**Fix:** Optional. Keep them, but group them under one `// Diagnostics` banner, or move them behind a test-only friend.

### IN-02: Settings tooltip hard-codes a display model and a default

**File:** `src/app/SettingsDialog.ui:110`, `src/core/ApplicationSettings.cpp:25`
**Issue:** The tooltip names the "Studio Display" and the number 250. The default changed from 200 to 250, which also changes the file name (`q250`) of every new thumbnail at ratio 1. The tooltip text and `DEFAULT_THUMBNAIL_QUALITY` must be edited together by hand.
**Fix:** Drop the product name ("on a Retina display the image is built at this size multiplied by the display's pixel ratio"). Optionally set the number in code from `DEFAULT_THUMBNAIL_QUALITY`.

### IN-03: ctest registration still sets `QT_QPA_PLATFORM=offscreen`, which `main()` immediately overrides

**File:** `src/qt_tests/CMakeLists.txt:17`, `src/qt_tests/main.cpp:37-38`
**Issue:** The ctest `ENVIRONMENT` property sets `QT_QPA_PLATFORM=offscreen`, but `main()` replaces it with `offscreen:configfile=...` plus `QT_SCREEN_SCALE_FACTORS`. A reader of the CMake file would think it is the platform in force. `main()` also does not clear `QT_SCALE_FACTOR`, `QT_ENABLE_HIGHDPI_SCALING` and similar variables that `retina_check` clears. A developer shell with `QT_SCALE_FACTOR` set would multiply the ratios and break the DPR assertions.
**Fix:** Remove the platform from the ctest `ENVIRONMENT`, and `qunsetenv` the same scale variables that `retina_check` clears before creating the `QApplication`.

### IN-04: Planning reference inside a source comment

**File:** `src/app/CMakeLists.txt:76-78`
**Issue:** The comment "open review finding WR-01 is about ui_snapshot" points to a planning finding ID that will not exist in the source tree later. `main.cpp` files in `qt_tests` and `retina_check` do the same with "D-00c" and "D-02".
**Fix:** Say what the constraint is ("links core and test_support only, so the GUI sources are not compiled twice") and drop the IDs.

### IN-05: `retina_check` output-path guard is lexical

**File:** `src/app/retina_check/main.cpp:1085-1091`
**Issue:** The "must lie below build/" check uses `QDir::cleanPath` on a joined path, so a symlink inside `build/` that points elsewhere passes. The tool then writes, and calls `removeRecursively()` on `thumbs-run`, below the link target. This is a manual developer tool and the risk is low. `out == root` is also accepted, which would put `tmp/` and `settings/` directly in `build/`.
**Fix:** Compare `QFileInfo(out).canonicalFilePath()` after `mkpath` against the canonical root, and require `out != root`.

---

_Reviewed: 2026-10-25_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
