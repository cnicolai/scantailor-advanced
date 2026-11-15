---
phase: 05-mac-menus-and-shortcuts
plan: 08
subsystem: ui
tags: [qt6, macos, copy, key-names, source-scan, qt-test]

requires:
  - phase: 05-mac-menus-and-shortcuts
    provides: Mac key names in the stage, zone and thumbnail texts (05-03, 05-04) and in the nine apply dialogs (05-07); gate procedure and gate log
provides:
  - Mac key names in the Margins help, the Go To Page tooltip, the navigation status tip and the deskew, spline and drag tips
  - SOURCE_ROOT compile definition on qt_tests and the helpers scanScopeFiles, uiStrings, cppStringLiterals, readUtf8 for source scans
  - TestMacQt::testNoPcKeyNames, a permanent scan that keeps PC key words out of user-visible text
  - TestMacQt::testRewrittenStrings, which pins the rewritten texts of plans 05-03, 05-04, 05-07 and 05-08 exactly
affects: [05-09, 05-11]

actuals:
  tokens: 6800
  tasks: 2
  commits: 4

plan_head_before: 88b96986c0ac78920824cdcb651f020970d63d10
plan_head_after: 96a6362e44e92cbc3197c7d9aa603ad117416023
commits: 4

tech-stack:
  added: []
  patterns:
    - "Source scans in qt_tests read the checked-out tree through the SOURCE_ROOT compile definition"
    - "Scanner helpers are checked on synthetic input inside the scan test, so the scan cannot pass by reading wrongly"

key-files:
  created: []
  modified:
    - src/core/filters/page_layout/OptionsWidget.ui
    - src/app/MainWindow.ui
    - src/core/filters/deskew/ImageView.cpp
    - src/core/interaction/InteractiveXSpline.cpp
    - src/core/interaction/DragHandler.cpp
    - src/qt_tests/CMakeLists.txt
    - src/qt_tests/tst_mac_qt.h
    - src/qt_tests/tst_mac_qt.cpp
    - .planning/phases/05-mac-menus-and-shortcuts/records/gate-log.md

key-decisions:
  - "cppStringLiterals merges adjacent literals into one entry, as the compiler does, so the multi-line zone and content tips are compared whole and a key word split across two literals is still found"
  - "#include lines are skipped by the C++ scanner (header file names are not user-visible text); comments, character literals, digit separators, escaped quotes and raw strings are handled"
  - "testNoPcKeyNames checks the scanners on synthetic .ui and C++ input first (empty string, notr, comment, escaped quote, character literal, raw string), then scans; this stays inside the two slots the plan names"

patterns-established:
  - "A raw-string sample inside a test must not use a delimiter that clang-format treats as C++ (CPP, cpp, cc); it reindents the sample and shifts its line numbers. TXT is safe"

requirements-completed: [MAC-06]

coverage:
  - id: D1
    description: "The Margins help, the Go To Page tooltip, the navigation status tip and the deskew, spline and drag tips name Mac keys (⌘, ⌥, ⇧) in Apple style; no rewritten tip contains a line break"
    requirement: MAC-06
    verification:
      - kind: test
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testRewrittenStrings"
        status: pass
      - kind: integration
        ref: "gates m10c-key-names-ui and m11-key-names-scan: Release and Debug 100% tests passed out of 7, goldens unchanged"
        status: pass
    human_judgment: false
  - id: D2
    description: "No user-visible string in src contains Ctrl, Alt, Control, Cmd, Command, Option, Del, Ins, LMB or Meta as a whole word; the scan reads at least 30 .ui and 500 C++ files and fails if it reads fewer"
    requirement: MAC-06
    verification:
      - kind: test
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testNoPcKeyNames"
        status: pass
    human_judgment: false
  - id: D3
    description: "The Margins help still renders in its 7pt bold styling in the wrapping read-only text field with the new glyphs"
    requirement: MAC-06
    verification:
      - kind: test
        ref: "testRewrittenStrings compares the unescaped HTML spans (7pt bold wrappers kept); nothing was rendered"
        status: unverified
    human_judgment: true
    rationale: "The span markup is pinned, but how the glyphs look in the help field was not rendered or looked at in this plan; it belongs to the closing check (05-11)"

duration: not measured
completed: 2026-11-15
status: complete
---

# Phase 5 Plan 8: Mac key names in the remaining text and a permanent scan Summary

**The Margins help, Go To Page tooltip, navigation status tip and the deskew, spline and drag tips now name ⌘, ⌥ and ⇧, and `testNoPcKeyNames` scans every `.ui` and C++ string of the app so no PC key word can come back; `testRewrittenStrings` pins the exact new texts**

## Performance

- **Duration:** not measured
- **Completed:** 2026-11-15
- **Tasks:** 2
- **Files modified:** 8 source files plus the gate log

## Accomplishments

- Margins help: `‣ ⌥⌘-drag`, `‣ ⇧/⌘-drag` and bold `⇧`, `⌘`, `⇧⌘` in lines 4 and 5; the 7pt bold spans, the escaping and `‣ Double-click` are unchanged (commit `c4594341`).
- Main window: `Go To Page…  (⌘G)` and the navigation status tip with `⇧PgUp/Q`, `⇧PgDown/W`, the fn arrow keys of a MacBook and `⌥-wheel scales thumbnails.` (same commit).
- Deskew tip `Use ⌘-wheel to rotate or ⇧⌘-wheel for finer rotation.`, spline tip `This point can be dragged. Hold ⌘ or ⇧ to drag along axes.`, drag tip `Unrestricted dragging is possible by holding down the ⇧ key.` (commit `a4314bc3`). The code comment at `InteractiveXSpline.cpp:206` and every `ControlModifier` and `AltModifier` branch are untouched.
- `SOURCE_ROOT` compile definition on `qt_tests`; helpers `scanScopeFiles`, `uiStrings`, `cppStringLiterals`, `readUtf8` (plus `uiStringsFromXml`, `cppLiteralsFromText`, `pcKeyWords`, `elided`).
- `testNoPcKeyNames` passes on the real tree with zero hits and fails when a PC key word is put back: a temporary `Ctrl` in `DragHandler.cpp` made both new tests fail with `DragHandler.cpp:22: ... Ctrl key.`, then the file was restored (the test binary reads the sources at run time).
- Gates: `m10c-key-names-ui` command diff 0 / 0; `m11-key-names-scan` command diff 18 / 18, all nine changed `qt_tests` compile lines gain only `-DSOURCE_ROOT=...`. Both `result: PASS`, `goldens: unchanged`, 100% tests passed out of 7 in Release and Debug, tested commits match.
- Output-path guard (convention 14) prints nothing.

## Task Commits

1. **Task 1: Margins help and main window texts** - `c4594341` (fix); gate record `9a126d28` (`m10c-key-names-ui`)
2. **Task 2: C++ tips and the scan** - `a4314bc3` (fix); gate record `96a6362e` (`m11-key-names-scan`)

**Plan metadata:** the SUMMARY commit follows this file.

## Files Created/Modified

- `src/core/filters/page_layout/OptionsWidget.ui`, `src/app/MainWindow.ui`: text only.
- `src/core/filters/deskew/ImageView.cpp`, `src/core/interaction/InteractiveXSpline.cpp`, `src/core/interaction/DragHandler.cpp`: one string literal each.
- `src/qt_tests/CMakeLists.txt`: `SOURCE_ROOT` definition with a comment.
- `src/qt_tests/tst_mac_qt.h`, `src/qt_tests/tst_mac_qt.cpp`: helpers and two slots.
- `records/gate-log.md`: rows `m10c-key-names-ui` and `m11-key-names-scan`.

## Decisions Made

See `key-decisions` above. Scan scope on this tree: the scan passes the lower bounds 30 `.ui` and 500 C++ files; no non-user-visible literal needed an exclusion (zero hits at the first full run).

## Deviations from Plan

None - plan executed exactly as written.

Two notes, not deviations: (1) the plan's `testRewrittenStrings` list names "the three zone tips"; the test pins the vertex, zone-area and creation tips of `ZoneDefaultInteraction.cpp` plus the Select Content tip and the Output tab status tip, all of which already existed from 05-03 and 05-04. (2) The first run of `testNoPcKeyNames` failed on a line number of my own synthetic sample because `clang-format` (Chromium style) reindented a raw string with the delimiter `CPP`; the delimiter was changed to `TXT`, no source behaviour was involved.

## Auth Gates

None.

## Issues Encountered

None in the product code. Build warnings seen (`unused parameter` in `InteractionHandler.h`) are pre-existing and out of scope.

## Known Stubs

None.

## Threat Flags

None. Text edits and a test that reads the checked-out source tree; no new endpoint, auth path or schema.

## Next Phase Readiness

- Plan 05-09 can reuse `scanScopeFiles` and `readUtf8` for its source checks (`testNoDontUseNativeDialog`).
- Open item for the closing check (05-11): the new glyphs in the 7pt bold Margins help field and the shorter dialog hints were argued from markup and string length, not rendered.

## Self-Check: PASSED

- Files found: the six touched source files, `src/qt_tests/CMakeLists.txt`, `tst_mac_qt.{h,cpp}` and `records/gate-log.md` contain the changes; `grep -cF 'Ctrl+Alt+LMB'` on `OptionsWidget.ui` prints 0, `SOURCE_ROOT=` is in `src/qt_tests/CMakeLists.txt`.
- Commits found: `c4594341`, `9a126d28`, `a4314bc3`, `96a6362e`.
- Gates: `m10c-key-names-ui` and `m11-key-names-scan` both `result: PASS`, goldens unchanged, tested commits match their subjects, gate-log rows present.
- `build/scratch/qt-m11.txt` has PASS lines for `testNoPcKeyNames` and `testRewrittenStrings`; all four Qt test classes started, no failure or skip in any Totals line.
- clang-format `--dry-run -Werror` is clean on the five touched C++ files.
