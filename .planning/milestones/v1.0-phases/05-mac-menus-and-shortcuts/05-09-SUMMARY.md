---
phase: 05-mac-menus-and-shortcuts
plan: 09
subsystem: ui
tags: [qt6, macos, beep, native-dialogs, source-scan, qt-test]

requires:
  - phase: 05-mac-menus-and-shortcuts
    provides: SOURCE_ROOT, scanScopeFiles and readUtf8 source-scan helpers (05-08); gate procedure and gate log
provides:
  - Batch-finished alarm that plays QApplication::beep() and runs no shell command
  - Relinking dialog with the native macOS file and folder panels
  - TestMacQt::testBatchAlarmUsesBeep and TestMacQt::testNoDontUseNativeDialog, permanent source checks
affects: [05-10, 05-11]

actuals:
  tokens: 2600
  tasks: 2
  commits: 4

plan_head_before: 8ebd4140aa31fd2a3d6e0c3b07b3e19eb97cab1a
plan_head_after: 226da90febfeca0e7e2350ffb235abd5543760de
commits: 4

tech-stack:
  added: []
  patterns:
    - "Every source scan carries a positive control on synthetic text, so a 'must find nothing' check cannot pass by searching for the wrong word"

key-files:
  created: []
  modified:
    - src/app/MainWindow.cpp
    - src/app/RelinkingDialog.cpp
    - src/qt_tests/tst_mac_qt.h
    - src/qt_tests/tst_mac_qt.cpp
    - .planning/phases/05-mac-menus-and-shortcuts/records/gate-log.md

key-decisions:
  - "The MainWindow comment on the beep describes the old setting without naming its key, because the test (and the acceptance count) requires that MainWindow.cpp contains no external_alarm_cmd; the stored value stays in the settings file and is simply not read (D-00d)"
  - "Both new scans also read .mm files of the scan scope, so an Objective-C++ source cannot hide a shell call or a forced non-native dialog"

patterns-established:
  - "Source checks run on the checked-out tree and were proven to fail by temporarily appending the forbidden word to the real file, then restoring it"

requirements-completed: [MAC-10, MAC-11]

coverage:
  - id: D1
    description: "A finished batch run calls QApplication::beep() once after QApplication::alert(this) when Beep when finished is checked; MainWindow.cpp has no shell-command path and no read of the alarm setting; no C++ file of the app contains std::system, popen( or QProcess"
    requirement: MAC-10
    verification:
      - kind: test
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testBatchAlarmUsesBeep"
        status: pass
      - kind: integration
        ref: "gate m12-beep: Release and Debug 100% tests passed out of 7, goldens unchanged, command diff 0 / 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "The sound itself is the system alert sound of NSBeep; whether it is audible was not observed (no key pressed, no sound heard, nothing launched)"
    requirement: MAC-10
    verification:
      - kind: manual
        ref: "closing check of 05-11 (D-11: the beep was not heard in this plan)"
        status: unverified
    human_judgment: true
    rationale: "A beep cannot be observed in-process; the source check proves the call, a person has to hear it"
  - id: D3
    description: "RelinkingDialog opens the file and folder choosers without an option that forces Qt's own dialog; a cancelled panel still returns early through the unchanged empty-path check; no file dialog in src forces the non-native dialog and nothing turns off the native menu bar or swaps Ctrl and Meta"
    requirement: MAC-11
    verification:
      - kind: test
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testNoDontUseNativeDialog"
        status: pass
      - kind: integration
        ref: "gate m13-native-relink: Release and Debug 100% tests passed out of 7, goldens unchanged, command diff 0 / 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "The native panels actually appear, show files greyed in the folder panel, and return a chosen path on macOS"
    requirement: MAC-11
    verification:
      - kind: manual
        ref: "live panel check of plan 05-10"
        status: unverified
    human_judgment: true
    rationale: "No native file panel was opened in this plan (offscreen tests only); plan 05-10 measures it"

duration: not measured
completed: 2026-11-15
status: complete
---

# Phase 5 Plan 9: System beep and native relinking panels Summary

**The batch-finished alarm now calls `QApplication::beep()` instead of running a shell command from a settings value, relinking uses the native macOS file and folder panels, and `testBatchAlarmUsesBeep` and `testNoDontUseNativeDialog` pin both by scanning the app sources**

## Performance

- **Duration:** not measured
- **Completed:** 2026-11-15
- **Tasks:** 2
- **Files modified:** 4 source files plus the gate log

## Accomplishments

- `MainWindow::filterResult`: after `stopBatchProcessing()` the block keeps `QApplication::alert(this);` and, when "Beep when finished" is checked, calls `QApplication::beep();` once. The `QSettings` local, the `play /usr/share/sounds/...` default and the `std::system` call are gone; a stored `main_window/external_alarm_cmd` is no longer read and is left in the settings file (D-00d). A stopped batch run still returns before this block, so it stays silent.
- `testBatchAlarmUsesBeep`: checks the scanner on synthetic text first, then requires that `MainWindow.cpp` has none of `std::system`, `system(`, `QProcess`, `popen`, `/usr/share/sounds`, `external_alarm_cmd`; that the block from `m_batchQueue->allProcessed()` to the next `return;` holds `QApplication::alert(this);` once, `m_checkBeepWhenFinished()` once and `QApplication::beep();` once, the beep after the alert; and that no `.cpp`, `.h` or `.mm` file of the scan scope (at least 500 read) contains `std::system`, `popen(` or `QProcess`. `src/test_support/Manifest.cpp` (git via `QProcess`) is outside the scope, as planned.
- `RelinkingDialog::pathButtonClicked`: `getOpenFileName(this, title, startDir)` and `getExistingDirectory(this, title, startDir)` without an options argument; the four-line comment about non-native dialogs became one line about the native panels. `RelinkablePath::normalize` and both early returns are unchanged, so a cancelled panel changes nothing.
- `testNoDontUseNativeDialog`: control on synthetic text, then no scope file contains `DontUseNativeDialog`, `AA_DontUseNativeDialogs`, `AA_DontUseNativeMenuBar`, `setNativeMenuBar(false)` or `AA_MacDontSwapCtrlAndMeta`; `RelinkingDialog.cpp` still calls both choosers.
- Both tests were shown to fail on the real tree: a temporary `// std::system` line in `MainWindow.cpp` failed the beep test, a temporary `// DontUseNativeDialog` in `RelinkingDialog.cpp` failed the dialog test (`RelinkingDialog.cpp` named in the hit list); both files were restored before the next step.
- Gates: `m12-beep` and `m13-native-relink` both `result: PASS`, Release and Debug 100% tests passed out of 7, `goldens: unchanged`, command diff 0 / 0, tested commits match the subjects. Output-path guard (convention 14) prints nothing.

## Task Commits

1. **Task 1: system beep** - `f0ef9114` (fix); gate record `0f799acb` (`m12-beep`)
2. **Task 2: native relinking panels** - `a1a30357` (fix); gate record `226da90f` (`m13-native-relink`)

**Plan metadata:** the SUMMARY commit follows this file.

## Files Created/Modified

- `src/app/MainWindow.cpp`: batch-finished block only.
- `src/app/RelinkingDialog.cpp`: two chooser calls and one comment.
- `src/qt_tests/tst_mac_qt.h`, `src/qt_tests/tst_mac_qt.cpp`: two slots.
- `records/gate-log.md`: rows `m12-beep` and `m13-native-relink`.

## Decisions Made

See `key-decisions` above. `<cstdlib>` was never included explicitly in `MainWindow.cpp`, so no include was removed.

## Deviations from Plan

None - plan executed exactly as written.

One wording note, not a deviation: the plan asks for a code comment that names the stored setting, and also requires `grep -cF 'external_alarm_cmd' src/app/MainWindow.cpp` to print 0. The comment says "an alarm command stored by an earlier version is no longer read" instead.

## Auth Gates

None.

## Issues Encountered

None.

## Known Stubs

None.

## Threat Flags

None. T-05-15 (OS command injection through the alarm setting) is mitigated by removing the code path and pinned by `testBatchAlarmUsesBeep`; T-05-16 is unchanged (the path handling after the panel is untouched). No new endpoint, auth path or schema.

## Next Phase Readiness

- Plan 05-10 can open the native relinking panels in the live harness; nothing about their look or return value was observed here.
- The beep was not heard in this plan (D-11); the closing record of 05-11 states that.

## Self-Check: PASSED

- Files: `src/app/MainWindow.cpp` has `QApplication::beep();` and no `std::system` or `external_alarm_cmd`; `src/app/RelinkingDialog.cpp` has no `DontUseNativeDialog` and still has `getExistingDirectory`; `tst_mac_qt.cpp` has both new slots.
- Commits found: `f0ef9114`, `0f799acb`, `a1a30357`, `226da90f`.
- Gates: `m12-beep` and `m13-native-relink` both `result: PASS`, goldens unchanged, tested commits match their subjects, gate-log rows present.
- `build/scratch/qt-m13.txt` has PASS lines for `testBatchAlarmUsesBeep`, `testNoDontUseNativeDialog`, `testNoPcKeyNames` and `testRewrittenStrings`; all four Qt test classes started, no failure or skip in any Totals line.
- clang-format `--dry-run -Werror` is clean on the four touched C++ files.
