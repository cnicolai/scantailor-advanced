---
phase: 05-mac-menus-and-shortcuts
plan: 11
subsystem: testing
tags: [documentation, live-harness, closing-record, sanitizers, ctest, mac_check]

requires:
  - phase: 05-mac-menus-and-shortcuts
    provides: TestMacQt, app_gui, mac_check sections and the records of plans 05-01 to 05-10
provides:
  - TESTING.md section "Live Mac check" with both mac_check invocations, the TestMacQt row and the app_gui note
  - records/closing-check.md mapping roadmap criteria 1 to 5 and MAC-01 to MAC-11 to tests and records, with the "For the user" list
  - Closing evidence at the final HEAD, both harness invocations PASS, Release and Debug ctest 7 of 7, sanitizer run, static checks
affects: [phase-05-verification, phase-06]

actuals:
  tokens: 8000
  tasks: 2
  commits: 3

plan_head_before: 3d0a3c30
plan_head_after: daab26219b380e14d48d420dc7d62a90c2ca25d7
commits: 3

tech-stack:
  added: []
  patterns:
    - "Closing record in the Phase 4 format: criteria table, requirements table, gate log, static checks with positive controls, sanitizer attribution, user list"

key-files:
  created:
    - .planning/phases/05-mac-menus-and-shortcuts/records/closing-check.md
  modified:
    - TESTING.md
    - .planning/phases/05-mac-menus-and-shortcuts/records/gate-log.md

key-decisions:
  - "TESTING.md states that dialogs and panels without the redirected home end FAIL with 'home not redirected' (exit 1), while identity exits 2, as the code does; the plan text said exit 2 for both"
  - "The one sanitizer report (BinaryImage.cpp:1044, raised by the Phase 4 test helper makeZoneMask) is older code and is recorded, not fixed; no m16-sanitizer-fix commit"

requirements-completed: [MAC-01, MAC-02, MAC-03, MAC-04, MAC-05, MAC-06, MAC-07, MAC-08, MAC-09, MAC-10, MAC-11]

coverage:
  - id: D1
    description: "TESTING.md documents TestMacQt, the app_gui library and both live Mac check invocations with sections, output rule, exit codes and the no-permission, pointer-moves statement"
    requirement: MAC-11
    verification:
      - kind: integration
        ref: "gate m15-docs: 100% tests passed out of 7 in Release and Debug, goldens unchanged, command diff 0 / 0; Task 1 verify command printed TASK1-VERIFY-OK"
        status: pass
    human_judgment: false
  - id: D2
    description: "Both closing harness invocations end with verdict PASS at the final HEAD (menubar, layout, keys, zone, context; dialogs, panels)"
    requirement: MAC-09
    verification:
      - kind: other
        ref: "build/scratch/mac/closing-input/summary.txt and build/scratch/mac/closing-panels/summary.txt: verdict: PASS, exit 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "Release and Debug ctest report 100% tests passed out of 7; qt_tests runs clean under AddressSanitizer and UndefinedBehaviorSanitizer for every Phase 5 file"
    requirement: MAC-01
    verification:
      - kind: integration
        ref: "build/scratch/closing-ctest-release.txt, closing-ctest-debug.txt, closing-asan-ctest.txt, closing-asan-qt-tests.txt"
        status: pass
    human_judgment: false
  - id: D4
    description: "Static invariants: goldens, output generation and filter tasks unchanged, one Objective-C++ file, no forbidden menu-bar or modifier attribute outside the tests"
    requirement: MAC-05
    verification:
      - kind: other
        ref: "build/scratch/closing/static-checks.txt (with positive controls); Task 2 verify commands V1=0 and V2=0"
        status: pass
    human_judgment: false
  - id: D5
    description: "The closing record maps criteria and requirements to evidence and lists what the user should know, including the two unverified residuals (two-finger click, beep)"
    requirement: MAC-08
    verification:
      - kind: other
        ref: ".planning/phases/05-mac-menus-and-shortcuts/records/closing-check.md"
        status: pass
    human_judgment: true
    rationale: "The two-finger click and the beep rest on equivalence and source checks; the rendering of the shorter dialog hints, the Margins help glyphs and the relinking panel title was never observed"

duration: 20min
completed: 2026-11-15
status: complete
---

# Phase 5 Plan 11: Documentation and closing check Summary

**TESTING.md now documents TestMacQt, app_gui and both mac_check invocations; at the final HEAD both live runs PASS, Release and Debug ctest are 7 of 7, qt_tests is clean under ASan and UBSan for all Phase 5 files, and records/closing-check.md maps every criterion and MAC requirement to its evidence**

## Performance

- **Duration:** about 20 min (the start time was not captured; the estimate comes from the commit times and the two gate-sized runs)
- **Completed:** 2026-11-15T11:40Z
- **Tasks:** 2
- **Files modified:** 3 (TESTING.md, gate-log.md, closing-check.md)
- **Live runs made:** 2 (one input run, one dialogs and panels run); neither needed a repeat, interference counters were 0

## Accomplishments

- `TESTING.md`: the `qt_tests` row lists `TestMacQt`, the UI snapshot section says the GUI sources are compiled once into `app_gui`, and the new section `## Live Mac check` gives both commands (the second with the redirected `HOME` and `CFFIXED_USER_HOME`), the section names (`all` leaves out `identity`), the output rule below `build/`, the refusal of `dialogs` and `panels` without the redirected home, the statement that all input is synthesised inside the harness process with no permission and that the pointer moves, the results in `summary.txt`, the exit codes and the path of the Phase 5 records. Each statement was checked against `src/app/mac_check/main.cpp`.
- Closing runs at `42d28934`: input run (menubar, layout, keys, zone, context) `verdict: PASS`; dialogs and panels run with the redirected home `verdict: PASS`; Release and Debug ctest `100% tests passed out of 7`; `macos-asan` build of `qt_tests` and ctest `100% tests passed out of 1`, plus a direct run (Totals 8 + 7 + 28 + 27 passed).
- Sanitizer result: 0 AddressSanitizer errors and one `runtime error:` at `src/imageproc/BinaryImage.cpp:1044`, raised by the Phase 4 test helper `makeZoneMask`; the file is older code that Phase 5 did not change and a known finding, so it is recorded and not fixed. `m16-sanitizer-fix` does not exist and no `fix(05-11)` commit was made.
- Static checks with positive controls: no diff against `f6cc4457` in goldens, `imageproc`, `dewarping`, `OutputGenerator.cpp` or any filter `Task`; goldens equal `a796c2ae`; the only `.mm` file is `CocoaProbe.mm`; the forbidden menu-bar and modifier attributes appear only in `src/qt_tests`; all 21 gate-log rows read 100% of 7 twice and `unchanged`.
- `records/closing-check.md` maps roadmap criteria 1 to 5 and MAC-01 to MAC-11 to tests and records, carries the criterion-3 sentence on D-10's under-pointer reading, the no-physical-input statement and the "For the user" list.

## Task Commits

1. **Task 1: TESTING.md documents TestMacQt, app_gui and the live Mac check** - `0465921d` (docs); gate `m15-docs` PASS, record `42d28934`
2. **Task 2: closing runs and the closing record** - `daab2621` (docs)

**Plan metadata:** the SUMMARY commit follows this file.

## Files Created/Modified

- `TESTING.md` - `TestMacQt` row, `app_gui` sentence, section `## Live Mac check`
- `.planning/phases/05-mac-menus-and-shortcuts/records/gate-log.md` - row `m15-docs`
- `.planning/phases/05-mac-menus-and-shortcuts/records/closing-check.md` - the closing record

## Decisions Made

- The documented exit codes follow the code, not the plan text: `identity` without the redirected home exits 2, while `dialogs` and `panels` without it open nothing, print `home not redirected` and end with a FAIL verdict and exit 1 (05-10 recorded this). TESTING.md says so.
- Sanitizer report in older code is recorded and left alone (Phase 2 rule, same as Phase 4).
- No source file was changed in this plan.

## Deviations from Plan

None - plan executed exactly as written. The plan's wording "exit codes 0 to 3 ... 2 usage error or output path outside build/" matches the documented codes; the home-guard wording above follows the existing harness behaviour and is a precision, not a change.

## Authentication Gates

None.

## Issues Encountered

- The `PLAN_START_TIME` and the plan commit ledger were not captured at the start of this run, so the duration is an estimate and `plan_head_before` is the HEAD before the first plan commit (`3d0a3c30`), read from the git log. `git rev-list --count 3d0a3c30..HEAD` printed 3 at the time the record was committed.
- The harness console shows the existing `QObject::disconnect: wildcard call disconnects from destroyed signal` lines; no test fails on them.

## Known Stubs

None.

## Threat Flags

None. T-05-21 (path leaks) mitigated: `grep -c "$HOME"` on the closing record prints 0, repository paths are `{repo}`. T-05-22 mitigated: the dialogs and panels run used the redirected home and left it empty; the input run opens no panel; the app binary was not launched.

## Open items for the user and orchestrator

From `records/closing-check.md`, "For the user": chord group `⌥⌘` for Output tabs and thumbnails; Accessibility Zoom can take `⌥⌘+` and `⌥⌘-`; unverified residuals two-finger click and beep; the native Settings item reads "Preferences..."; decisions on app name, Help menu, 200 ms delay and fallback bar; `⌘N` is inert under any dialog; the `Files to insert` ProxyModel observation; hints and glyphs never rendered; WR-01 closed by `app_gui`; `.planning/codebase/` predates Phase 3 and Phase 5 (advisory, not refreshed here).

## Next Phase Readiness

- Phase 5 has all 11 plan summaries; the orchestrator can run phase verification, code review and phase completion.
- REQUIREMENTS.md still lists MAC-01 and MAC-09 as unchecked in the traceability table until `requirements.mark-complete` runs for this plan.

## Self-Check: PASSED

- Files present: `TESTING.md` (section `## Live Mac check`), `records/closing-check.md`, `records/gate-log.md` (row `| m15-docs |`).
- Commits `0465921d`, `42d28934`, `daab2621` are ancestors of HEAD.
- `build/scratch/gate/m15-docs/summary.txt` contains `result: PASS` and `goldens: unchanged`; both closing summaries contain `verdict: PASS`; both closing ctest files contain `100% tests passed out of 7`.

---
*Phase: 05-mac-menus-and-shortcuts*
*Completed: 2026-11-15*
