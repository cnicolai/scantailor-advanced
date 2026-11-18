---
phase: 02-golden-baselines
plan: 02
subsystem: testing
tags: [boost-test, golden-images, image-comparison, qsavefile, toolchain-manifest, tolerances, qt6]

requires:
  - phase: 02-golden-baselines
    provides: "test_support static library with Rng and PageGenerator; pipeline_tests executable with GOLDEN_DIR / ARTIFACT_DIR / TEST_TMP_ROOT definitions (plan 02-01)"
provides:
  - "compareImages(): per-kind comparator (bi-level edge-tolerant XOR, component count, shift; grey/colour MAE, max error, shift) with all limits in Tolerances.h"
  - "Text goldens: sorted key = value files, D-14 tolerances by key suffix (_deg, _px, _mm), exact otherwise"
  - "Manifest: currentToolchain(), currentSourceCommit(), toolchainDifferences() with the frozen -ffp-contract value read from the top-level CMake options"
  - "GoldenStore: golden/artifact layout, UPDATE_GOLDENS=1 workflow, QSaveFile writes, per-suite MANIFEST.txt, name validation"
  - "GoldenChecks.h (GOLDEN-METRIC log lines, CHECK_IMAGE_GOLDEN, CHECK_TEXT_GOLDEN, pipelineGoldenStore) and ExpectedFailure.h (XFAIL_CHECK trip-wire)"
  - "Suites ImageCompareTestSuite (11 cases) and GoldenStoreTestSuite (15 cases)"
affects: [02-03, 02-04, 02-05, 02-06, 02-07, 02-08, 02-09, 02-10]

actuals:
  tokens: 21366
  tasks: 3
  commits: 6

tech-stack:
  added: []
  patterns:
    - "test_support .cpp files return results and never include Boost.Test; Boost-dependent helpers are header-only (GoldenChecks.h, ExpectedFailure.h)"
    - "Every golden and manifest write goes through QSaveFile; an identical manifest is not rewritten, so a repeated update changes nothing"
    - "Golden layout <GOLDEN_DIR>/<suite>/<name>.png|.txt plus one MANIFEST.txt per suite; failure artifacts <ARTIFACT_DIR>/<suite>/<name>/expected|actual|diff"
    - "Log line GOLDEN-METRIC suite= name= kind= metric= value= limit= result= for every metric of every comparison"
    - "CMake: toolchain facts become compile definitions of test_support (ST_CMAKE_VERSION, ST_BUILD_TYPE, ST_XCODE_VERSION, ST_FP_CONTRACT, ST_SOURCE_DIR); no production CMake file touched"

key-files:
  created:
    - src/test_support/Tolerances.h
    - src/test_support/ImageCompare.h
    - src/test_support/ImageCompare.cpp
    - src/test_support/TextGolden.h
    - src/test_support/TextGolden.cpp
    - src/test_support/Manifest.h
    - src/test_support/Manifest.cpp
    - src/test_support/GoldenStore.h
    - src/test_support/GoldenStore.cpp
    - src/test_support/GoldenChecks.h
    - src/test_support/ExpectedFailure.h
    - src/pipeline_tests/TestImageCompare.cpp
    - src/pipeline_tests/TestGoldenStore.cpp
  modified:
    - src/test_support/CMakeLists.txt

key-decisions:
  - "The metric size is always reported (value 0 when the sizes match), so every comparison prints the same leading GOLDEN-METRIC line; on a mismatch it is the only metric"
  - "Update mode refreshes a suite's MANIFEST.txt on the first check of that suite that passes or whose sanity check passes; a check blocked by sanity or by an invalid name writes nothing at all, and an unchanged manifest is not rewritten"
  - "A golden that fails in update mode and is replaced reports no metrics (they described the replaced file); its message carries the old failure, and expected/actual/diff are left in the artifact folder"
  - "A non-numeric value under a _deg, _px or _mm key fails, even if both sides are equal; the numeric limit gets 1e-9 slack (kTextToleranceSlack) so that 10.0500 vs 10.0000 passes the 0.05 limit"
  - "Toolchain warnings (differences, missing or unreadable manifest) are attached to the first outcome of a suite per store instance only"

patterns-established:
  - "Tests of the store use QTemporaryDir roots under build/<preset>/test-tmp and never the real golden directory"
  - "Mutation check after green: disabling name validation, the sanity gate, the normal-mode write guard and the pass shortcut each makes specific cases fail"

requirements-completed: [TEST-02, TEST-03]

coverage:
  - id: D1
    description: "compareImages() applies one fixed limit set per image kind from Tolerances.h; a 1-px shift, 300 isolated specks and a +10 level change fail while identical images and a few edge flips pass"
    requirement: "TEST-02"
    verification:
      - kind: unit
        ref: "build/macos-release/pipeline_tests --run_test=ImageCompareTestSuite (11 cases)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Text goldens compare stage decisions with the D-14 tolerances by key suffix and exactly otherwise"
    requirement: "TEST-02"
    verification:
      - kind: unit
        ref: "GoldenStoreTestSuite/test_text_roundtrip_and_parse_errors, test_text_tolerances_by_suffix"
        status: pass
    human_judgment: false
  - id: D3
    description: "A normal run never writes under the golden directory; a failing or missing golden reports metric, value, limit and artifact folder and leaves expected, actual and diff files"
    requirement: "TEST-03"
    verification:
      - kind: unit
        ref: "GoldenStoreTestSuite/test_normal_run_never_writes, test_failure_writes_artifacts_and_reports_metric"
        status: pass
    human_judgment: false
  - id: D4
    description: "UPDATE_GOLDENS=1 rewrites only missing or failing goldens whose sanity check passed, is idempotent, leaves no temporary file and ignores any other flag value"
    requirement: "TEST-03"
    verification:
      - kind: unit
        ref: "GoldenStoreTestSuite/test_update_records_missing_golden_and_manifest, test_update_rewrites_only_failing, test_update_twice_is_idempotent, test_sanity_failure_blocks_write, test_update_leaves_no_temporary_files, test_update_flag_parsing"
        status: pass
    human_judgment: false
  - id: D5
    description: "MANIFEST.txt records the toolchain versions and source commit; a differing toolchain yields one warning per key and the comparison still runs; names outside [a-z0-9_]+ are refused"
    requirement: "TEST-03"
    verification:
      - kind: unit
        ref: "GoldenStoreTestSuite/test_manifest_has_all_keys, test_toolchain_difference_is_reported, test_toolchain_mismatch_warns_and_compares, test_invalid_name_rejected"
        status: pass
    human_judgment: false
  - id: D6
    description: "XFAIL_CHECK logs KNOWN DEFECT while the defect persists and raises a Boost error once the condition holds"
    verification:
      - kind: unit
        ref: "GoldenStoreTestSuite/test_expected_failure_trip_wire (expected_failures(1))"
        status: pass
    human_judgment: false

duration: 19min
completed: 2026-10-23
status: complete
plan_head_before: 17acbe7ec3ad96152d07a6a4e10e1e77e10070b9
plan_head_after: 344196d1ef557442c772773e1c0391ee7bb7d384
commits: 6
---

# Phase 2 Plan 2: Comparator, Golden Store and Manifest Summary

**Per-kind image comparator (edge-tolerant XOR, component count and no-shift check for bi-level; MAE, max error and no-shift check for grey/colour) with every limit in `Tolerances.h`, text goldens with D-14 suffix tolerances, and a `GoldenStore` whose goldens change only under `UPDATE_GOLDENS=1`, atomically through `QSaveFile`, with a per-suite toolchain `MANIFEST.txt`, failure artifacts, Boost reporting helpers and an expected-failure trip-wire.**

## Performance

- **Duration:** 19 min
- **Started:** 2026-10-23T18:01:29Z
- **Completed:** 2026-10-23T18:20:00Z
- **Tasks:** 3 (each as a RED commit and a GREEN commit)
- **Files modified:** 14 (13 created, 1 modified)

## Accomplishments

- `compareImages()` separates the regressions D-13 names from edge noise: a 1-px shift fails through the `shift` metric (the edge-tolerant XOR alone tolerates it), 300 isolated specks fail `component_count_relative` while staying under the XOR ceiling, +10 levels fail `mae` and `max_abs_error`, five pixels flipped next to glyph edges pass. Size mismatches stop with the single metric `size`; every failure returns an RGB32 diff image of the expected size.
- Text goldens (`serializeText`, `parseText`, `compareText`, `textDiff`) with `formatNumber` that ignores the process locale and prints no negative zero.
- `Manifest` reports Qt (runtime and compile), Boost, libtiff, libpng, libjpeg-turbo, Apple clang, Xcode, CMake, macOS, `ffp_contract` (read from the top-level directory's `COMPILE_OPTIONS`, `off` in this tree) and build type; `source_commit` comes from `git rev-parse HEAD`. Observed on this Mac: Xcode 27.1 (27A9275), CMake 4.4.4.
- `GoldenStore` with the full update workflow; `GoldenChecks.h` prints `GOLDEN-METRIC suite=... name=... kind=... metric=... value=... limit=... result=...` for every metric; `ExpectedFailure.h` provides `XFAIL_CHECK`.

## Task Commits

Each task was committed as a RED commit (tests plus interface stubs) and a GREEN commit:

1. **Task 1: Per-kind image comparator with its limits in Tolerances.h** - `78f25f7` (test), `eef23dc` (feat)
2. **Task 2: Text goldens and the toolchain manifest** - `6a0524c` (test), `0d530f0` (feat)
3. **Task 3: GoldenStore, reporting helpers and the expected-failure trip-wire** - `5aaf157` (test), `344196d` (feat)

**Plan metadata:** `e12a5a9` (docs: plan summary), followed by the state and roadmap commit

## Files Created/Modified

- `src/test_support/Tolerances.h` - the one place for comparator limits (ceilings until plan 02-09)
- `src/test_support/ImageCompare.{h,cpp}` - `ImageKind`, `MetricResult`, `CompareResult`, `compareImages`, `canonicalImage`
- `src/test_support/TextGolden.{h,cpp}` - text goldens and their comparison
- `src/test_support/Manifest.{h,cpp}` - toolchain facts and their comparison
- `src/test_support/GoldenStore.{h,cpp}` - golden layout, update workflow, artifacts, manifest handling
- `src/test_support/GoldenChecks.h`, `ExpectedFailure.h` - header-only Boost helpers for test sources
- `src/test_support/CMakeLists.txt` - image-library link, `xcodebuild -version`, frozen `-ffp-contract` value, `ST_*` definitions
- `src/pipeline_tests/TestImageCompare.cpp`, `TestGoldenStore.cpp` - the two suites

## Decisions Made

See `key-decisions` above. Additions beyond the plan's interface list, all additive: `textDiff()` (needed for `diff.txt`), `sanityOk()`, `kindLabel()`, and the constant `kTextToleranceSlack`.

## TDD Gate Compliance

The plan is `type: execute`, so the `tdd-red-evidence` classifier was not required, and Boost.Test console output is not one of its supported report formats. Each task still ran RED then GREEN:

- **RED** (`78f25f7`, `6a0524c`, `5aaf157`): tests plus the final headers and compile-clean stub sources. Every behaviour case failed on its own assertions (18, 20 and 25 failed checks respectively; no build failure, no zero-test discovery).
  - Semantic assessment: `test_invalid_name_rejected` passed vacuously against the stub, because the stub never writes. It was checked afterwards by mutation (see below).
- **GREEN** (`eef23dc`, `0d530f0`, `344196d`): implementation; all cases pass in Release and `ctest --preset macos-debug -R pipeline_tests`.
- **REFACTOR:** none.
- **Mutation check after green:** disabling name validation fails `test_invalid_name_rejected` (29 checks); ignoring the sanity flag fails `test_sanity_failure_blocks_write`; letting normal mode write fails four cases; skipping the pass shortcut in update mode fails `test_update_rewrites_only_failing` and `test_update_twice_is_idempotent`. All mutations were reverted (file identical to the committed one).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Xcode version separator**
- **Found during:** Task 2 (`src/test_support/CMakeLists.txt`)
- **Issue:** the plan replaces the line breaks of `xcodebuild -version` with `; `. A semicolon inside a `target_compile_definitions` value splits the CMake list and breaks the definition.
- **Fix:** line breaks become `, ` (`Xcode 27.1, Build version 27A9275`).
- **Files modified:** `src/test_support/CMakeLists.txt`
- **Committed in:** `0d530f0`

**2. [Rule 2 - Missing critical] Defence in depth for the only recursive delete**
- **Found during:** Task 3, mutation check
- **Issue:** with name validation deliberately disabled, `test_invalid_name_rejected` (names `..`, `../x`) made `clearArtifacts` run `removeRecursively()` on `<artifact root>/../..`, which deleted `build/macos-release` (build tree only; no tracked or source file was touched; the other build directories were untouched). The released code validates first and was never affected, but the delete trusted that validation alone.
- **Fix:** `clearArtifacts(artifactRoot, folder)` now refuses any path that, after `QDir::cleanPath`, is not below the artifact root. `build/macos-release` was regenerated with `cmake --preset macos-release` and a build of `pipeline_tests` (49 s), and the mutation checks were then repeated with the remaining three mutations only.
- **Files modified:** `src/test_support/GoldenStore.cpp`
- **Committed in:** `344196d`

**3. [Process] RED commits carry interface stubs**
- The plan says to write the tests first and see them fail to link or fail. To get failures on assertions instead of a build failure, each RED commit also holds the final header and a stub source, which the GREEN commit replaces. No stub remains in the final tree.

---

**Total deviations:** 2 auto-fixed (1 blocking, 1 missing critical) and 1 process note
**Impact on plan:** none on the interfaces; the fixes are needed for correctness and safety. No scope creep.

## Issues Encountered

- The plan's verify text for the Debug run expects `100% tests passed, 0 tests failed out of 1`; the installed CMake 4.4.4 prints `100% tests passed out of 1`. Exit code 0 and the pass line agree, only the wording differs.
- `test_expected_failure_trip_wire` triggers a Boost error on purpose and counts it with `expected_failures(1)`. The log therefore contains one `error: in "GoldenStoreTestSuite/test_expected_failure_trip_wire": expected failure now passes, remove the mark: ...` line, preceded by a message saying so; the run still ends with `*** No errors detected`. Anything that scans logs for `error:` must account for it.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plans 02-04 to 02-10 can call `CHECK_IMAGE_GOLDEN` / `CHECK_TEXT_GOLDEN` with `pipelineGoldenStore()`; the macros compile (kept alive by an uncalled function in `TestGoldenStore.cpp`) but no golden suite uses them yet, and `src/pipeline_tests/golden` does not exist.
- Plan 02-09 should parse the `GOLDEN-METRIC` lines and freeze the image-kind limits in `Tolerances.h` just above the measured Release/Debug spread, never above the ceilings written there.
- The `shift` metric can report 1 for strongly periodic content (period of 2 px or less) when the unshifted comparison already differs; the generated pages and expected outputs are not of that kind.

## Self-Check: PASSED

- All 14 files listed above exist.
- All six commits are ancestors of HEAD: 78f25f7, eef23dc, 6a0524c, 0d530f0, 5aaf157, 344196d.
- `ImageCompareTestSuite` and `GoldenStoreTestSuite` pass in Release; `ctest --preset macos-debug -R pipeline_tests` and `ctest --preset macos-release -R pipeline_tests` pass.
- No limit literal in code lines of `ImageCompare.cpp`; no Boost.Test include in any `test_support` `.cpp`; `src/pipeline_tests/golden` untouched; `CMakeLists.txt` and `CMakePresets.json` equal `cde6825`.

---
*Phase: 02-golden-baselines*
*Completed: 2026-10-23*
