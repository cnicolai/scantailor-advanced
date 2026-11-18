---
phase: 02-golden-baselines
plan: 09
subsystem: testing
tags: [boost-test, golden-images, tolerances, comparator-self-test, qtest, qsettings, ctest, offscreen]

requires:
  - phase: 02-golden-baselines
    provides: "02-02 comparator, GoldenStore and Tolerances.h ceilings; 02-05 to 02-07 recorded flat, book and mixed goldens; 02-08 round-trip suite"
provides:
  - "Image-kind limits frozen in Tolerances.h from a measured Release/Debug spread of 0 over 18 image and 14 text goldens, with records/tolerance-spread.md"
  - "ComparatorSelfTestSuite (nine cases): altered copies of recorded goldens fail under the frozen limits and leave expected, actual and diff images; the golden directory is hashed before and after"
  - "qt_tests runs offscreen with QSettings and temporary files under build/<preset>/test-tmp, from ctest and from a direct start"
  - "records/ctest-phase2.md: 7 of 7 in macos-release and macos-debug, random-order pipeline_tests, empty git status of src"
affects: [02-10, 03, 04, 05]

actuals:
  tokens: 8700
  tasks: 3
  commits: 5

tech-stack:
  added: []
  patterns:
    - "Comparator limits are frozen by a written rule: first grid step strictly above the measured maximum, capped at the D-12 ceiling; the measurement lives in a committed record"
    - "A self-test store is built with update mode hard-wired off and a QTemporaryDir artifact root under TEST_TMP_ROOT; a fixture hashes the golden directory around every case"
    - "Executables with a Qt application set offscreen, locale, TMPDIR and QSettings::setPath before the application exists and abort (qt_tests: exit 2) if the settings file is elsewhere"

key-files:
  created:
    - src/pipeline_tests/TestComparatorSelfTest.cpp
    - .planning/phases/02-golden-baselines/records/tolerance-spread.md
    - .planning/phases/02-golden-baselines/records/ctest-phase2.md
  modified:
    - src/test_support/Tolerances.h
    - src/qt_tests/main.cpp
    - src/qt_tests/CMakeLists.txt

key-decisions:
  - "Frozen limits: edge_tolerant_xor_fraction 0.0001, component_count_relative 0.001, mae 0.05, max_abs_error 1; every measured maximum was 0, so each is the first grid step above 0 and far below its ceiling (0.001, 0.02, 0.5, 8)"
  - "The single-grey-level case picks the most frequent level that covers at most 0.2% of the pixels, so the mean error stays under its limit (0.0067 against 0.05) and max_abs_error alone catches the change"
  - "test_golden_directory_untouched also sets UPDATE_GOLDENS=1 for its own duration and restores it; the guard fixture checks the golden directory after every case of the suite, so the suite is order independent"
  - "qt_tests keeps its two QTest::qExec calls unchanged; the settings isolation check returns 2 before any test runs"

patterns-established:
  - "Mutation evidence for tests of tests: restore a ceiling, or switch off the shift search (kShiftSearchRadius 0), and watch the named cases fail; revert with git checkout of that one file"

requirements-completed: [TEST-02, TEST-11]

coverage:
  - id: D1
    description: "Release/Debug spread of every image-kind metric measured over all recorded goldens and each limit frozen at the first grid step above it (D-12), with the measurement in records/tolerance-spread.md"
    requirement: "TEST-02"
    verification:
      - kind: integration
        ref: "build/macos-release/pipeline_tests and build/macos-debug/pipeline_tests --log_level=message: 461 GOLDEN-METRIC lines each, none failing, all values 0; ctest -R pipeline_tests passes in both presets under the frozen limits"
        status: pass
    human_judgment: false
  - id: D2
    description: "ComparatorSelfTestSuite: 300 isolated specks, a 1 px shift (bi-level and colour), +10 levels on all channels, +10 on one grey level and a +10 mixed output fail under the frozen limits, each leaving expected.png, actual.png, diff.png and a message with metric, value, limit and folder; the unaltered goldens pass"
    requirement: "TEST-02"
    verification:
      - kind: unit
        ref: "build/macos-release/pipeline_tests --run_test=ComparatorSelfTestSuite (9 cases)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The self-test cannot write a golden: update mode hard-wired off, golden directory hashed before and after, a run with UPDATE_GOLDENS=1 leaves git status of src/pipeline_tests/golden empty"
    requirement: "TEST-02"
    verification:
      - kind: unit
        ref: "UPDATE_GOLDENS=1 build/macos-release/pipeline_tests --run_test=ComparatorSelfTestSuite; ComparatorSelfTestSuite/test_golden_directory_untouched"
        status: pass
    human_judgment: false
  - id: D4
    description: "qt_tests runs offscreen with QSettings and temporary files under build/<preset>/test-tmp whether started by ctest or directly, and refuses to run (exit 2) if the settings file is elsewhere"
    requirement: "TEST-11"
    verification:
      - kind: integration
        ref: "ctest --preset macos-release / macos-debug (qt_tests Passed); build/macos-release/qt_tests direct run exit 0; mutant without QSettings::setPath exits 2 with the path message"
        status: pass
    human_judgment: false
  - id: D5
    description: "One ctest command runs all seven tests in both gating builds and passes; pipeline_tests passes in random order; no normal run writes into src"
    requirement: "TEST-11"
    verification:
      - kind: integration
        ref: "ctest --preset macos-release (100% tests passed out of 7, 55.61 s); ctest --preset macos-debug (100% tests passed out of 7, 230.21 s); pipeline_tests --random=1; git status --porcelain -- src empty"
        status: pass
    human_judgment: false

duration: 22min
completed: 2026-10-23
status: complete
plan_head_before: 147eff31b302b01f10a812eda7dd1bc8f1626cb3
plan_head_after: 78ddb6c1290af476812db3c847578527b9468709
commits: 5
---

# Phase 2 Plan 9: Frozen Comparator Limits, Self-Test and Isolated qt_tests Summary

**Image-kind comparator limits frozen at 0.0001 / 0.001 / 0.05 / 1 from a measured Release/Debug spread of 0 over all recorded goldens, a permanent `ComparatorSelfTestSuite` that proves specks, a 1 px shift and a +10 level offset still fail on the recorded goldens (with expected, actual and diff images and an untouched golden directory), `qt_tests` isolated offscreen under `build/<preset>/test-tmp`, and 7 of 7 ctest tests passing in Release and Debug.**

## Performance

- **Duration:** 22 min
- **Started:** 2026-10-23T20:22:21Z
- **Completed:** 2026-10-23T20:44:13Z (before the metadata commit)
- **Tasks:** 3
- **Files modified:** 6 (3 created, 3 modified)

## Accomplishments

- **Measured and frozen limits (D-12).** Both presets ran `pipeline_tests --log_level=message` on `147eff3`: 461 `GOLDEN-METRIC` lines each, pairwise identical metrics, no failing line. The four lines of `GoldenStoreTestSuite` (`suite_a`, a throw-away golden) were excluded. Over 9 bi-level images, 9 colour/grey images and 385 text key comparisons every value is 0 in Release and in Debug, so `Tolerances.h` now holds 0.0001, 0.001, 0.05 and 1.0 (ceilings 0.001, 0.02, 0.5, 8), each with a comment pointing to `records/tolerance-spread.md`. Text limits (0.01 deg, 1 px, 0.05 mm, exact) stay; their measured spread of 0 lies below them. Geometry constants and `kShiftSearchRadius` are unchanged.
- **Comparator self-test (D-13).** `ComparatorSelfTestSuite` loads `flat_600_bw`, `flat_300_color` and `mixed_300_mixed` from `GOLDEN_DIR`, alters copies and compares through `GoldenStore(GOLDEN_DIR, <QTemporaryDir under TEST_TMP_ROOT>, false)`. Observed values under the frozen limits: 300 specks give `component_count_relative` 1.21 (limit 0.001) and `edge_tolerant_xor_fraction` 0.000109 (limit 0.0001); a 1 px shift gives `shift` 1 (limit 0) for bi-level and colour; +10 on every channel gives `mae` 10 and `max_abs_error` 10; +10 on one grey level gives `max_abs_error` 10 with `mae` 0.0067 (passing); the mixed output gives `mae` 2.01 and `max_abs_error` 10. Unaltered goldens pass with every metric 0 and leave no artifact folder. The suite takes about 12 s in Release and passes in random order.
- **qt_tests isolation (TEST-11).** `main.cpp` sets `QT_QPA_PLATFORM=offscreen`, `LC_ALL=en_US.UTF-8` and `TMPDIR`, organisation `scantailor-tests`, application `qt_tests`, INI format and `QSettings::setPath` under `TEST_TMP_ROOT/qt_tests-settings` before `QApplication`, and returns 2 if `QSettings().fileName()` is elsewhere. `CMakeLists.txt` defines `TEST_TMP_ROOT` and gives the ctest entry the same environment.
- **Full runs.** `ctest --preset macos-release`: 100% passed out of 7 in 55.61 s; `ctest --preset macos-debug`: 100% passed out of 7 in 230.21 s; `pipeline_tests --random=1` passes in a different suite order; `git status --porcelain -- src` is empty afterwards.

## Task Commits

1. **Task 1: Measure the spread and freeze the limits** - `451985a` (test, `Tolerances.h`), `4a8774e` (test, `records/tolerance-spread.md`)
2. **Task 2: Comparator self-test** - `29f6f66` (test)
3. **Task 3: Isolate qt_tests, run the suite** - `59143f5` (test, `src/qt_tests`), `78ddb6c` (test, `records/ctest-phase2.md`)

**Plan metadata:** the docs commit that follows this summary.

## Files Created/Modified

- `src/test_support/Tolerances.h` - the four image-kind constants frozen, each with its measured maximum and ceiling in a comment
- `src/pipeline_tests/TestComparatorSelfTest.cpp` - `ComparatorSelfTestSuite`, nine cases, golden-directory guard fixture
- `src/qt_tests/main.cpp`, `src/qt_tests/CMakeLists.txt` - offscreen, locale, TMPDIR and settings isolation; `TEST_TMP_ROOT`; ctest environment
- `.planning/phases/02-golden-baselines/records/tolerance-spread.md` - method, spread table, frozen limits
- `.planning/phases/02-golden-baselines/records/ctest-phase2.md` - both ctest summaries, run times, random order, empty source status, Phase 1 comparison

## Decisions Made

See `key-decisions`. In addition: the self-test uses `Format_Grayscale8` copies for the bi-level alterations (the comparator canonicalises them) and fills the strip uncovered by a shift with white.

## TDD Gate Compliance

Task 2 carries `tdd="true"`, but the plan is `type: execute` and the code under test (comparator, store, frozen limits) existed before the test, so a RED commit that fails on an assertion cannot exist: the nine cases pass on the first complete run. There is no `feat` commit for Task 2; the single `test(02-09)` commit adds the suite. Discrimination was shown by mutation instead, each reverted with `git checkout -- src/test_support/Tolerances.h` and the file compared with a saved copy:

- `kShiftSearchRadius = 0`: 17 failures; `test_one_pixel_shift_bilevel_fails`, `test_one_pixel_shift_color_fails` and the two whole-set cases fail on "metric shift should fail", and the artifact case reports the missing `expected.png`, `actual.png` and `diff.png`.
- Image-kind limits back at the D-12 ceilings: **no case failed.** The planned alterations (300 specks, 1 px shift, +10 levels) are large enough to fail under the ceilings as well (`component_count_relative` 1.21 against 0.02, `max_abs_error` 10 against 8). The self-test therefore proves that the comparator catches the regressions named in D-13; it does not distinguish the frozen limits from the ceilings. That distinction rests on the measurement in `records/tolerance-spread.md`. A fourth alteration of +2 levels (fails at 1, passes at 8) would close this; it is not in the plan and was not added.

## Deviations from Plan

None - plan executed exactly as written, apart from two small choices inside the plan's freedom:

- The single-grey-level case chooses the most frequent level covering at most 0.2% of the pixels. The first version chose the most frequent non-paper level, which turned out to be the page's off-white paper level (mean error 5.03), so the case did not show what it is for; the share cap makes `max_abs_error` the only failing metric of the two (0.0067 against 0.05).
- Measurement lines of `GoldenStoreTestSuite` (`suite_a/page_one`) were excluded from the spread because they compare against a temporary golden, not a recorded one.

**Total deviations:** 0 auto-fixed. **Impact on plan:** none.

## Issues Encountered

- The plan's verify text expects `100% tests passed, 0 tests failed out of 7`; ctest 4.x prints `100% tests passed out of 7`. Exit code 0 and the pass line agree.
- The plan's Task 3 verify (`grep 'out of 7'` in the record) is satisfied by the record's summary lines; the Release `Total Test time` is 55.61 s, within "a minute or two".
- In the Debug build the whole suite takes 230 s, almost all of it `pipeline_tests` (220 s).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 02-10 can rely on frozen limits in one file, a permanent self-test and an isolated, offscreen `ctest` that passes 7 of 7 in both gating builds.
- Open items for 02-10 and the user are unchanged: the findings F-05-1, F-06-1, F-06-2, F-07-1, F-08-1 and the two deviations are in `.planning/WINDOWS.md`.
- A later toolchain change that moves any image by more than one grid step fails the golden tests on purpose; the response is to look at `diff.png`, not to widen a limit.

## Known Stubs

None.

## Threat Flags

None. The plan's register (T-02-23 to T-02-25) is covered: one constants file with a written freezing rule and a committed measurement; self-test store with update mode off, hashed golden directory and `UPDATE_GOLDENS=1` check; `qt_tests` settings under the build tree with an exit-2 guard.

## Self-Check: PASSED

- Files exist: `src/pipeline_tests/TestComparatorSelfTest.cpp`, `records/tolerance-spread.md`, `records/ctest-phase2.md`, `src/test_support/Tolerances.h`, `src/qt_tests/main.cpp`, `src/qt_tests/CMakeLists.txt`.
- Commits are ancestors of HEAD: 451985a, 4a8774e, 29f6f66, 59143f5, 78ddb6c.
- `xcrun clang-format --dry-run -Werror` exits 0 for `Tolerances.h`, `TestComparatorSelfTest.cpp` and `src/qt_tests/main.cpp`.
- `git status --porcelain -- src` is empty; the golden directory is unchanged since the plan started.

---
*Phase: 02-golden-baselines*
*Completed: 2026-10-23*
