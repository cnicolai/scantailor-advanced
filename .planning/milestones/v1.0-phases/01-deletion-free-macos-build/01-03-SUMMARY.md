---
phase: 01-deletion-free-macos-build
plan: 03
subsystem: testing
tags: [ctest, boost-test, qt-test, otool, dyld, apple-clang, warnings, arm64]

requires:
  - phase: 01-deletion-free-macos-build
    provides: "plan 01-01 presets and build settings; plan 01-02 first full build of the unmodified sources"
provides:
  - "Full output of a clean release build and warning counts per flag, unique site and origin (BUILD-05, D-08)"
  - "arm64 and one-copy-per-library evidence for libjpeg, libpng, libtiff in the load commands and at runtime (BUILD-01, BUILD-02)"
  - "ctest and per-case test state of Release and Debug on the first tree that builds (BUILD-04): 4 of 5 tests pass, imageproc_tests fails one case"
  - "qt_tests platform decision: runs as registered under Cocoa, no offscreen preset entry needed"
affects: [01-04, 01-05, 01-06, 01-07, phase-02-baselines]

actuals:
  tokens: 167900
  tasks: 3
  commits: 3

plan_head_before: 1985dcc6012ef94693dcb90c23cfce0cdafb1c00
plan_head_after: 93decb7f88b8e4709c48f7d6122a70617c31e030

tech-stack:
  added: []
  patterns:
    - "ctest records start with the tested commit and every source commit since 5eaac18 matched to its fix-log row"
    - "Warning counts produced by script from the build log, split by origin (project / generated / third-party)"

key-files:
  created:
    - .planning/phases/01-deletion-free-macos-build/records/warnings-release.txt
    - .planning/phases/01-deletion-free-macos-build/records/warning-counts.txt
    - .planning/phases/01-deletion-free-macos-build/records/binary-check.txt
    - .planning/phases/01-deletion-free-macos-build/records/ctest-release.txt
    - .planning/phases/01-deletion-free-macos-build/records/test-cases-release.txt
    - .planning/phases/01-deletion-free-macos-build/records/ctest-debug.txt
    - .planning/phases/01-deletion-free-macos-build/records/test-cases-debug.txt
    - .planning/phases/01-deletion-free-macos-build/records/ctest-summary.md
    - .planning/phases/01-deletion-free-macos-build/deferred-items.md
  modified: []

key-decisions:
  - "No CMakePresets.json change: qt_tests passes under the default Cocoa platform in both presets, so the offscreen environment entry was not added"
  - "The failing imageproc_tests case is recorded as the BUILD-04 baseline, not fixed (D-08); it is listed in deferred-items.md"
  - "Apple's private libPng/libTIFF/libJPEG inside ImageIO.framework are not counted as duplicates: the plan's case-sensitive check finds one image per Homebrew library, and the Apple copies are noted in binary-check.txt"

patterns-established:
  - "Baseline header: line 1 `tested commit: <40-char hash>`, then uncommitted-changes line, source commits since 5eaac18 with fix-log row numbers, then `## ctest output`"

requirements-completed: [BUILD-01, BUILD-02, BUILD-04, BUILD-05]

coverage:
  - id: D1
    description: "scantailor-advanced is a Mach-O 64-bit arm64-only executable"
    requirement: BUILD-01
    verification:
      - kind: integration
        ref: "file build/macos-release/scantailor-advanced; lipo -archs (records/binary-check.txt section 1)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Exactly one libjpeg, libpng and libtiff in the load commands (otool -L) and as loaded images at runtime (DYLD_PRINT_LIBRARIES)"
    requirement: BUILD-02
    verification:
      - kind: integration
        ref: "otool -L | grep -c per library, and DYLD_PRINT_LIBRARIES=1 core_tests --list_content | grep -c /<lib> (records/binary-check.txt sections 2 and 3)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Full warning output of the clean release build and counts per flag, unique sites and origin are committed"
    requirement: BUILD-05
    verification:
      - kind: other
        ref: "grep -E '^total warnings: [0-9]+' records/warning-counts.txt (1143); origin labels project, generated, third-party present"
        status: pass
    human_judgment: false
  - id: D4
    description: "ctest state of all five tests and per-case results of every test executable, Release and Debug, with tested-commit headers matched to fix-log rows"
    requirement: BUILD-04
    verification:
      - kind: integration
        ref: "ctest --preset macos-release -V and macos-debug -V (records/ctest-release.txt, records/ctest-debug.txt); header-check commands of Tasks 2 and 3"
        status: pass
    human_judgment: false
  - id: D5
    description: "The recorded baseline contains a failing test (imageproc_tests, ScaleTestSuite/test_random_image); whether and when to fix it is a project decision"
    requirement: BUILD-04
    verification: []
    human_judgment: true
    rationale: "Cause not examined (D-08 / BUILD-04 forbid fixing here); the user decides whether Phase 2 treats it as a known failure or fixes it"

duration: 25min
completed: 2026-10-23
status: complete
---

# Phase 1 Plan 03: Warning, binary and test baseline Summary

**Starting state of the first complete macOS build, recorded without a source edit: 1143 warnings counted by flag and origin, an arm64 binary with exactly one libjpeg/libpng/libtiff, and 4 of 5 ctest tests passing in Release and Debug with `imageproc_tests` failing `ScaleTestSuite/test_random_image`.**

## Performance

- **Duration:** about 25 min (start time not captured at plan start; estimated from file and commit times)
- **Completed:** 2026-10-23T12:12:25Z (records committed); SUMMARY follows
- **Tasks:** 3
- **Files modified:** 9 created (8 records, `deferred-items.md`), none modified; `src/` and `CMakePresets.json` untouched

## Accomplishments

- Clean release build (`--fresh`, `--clean-first`, 456 steps, exit status 0) with the whole output committed; `warning-counts.txt` is produced by script: 1143 warnings (1125 `-Wunused-parameter`, 7 `-Winconsistent-missing-override`, 6 `-Wdeprecated-pragma`, 1 `-Wreorder-ctor`, 1 `-Wdeprecated-declarations`, 3 linker `ignoring duplicate libraries`), 112 unique sites. Origin: project 1137 occurrences, generated (moc output) 3, third-party 0. The numbers match the build-attempt-1 ledger of plan 01-02.
- Binary check: `Mach-O 64-bit executable arm64`, `lipo -archs` prints `arm64`. `otool -L` shows libtiff 4.7.2, libpng 1.6.59 and jpeg-turbo 3.2.0 once each. At runtime, `DYLD_PRINT_LIBRARIES` on `core_tests` loads one image of each (Homebrew Cellar paths). All 408 `compile_commands.json` entries carry `-Wall -Wextra -Werror=return-type -Wno-unused -ffp-contract=off` and no sanitizer token.
- ctest, Release and Debug alike: `80% tests passed, 1 tests failed out of 5`. core 55/0, foundation 22/0, math 32/0, qt 15/0 (TestCoreQt 8, TestUxQt 7), imageproc 93/1. Both ctest records start with `tested commit:` and list the four build-setting commits since `5eaac18` (rows 1, 3, 4, 5 of `fix-log.md`).
- qt_tests passes as registered (default Cocoa platform, from the executor's shell) and with `QT_QPA_PLATFORM=offscreen`, in both presets, so the test preset needed no environment entry.

## Task Commits

1. **Task 1: Clean release build, warning counts, architecture and library checks** - `de61f9e` (docs)
2. **Task 2: Release ctest run with per-case results** - `a3c7076` (docs)
3. **Task 3: Debug ctest run, per-case results and qt_tests decision** - `93decb7` (docs)

**Plan metadata:** committed with this SUMMARY (docs: complete plan)

## Files Created/Modified

- `records/warnings-release.txt` - configure and build output of the clean release build
- `records/warning-counts.txt` - total, per-flag, unique-site and origin counts
- `records/binary-check.txt` - architecture, `otool -L`, runtime image counts, compile_commands flag check, full DYLD output appendix
- `records/ctest-release.txt`, `records/ctest-debug.txt` - ctest -V output with tested-commit header
- `records/test-cases-release.txt`, `records/test-cases-debug.txt` - per-case reports of the four Boost.Test executables and both qt_tests runs
- `records/ctest-summary.md` - five-row table for both presets, failed-case list, qt_tests platform decision
- `deferred-items.md` - the failing imageproc case

## Decisions Made

- `CMakePresets.json` stays unchanged: the offscreen branch of the plan's decision rule applies only when the default qt_tests run fails.
- The failing test is recorded as the baseline, not repaired (D-08, BUILD-04 prohibition). Its cause is not diagnosed; the plan does not ask for it.
- The tested commit is a records commit (`de61f9e` for Release, `a3c7076` for Debug) because the records of Task 1 were committed before the tests ran; the header check shows no `src/`, `CMakeLists.txt` or `cmake/` change in between.

## Deviations from Plan

None - plan executed exactly as written.

Two small executor choices inside the plan's latitude, recorded for transparency: the ANSI colour codes of Boost.Test were stripped from the committed ctest and per-case logs (a header line in each record says so; `grep` on the raw output would otherwise be unreliable), and the `tested commit` of each ctest record is HEAD at the time of the run, not the commit the build objects were first compiled from, because no tracked build input changed in between.

## Issues Encountered

- `imageproc_tests` fails in both presets: `src/imageproc/tests/TestScale.cpp:68` and `:70` (`checkScale` at 50x50 and 80x80 compare `scaleToGray()` with `QImage::scaled(..., Qt::SmoothTransformation)` with a tolerance of 1 grey level). Recorded, not fixed. Phase 2 baselines must carry it as a known failure.
- Observation, not a failure: the case-insensitive view of the runtime image list also contains Apple's private `libPng`, `libTIFF` and `libJPEG` inside `ImageIO.framework`, which the app links directly. The plan's criterion (case-sensitive `/libjpeg`, `/libpng`, `/libtiff`) finds one image each; the Apple copies are noted in `binary-check.txt`. The app itself was not launched (D-02 waits for plan 01-05).

## User Setup Required

None - no external service configuration required.

## Known Stubs

None. No source files were created or modified.

## Threat Flags

None. T-01-06 (duplicate image libraries) is mitigated and verified by the Task 1 counts; T-01-07 (record integrity) by captured output, scripted counts and the tested-commit headers.

## Next Phase Readiness

- Plans 01-04 (ASan), 01-06 and 01-07 can use the recorded Release/Debug baseline. Plan 01-07 should add the closing cross-check that no `src/` edit exists (`git diff --stat 5eaac18 -- src` is empty at the end of this plan).
- Open for the user: whether the `imageproc_tests` failure is fixed before the Phase 2 golden baselines or carried as a known failure.

## Self-Check: PASSED

- All eight record files exist on disk; commits `de61f9e`, `a3c7076`, `93decb7` are ancestors of HEAD; `evaluation-scope --plan 01-03 --commits-only` returns the three commits.
- Task verify commands re-run: arm64 `file` output, library counts 1/1/1 (static and runtime), `total warnings: 1143`, both ctest summary lines over 5 tests, both tested-commit header checks ("header ok"), summary table rows >= 5, `## qt_tests platform` present, `CMakePresets.json` parses, `git diff --stat 5eaac18 -- src` prints nothing.

---
*Phase: 01-deletion-free-macos-build*
*Completed: 2026-10-23*
