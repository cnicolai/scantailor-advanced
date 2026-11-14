---
phase: 05-mac-menus-and-shortcuts
plan: 01
subsystem: ui
tags: [qt6, qaction, menuRole, macos-menu, qt-test, cmake, static-library]

requires:
  - phase: 03-qt-6-and-macos-only
    provides: gate.sh per-commit gate, ui_snapshot, Qt 6 only build
  - phase: 04-retina-rendering
    provides: qt_tests offscreen two-screen setup, gate-log table layout
provides:
  - CMake static library app_gui (GUI sources without main.cpp), linked by scantailor-advanced, ui_snapshot and qt_tests
  - TestMacQt with seven offscreen tests that construct a real MainWindow
  - Settings in File with PreferencesRole and Ctrl+, (Cmd-comma); About and Quit roles; NoRole on every other top-level menu action
  - Mac menu texts, Shift-Cmd-S for Save As, Tools menu without Settings, Close Project disabled without a project
  - records/gate-log.md of Phase 5
affects: [05-02, 05-03, 05-04, 05-05, 05-06, 05-07, 05-08, 05-09, 05-10, 05-11]

actuals:
  tokens: 5459
  tasks: 2
  commits: 4

plan_head_before: 38e5643a1a1cdbcab8d3b576b2f2a80254054aa1
plan_head_after: a3a18d07d00c324fcf0510273086011b138b4b13
commits: 4

tech-stack:
  added: []
  patterns:
    - "GUI sources as a static library (app_gui); resources (.qrc) stay on executables"
    - "TestMacQt: shown MainWindow, findChild<QAction*> by object name, QTest::keyClick with explicit modifiers"
    - "Disconnect production slots from an action, then spy on triggered, to test a key path without opening panels"

key-files:
  created:
    - src/qt_tests/tst_mac_qt.h
    - src/qt_tests/tst_mac_qt.cpp
    - .planning/phases/05-mac-menus-and-shortcuts/records/gate-log.md
  modified:
    - src/app/CMakeLists.txt
    - src/app/MainWindow.ui
    - src/app/MainWindow.cpp
    - src/qt_tests/CMakeLists.txt
    - src/qt_tests/main.cpp

key-decisions:
  - "Explicit key sequences (Ctrl+, Ctrl+Shift+S) instead of QKeySequence::Preferences/SaveAs, because the StandardKeys are empty on the offscreen platform"
  - "Settings sits between Close Project and Quit in File with no separator, so File never ends on an orphan separator after the roles move items"
  - "Debug Mode separator is inserted in code only under ENABLE_DEBUG_FEATURES, so Tools never ends on a separator in Release"

requirements-completed: [MAC-01, MAC-02, MAC-03, MAC-05]

coverage:
  - id: D1
    description: "MainWindow can be built and driven offscreen in qt_tests through the shared app_gui library (WR-01 closed: the GUI is compiled once)"
    requirement: MAC-01
    verification:
      - kind: integration
        ref: "ctest --preset macos-release (gate m01-tracer, m02-menu-roles: 100% tests passed out of 7)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Settings is a direct child of File with PreferencesRole, text Settings…, shortcut Ctrl+, (native text ⌘,); the shortcut opens SettingsDialog"
    requirement: MAC-02
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testSettingsInFileWithPreferencesRole"
        status: pass
      - kind: automated_ui
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testSettingsShortcutOpensSettings"
        status: pass
    human_judgment: false
  - id: D3
    description: "About, Settings and Quit carry explicit roles; every other direct child of a top-level menu has NoRole"
    requirement: MAC-01
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testMenuRoles"
        status: pass
    human_judgment: true
    rationale: "Offscreen tests prove the roles and placement; whether the native macOS application menu shows each item exactly once is measured by the live harness of plan 05-02"
  - id: D4
    description: "File and Tools menus read as UI-SPEC specifies; ⇧⌘S is Save As; the file shortcuts fire exactly once per press"
    requirement: MAC-03
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testFileMenuTextsAndKeys"
        status: pass
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testToolsMenuWithoutSettings"
        status: pass
    human_judgment: false
  - id: D5
    description: "Quit closes the window through close() with no prompt and no project; project-bound actions (including Close Project) are disabled without a project"
    requirement: MAC-05
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testQuitClosesWindow"
        status: pass
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testProjectActionsDisabledWithoutProject"
        status: pass
    human_judgment: false

duration: 30min
completed: 2026-11-14
status: complete
---

# Phase 5 Plan 1: Mac application menu and MainWindow tests Summary

**Settings… with ⌘, in the application menu via explicit PreferencesRole/AboutRole/QuitRole (NoRole elsewhere), Mac menu texts and ⇧⌘S, proven by seven offscreen TestMacQt tests on a real MainWindow built from the new `app_gui` static library**

## Performance

- **Duration:** about 30 min (the start time was not captured; the estimate comes from the gate runs, about 8 min each)
- **Completed:** 2026-11-14T22:31Z
- **Tasks:** 2
- **Files modified:** 8 (7 under `src/`, plus the gate log)

## Accomplishments

- The GUI sources (all of `src/app` except `main.cpp`) are the static library `app_gui`. `scantailor-advanced`, `ui_snapshot` and `qt_tests` link it, so the GUI compiles once (Phase 3 review finding WR-01 closed). `qt_tests` carries the three `.qrc` files itself.
- `TestMacQt` (7 tests) builds a shown `MainWindow` offscreen: Settings placement, role, key and ⌘, end to end (opens `SettingsDialog`), role audit of every top-level menu action, File menu texts and keys (each key fires exactly once, ⇧⌘S twice after two presses), Tools menu contents in Release and Debug, Quit through `close()`, and the empty-project state.
- `MainWindow.ui`: Settings moved from Tools to File (after Close Project, before Quit, no orphan separator); `menuRole` on 13 actions; menu texts with a single `…` and Title Case; Save As gets `Ctrl+Shift+S`; Tools is Fix DPI…, Relinking…, Reverse Two-Page Spread Order, separator, Default Parameters…, Units, then Debug Mode (with its own separator inserted in code under `ENABLE_DEBUG_FEATURES`).
- `MainWindow::updateProjectActions()` also enables Close Project only with a project.
- Both code commits passed `gate.sh` (Release and Debug 100% of 7, goldens unchanged against a796c2ae). `records/gate-log.md` is started with both rows.

## Task Commits

1. **Task 1 (tracer): Settings with Cmd-comma in the application menu, MainWindow built inside qt_tests** - `df295683` (feat); gate record `18ab4ffa`
2. **Task 2: explicit roles, Mac File and Tools menus, Shift-Cmd-S, project-bound actions disabled** - `47a22164` (feat); gate record `a3a18d07`

**Plan metadata:** the SUMMARY commit follows this file (docs: complete plan).

## Files Created/Modified

- `src/app/CMakeLists.txt` - `app_gui` static library, executables link it
- `src/app/MainWindow.ui` - roles, Settings in File, menu texts, ⇧⌘S, Tools order
- `src/app/MainWindow.cpp` - Debug Mode separator, Close Project enabled state
- `src/qt_tests/CMakeLists.txt` - `tst_mac_qt.cpp`, the three `.qrc` files, `app_gui` link
- `src/qt_tests/main.cpp` - registers `TestMacQt` after `TestHiDpiQt`
- `src/qt_tests/tst_mac_qt.h`, `src/qt_tests/tst_mac_qt.cpp` - the seven tests and helpers `makeShownMainWindow`, `findAction`, `findMenu`, `visibleEntries`
- `.planning/phases/05-mac-menus-and-shortcuts/records/gate-log.md` - gate rows `m01-tracer`, `m02-menu-roles`

## Decisions Made

- Explicit sequences `Ctrl+,`, `Ctrl+Q`, `Ctrl+Shift+S` instead of the StandardKeys named in UI-SPEC (empty on the offscreen platform; they are what Cocoa resolves to). Plan 05-02's live harness proves the equality on Cocoa.
- Settings sits between Close Project and Quit with no separator, so File does not end on an orphan separator once Quit is hoisted by its role.
- The tracer gate command diff was recorded as the classes the plan names (100 / 100 diff lines; app_gui compile, archive, link, qt_tests additions).

## Deviations from Plan

None - plan executed exactly as written.

Notes that are not deviations: the plan's source text for `actionDefaults` read "Default Parameters ..." but the `.ui` held "Default parameters ..." (lower-case p); the same target text "Default Parameters…" was applied. The gate-log file is committed separately from the code commit as convention 9 says, although the Task 1 file list names it.

## Issues Encountered

- The qt_tests output contains `QObject::disconnect: wildcard call disconnects from destroyed signal` warnings when a MainWindow is destroyed. They come from existing MainWindow teardown code, not from this plan, and no test fails on them.
- The `PLAN_START_TIME` of this run was not captured, so the duration above is an estimate.

## User Setup Required

None - no external service configuration required.

## Known Stubs

None.

## Threat Flags

None. The mitigations T-05-01 (gate on the library move, command diff classified), T-05-02 (qt_tests keeps its settings under `build/<preset>/test-tmp`, the check in `main.cpp` is unchanged) and T-05-03 (NoRole everywhere else, `testMenuRoles`) are in place.

## Next Phase Readiness

- Plan 05-02 can link `app_gui` from the live harness `mac_check` and measure the native application menu, the Help menu and the application name.
- The conventions and the gate log are in place for 05-02 to 05-11.
- Open for 05-02: whether macOS shows the empty Help menu and whether the About item names the process or "ScanTailor Advanced".

## Self-Check: PASSED

- Files present: `src/qt_tests/tst_mac_qt.h`, `src/qt_tests/tst_mac_qt.cpp`, `.planning/phases/05-mac-menus-and-shortcuts/records/gate-log.md`.
- Commits `df295683`, `47a22164`, `18ab4ffa`, `a3a18d07` are ancestors of HEAD.
- `build/scratch/gate/m01-tracer/summary.txt` and `build/scratch/gate/m02-menu-roles/summary.txt` both contain `result: PASS` and `goldens: unchanged`; `ctest --preset macos-release -N` prints `Total Tests: 7`; the output-path guard of convention 14 prints nothing.

---
*Phase: 05-mac-menus-and-shortcuts*
*Completed: 2026-11-14*
