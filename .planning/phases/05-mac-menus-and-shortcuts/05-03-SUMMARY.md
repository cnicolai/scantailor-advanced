---
phase: 05-mac-menus-and-shortcuts
plan: 03
subsystem: ui
tags: [qt6, qaction, shortcuts, view-menu, zoom, qstackedlayout, macos]

requires:
  - phase: 05-mac-menus-and-shortcuts
    provides: app_gui library, TestMacQt helpers, gate log (05-01); German-layout chord measurement, no Help menu (05-02)
provides:
  - View menu (between File and Tools) with the six stages, Zoom In/Out and Larger/Smaller Thumbnails, every action NoRole
  - Cmd-1 to Cmd-6 for the stages with the letters O, E, N, V, K, P kept as second sequences
  - Output tabs on Option-Cmd-1 to Option-Cmd-5 (same commit as the stage keys)
  - Cmd-plus / Cmd-equals / Cmd-minus zoom of the page view, disabled without one
  - Thumbnail keys on Option-Cmd-plus / Option-Cmd-minus with new toolbar tooltips
affects: [05-04, 05-05, 05-07, 05-08, 05-11]

actuals:
  tokens: 7923
  tasks: 2
  commits: 4

plan_head_before: f839849da064a1412a97b530fcabcf77879aa5ff
plan_head_after: 6ef142c06fa9c7d8136cbb35def75046f7e901a9
commits: 4

tech-stack:
  added: []
  patterns:
    - "Multi-sequence shortcut lists live in MainWindow::setupMacKeys(), called after setupUi() and after retranslateUi()"
    - "The stacked main-area layout signals (currentChanged, widgetRemoved) drive updateZoomActions(), so any widget that enters or leaves the area updates the zoom actions"

key-files:
  created: []
  modified:
    - src/app/MainWindow.ui
    - src/app/MainWindow.h
    - src/app/MainWindow.cpp
    - src/core/filters/output/TabbedImageView.cpp
    - src/qt_tests/tst_mac_qt.h
    - src/qt_tests/tst_mac_qt.cpp
    - .planning/phases/05-mac-menus-and-shortcuts/records/gate-log.md

key-decisions:
  - "Option-Command is bound for both the Output tabs and the thumbnail keys (layout.md: output_tabs_group and thumbnails_group are option-command); no Control-Command code exists"
  - "The six stage actions are disabled while no project is loaded (UI-SPEC E3); the letter keys of D-03 stay bound to the same actions"
  - "zoomCurrentView takes the focus from QCursor::pos() when the cursor is over the view, else the view centre; it does nothing while the view interaction is captured"

patterns-established:
  - "Test mechanics for the main area: imageFrameLayout() and emptyImageFrame() hide and release the placeholder widgets so a test can put its own view in"

requirements-completed: [MAC-04, MAC-05, MAC-06]

coverage:
  - id: D1
    description: "A View menu between File and Tools lists the six stages (texts equal Filter::getName()), zoom and thumbnail items in the specified order, all NoRole"
    requirement: MAC-05
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testViewMenuStages, #testViewMenuContents, #testMenuRoles"
        status: pass
    human_judgment: false
  - id: D2
    description: "Cmd-1 to Cmd-6 and the letters trigger each stage action once and select the stage; disabled without a project; the letter types into a line edit while Cmd-1 still switches"
    requirement: MAC-05
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testStageActions"
        status: pass
    human_judgment: false
  - id: D3
    description: "The Output tabs switch with Option-Cmd-1 to 5, Cmd-1 changes no tab and Option-Cmd-n triggers no stage action"
    requirement: MAC-05
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testOutputTabKeys"
        status: pass
      - kind: other
        ref: "records/layout.md output_tabs_group: option-command (live German layout)"
        status: pass
    human_judgment: false
  - id: D4
    description: "Cmd-plus, Cmd-equals and Cmd-minus zoom the page view by 2^(1/6) steps (never below 1.0), do not touch the thumbnails, and are disabled when the main area holds no page view"
    requirement: MAC-04
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testZoomActions, #testZoomDisabledWithoutView"
        status: pass
    human_judgment: true
    rationale: "The cursor-focus branch of zoomCurrentView (cursor over the view) is not exercised offscreen; the tests cover the view-centre branch. Whether the zoom feels right under the cursor is a manual check."
  - id: D5
    description: "Larger and Smaller Thumbnails use Option-Cmd-plus/equals and Option-Cmd-minus; the toolbar tooltips name the keys; the plain keys of ZoomHandler are unchanged"
    requirement: MAC-06
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testThumbnailKeys"
        status: pass
      - kind: integration
        ref: "gates m04-stage-keys and m05-zoom-keys: 100% tests passed out of 7 in Release and Debug, goldens unchanged, command diff 0 / 0"
        status: pass
    human_judgment: true
    rationale: "With 'Use keyboard shortcuts to zoom' on in Accessibility Zoom, macOS takes Option-Cmd-plus and minus before the app sees them (layout.md); the harness cannot see that setting"

duration: 15min
completed: 2026-11-14
status: complete
---

# Phase 5 Plan 3: View menu, stage keys and zoom keys Summary

**A View menu names the six stages and the zoom and thumbnail commands; Cmd-1 to Cmd-6 switch stages (letters kept), the Output tabs and thumbnail keys moved to Option-Cmd, and Cmd-plus/equals/minus zoom the page view through MainWindow::zoomCurrentView**

## Performance

- **Duration:** 15 min (measured, from the start-time file)
- **Started:** 2026-11-14T23:10:06Z
- **Completed:** 2026-11-14T23:25Z
- **Tasks:** 2
- **Files modified:** 7 (six under `src/`, the gate log)

## Accomplishments

- `menuView` sits between File and Tools: Fix Orientation, Split Pages, Deskew, Select Content, Margins, Output, separator, Zoom In, Zoom Out, separator, Larger Thumbnails, Smaller Thumbnails. The stage texts equal the `getName()` of the six filters (checked against a `StageSequence`), the tooltips read `Switch to <name>`, every View action has `NoRole`.
- `MainWindow::setupMacKeys()` gives each stage action the list {Cmd digit, letter}, so menus draw the Command digit and the letter still works; it also sets the zoom lists ({Cmd-plus, Cmd-equals}, {Cmd-minus}) and the thumbnail lists ({Opt-Cmd-plus, Opt-Cmd-equals}, {Opt-Cmd-minus}). It runs after `setupUi` and after `retranslateUi`.
- The six stage actions follow `isProjectLoaded()` in `updateProjectActions()`; with a project both sequences trigger the action exactly once per press.
- `TabbedImageView` now binds `Qt::CTRL | Qt::ALT | Qt::Key_1..5` and its status tip reads "Use ⌥⌘1 to ⌥⌘5 to switch the tabs." The change is in the same commit that gives the stage actions Cmd-1 to Cmd-6, so no commit holds two owners of Cmd-1 to Cmd-5.
- Zoom: `currentPageView()` resolves the primary `ImageViewBase` (current Output tab included); `zoomCurrentView` multiplies the zoom by 2^(±1/6), never below 1.0, around the cursor (inside the view) or the view centre, and stays out while the view interaction is captured. `updateZoomActions()` enables the two actions only with a page view, driven by the stacked layout's `currentChanged` and `widgetRemoved`.
- Toolbar tooltips: "Larger thumbnails.  (⌥⌘+)" and "Smaller thumbnails.  (⌥⌘-)". `ZoomHandler.cpp` is untouched; the plain plus key still zooms.
- Seven new `TestMacQt` tests (`testStageActions`, `testViewMenuStages`, `testOutputTabKeys`, `testZoomActions`, `testZoomDisabledWithoutView`, `testThumbnailKeys`, `testViewMenuContents`); `qt_tests` now runs 16 TestMacQt tests, all passing.

## Task Commits

1. **Task 1: stage keys, View menu, Output tabs on Option-Cmd** - `30b8af67` (feat); gate record `84fe4b7c`
2. **Task 2: View zoom keys and thumbnail keys on Option-Cmd** - `f80454cf` (feat); gate record `6ef142c0`

**Plan metadata:** the SUMMARY commit follows this file (docs: complete plan).

## Files Created/Modified

- `src/app/MainWindow.ui` - `menuView`, `actionZoomIn`, `actionZoomOut`, renamed stage and thumbnail actions, new toolbar tooltips
- `src/app/MainWindow.h`, `MainWindow.cpp` - `setupMacKeys`, `currentPageView`, `zoomCurrentView`, `zoomCurrentViewIn/Out`, `updateZoomActions`, stage action enabling
- `src/core/filters/output/TabbedImageView.cpp` - tab shortcuts and status tip on Option-Cmd
- `src/qt_tests/tst_mac_qt.h`, `tst_mac_qt.cpp` - seven tests, helpers `EmptySelectionProvider`, `imageFrameLayout`, `emptyImageFrame`, `greyImage`, `stageList`, `selectedStageRow`
- `records/gate-log.md` - rows `m04-stage-keys`, `m05-zoom-keys`

## Decisions Made

- Option-Command for both groups, as measured on the German layout (layout.md); the Control-Command fallback was not needed and is not in the code.
- Stage actions disabled without a project (orchestrator decision on D-03 vs UI-SPEC E3, written into the plan); the letters stay bound.
- Cursor-over-view zoom reads `QCursor::pos()` directly instead of ZoomHandler's internal mouse position, which stays (0, 0) until the first move.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The status bar is reached through findChild in the test**
- **Found during:** Task 1 (first build of the tests)
- **Issue:** `window->statusBar()` is ambiguous in `MainWindow` (`QMainWindow::statusBar()` against the `Ui::MainWindow::statusBar` member), so the line-edit focus test did not compile.
- **Fix:** the test uses `window->findChild<QStatusBar*>("statusBar")->addWidget(edit)`.
- **Files modified:** `src/qt_tests/tst_mac_qt.cpp`
- **Verification:** `testStageActions` passes
- **Committed in:** `30b8af67`

**2. [Adaptation to the current tree] No Help menu**
- The plan allows `menuHelp` "if it still exists". Plan 05-02 removed it, so the menu bar order is File, View, Tools; `testViewMenuStages` asserts exactly that.

---

**Total deviations:** 1 auto-fixed (blocking, test only), 1 adaptation to the tree
**Impact on plan:** none on scope. `src/app/MainWindow.ui` is also committed with each task although the plan's Task 1 commit sentence says "six code files"; the `.ui` file is in the task's `files` list.

## Issues Encountered

- None blocking. Both gates passed on the first run.
- **Open points for the user (manual, not testable offscreen):** (1) with the cursor over the page, Cmd-plus should zoom around the cursor; offscreen tests only exercise the view-centre branch. (2) If Option-Cmd-plus and Option-Cmd-minus do nothing in daily use, check "Use keyboard shortcuts to zoom" in System Settings > Accessibility > Zoom (see layout.md).

## User Setup Required

None - no external service configuration required.

## Known Stubs

None.

## Threat Flags

None. T-05-08 (ambiguous shortcuts): Output tab keys and stage keys changed in one commit, and `testOutputTabKeys` and `testThumbnailKeys` show each key reaches one owner. T-05-09 (zoom on the wrong widget): `zoomCurrentView` only uses `findPrimaryImageView`; `testZoomActions` shows no thumbnail action fires.

## Next Phase Readiness

- Plans 05-04 onward can rely on `emptyImageFrame`/`imageFrameLayout` in `tst_mac_qt.cpp` for tests that need a view in the main area.
- Plan 05-08 (key-name scan) should find "Ctrl+1..5" gone from `TabbedImageView.cpp`; other PC key names elsewhere are its job.
- `mac_check` sections of 05-05 can drive Cmd-1 to Cmd-6, Cmd-plus/minus and the Option-Cmd chords live.

## Self-Check: PASSED

- Files present: `src/app/MainWindow.ui`, `MainWindow.h`, `MainWindow.cpp`, `src/core/filters/output/TabbedImageView.cpp`, `src/qt_tests/tst_mac_qt.h`, `tst_mac_qt.cpp`, `records/gate-log.md`.
- Commits `30b8af67`, `84fe4b7c`, `f80454cf`, `6ef142c0` are ancestors of HEAD; `git rev-list --count f839849d..HEAD` printed 4 before this file.
- Gates `m04-stage-keys` and `m05-zoom-keys` record `result: PASS`, `goldens: unchanged`, command diff 0 / 0; output-path guard prints nothing; `git diff f6cc4457 -- src/core/interaction/ZoomHandler.cpp` is empty.

---
*Phase: 05-mac-menus-and-shortcuts*
*Completed: 2026-11-14*
