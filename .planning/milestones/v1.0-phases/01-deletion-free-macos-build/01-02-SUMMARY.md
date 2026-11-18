---
phase: 01-deletion-free-macos-build
plan: 02
subsystem: infra
tags: [cmake, ninja, apple-clang, libc++, qt6, boost, build-fixes]

requires:
  - phase: 01-deletion-free-macos-build
    provides: "plan 01-01: Homebrew toolchain, macos-release and macos-debug presets, Clang warning set, frozen -ffp-contract=off"
provides:
  - "Full app and all five test executables built from unmodified 1.2.1 sources in Release and Debug (arm64)"
  - "records/build-attempt-1.txt: raw keep-going output of both builds"
  - "records/fix-log.md: Build attempt 1 section (no error classes), warning ledger, formatting decision, fix-loop result"
affects: [01-03, 01-04, 01-05, 01-06, 01-07, phase-02-baselines]

actuals:
  tokens: 6500
  tasks: 3
  commits: 2

tech-stack:
  added: []
  patterns:
    - "First-build record: keep-going output of every preset, path-normalised, committed before any fix"

key-files:
  created:
    - .planning/phases/01-deletion-free-macos-build/records/build-attempt-1.txt
  modified:
    - .planning/phases/01-deletion-free-macos-build/records/fix-log.md

key-decisions:
  - "No src/ edit was needed: the unmodified 5eaac18 sources compile and link with Apple Clang, libc++, Qt 6.12 and Boost 1.92, so the formatting decision (Task 2) was recorded as not needed and clang-format was not installed"
  - "Warnings (unused-parameter, missing override, Q_OS_MAC deprecated, qAsConst deprecated, reorder-ctor) are recorded only, not fixed (D-08)"

patterns-established:
  - "Phase 1 fix log stays additive: this plan added records only; git diff 5eaac18 -- src is empty"

requirements-completed: [BUILD-01]

coverage:
  - id: D1
    description: "scantailor-advanced and the five test executables build in Release with the macos-release preset (arm64)"
    requirement: BUILD-01
    verification:
      - kind: integration
        ref: "cmake --build --preset macos-release -- -k 0: 434/434, exit status 0; ls of the six executables; file reports Mach-O 64-bit executable arm64"
        status: pass
    human_judgment: false
  - id: D2
    description: "The same targets build in Debug (DEVELOPER_VERSION code included) with the macos-debug preset"
    requirement: BUILD-01
    verification:
      - kind: integration
        ref: "cmake --build --preset macos-debug -- -k 0: 456/456, exit status 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "First full build is on record before any fix, and every file differing from 5eaac18 under src/ or CMakeLists.txt is named in fix-log.md"
    requirement: BUILD-01
    verification:
      - kind: other
        ref: "records/build-attempt-1.txt sections and exit lines; loop over git diff --name-only 5eaac18 -- src CMakeLists.txt prints all-logged; git diff --quiet 5eaac18 -- src exits 0"
        status: pass
    human_judgment: false

duration: 5min
completed: 2026-10-18
status: complete
commits: 2
plan_head_before: cb5284369316dd62acf432581c688c7d99421f02
plan_head_after: 46d777ff7dcae9fada606175ac54ca7aa4f62498
---

# Phase 1 Plan 02: First full build and fix loop Summary

**The unmodified 1.2.1 sources build with no compile or link errors under Apple Clang + libc++, Qt 6.12 and Boost 1.92: release (434 steps) and debug (456 steps) both exit 0 and produce the app and five test executables as arm64, so no source fix and no formatting decision were needed.**

## Performance

- **Duration:** about 5 min (two keep-going builds in the background took most of it)
- **Completed:** 2026-10-18
- **Tasks:** 3 (Task 1 auto, Task 2 decision skipped as not needed, Task 3 auto with an empty fix loop)
- **Files modified:** 1 new record, 1 edited record; no file under `src/` or `CMakeLists.txt`

## Accomplishments

- Task 1: `cmake --build --preset macos-release -- -k 0` and `cmake --build --preset macos-debug -- -k 0` finished with `exit status: 0`. A search of both outputs finds no `error:` and no `FAILED`. The full output (path-normalised) is in `records/build-attempt-1.txt`, and `fix-log.md` has a `## Build attempt 1` section with `No compile or link errors.` and `src/ edits needed: no`. The release tree already held the `foundation` objects from plan 01-01, so its 434 steps are the remaining ones; the debug tree was built from nothing.
- Task 2: skipped by its own rule (`src/ edits needed: no`); `fix-log.md` carries `Formatting decision: not needed (no src/ edits).` clang-format was not installed.
- Task 3: `Fix loop: no fixes needed.` Re-running both preset builds reports `ninja: no work to do`; the six executables exist and `file` reports `Mach-O 64-bit executable arm64` for the release and debug app. The log-coverage loop prints `all-logged` (the files differing from `5eaac18` are only the `CMakeLists.txt` edits of plan 01-01, already in the log), and `git log 5eaac18..HEAD -- src` is empty.
- Warnings are recorded only (D-08): about 1,140 lines per build, 1,125 of them `-Wunused-parameter`; the rest are `-Winconsistent-missing-override` (7), `Q_OS_MAC` deprecated (6, `ZoneContextMenuInteraction`), `-Wreorder-ctor` (1, `page_layout/OptionsWidget.cpp:32`), `qAsConst` deprecated (1, `ContentBoxFinder.cpp:771`) and one linker note about duplicate static libraries.

## Task Commits

1. Task 1: `f5969f5` docs(01-02): record first full build and error classes
2. Task 2: no commit (decision not needed)
3. Task 3: `46d777f` docs(01-02): record compile fixes

## Files Created/Modified

- `.planning/phases/01-deletion-free-macos-build/records/build-attempt-1.txt` - new, raw output of both keep-going builds (`## macos-release`, `## macos-debug`, each with its `exit status:` line)
- `.planning/phases/01-deletion-free-macos-build/records/fix-log.md` - `## Build attempt 1` section, warning ledger, formatting decision and fix-loop lines
- `records/toolchain.txt` and `CMakeLists.txt` are listed in the plan but were not touched: no clang-format install and no Boost define were needed.

## Decisions Made

- The research expectation of Qt 6 arm breakage (Pitfall 5) and Boost 1.92 header changes did not materialise at compile time; the plan's 12-class limit was never in play.
- Warnings stay as they are. The `Q_OS_MAC` deprecation and `qAsConst` notes are candidates for the Phase 3 and 6 cleanup, not for Phase 1.

## Deviations from Plan

None - plan executed exactly as written. Task 2's decision was skipped as the plan itself prescribes when no `src/` edit is needed.

## Authentication Gates

None.

## Issues Encountered

None. The `PropertySet.cpp:25` warning from plan 01-01 did not appear in this build output (the release objects were reused, and the debug compile printed no such warning); it stays listed under "Open errors" in the fix log.

## Known Stubs

None.

## Threat Flags

None. No source or dependency changed; no package was installed.

## Next Phase Readiness

- The built `build/macos-release/scantailor-advanced` and `build/macos-debug/scantailor-advanced` exist for the next plans (tests, ASan, smoke launch).
- The ASan tree (`build/macos-asan`) has been configured but not built in this plan; whichever later plan builds it should expect to find ASan-only or sanitizer-link issues.
- Not run in this plan: the test executables themselves (only built); `ctest` is the next plan's job.

## Self-Check: PASSED

`build-attempt-1.txt` and `fix-log.md` exist; commits `f5969f5` and `46d777f` are ancestors of HEAD; `build/macos-release/` holds all six executables; `git diff --quiet 5eaac18 -- src` exits 0.
