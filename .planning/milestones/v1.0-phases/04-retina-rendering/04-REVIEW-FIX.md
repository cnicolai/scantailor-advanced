---
phase: 04-retina-rendering
fixed_at: 2026-10-25T00:00:00Z
review_path: .planning/phases/04-retina-rendering/04-REVIEW.md
iteration: 1
findings_in_scope: 3
fixed: 3
skipped: 0
status: all_fixed
---

# Phase 4: Code Review Fix Report

**Fixed at:** 2026-10-25
**Source review:** .planning/phases/04-retina-rendering/04-REVIEW.md
**Iteration:** 1

**Summary:**
- Findings in scope: 3 (0 critical, 3 warning; the five IN-xx findings are out of scope and untouched)
- Fixed: 3
- Skipped: 0

**Verification:** Before each commit, `cmake --build` and `ctest` ran for both `macos-release` and `macos-debug`, and both printed `100% tests passed out of 7`. All runs were in the main checkout (`build/macos-release`, `build/macos-debug`), not in an isolated worktree, as the caller instructed (the build directories live in the main checkout). Formatting: `xcrun clang-format -i` then `--dry-run -Werror` on every touched C++ file. No output-generation code changed; all three fixes are thumbnail- or display-side.

## Fixed Issues

### WR-01: `removeLoadedItemsLocked()` resets `m_endOfLoadedItems` to `end()` while non-LOADED items remain, and a display change now reaches it

**Files modified:** `src/core/ThumbnailPixmapCache.cpp`, `src/qt_tests/tst_hidpi_qt.cpp`, `src/qt_tests/tst_hidpi_qt.h`
**Commit:** 68ce944c
**Applied fix:** After the flush, `m_endOfLoadedItems` is set to `m_removeQueue.begin()` (the first remaining non-LOADED item). For an empty queue `begin() == end()`, so the `request()` check still holds.
**Regression test:** `testFlushKeepsLoadedBound` uses only the public API. A cache with `maxCachedPixmaps = 3` holds two LOADED items and one LOAD_FAILED item (a missing image requested through `loadRequest`). A ratio change flushes it, five more pages are loaded with `loadNow`, and the test checks that only the three newest are cached. I confirmed it fails without the fix (Release build: page 3 stays cached, expected LOAD_FAILED) and passes with it. In Debug the old code would also trip the `removeExcessLocked()` assert. The QUEUED and IN_PROGRESS cases from the review are not covered, because the background thread drains them too quickly to hold one deterministically without new test seams; the fix is the same single marker for all non-LOADED kinds.
**Status note:** logic fix, so the marker semantics deserve a human look (the test covers the LOAD_FAILED path).

### WR-02: `recreateThumbnail()` refreshes only the thumbnail file of the current ratio, and drops its in-memory invalidation when the generation changed

**Files modified:** `src/core/ThumbnailPixmapCache.cpp`, `src/core/ThumbnailPixmapCache.h` (doc comment), `src/qt_tests/tst_hidpi_qt.cpp`, `src/qt_tests/tst_hidpi_qt.h`
**Commit:** 00fc131e
**Applied fix:** `recreateThumbnail()` now loops: read thumb dir, effective size and generation under the mutex, unlock, write the file, delete the image's other `<prefix>_q<digits>.png` siblings (new file-local helper `removeOtherSizeThumbnails()`, exact prefix and all-digit width match, no wildcard globbing), then relock. If the generation changed meanwhile, it writes again for the new size; once stable it invalidates the in-memory item as before. All file I/O happens outside the cache mutex. The header comment of `recreateThumbnail()` now states that other-size files are deleted. Output images are not affected.
**Regression test:** `testRecreateReplacesAllSizes` creates `q250` and `q500` files, replaces the page with a portrait one, calls `recreateThumbnail()` at ratio 2, and checks that the `q250` file is gone and that both ratios then load the portrait thumbnail. The race branch (generation change during the write) has no deterministic test.
**Status note:** logic fix, requires human verification.

### WR-03: `ImageViewBase::event()` reads the viewport's ratio inside the scroll area's own `DevicePixelRatioChange`, which the tests show can lag behind

**Files modified:** `src/core/ImageViewBase.cpp`, `src/core/ImageViewBase.h` (doc comment)
**Commit:** 8f28cc37
**Applied fix:** This is the review's second option. `event()` no longer compares ratios and no longer drops the pixmap; on `DevicePixelRatioChange` it only calls `update()`. The paint-time `validateHqPixmap()` stays the source of truth. `scheduleHqVersionRebuild()` now also cancels an in-flight HQ task whose `m_hqDpr` differs from the viewport's current ratio, which closes the "old-ratio task is not cancelled promptly" gap. `testHqRebuildsOnScreenChange` and `testRepeatedRatioEventKeepsHqPixmap` pass unchanged (qt_tests was run four more times in Release without a flake).
**No new test:** the lagging-viewport case needs a long-running in-flight HQ task and control over when the viewport learns its ratio, which the offscreen platform does not give. With the handler no longer relying on the viewport ratio, there is no remaining branch that such a test would pin.
**Status note:** logic fix, requires human verification (best confirmed with `retina_check` on a real two-display setup, which I did not run).

---

_Fixed: 2026-10-25_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
