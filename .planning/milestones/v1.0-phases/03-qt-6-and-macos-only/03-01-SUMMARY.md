---
phase: 03-qt-6-and-macos-only
plan: 01
subsystem: testing
tags: [qt6, offscreen, ui-snapshot, gate, ninja, ctest, cmake]

requires:
  - phase: 02-golden-baselines
    provides: golden baselines at a796c2ae, the 7-suite ctest set, the settings-isolation pattern of qt_tests
provides:
  - gate.sh (modes snapshot and check) that gates every strip commit on both presets with a build-command diff
  - ui_snapshot executable, an offscreen grab of the main window and the Settings dialog with a language-control assertion
  - before screenshots, before-state counts and the log of execution decisions for the later after checks
affects: [03-02, 03-03, 03-04, 03-05, 03-06, 03-07, 03-08, 03-09]

actuals:
  tokens: 6800
  tasks: 2
  commits: 3
plan_head_before: 72e7724eccc39b65f939c3316e1691e5916cc2bb
plan_head_after: 0605da61bca020ab6a1bfff49bc55d336e66f313

tech-stack:
  added: []
  patterns:
    - "Commit first, gate the committed tree, amend until PASS"
    - "Build-command diff keyed by the git hash of the build-input tree"
    - "Offscreen test tool with QSettings redirected below an output directory under build/"

key-files:
  created:
    - .planning/phases/03-qt-6-and-macos-only/gate.sh
    - src/app/ui_snapshot/main.cpp
    - .planning/phases/03-qt-6-and-macos-only/records/gate-log.md
    - .planning/phases/03-qt-6-and-macos-only/records/before-state.md
    - .planning/phases/03-qt-6-and-macos-only/records/execution-decisions.md
    - .planning/phases/03-qt-6-and-macos-only/records/screenshots/before/main-window.png
    - .planning/phases/03-qt-6-and-macos-only/records/screenshots/before/settings-dialog.png
    - .planning/phases/03-qt-6-and-macos-only/records/screenshots/before/texts.txt
  modified:
    - src/app/CMakeLists.txt

key-decisions:
  - "ui_snapshot is a separate executable built under BUILD_TESTS and not registered with ctest, so the suite count stays 7"
  - "The D-15 define will be named ST_QT_COMPILE_VERSION so the criterion-1 search cannot match it"
  - "The profiles path for the README is the measured ~/Library/Application Support/scantailor-advanced/scantailor-advanced/profiles"

requirements-completed: [STRIP-06]

coverage:
  - id: D1
    description: "gate.sh configures, builds and runs ctest for macos-release and macos-debug on a committed tree, checks goldens against a796c2ae, refuses UPDATE_GOLDENS and writes the build-command diff"
    requirement: STRIP-06
    verification:
      - kind: other
        ref: "bash .planning/phases/03-qt-6-and-macos-only/gate.sh check c00-ui-snapshot (summary result: PASS)"
        status: pass
    human_judgment: false
  - id: D2
    description: "ui_snapshot grabs the main window and Settings dialog offscreen, keeps QSettings below the output directory, exits 2 outside build/, and --assert-no-language exits 1 while the Language row exists"
    requirement: STRIP-06
    verification:
      - kind: other
        ref: "build/macos-release/ui_snapshot build/scratch/ui-verify [--assert-no-language]; exit codes 0, 1 and 2 checked"
        status: pass
    human_judgment: false
  - id: D3
    description: "Command diff of c00 against the unmodified tree adds only ui_snapshot lines in both presets, and ctest still lists 7 tests"
    verification:
      - kind: other
        ref: "build/scratch/gate/c00-ui-snapshot/cmds-macos-{release,debug}.diff; ctest --preset macos-release -N"
        status: pass
    human_judgment: false
  - id: D4
    description: "Before-state record, before screenshots and execution decisions exist for the later after checks"
    verification:
      - kind: other
        ref: "records/before-state.md, records/screenshots/before/, records/execution-decisions.md"
        status: pass
    human_judgment: true
    rationale: "Whether the screenshots show what the record claims is a visual judgment; the executor viewed both images and wrote the verdict, a reviewer may re-read them"

duration: 50min
completed: 2026-10-24
status: complete
---

# Phase 3 Plan 01: Gate tooling and offscreen UI snapshot Summary

**Gated strip workflow for Phase 3: gate.sh with a build-command diff, an offscreen `ui_snapshot` tool that photographs the unmodified Settings dialog (Language row present) without touching user settings, and recorded before values for every later after check**

## Performance

- **Duration:** about 50 min (the c00 gate itself took about 5 min)
- **Completed:** 2026-10-24T19:52:00Z
- **Tasks:** 2 (Task 1 tracer, Task 2 auto)
- **Files modified:** 9 (8 created, 1 modified)

## Accomplishments

- `gate.sh` runs configure, build and ctest on macos-release and macos-debug, requires `100% tests passed out of 7` in both, requires the goldens unchanged against a796c2ae, refuses to run with `UPDATE_GOLDENS` set or with uncommitted build inputs, and writes the sorted build-command diff against the previous strip commit.
- `ui_snapshot` (new executable, built under `BUILD_TESTS`, not in ctest) writes `main-window.png`, `settings-dialog.png` and `texts.txt`, keeps its QSettings file below `<out>/settings/`, exits 2 for an output directory outside `build/`, prints `language control present: yes` on the unmodified tree and exits 1 under `--assert-no-language`.
- The harness commit c00 (`8d0105ab`) passed the gate: 7/7 in both presets, goldens unchanged, command diff of 28 lines in each preset, all of them added `ui_snapshot` lines and none removed.
- Before artefacts: screenshots of the unmodified UI, the four D-12 counts (4, 5, 1, 40), the leftover-search counts (54, 60, 54, 249, 574) and the FlatDocumentTestSuite baseline (exit 0, no TOOLCHAIN DIFFERS line), all in `records/before-state.md`.
- Twelve execution decisions (ED-01 to ED-12) recorded with reasons.

## Task Commits

1. **Task 1 (tracer): gate tooling, snapshot harness, c00 through the gate** - `8d0105ab` (test: harness), `54c1b366` (docs: gate tooling, gate record, before screenshots)
2. **Task 2: before-state record and execution decisions** - `0605da61` (docs)

**Plan metadata:** added by the final docs commit of this plan.

## Files Created/Modified

- `.planning/phases/03-qt-6-and-macos-only/gate.sh` - D-11 gate, modes `snapshot` and `check <label>`
- `src/app/ui_snapshot/main.cpp` - offscreen snapshot tool with the language-control assertion
- `src/app/CMakeLists.txt` - `ui_snapshot` target under `BUILD_TESTS` (16 lines appended, none removed)
- `.planning/phases/03-qt-6-and-macos-only/records/gate-log.md` - c00 row
- `.planning/phases/03-qt-6-and-macos-only/records/before-state.md` - before values
- `.planning/phases/03-qt-6-and-macos-only/records/execution-decisions.md` - ED-01 to ED-12
- `.planning/phases/03-qt-6-and-macos-only/records/screenshots/before/` - `main-window.png`, `settings-dialog.png`, `texts.txt`

## Decisions Made

- `ui_snapshot` is a separate executable, not a `qt_tests` case: `MainWindow` and `SettingsDialog` live only in the app sources, and ctest must keep listing 7 tests.
- Later plans use `ST_QT_COMPILE_VERSION` for the D-15 define and the measured profiles path `~/Library/Application Support/scantailor-advanced/scantailor-advanced/profiles` (ED-02, ED-08).
- The full list is in `records/execution-decisions.md`.

## Deviations from Plan

None - plan executed exactly as written. One implementation choice inside the plan's freedom: `QTextStream` keeps its Qt 6 default encoding (UTF-8) instead of an explicit encoding call, so the harness needs no Qt-version-specific include.

## Issues Encountered

- `gsd_run` is not defined as a shell function in the Bash tool, so the "is protected branch" query of the pre-commit assertion fell back to the five-name list; the working branch `gsd/phase-01-deletion-free-macos-build` is not one of them.
- `brew list --versions` prints no `cmake` line: CMake 4.4.4 is installed outside Homebrew formulae. Recorded in `before-state.md` for the README rewrite in plan 03-07.
- The harness run prints a few Qt messages on stderr (missing OpenGL context offscreen, `QObject::disconnect` wildcard notes at teardown). They are harmless and unchanged by this plan.

## Known Stubs

None.

## Threat Flags

None. The only new surface is the test tool; T-03-01 is mitigated as planned (settings redirected before the application object exists, distinct organisation and application names, exit 2 for a settings file or output directory outside the allowed locations, verified by the exit-2 runs and the `settings file:` line).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 03-02 can start with `gate.sh check c01-qt5-cpp` against the reference written by the c00 gate (`build/scratch/gate/ref/`, gitignored, so it exists only in this checkout). If the reference is missing, run `gate.sh snapshot` on the parent commit first.
- The after checks of SC1, SC2, SC4 and D-15 compare with `records/before-state.md`.

## Self-Check: PASSED

- Files created exist: gate.sh, `src/app/ui_snapshot/main.cpp`, the three records and the three screenshot files all found.
- Commits `8d0105ab`, `54c1b366`, `0605da61` are ancestors of HEAD.
- Gate summary of c00: `result: PASS`; all four verify commands of Task 1 and both verify commands of Task 2 passed; goldens unchanged against a796c2ae.

---
*Phase: 03-qt-6-and-macos-only*
*Completed: 2026-10-24*
