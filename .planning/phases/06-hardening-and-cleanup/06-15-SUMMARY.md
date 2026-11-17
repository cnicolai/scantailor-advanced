---
phase: 06-hardening-and-cleanup
plan: 15
subsystem: main-window-split
tags: [cpp, qt, clean-02, main-window, pure-move, move-check, gate]

requires:
  - phase: 06-hardening-and-cleanup
    provides: "plan 06-12: ProjectSaveFlow (the MainWindow save code this plan moves); plans 06-13 and 06-14: move_check.py and the cut-script method"
provides:
  - "src/app/MainWindowBatch.cpp: the batch-processing flow of MainWindow (7 member functions)"
  - "src/app/MainWindowActions.cpp: the private MainWindow::setupActions() (the constructor's addAction/connect block) and the action, menu, zoom, page-navigation and stage-switch functions"
  - "src/app/MainWindowProject.cpp: project lifecycle (new, open, close, save, autosave, switch)"
  - "src/app/MainWindowPages.cpp: page insertion and removal"
  - "MainWindow.cpp reduced from 2305 to 1237 lines; records/clean-02.md"
affects: [06-16]

actuals:
  tokens: 17100
  tasks: 3
  commits: 7

plan_head_before: 6993a20d5f872d4adfdcc11aa363823b15f66633
plan_head_after: b5381597450c0db460a1f96861567a62180274d2

tech-stack:
  added: []
  patterns:
    - "Same-class translation units: member definitions move verbatim between .cpp files of one class, SIGNAL()/SLOT() strings and object names stay valid"
    - "Chunk-multiset move check over all new files against the last pre-split commit, with a three-line allowance for the constructor and setupActions, plus a constructor-inlining order proof"

key-files:
  created:
    - src/app/MainWindowBatch.cpp
    - src/app/MainWindowActions.cpp
    - src/app/MainWindowProject.cpp
    - src/app/MainWindowPages.cpp
    - .planning/phases/06-hardening-and-cleanup/records/clean-02.md
  modified:
    - src/app/MainWindow.cpp
    - src/app/MainWindow.h
    - src/app/CMakeLists.txt
    - src/qt_tests/tst_mac_qt.cpp
    - .planning/phases/06-hardening-and-cleanup/records/gate-log.md

key-decisions:
  - "setupActions() takes the constructor block from addAction(actionFirstPage) to the sortOptions connect only: the lambdas after it capture the constructor's local `settings`, and a second local would add a line the move check does not allow; the rest (thumbColumnViewBtn to actionQuit) stays in the constructor"
  - "switchToNewProject and showNewOpenProjectPanel moved to MainWindowProject.cpp as well, because the plan's project list alone left MainWindow.cpp at 1336 lines (limit 1300)"
  - "goToPage(const PageId&) stays in MainWindow.cpp: it is not one of the six page-navigation actions of the plan's list"
  - "PageSelectionProviderImpl and its only user newPageSelectionAccessor stay in MainWindow.cpp, so no internal header is needed"

requirements-completed: []

coverage:
  - id: D1
    description: "MainWindow.cpp is split into five same-class translation units by verbatim moves; chunk multiset of the five files equals MainWindow.cpp at 590e751f (116 original chunks, 117 new with setupActions), only the constructor and setupActions exempt"
    requirement: CLEAN-02
    verification:
      - kind: other
        ref: "build/scratch/move-s22-verify.txt equivalent run (RESULT: PASS); controls with a changed character, a left-out file, a dropped connect line and a dropped function all give RESULT: FAIL"
        status: pass
    human_judgment: false
  - id: D2
    description: "The connection order of the constructor is unchanged: the lines of setupActions() put back at the call give exactly the original constructor (256 lines)"
    requirement: CLEAN-02
    verification:
      - kind: other
        ref: "scratch inlining check (records/clean-02.md, section The setupActions boundary)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Every offscreen qt_tests slot passes after each of the three steps (90 PASS lines, 0 failed, 0 skipped), including the menu, key, native-dialog, PC-key-name and save-as tests; the PASS sets of s20 and s22 are identical"
    requirement: CLEAN-02
    verification:
      - kind: other
        ref: "build/scratch/qt-s20.txt, qt-s21.txt, qt-s22.txt"
        status: pass
    human_judgment: false
  - id: D4
    description: "testBatchAlarmUsesBeep reads the batch-finished block from MainWindowBatch.cpp, keeps its alert-before-beep and single-occurrence assertions and scans every MainWindow*.cpp for shell-command words (at least two files, none empty)"
    requirement: CLEAN-02
    verification:
      - kind: other
        ref: "TestMacQt::testBatchAlarmUsesBeep PASS in s20 to s22"
        status: pass
    human_judgment: false
  - id: D5
    description: "Gates s20-split-batch, s21-split-actions and s22-split-project pass: Release and Debug 100% tests passed out of 7, goldens unchanged, command diff only the new compile lines and archive members"
    requirement: CLEAN-02
    verification:
      - kind: other
        ref: "build/scratch/gate/s20-split-batch, s21-split-actions, s22-split-project summary.txt; records/gate-log.md rows"
        status: pass
    human_judgment: false
  - id: D6
    description: "Menus, shortcuts and the batch flow behave as before in the real application"
    requirement: CLEAN-02
    verification: []
    human_judgment: true
    rationale: "Pure move proven by multiset, order proof and offscreen tests; the batch flow has no behavioural test and the live mac_check run is plan 06-16 after the user's go"

duration: about 40min
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 15: CLEAN-02, MainWindow.cpp split into five translation units Summary

**`MainWindow.cpp` went from 2305 to 1237 lines: the batch flow, the action wiring (with the constructor block as the private `setupActions()`), the project lifecycle and the page insertion and removal moved verbatim into `MainWindowBatch.cpp`, `MainWindowActions.cpp`, `MainWindowProject.cpp` and `MainWindowPages.cpp`, proven by chunk multiset, a constructor-order proof, 90 passing offscreen Qt test slots on every step and three passing Release and Debug gates.**

## Performance

- **Duration:** about 40 min of clock time (three Release builds, three qt_tests runs, three gate runs of about 7 min each)
- **Completed:** 2026-11-17
- **Tasks:** 3 (tracer, auto, auto)
- **Files:** 4 created, 4 source files edited, 2 records edited, 1 record created

## Accomplishments

- A scratch cut script (`build/scratch/s15/cut_s15.py`) copied line ranges of `MainWindow.cpp` at `590e751f` (the s16 commit of plan 06-12, taken from `git log --grep`) into the four new files; no code was retyped. `xcrun clang-format -i` changed no moved line.
- Step 1 (tracer): the seven batch functions moved to `MainWindowBatch.cpp`; `TestMacQt::testBatchAlarmUsesBeep` now reads the batch-finished block from that file and scans every `MainWindow*.cpp` for shell-command words, with a control of at least two non-empty files. Gate `s20-split-batch` (tested `6079f902`) passed.
- Step 2: the constructor's `addAction` and `connect` block became `MainWindow::setupActions()`, called at the same place; with the 19 action, menu, zoom, page-navigation and stage-switch chunks it forms `MainWindowActions.cpp`. Gate `s21-split-actions` (tested `62e21bb9`) passed.
- Step 3: the project lifecycle (15 chunks) and page flows (5 chunks) moved to `MainWindowProject.cpp` and `MainWindowPages.cpp`; `MainWindow.cpp` has 1237 lines (limit 1300). Gate `s22-split-project` (tested `a9d17d1c`) passed.
- Proof: `move_check.py` reports `RESULT: PASS` (116 original chunks, 117 new) with only the constructor and `setupActions` exempt; four positive controls fail as they should. Putting the lines of `setupActions()` back at the call reproduces the original constructor line for line (256 of 256), so the order of `addAction` and `connect` calls is unchanged. `MainWindow.ui` is unchanged.
- `qt_tests` (ctest environment, offscreen): 90 PASS lines and `0 failed, 0 skipped` on each of the three steps; the PASS sets of s20 and s22 are identical.
- `records/clean-02.md` holds the split map, the setupActions boundary, the move-check outputs and controls, the beep-test change, the test results, line counts and gate rows.

## Task Commits

1. **Task 1 (tracer): the batch flow and the beep test** - `6079f902` (refactor, gate `s20-split-batch`) and `adaacabc` (docs, gate record)
2. **Task 2: the action wiring and setupActions** - `62e21bb9` (refactor, gate `s21-split-actions`) and `264eeed8` (docs, gate record)
3. **Task 3: the project and page flows, and the record** - `a9d17d1c` (refactor, gate `s22-split-project`), `0780c637` (docs, gate record) and `b5381597` (docs, `records/clean-02.md`)

Tracer feedback gate: the tracer `<verify>` carries only automated checks, so move check, qt_tests result, gate summary, gate-log row and clang-format were re-run end to end after the gate record commit; they passed, and the expansion tasks followed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The constructor block is not free of locals, so setupActions() takes a shorter block**
- **Found during:** Task 2, step 1
- **Issue:** the block of the plan (`addAction(actionFirstPage)` to `connect(actionQuit, ...)`) contains lambdas that capture the constructor's local `settings`; moving them would need a second local in `setupActions()`, an added line the move check's three allowed lines do not cover.
- **Fix:** as the plan prescribes for this case, the block was shrunk to end at `connect(sortOptions, ...)`, the line before the first `settings` user; the rest stays in the constructor. Recorded in `records/clean-02.md`.
- **Files modified:** `src/app/MainWindow.cpp`, `src/app/MainWindowActions.cpp`
- **Commit:** `62e21bb9`

**2. [Rule 3 - Blocking] The plan's project list leaves MainWindow.cpp above 1300 lines**
- **Found during:** Task 3, step 1 (measured with the cut script before writing)
- **Issue:** moving only the listed project and page functions gave 1336 lines; the plan requires below 1300.
- **Fix:** `switchToNewProject` and `showNewOpenProjectPanel`, the project switch the listed functions end in, moved to `MainWindowProject.cpp` too; `MainWindow.cpp` is 1237 lines.
- **Files modified:** `src/app/MainWindow.cpp`, `src/app/MainWindowProject.cpp`
- **Commit:** `a9d17d1c`

**3. [Plan wording] qt_tests run with the ctest environment**
- **Issue:** the plan's `<verify>` runs `build/macos-release/qt_tests` bare; the run instructions of the dispatch require the ctest environment (`QT_QPA_PLATFORM=offscreen`, `LC_ALL`, `TMPDIR`).
- **Fix:** each run used that environment from the `TestMacQt` working directory; the same PASS and Totals checks were applied.

**4. [Plan wording] The scratch chunk index of plan 06-14 is OutputGenerator-specific**
- **Fix:** a new scratch script `chunk_index_mw.py` produced the MainWindow index; stray `__pycache__` under `records/` was removed (not committed).

**Total deviations:** 4 (2 blocking Rule 3, 2 plan wording). **Impact:** none on behaviour or proof.

## Issues Encountered

- A scripted wait loop was moved to the background by the Bash tool after 180 s; both such loops ended when the gate summary appeared, none outlives this run.
- The application binary was not launched and no live harness (`mac_check`, `retina_check`) was run. Nothing outside the project was read or written.

## Known Stubs

None.

## Threat Flags

None beyond the plan's threat model. T-06-20 (changed connection order or body) is mitigated: chunk multiset equal, constructor-inlining proof equal, 90 offscreen test slots pass on every step, live `mac_check` follows in plan 06-16. T-06-21 (source scan reading a file that no longer holds the code) is mitigated: the beep test reads `MainWindowBatch.cpp`, requires `filterResult` in it and scans every `MainWindow*.cpp` with a file-count control.

## Next Phase Readiness

CLEAN-02 is left open by `requirements.ready-ids` because plan 06-16 also declares it; it closes when 06-16 has its SUMMARY. Plan 06-16 follows with the live `mac_check` confirmation (after the user's go) and the closing check.

## Self-Check: PASSED

- `MainWindowBatch.cpp`, `MainWindowActions.cpp`, `MainWindowProject.cpp`, `MainWindowPages.cpp` and `records/clean-02.md` exist (pass).
- Commits `6079f902`, `adaacabc`, `62e21bb9`, `264eeed8`, `a9d17d1c`, `0780c637` and `b5381597` are ancestors of HEAD; `git rev-list --count` from the recorded base `6993a20d` gives 7 (pass).
- Gate summaries `s20-split-batch`, `s21-split-actions` and `s22-split-project` say `result: PASS` and `goldens: unchanged`; each tested commit equals the commit with the matching subject; the three gate-log rows exist (pass).
- `move_check.py` ends with `RESULT: PASS`; `wc -l src/app/MainWindow.cpp` is 1237; `filterResult` is in `MainWindowBatch.cpp` and not in `MainWindow.cpp`; `setupActions` is defined in `MainWindowActions.cpp` and declared in `MainWindow.h`; `MainWindow.ui` is unchanged; the touched C++ files pass `clang-format --dry-run -Werror` (pass).
