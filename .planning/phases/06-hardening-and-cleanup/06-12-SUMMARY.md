---
phase: 06-hardening-and-cleanup
plan: 12
subsystem: app-save-flow
tags: [cpp, qt, clean-05, save-project-as, qt-test, gate]

requires:
  - phase: 06-hardening-and-cleanup
    provides: "plan 06-01 conventions (gate, labels, output-path guard); plan 06-11 gate-log state"
provides:
  - "src/app/ProjectSaveFlow.{h,cpp}: the one Save Project As flow (saveAs, writeWithFeedback, showSaveError, withProjectExtension, remember)"
  - "OutOfMemoryDialog and MainWindow call it; neither holds a file panel call, a project writer call or the error text any more"
  - "TestMacQt::testProjectExtension, testSaveAsFlowFromOutOfMemoryDialog, testSaveAsFlowRunsTwice"
affects: [06-15]

actuals:
  tokens: 6300
  tasks: 2
  commits: 4

plan_head_before: 0aa754c70cfa500bd8ecc6049f01adc6e147b30a
plan_head_after: f9c724290e85e0964c1f9b73631b38ffdd703581

tech-stack:
  added: []
  patterns:
    - "Non-QObject static helper class with Q_DECLARE_TR_FUNCTIONS for flows shared by a window and a dialog"
    - "Offscreen file-dialog test: two timers answer the modal QFileDialog (name typed into its fileNameEdit, then accept) and the overwrite QMessageBox (Yes); settings under project/ saved and restored by an RAII guard"

key-files:
  created:
    - src/app/ProjectSaveFlow.h
    - src/app/ProjectSaveFlow.cpp
  modified:
    - src/app/CMakeLists.txt
    - src/app/OutOfMemoryDialog.h
    - src/app/OutOfMemoryDialog.cpp
    - src/app/MainWindow.cpp
    - src/qt_tests/tst_mac_qt.h
    - src/qt_tests/tst_mac_qt.cpp
    - .planning/phases/06-hardening-and-cleanup/records/gate-log.md

key-decisions:
  - "ProjectSaveFlow is a class of static functions with Q_DECLARE_TR_FUNCTIONS (no Q_OBJECT), so the new file adds a compile line and an archive member but no automoc change"
  - "saveAs now writes the file, project/lastDir and the recent list; the caller sets its project file and UI afterwards (same synchronous GUI-thread sequence, nothing observable changes)"
  - "The error box has one function, showSaveError, because closeProjectInteractive in MainWindow shows the same text when the backup rename fails"

requirements-completed: []

coverage:
  - id: D1
    description: "One function holds Save Project As: folder from the project file or project/lastDir, native panel with the unchanged filter text, .ScanTailor appended case-insensitively, writer with the error box, then project/lastDir and the recent list; both callers use it"
    requirement: CLEAN-05
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#testProjectExtension and #testSaveAsFlowFromOutOfMemoryDialog"
        status: pass
      - kind: other
        ref: "git grep: getSaveFileName and saveProjectWithFeedback absent from OutOfMemoryDialog.{h,cpp}; 'Error saving the project file!' only in ProjectSaveFlow.cpp"
        status: pass
    human_judgment: false
  - id: D2
    description: "Clicking Save Project As in the out-of-memory dialog and accepting the file dialog with '<tmp>/oom' writes '<tmp>/oom.ScanTailor' that ProjectReader reads back, sets project/lastDir, puts the file first in the recent list and shows the success page; cancelling changes nothing"
    requirement: CLEAN-05
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#testSaveAsFlowFromOutOfMemoryDialog"
        status: pass
    human_judgment: false
  - id: D3
    description: "Saving three times to the same project name overwrites the file (a stale body is replaced), lists the path once and first, and the file dialog asks before replacing when the full name is typed"
    requirement: CLEAN-05
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#testSaveAsFlowRunsTwice"
        status: pass
    human_judgment: false
  - id: D4
    description: "The native panel, the filter text and the shift-command-S shortcut are unchanged; both commits pass the gate with goldens unchanged"
    requirement: CLEAN-05
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#testNoDontUseNativeDialog and #testFileMenuTextsAndKeys (PASS in qt-s15.txt and qt-s16.txt)"
        status: pass
      - kind: other
        ref: "gate.sh check s15-save-as-flow (tested 8c606bac) and s16-save-as-main (tested 590e751f): result PASS, goldens unchanged"
        status: pass
    human_judgment: false
  - id: D5
    description: "The real Mac panel behind actionSaveProjectAs works as before"
    requirement: CLEAN-05
    verification: []
    human_judgment: true
    rationale: "The offscreen platform has no native file panel, so the tests drive Qt's own dialog; the live mac_check run of plan 06-16 covers the native panel (needs the user's go)"

duration: about 50min
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 12: CLEAN-05, one shared Save Project As flow Summary

**`ProjectSaveFlow` now holds the Save Project As flow (native panel, extension, writer with error box, remembered folder, recent list) once; the out-of-memory dialog and the main window both call it, proven offscreen by clicking the dialog's button and answering Qt's file dialog, with cancel and save-three-times cases, and gated in Release and Debug.**

## Performance

- **Duration:** about 50 min of clock time (two builds, two gate runs of about 7 min each, seven mutation builds)
- **Completed:** 2026-11-17
- **Tasks:** 2 (tracer, auto)
- **Files:** 2 created, 6 edited, 1 record appended; 132 lines of source and test added, 90 removed (`MainWindow.cpp` 8 / 37, `OutOfMemoryDialog.cpp` 7 / 44)

## Accomplishments

- `src/app/ProjectSaveFlow.{h,cpp}`: `saveAs` (folder from the project file or `project/lastDir`, `QFileDialog::getSaveFileName` with no option and the unchanged filter text, extension, write, remember), `writeWithFeedback`, `showSaveError`, `withProjectExtension`, `remember`. The `tr` context is now `ProjectSaveFlow`; texts stay English. Registered in `gui_only_sources`.
- `OutOfMemoryDialog`: its own `saveProjectWithFeedback`, panel call, settings code and the duplication comment are gone; `saveProject` calls `writeWithFeedback`, `saveProjectAs` calls `saveAs` and then shows the success page.
- `MainWindow`: `saveProjectAsTriggered` is the same thin form (set the project file, update the title); `saveProjectWithFeedback` is a one-line call of `writeWithFeedback` and its callers are untouched; the rename-failure box in `closeProjectInteractive` calls `showSaveError`.
- `TestMacQt::testProjectExtension` (`a`, `a.ScanTailor`, `a.scantailor`, `a.ScanTailor.bak`, `dir/b.SCANTAILOR`); `testSaveAsFlowFromOutOfMemoryDialog` (save: file exists and `ProjectReader` reads one page, `project/lastDir` equals the folder, the recent list is exactly the file, the stack shows `saveSuccessPage`; cancel: no new file, settings and list unchanged, stack on `mainPage`); `testSaveAsFlowRunsTwice` (answered with `twice` twice and with `twice.ScanTailor` once; a stale body is replaced each time; the recent list stays `[twice, other]`; the third answer meets the "already exists" box, which is answered Yes).
- Mutation runs (uncommitted, restored from byte copies, `cmp` identical): no `project/lastDir` write fails with an empty folder; no extension fails both the extension test and the file-exists check; cancel that remembers a ghost file fails on the recent-list size; no `setMostRecent` fails on list size 0; no success page fails with `mainPage` instead of `saveSuccessPage`; writer skipped when the file exists fails `readsAsProject` after the stale body; duplicated recent entry fails at index 0 of the list.
- Gates: `s15-save-as-flow` (tested `8c606bac`) and `s16-save-as-main` (tested `590e751f`): Release and Debug `100% tests passed out of 7`, goldens unchanged. Command diff of s15: 3 lines per preset, the new `ProjectSaveFlow.cpp` compile line and the `ProjectSaveFlow.cpp.o` member on the `libapp_gui.a` archive line (token comparison: one token added, none removed); no automoc line, because the class has no `Q_OBJECT`. s16: empty.
- `qt_tests` run with the ctest environment: every `Totals:` line says `0 failed, 0 skipped`; PASS lines for the three new slots, `testNoDontUseNativeDialog` and `testFileMenuTextsAndKeys` in `qt-s15.txt` / `qt-s16.txt`. The mtime of the user's real `~/.config/scantailor-advanced/scantailor-advanced.ini` was read before and after the runs and did not change (the tests use `build/macos-release/test-tmp/qt_tests-settings`).

## Task Commits

1. **Task 1 (tracer): shared flow, out-of-memory dialog, offscreen proof** - `8c606bac` (refactor), gated as `s15-save-as-flow`; gate record `d2402db1`
2. **Task 2: main window uses the shared flow; saving three times pinned** - `590e751f` (refactor), gated as `s16-save-as-main`; gate record `f9c72429`

Tracer feedback gate: the tracer `<verify>` carries only automated checks, so it was re-run end to end (qt_tests and the gate) before expansion; it passed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] A third copy of the error text in MainWindow**
- **Found during:** Task 2 (checking the acceptance criterion that `Error saving the project file!` appears only in `ProjectSaveFlow.cpp`)
- **Issue:** the plan counted two copies of the text. `MainWindow::closeProjectInteractive` shows the same box when `Utils::overwritingRename` fails for the backup file, so the criterion could not hold with only `saveAs` and `writeWithFeedback`.
- **Fix:** added `ProjectSaveFlow::showSaveError(QWidget*)`; `writeWithFeedback` and `closeProjectInteractive` both call it. Behaviour is unchanged (same box, same text, same parent). The header comments avoid the literal text so the criterion's file count is 1.
- **Files modified:** `src/app/ProjectSaveFlow.h`, `src/app/ProjectSaveFlow.cpp` (Task 1), `src/app/MainWindow.cpp` (Task 2)
- **Commit:** `8c606bac`, `590e751f`

**2. [Rule 1 - Bug in the plan's test recipe] `selectFile` does not set the name in a focused dialog**
- **Found during:** Task 1, first run of `testSaveAsFlowFromOutOfMemoryDialog` (it waited in the file dialog until killed)
- **Issue:** `QFileDialog::selectFile` leaves the name edit unchanged while that edit has the focus, which it has in a shown save dialog, so `accept()` found no file name and the dialog stayed open. `accept()` is also protected on `QFileDialog`.
- **Fix:** the test helper types the absolute path into the dialog's `fileNameEdit` and calls `QDialog::accept` through a `QDialog*`. The overwrite box needs its own timer (a timer does not fire again while its own slot waits in a nested loop), so `PanelAnswerer` has two timers.
- **Files modified:** `src/qt_tests/tst_mac_qt.cpp`
- **Commit:** `8c606bac`

**3. [Plan wording] The "overwrite confirmation" appears only for the full name**
- **Found during:** Task 2, designing `testSaveAsFlowRunsTwice`
- **Issue:** the behaviour block says the second save with `<tmp>/twice` meets the overwrite confirmation. Qt's dialog checks whether `twice` exists, not `twice.ScanTailor`, so it does not ask; the extension is appended after the dialog returns. The overwrite itself still happens, as before the refactor.
- **Fix:** the test saves with `twice`, `twice` and `twice.ScanTailor`; the third answer shows the confirmation, which the test requires and accepts. The first two prove the overwrite with a stale body.
- **Files modified:** `src/qt_tests/tst_mac_qt.cpp`
- **Commit:** `590e751f`

**Total deviations:** 3 (1 blocking in the plan's counting of copies, 1 in the test recipe, 1 plan wording). **Impact:** none on behaviour; the added `showSaveError` is the only API addition beyond the plan's interface list.

## Issues Encountered

- A first test run waited in an unanswered file dialog for about 3 minutes (see deviation 2); the processes were stopped and the leftover `qt_tests-*` temporary folders in `build/macos-release/test-tmp` removed.
- The `QWARN` lines "wildcard call disconnects from destroyed signal" in the new tests come from destroying a `StageSequence`'s option widgets; they are warnings, not failures.
- `MainWindow::saveProjectAsTriggered` itself is not driven by a test (a whole `MainWindow` with a project would be needed); it is a three-line caller of the tested flow, and the closing `mac_check` run in plan 06-16 covers `actionSaveProjectAs` live.
- The application binary was not launched and no live harness was run.

## Known Stubs

None.

## Threat Flags

None beyond the plan's threat model. T-06-16 (behaviour drift between the two copies) is mitigated: the bodies moved verbatim, the end-to-end test goes through the dialog, and the native-dialog scan and File-menu key tests still pass. T-06-17 (settings written by the tests) is mitigated: `qt_tests` refuses to run outside `build/<preset>/test-tmp`, the tests restore `project/*` through `ProjectSettingsGuard`, and the real settings file did not change.

## Next Phase Readiness

CLEAN-05 stays open at phase level: plan 06-16 also declares it, so it is marked complete once that plan finishes. Plan 06-13 (CLEAN-01, output digest probe) is next; plan 06-15 (the `MainWindow.cpp` split) now moves one call instead of a copy of the flow.

## Self-Check: PASSED

- `src/app/ProjectSaveFlow.h` and `src/app/ProjectSaveFlow.cpp` exist (pass).
- Commits `8c606bac`, `d2402db1`, `590e751f` and `f9c72429` are ancestors of HEAD; `git rev-list --count` from the recorded base `0aa754c7` gives 4 (pass).
- Both gate summaries say `result: PASS` and `goldens: unchanged`; the tested commits equal the commits with the plan subjects; both gate-log rows exist (pass).
- `getSaveFileName` and `saveProjectWithFeedback` are absent from `OutOfMemoryDialog.{h,cpp}`; `git grep -l -F 'Error saving the project file!' -- src/app` prints only `src/app/ProjectSaveFlow.cpp`; no `this function is duplicated` comment remains; the filter text line is in `ProjectSaveFlow.cpp` (pass).
- Output-path guard prints nothing; all touched C++ files pass `clang-format --dry-run -Werror` (pass).
