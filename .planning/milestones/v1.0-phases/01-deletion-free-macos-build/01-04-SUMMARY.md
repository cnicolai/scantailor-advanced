---
phase: 01-deletion-free-macos-build
plan: 04
subsystem: testing
tags: [asan, ubsan, sanitizers, apple-clang, ctest, boost-test, qt-test, arm64]

requires:
  - phase: 01-deletion-free-macos-build
    provides: "plan 01-01 SANITIZE option and macos-asan presets; plan 01-02 first full build of the unmodified sources; plan 01-03 Debug baseline to compare with"
provides:
  - "Full output of the ASan+UBSan build of all five test executables, with the ASan runtime in the load commands (BUILD-06)"
  - "ctest and per-executable test output of the sanitizer build, with every sanitizer report in place"
  - "De-duplicated, counted sanitizer findings of the existing tests, recorded and not fixed (success criterion 4): 0 ASan reports, 3 UBSan reports at 2 distinct project locations"
affects: [01-07, phase-02-baselines, phase-06-safety]

actuals:
  tokens: 127054
  tasks: 2
  commits: 2

plan_head_before: c0c129a145ffd63619f7392dd0f362f59534429c
plan_head_after: ca3009fd60ccae216803d3a95235ab27fedb3281

tech-stack:
  added: []
  patterns:
    - "Sanitizer findings table produced by script (python3 -I) from the committed per-executable output, so totals can be re-counted with grep"
    - "Sanitizer output captured with stdout and stderr merged, so each report sits between the Entering/Leaving lines of the test case that raised it"

key-files:
  created:
    - .planning/phases/01-deletion-free-macos-build/records/build-asan.txt
    - .planning/phases/01-deletion-free-macos-build/records/ctest-asan.txt
    - .planning/phases/01-deletion-free-macos-build/records/test-cases-asan.txt
    - .planning/phases/01-deletion-free-macos-build/records/sanitizer-findings.md
  modified: []

key-decisions:
  - "No CMakeLists.txt change: the SANITIZE block from plan 01-01 configured, compiled and linked as committed, so the permitted smallest-change exception was not used"
  - "Sanitizer options exactly as in the preset (detect_leaks=0, halt_on_error=0, print_stacktrace=1); no suppression file and no -fno-sanitize"
  - "Findings counted from test-cases-asan.txt, not ctest-asan.txt, because ctest prints the failing test's output twice (live and via outputOnFailure)"

patterns-established:
  - "UBSan reports each location once per process: the findings record states that counts are distinct reports, not occurrences, and lists only the first test case per location"

requirements-completed: [BUILD-06]

coverage:
  - id: D1
    description: "The macos-asan preset configures and builds all five test executables with -fsanitize=address,undefined in every compile command and links core_tests against libclang_rt.asan"
    requirement: BUILD-06
    verification:
      - kind: integration
        ref: "otool -L build/macos-asan/core_tests | grep libclang_rt.asan; jq over build/macos-asan/compile_commands.json (408 of 408 entries carry -fsanitize=address,undefined) (records/build-asan.txt)"
        status: pass
    human_judgment: false
  - id: D2
    description: "ctest --preset macos-asan runs all five existing tests; full output committed (80% tests passed, 1 tests failed out of 5)"
    requirement: BUILD-06
    verification:
      - kind: integration
        ref: "grep -E '[0-9]+% tests passed, [0-9]+ tests failed out of 5' records/ctest-asan.txt"
        status: pass
    human_judgment: false
  - id: D3
    description: "Every ASan and UBSan report of the five executables is listed in sanitizer-findings.md, de-duplicated by kind and project location, with counts, and not fixed"
    requirement: BUILD-06
    verification:
      - kind: other
        ref: "grep -c 'runtime error:' / 'ERROR: AddressSanitizer' records/test-cases-asan.txt gives 3 / 0, equal to the Totals section; git diff --stat 5eaac18 -- src is empty"
        status: pass
    human_judgment: false
  - id: D4
    description: "The two UBSan findings (BinaryImage.cpp:1044 null-pointer offsetof idiom, SlicedHistogram.cpp:49 left shift of -1) are real defects in the unmodified code that Phase 6 must triage; whether they matter for scans is a project judgment"
    requirement: BUILD-06
    verification: []
    human_judgment: true
    rationale: "Phase 1 records only (success criterion 4); severity and fix priority for SAFE-01 to SAFE-03 are the user's decision"

duration: 5min
completed: 2026-10-23
status: complete
---

# Phase 1 Plan 04: ASan and UBSan build and findings Summary

**The unmodified sources build and run under AddressSanitizer plus UndefinedBehaviorSanitizer through `cmake --preset macos-asan` with no source or CMake edit: 0 ASan reports and 3 UBSan reports at 2 project locations (`BinaryImage.cpp:1044`, `SlicedHistogram.cpp:49`), recorded and not fixed, with the same 4 of 5 ctest result as Debug.**

## Performance

- **Duration:** about 5 min (a 456-step sanitizer build plus six test runs)
- **Started:** 2026-10-23T12:15:09Z
- **Completed:** 2026-10-23T12:20Z (records committed); SUMMARY follows
- **Tasks:** 2
- **Files modified:** 4 records created; `src/` and `CMakeLists.txt` untouched

## Accomplishments

- `cmake --preset macos-asan --fresh` and `cmake --build --preset macos-asan` both exit 0 (456 steps). `otool -L build/macos-asan/core_tests` lists `@rpath/libclang_rt.asan_osx_dynamic.dylib`. All 408 compile commands carry `-fsanitize=address,undefined` and `-fsanitize-recover=address`. All five test executables exist.
- `ctest --preset macos-asan -V`: `80% tests passed, 1 tests failed out of 5`. foundation 22, math 32, core 55 and qt 15 cases pass; imageproc fails `ScaleTestSuite/test_random_image` exactly as in Release and Debug (no sanitizer report is attached to it).
- Sanitizer findings, counted with `grep` on `records/test-cases-asan.txt`: 0 `ERROR: AddressSanitizer`, 3 `runtime error:`.
  - `src/imageproc/BinaryImage.cpp:1044:37`, "member access within null pointer of type 'SharedData'": `BinaryImage::SharedData::operator new` computes the size with `&sd->m_data[0]` on a null `sd` (an `offsetof` idiom). Reported in `imageproc_tests` (`test_from_to_qimage`) and `core_tests` (`ObliqueFinderTestSuite/build_sausage_mask_preserves_size`). It sits in every `BinaryImage` allocation, so every test and every page that makes a binary image passes through it.
  - `src/imageproc/SlicedHistogram.cpp:49:47`, "left shift of negative value -1": `lastWordIdx << 5` with `lastWordIdx = -1` for an empty/null area, reported in `imageproc_tests` (`SlicedHistogramTestSuite/test_null_image`).
- `core_tests` passes while printing one UBSan report, which is the case the findings record's Method section warns about (a ctest PASS is not zero findings).
- No third-party finding: no check fired on Qt, Boost or system code, and the check set was not narrowed.

## Task Commits

1. **Task 1: Configure and build the macos-asan preset** - `8038a24` (docs)
2. **Task 2: Run the tests under ASan/UBSan and record de-duplicated findings** - `ca3009f` (docs)

**Plan metadata:** committed with this SUMMARY (docs: complete plan)

## Files Created/Modified

- `records/build-asan.txt` - configure and build output with exit statuses, `otool -L` runtime check, compile-command flag counts, `## Build fix` (none)
- `records/ctest-asan.txt` - `ctest -V` output with tested-commit header
- `records/test-cases-asan.txt` - per-executable output (four Boost.Test executables, qt_tests twice), sanitizer reports in place
- `records/sanitizer-findings.md` - Method, project findings table, third-party section (none), Totals; generated by script from `test-cases-asan.txt`

## Decisions Made

- The `if (SANITIZE)` block needed no change; the exception in the plan was not used.
- Counts come from `test-cases-asan.txt`. `ctest-asan.txt` contains the failing test's output twice (live plus `outputOnFailure`), so `grep -c 'runtime error:'` on it gives 5; the file's header says so.
- UBSan reports each location once per process, so the counts are distinct reports. The findings record states this; a location hit by several test cases of one executable appears once.

## Deviations from Plan

None - plan executed exactly as written.

Executor choices inside the plan's latitude: the Boost.Test runs use `--log_level=test_suite --report_level=detailed` like plan 01-03 (the plan names only `--log_level=test_suite`), so the sanitizer records compare line by line with the Debug records; ANSI colour codes were stripped from the committed logs (noted in each header); the `tested commit` of each record is HEAD at run time (a records commit, with no `src/`, `CMakeLists.txt` or `cmake/` change since the objects were built at `c0c129a`).

## Issues Encountered

- Task 2's `qt_tests` runs (default Cocoa and offscreen) both pass with no report, so no preset environment change is needed here either.
- The sanitizer build emits the same compiler warnings as the Debug build (about 1140 lines containing `warning:`); they are in `build-asan.txt` and are not re-counted here (plan 01-03 owns the warning counts).

## User Setup Required

None - no external service configuration required.

## Known Stubs

None. No source files were created or modified.

## Threat Flags

None. T-01-08 (accept): the two defects are recorded with locations for the Phase 6 safety work, not fixed. T-01-09 (mitigate): only `detect_leaks=0`, `halt_on_error=0` and `print_stacktrace=1` were used, all stated in `## Method`; no suppression file; the third-party section exists and is empty. T-01-SC: nothing installed.

## Next Phase Readiness

- Plan 01-07 can cite `records/sanitizer-findings.md` and `git diff --stat 5eaac18 -- src` (empty after this plan) in the closing cross-check.
- For Phase 6 (SAFE-01 to SAFE-03) the two locations are known; the `BinaryImage::SharedData::operator new` finding is likely to fire again in any later sanitizer run of image code, so a clean run there needs that fix first.
- Open for the user: severity and timing of the two UBSan findings; the earlier open question about the `imageproc_tests` failure is unchanged.

## Self-Check: PASSED

- All four record files exist on disk; commits `8038a24` and `ca3009f` are ancestors of HEAD; `git rev-list --count c0c129a..HEAD` was 2 before this SUMMARY.
- Task verify commands re-run: `libclang_rt.asan` line printed, `asan missing=0`, the five test executables exist, ctest summary line over 5 tests present, `## Totals` and `## Method` present, grep counts (3 UBSan, 0 ASan) equal the Totals, `git diff --stat 5eaac18 -- src` prints nothing.

---
*Phase: 01-deletion-free-macos-build*
*Completed: 2026-10-23*
