---
phase: 05-mac-menus-and-shortcuts
reviewed: 2026-11-15T00:00:00Z
depth: standard
files_reviewed: 33
files_reviewed_list:
  - src/app/CMakeLists.txt
  - src/app/Info.plist
  - src/app/mac_check/CocoaProbe.h
  - src/app/mac_check/CocoaProbe.mm
  - src/app/mac_check/main.cpp
  - src/app/MainWindow.cpp
  - src/app/MainWindow.h
  - src/app/MainWindow.ui
  - src/app/RelinkingDialog.cpp
  - src/core/filters/deskew/ApplyDialog.ui
  - src/core/filters/deskew/ImageView.cpp
  - src/core/filters/fix_orientation/ApplyDialog.ui
  - src/core/filters/output/ApplyColorsDialog.ui
  - src/core/filters/output/ChangeDewarpingDialog.ui
  - src/core/filters/output/ChangeDpiDialog.ui
  - src/core/filters/output/TabbedImageView.cpp
  - src/core/filters/page_layout/ApplyDialog.ui
  - src/core/filters/page_layout/ApplyMarginsDialog.ui
  - src/core/filters/page_layout/OptionsWidget.ui
  - src/core/filters/page_split/SplitModeDialog.ui
  - src/core/filters/select_content/ApplyDialog.ui
  - src/core/filters/select_content/ImageView.cpp
  - src/core/interaction/DragHandler.cpp
  - src/core/interaction/InteractiveXSpline.cpp
  - src/core/zones/ZoneContextMenuInteraction.cpp
  - src/core/zones/ZoneContextMenuInteraction.h
  - src/core/zones/ZoneDefaultInteraction.cpp
  - src/core/zones/ZoneDefaultInteraction.h
  - src/qt_tests/CMakeLists.txt
  - src/qt_tests/main.cpp
  - src/qt_tests/tst_mac_qt.cpp
  - src/qt_tests/tst_mac_qt.h
  - TESTING.md
findings:
  critical: 0
  warning: 2
  info: 6
  total: 8
  necessity: 0
status: issues_found
---

# Phase 5: Code Review Report

**Reviewed:** 2026-11-15
**Depth:** standard
**Files Reviewed:** 33
**Status:** issues_found

## Summary

Reviewed the diff against `913ce783` for the shipped app code (menu structure and roles, shortcut lists, zoom actions, the batch alarm change, the zone and content-box keys, native file dialogs, the removal of the 200 ms context-menu deferral), the CMake restructuring (`app_gui` static library, embedded Info.plist, `mac_check` target), the `TestMacQt` suite and the `mac_check` harness. I built nothing new (the release `qt_tests` binary was up to date) and ran it offscreen: all four suites pass (8, 7, 28 and 27 cases).

The shipped code is in good shape. I traced these and found no defect:

- The `retranslateUi` then `setupMacKeys` order.
- The stage-action enabling in `updateProjectActions`.
- The role assignments and the Tools-menu separator.
- The removed `std::system` alarm path (no shell command is left anywhere in the app).
- Queued `aboutToHide` ordering after the removal of the deferral. Qt hides a Qt-drawn `QMenu` before it emits `triggered`, so the queued slot still runs second. `records/context.md` backs this with 20 of 20 live runs.

No Critical items. The two Warnings are a stale enabled state of the new zoom actions in the Output stage, and a missing safety check in the `mac_check` keys section, which can open a native file panel on the real home folder if a slot disconnect fails. The rest is test-tooling robustness and minor quality.

Necessity (standard depth): the rubric was read; no necessity findings.

## Warnings

### WR-01: Zoom actions keep a stale enabled state when the Output tab or its view changes

**File:** `src/app/MainWindow.cpp:151-152` (connections), `src/app/MainWindow.cpp:1647-1656` (`currentPageView`, `updateZoomActions`)
**Issue:** `updateZoomActions()` runs only on `QStackedLayout::currentChanged` and `widgetRemoved` of the main-area layout. `currentPageView()` is not a function of that layout alone. For the Output stage it resolves through `findPrimaryImageView()` to `TabbedImageView::currentWidget()` and then the first `ImageViewBase` below it. That result changes with no layout signal in two cases:

- The user switches tabs (`QTabWidget::currentChanged`).
- `DespeckleView` swaps its processing indicator for the real image view. This is asynchronous and happens at `DespeckleView.cpp:97` and `:179`.

The visible effect:

- If the Despeckling tab is current while its view is still the indicator, `actionZoomIn` and `actionZoomOut` are computed as disabled. They stay disabled after the image view appears, until some unrelated layout change.
- When that happens, the View menu shows Zoom In and Zoom Out greyed and ⌘+ and ⌘- do nothing, although the page is zoomable.
- The reverse also occurs: they stay enabled on a tab with no view.

`zoomCurrentView()` guards against null, so there is no crash. The tests do not cover it: `testZoomActions` and `testZoomDisabledWithoutView` only add and remove widgets from the main layout.
**Fix:** Re-evaluate on tab change and on view creation, for example:
```cpp
// in setImageWidget() / wherever the main-area widget is installed
if (auto* tabs = qobject_cast<output::TabbedImageView*>(widget)) {
  connect(tabs, &QTabWidget::currentChanged, this, &MainWindow::updateZoomActions);
}
```
and connect the `DespeckleView::imageViewCreated` signal (already emitted) to `updateZoomActions` through the tab widget. Alternatively, leave both actions always enabled and let `zoomCurrentView()` do nothing without a view. That also removes the layout-signal wiring.

### WR-02: `mac_check` keys section does not verify that the production slots were disconnected

**File:** `src/app/mac_check/main.cpp:790-796`
**Issue:** The keys section sends Cmd-N, Cmd-O, Cmd-S, Cmd-Shift-S, Cmd-W, Cmd-comma and Cmd-Q to a real `MainWindow`. It relies on `QObject::disconnect(action, &QAction::triggered, &window, nullptr)` and the matching call with `action` as receiver to cut the production slots. The return values are ignored. If any slot stays connected, a key opens a native Open or Save panel or a modal dialog. For Cmd-O and Cmd-Shift-S that would list the user's real home folder. The `dialogs` and `panels` sections refuse to run without the `HOME` redirect (`homeRedirected()`), and TESTING.md presents that guard as the protection against reaching the home folder, but the keys section has no guard. TESTING.md runs it without the redirect in the first example command. A future connection added with a different receiver, such as a `QObject::connect(action, &QAction::triggered, someHelper, ...)`, would pass unnoticed.
**Fix:** Fail closed. Check that nothing but the watcher is connected, and skip the key if not:
```cpp
if (entry.disconnectProduction) {
  QObject::disconnect(action, &QAction::triggered, &window, nullptr);
  QObject::disconnect(action, &QAction::triggered, action, nullptr);
  if (action->receivers(SIGNAL(triggered(bool))) != 0) {
    summary.add(QString("keys_slot_still_connected_%1").arg(entry.objectName), "yes");
    return false;  // before any key is sent
  }
}
```
Do the check before `counts.watch()` adds the counter's connection. Alternatively, require `homeRedirected(out)` for `keys` as well.

## Info

### IN-01: `TestMacQt::testRightClickOpensZoneMenu` has a dead wait and a stale comment

**File:** `src/qt_tests/tst_mac_qt.cpp:1179-1181`
**Issue:** The comment says "The interaction restores the default one after a deferred check" and the test then does `QTest::qWait(300)`. The deferral was removed in this phase. The only assertion after the wait is `zoneCount == 1`, which the hide cannot change, so the 300 ms wait asserts nothing about restoring the default interaction. `testZoneMenuDismissRestoresDefaultAtOnce` covers the dismissal, but it asserts only that the menu object is gone.
**Fix:** Drop the wait and the comment. Better, assert the effect: after hiding the menu, `QTRY_VERIFY` that pressing Backspace over the zone removes it (the default interaction is back).

### IN-02: `mac_check` verdicts ignore the US-layout alias keys it measures

**File:** `src/app/mac_check/main.cpp:902-925`
**Issue:** The keys section records `keys_zoom_equal_ratio` (Cmd-=) and `keys_thumbs_larger_equal` (Option-Cmd-=). `zoomOk` and `thumbsOk` do not use either value, so a regression of the D-06 and D-07 aliases would not change the verdict. The layout section documents this for `cmd_equal` ("not part of the verdict"). The keys section has no such note and its `keys_verdict` reads PASS regardless. The offscreen `testZoomActions` and `testThumbnailKeys` do cover `Key_Equal`, so coverage exists, but not live.
**Fix:** Either include the two values in the verdict, or add a `summary.add("keys_equal_note", ...)` line stating they are informational, as the layout section does.

### IN-03: Verdicts hard-require the German layout, TESTING.md says "current keyboard layout"

**File:** `src/app/mac_check/main.cpp:600`, `:968`; `TESTING.md` (Live Mac check)
**Issue:** `layout_verdict` and `keys_verdict` include `layoutId.contains("German")`. On any other layout the section ends FAIL and the only hint is `keys_layout_german: no`. The `layout` section has no `*_reason` line at all. TESTING.md says the tool "uses the current keyboard layout (the Phase 5 records were made with the German layout)", which reads as if other layouts work. The `layout` section's `'+'` lookup (`keyCodeFor(..., false, ...)` without a Shift fallback) also only works on layouts that have `+` unshifted.
**Fix:** State in TESTING.md that PASS needs the German layout, and add `layout_reason` and `keys_reason` lines ("layout is not German") when the layout is the only failing clause.

### IN-04: `mac_check` open-panel row waits a fixed 800 ms instead of polling

**File:** `src/app/mac_check/main.cpp:1664-1676`
**Issue:** The `dialogs` section's open-panel probe runs once at 800 ms and takes `sawPanel = modalWindowIsFilePanel()` from that instant. If the native panel needs longer, the probe records no panel, sends Cmd-N to nothing and reports FAIL, and the 4000 ms rescue then closes the real panel. The `panels` section solves the same problem with a polling `step` lambda (`runPanelRow`, line 1735) that waits up to 3000 ms. The two rows are inconsistent and the first one can give false FAILs on a loaded machine.
**Fix:** Reuse the polling pattern: poll `modalWindowIsFilePanel()` every 200 ms up to about 3000 ms, then read the menu bar and send Cmd-N.

### IN-05: Output-directory guard is lexical and allows the build root itself

**File:** `src/app/mac_check/main.cpp:1991-1996`
**Issue:** The guard compares `QDir::cleanPath` strings. A symlink below `build/` that points outside the tree, such as `build/scratch -> ~/Desktop`, passes, and the tool then writes `tmp/`, `settings/` and `summary.txt` there. `out == root` is also accepted, so the output directory can be `build/` itself. The same lexical check is used in `isBelow()` for the home redirect, where `HOME` could equally be a symlink out of the tree. The risk is low (the path is typed by the user), but the guard exists to stop writes outside the checkout.
**Fix:** After `mkpath`, compare `QDir(out).canonicalPath()` with `QDir(root).canonicalPath()` and require a strict descendant (`startsWith(root + "/")` only). Apply the same canonicalisation in `isBelow()`.

### IN-06: Test brittleness and duplication

**File:** `src/qt_tests/tst_mac_qt.cpp:1278-1279`, `:1414`, `:1458`; `src/app/CMakeLists.txt:77`; `src/app/MainWindow.cpp:1665-1670`
**Issue:**
- The source scans assert `cppFileCount >= 500` and `uiFileCount >= 30`. These magic floors fail on any large deletion or re-layout of the tree, and they say little about whether the scan read the right files. The `"system("` marker would also match any future identifier ending in `system(`.
- `app_gui` adds `${CMAKE_CURRENT_BINARY_DIR}/app_gui_autogen/include` by hand. That path is the single-config AUTOUIC layout; a multi-config generator inserts a `<CONFIG>` directory and the include would break. The presets are single-config, so this is latent.
- `emptyImageFrame`, `imageFrameLayout`, `greyImage`, `makeZoneEditor`, `zonePage`, `addRectZone` and the `ZoneWindow` and `ContentWindow` helpers exist twice, in `tst_mac_qt.cpp` and `mac_check/main.cpp`. They already differ slightly and will drift.
- `zoomCurrentView()` is a near copy of `ZoomHandler::zoom()` without its `moveTowardsIdealPosition` branch at zoom 1.0, and it uses `interactionState().captured()` instead of the handler's interaction permitter. The comment says "the step is the one of ZoomHandler", but the behaviour at minimum zoom is not.

**Fix:**
- Keep the scan floors, but derive them from a named constant with a comment, or compare against a `QDirIterator` count taken in the same test.
- Use `$<TARGET_PROPERTY:app_gui,AUTOGEN_BUILD_DIR>` if multi-config is ever wanted.
- Move the shared test helpers into `src/test_support/`.
- Call `ZoomHandler::zoom()` if it can be exposed, or document the divergence.

---

_Reviewed: 2026-11-15_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
