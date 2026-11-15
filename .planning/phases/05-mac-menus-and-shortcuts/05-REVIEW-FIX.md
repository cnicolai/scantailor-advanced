---
phase: 05-mac-menus-and-shortcuts
fixed_at: 2026-11-15T00:00:00Z
review_path: .planning/phases/05-mac-menus-and-shortcuts/05-REVIEW.md
iteration: 1
findings_in_scope: 2
fixed: 2
skipped: 0
status: all_fixed
---

# Phase 5: Code Review Fix Report

**Fixed at:** 2026-11-15
**Source review:** .planning/phases/05-mac-menus-and-shortcuts/05-REVIEW.md
**Iteration:** 1

**Summary:**
- Findings in scope: 2
- Fixed: 2
- Skipped: 0

Scope was Critical and Warning (`critical_warning`). There were no Critical findings. IN-01 to IN-06 are out of scope and were not touched.

## Fixed Issues

### WR-01: Zoom actions keep a stale enabled state when the Output tab or its view changes

**Files modified:** `src/app/MainWindow.cpp`, `src/core/filters/output/TabbedImageView.h`, `src/core/filters/output/TabbedImageView.cpp`, `src/qt_tests/tst_mac_qt.h`, `src/qt_tests/tst_mac_qt.cpp`
**Commit:** 7a9c22dc
**Status:** fixed: requires human verification (logic change: when the zoom actions are enabled)
**Applied fix:**
- The "disabled when there is no zoomable view" rule stays (`testZoomDisabledWithoutView` is unchanged and passes). The actions were not switched to always enabled.
- `TabbedImageView` has a new signal `primaryViewChanged()`. It is emitted from `tabChangedSlot` (tab change) and, through a queued signal-to-signal connection, whenever a `DespeckleView` tab changes its current page (`QStackedWidget::currentChanged`).
- Deviation from the review text: the suggested `DespeckleView::imageViewCreated` was not used. The signal exists, but it is emitted after `widget.release()`, so its argument is always null. It is not emitted at all when `initiateDespeckling` removes the image view and the indicator becomes current, which is the "stays enabled on a tab with no view" half of the finding. `currentChanged` covers both directions. The connection is queued because the old image view is still a child of the stack until `removeImageViewWidget()` deletes it, so a direct evaluation would still find it.
- `MainWindow::updateZoomActions()` connects `primaryViewChanged` to itself (`Qt::UniqueConnection`) for the `TabbedImageView` it finds in the main area. It uses `Utils::castOrFindChild`, so a `TabbedImageView` nested in the debug tab widget is found too. The connection lives as long as the tab widget (Qt drops it when the sender dies), so nothing dangles when the main-area widget is replaced. The layout signals (`currentChanged`, `widgetRemoved`) call `updateZoomActions()`, so the hook is installed whether `setImageWidget()` or a test adds the widget.
- New regression test `TestMacQt::testZoomFollowsOutputTabs` (offscreen, real `TabbedImageView` and `DespeckleView`). It checks:
  - a tab without an image view disables both actions, and returning to a tab with one enables them;
  - removing and deleting the image view of the current despeckle tab disables them;
  - adding an image view back enables them;
  - removing and deleting the tab widget leaves them disabled.
  Without the `MainWindow.cpp` change the test fails at the first tab switch (checked by reverting that file), so it detects the defect.

**Verification (run in the main checkout, not in an isolated worktree):**
- `xcrun clang-format --dry-run -Werror` on the five touched files: clean.
- `cmake --build --preset macos-release` and `ctest --preset macos-release`: 7 of 7 passed (foundation_tests, math_tests, imageproc_tests, core_tests, qt_tests, pipeline_tests, dewarping_tests). qt_tests totals: TestCoreQt 8, TestUxQt 7, TestHiDpiQt 28, TestMacQt 28 (was 27), no failures.
- `cmake --build --preset macos-debug` and `ctest --preset macos-debug`: 7 of 7 passed, with TestMacQt 28/28 including `testZoomFollowsOutputTabs`.

### WR-02: `mac_check` keys section does not verify that the production slots were disconnected

**Files modified:** `src/app/mac_check/main.cpp`
**Commit:** 13519fcf
**Status:** fixed: requires human verification (the harness was built but, by instruction, not run live)
**Applied fix:**
- In the watch loop of `runKeysSection`, after the two existing disconnects of each production slot, a wildcard `QObject::disconnect(action, &QAction::triggered, nullptr, nullptr)` is made. It returns true only if some receiver was still connected. This is the public-API way to count receivers (`QObject::receivers` and `isSignalConnected` are protected).
- On a hit the section records `keys_slot_still_connected_<objectName>=yes`, skips `counts.watch()` and `setEnabled(true)` for that action, and after the loop writes `keys_reason` and `keys_verdict=FAIL` and returns false. The check therefore comes before the counter's connection (the `watch` call is the last statement in the loop body) and before any key is sent. The first key is sent only after the loop.
- Deviation from the review text: a plain "no receivers left" check fails on every run. A wildcard check on a real `MainWindow` (offscreen, temporary test, not committed) showed that Qt's own `QMenu` objects (`menuFile`, `menuView`) are connected to `triggered` of every watched action. The code now disconnects each `QMenu` in the window first (they only use the signal to close themselves). I confirmed offscreen that, with this exemption, the wildcard check finds nothing left on all 15 disconnected actions, and that it does return true for the still-connected `actionZoomIn`. Any other receiver, such as a future helper connection, now fails the section closed.
- `homeRedirected()` is not required for `keys`. The fail-closed check is the guard, so the TESTING.md example without the redirect still works.

**Verification:**
- `xcrun clang-format --dry-run -Werror src/app/mac_check/main.cpp`: clean.
- `cmake --build --preset macos-release` (builds the `mac_check` target): built. `ctest --preset macos-release`: 7 of 7 passed (same suites and counts as above).
- `cmake --build --preset macos-debug` and `ctest --preset macos-debug` (run after both commits): 7 of 7 passed, TestMacQt 28/28.
- `mac_check` and `retina_check` were not run (they move the pointer and take focus), and the app was not launched. A human should run `mac_check --sections keys` once to confirm the verdict still reads PASS with the new check.

## Notes

- The work was done in the main checkout, not in an isolated worktree, because the build presets and `build/` directories live there and a worktree has none. All edits and commits went onto the current branch. The `.planning/state.json` change that was already uncommitted was left alone and unstaged.
- Each fix is one commit. REVIEW-FIX.md is not committed.
