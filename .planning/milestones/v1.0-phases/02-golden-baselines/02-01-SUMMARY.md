---
phase: 02-golden-baselines
plan: 01
subsystem: testing
tags: [boost-test, ctest, qt6, offscreen, synthetic-fixtures, splitmix64, headless-pipeline]

requires:
  - phase: 01-deletion-free-macos-build
    provides: "macos-release / macos-debug CMake presets, Homebrew Qt 6 + Boost toolchain, baseline commit cde6825"
provides:
  - "records/access-approval.md: the user's Phase 2 approval for build-tool reads outside the project (plans 02-01 to 02-10)"
  - "test_support static library: splitmix64 Rng and font-free flat page generator with ground truth"
  - "pipeline_tests Boost.Test executable: core Application offscreen fixture with isolated settings, temp files and pinned locale"
  - "runPipeline(): headless chain of the six stage Tasks with explicit stage defaults, typed Settings reads and decoded output"
  - "ctest entry pipeline_tests with ENVIRONMENT and TIMEOUT, plus GOLDEN_DIR / ARTIFACT_DIR / TEST_TMP_ROOT definitions"
affects: [02-02, 02-03, 02-04, 02-05, 02-06, 02-07, 02-08, 02-09, 02-10]

actuals:
  tokens: 9050
  tasks: 2
  commits: 3

tech-stack:
  added: []
  patterns:
    - "Generator = flat 1-byte ink raster + inverse-mapping 3x3 supersampling rotation; hand-written Taylor series, no libm trigonometry"
    - "Global Boost fixture sets env with qputenv, QSettings::setPath and the INI format before constructing core Application"
    - "Stage Tasks built last-to-first with filter = nullptr; defaults loaded explicitly per stage; page_split run first to learn the sub-page count"
    - "CMake: file(GLOB ... CONFIGURE_DEPENDS) in the new test directories, wired only from src/CMakeLists.txt inside if (BUILD_TESTS)"

key-files:
  created:
    - .planning/phases/02-golden-baselines/records/access-approval.md
    - src/test_support/CMakeLists.txt
    - src/test_support/Rng.h
    - src/test_support/PageGenerator.h
    - src/test_support/PageGenerator.cpp
    - src/pipeline_tests/CMakeLists.txt
    - src/pipeline_tests/main.cpp
    - src/pipeline_tests/HeadlessPipeline.h
    - src/pipeline_tests/HeadlessPipeline.cpp
    - src/pipeline_tests/TestEnvironment.cpp
    - src/pipeline_tests/TestFlatDocument.cpp
  modified:
    - src/CMakeLists.txt

key-decisions:
  - "Sign convention measured once: the deskew angle the unmodified code stores is the negative of PageSpec::skewDeg (1.5 gives -1.5); recorded in PageGenerator.h"
  - "GroundTruth::textBlock is the tight box around all drawn ink, not the nominal 600x750 frame, so the content-box IoU is measured against what is really on the page"
  - "HeadlessPipeline reads the generated PNG back through ImageLoader, so the dpi reaches the stages through the file metadata exactly as in the application"
  - "StageSettings carries the shared ImageSettings as an extra member because fix_orientation and deskew Tasks need it"

patterns-established:
  - "Every temporary path in the tests resolves under build/<preset>/test-tmp (TMPDIR set by qputenv and by set_tests_properties)"
  - "A run of runPipeline() resets DefaultParamsProvider to a fresh Default profile, so no scenario depends on an earlier one"

requirements-completed: [TEST-05, TEST-11]

coverage:
  - id: D1
    description: "User approval for build-tool reads outside the project is recorded and is the first commit touching the Phase 2 records directory"
    requirement: "TEST-11"
    verification:
      - kind: other
        ref: "git log --reverse --format=%s -- .planning/phases/02-golden-baselines/records | head -1, plus head -1 of access-approval.md equals 'Decision: approve'"
        status: pass
    human_judgment: false
  - id: D2
    description: "A generated 300 dpi RGB flat page with 1.5 degree skew runs through all six stage Tasks headlessly; stored deskew angle, content box, non-blank output and output dpi match the generator ground truth"
    requirement: "TEST-05"
    verification:
      - kind: integration
        ref: "ctest --preset macos-release -R pipeline_tests (FlatDocumentTestSuite/test_flat_300_bw_deskew_and_content_box)"
        status: pass
    human_judgment: false
  - id: D3
    description: "pipeline_tests runs offscreen with settings file and temp path under build/<preset>/test-tmp, profile Default and a pinned locale, both under ctest and when started directly"
    requirement: "TEST-11"
    verification:
      - kind: unit
        ref: "build/macos-release/pipeline_tests --run_test=EnvironmentTestSuite (test_settings_and_temp_are_isolated)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The generator uses no font rendering, painter, library random generator, random distribution, QRandomGenerator or libm trigonometry"
    requirement: "TEST-05"
    verification:
      - kind: other
        ref: "forbidden-API grep over src/test_support (plan verify command 4)"
        status: pass
    human_judgment: false
  - id: D5
    description: "The only non-test path that differs from cde6825 is src/CMakeLists.txt, and only by the two add_subdirectory lines inside if (BUILD_TESTS)"
    requirement: "TEST-11"
    verification:
      - kind: other
        ref: "git diff cde6825 non-test path gate (plan verify command 5)"
        status: pass
    human_judgment: false

duration: 10min
completed: 2026-10-23
status: complete
plan_head_before: b0abcd14240b5cfa56bacda73b66519148a9e408
plan_head_after: 6d955207fbe8ec98a1571cab7ba47d8f7c10104d
commits: 3
---

# Phase 2 Plan 1: Tracer Slice and Access Approval Summary

**Seeded font-free flat-page generator feeding a headless six-stage Task chain in a new `pipeline_tests` ctest executable (core Application offscreen, isolated settings, pinned locale); the unmodified deskew stage returns -1.5 degrees for a 1.5 degree skew and the content box matches ground truth at IoU 0.98.**

## Performance

- **Duration:** about 10 min for this continuation (the approval gate was answered in an earlier run)
- **Started:** 2026-10-23T17:49:57Z (approval commit)
- **Completed:** 2026-10-23T17:58:00Z
- **Tasks:** 2 (Task 1 decision gate answered "Approve"; Task 2 tracer)
- **Files modified:** 12 (11 created, 1 modified)

## Accomplishments

- The user's answer to the global-rule question ("Approve", Phase 2 plans 02-01 to 02-10 only) is recorded word for word, and its commit is the oldest commit touching the Phase 2 records directory.
- `src/test_support/` (static library, Qt Gui and Core only) renders a flat page in 1000 x 1150 base units with 13 lines of pseudo text from a 26-letter 5x7 glyph table, rotated by inverse mapping with 3x3 supersampling, and returns the ground truth (tight text box, line tops, page size, dpi, skew).
- `src/pipeline_tests/` builds the core `Application` offscreen after redirecting `TMPDIR`, `QSettings` (INI under `build/macos-release/test-tmp`), locale and Qt logging, and aborts if the settings file is anywhere else.
- `runPipeline()` chains `fix_orientation`, `page_split`, `deskew`, `select_content`, `page_layout` and `output` Tasks with `filter = nullptr`, loads each stage's defaults explicitly, and returns typed results plus the decoded output image. Measured on the tracer page: stored deskew angle -1.5000 (applied +1.5), content-box IoU 0.9813, black fraction 0.1041, output 600 x 600 dpi.
- The whole executable runs in about 0.65 s in Release under ctest and also when started directly.

## Task Commits

1. **Task 1: Access question (decision gate)** - `dccdcda` (docs) records the approval (committed by this continuation from the user's reply)
2. **Task 2 (tracer): generator** - `f1afd49` (test) `test_support` library
3. **Task 2 (tracer): pipeline_tests** - `6d95520` (test) fixture, `HeadlessPipeline`, tests and the two CMake wiring lines

**Plan metadata:** recorded in the docs commit that follows this summary.

## Files Created/Modified

- `.planning/phases/02-golden-baselines/records/access-approval.md` - the approval record
- `src/CMakeLists.txt` - two `add_subdirectory` lines inside `if (BUILD_TESTS)`
- `src/test_support/{CMakeLists.txt,Rng.h,PageGenerator.h,PageGenerator.cpp}` - generator library
- `src/pipeline_tests/CMakeLists.txt` - target, definitions, ctest registration with ENVIRONMENT and TIMEOUT 3600
- `src/pipeline_tests/main.cpp` - global fixture
- `src/pipeline_tests/HeadlessPipeline.{h,cpp}` - headless Task chain
- `src/pipeline_tests/TestEnvironment.cpp` - `EnvironmentTestSuite`
- `src/pipeline_tests/TestFlatDocument.cpp` - `FlatDocumentTestSuite`

## Decisions Made

- Deskew sign: stored angle = -`skewDeg` (positive `skewDeg` turns content clockwise on screen). Documented in `PageGenerator.h`.
- `GroundTruth::textBlock` is the tight bounding box of the drawn ink (595 x 695 base px at 300 dpi) rather than the nominal 600 x 750 frame. With the nominal frame the IoU could not exceed about 0.93 because the 13 lines are 695 px high; the tight box gives 0.98 with detected-box margins of a few pixels.
- The generated page is written as `in.png` and read back with `ImageLoader`, so the stages see the dpi from file metadata as in the application (the plan only said "write `in.png`").
- `runPipeline()` runs the chain once up to `page_split` to learn the sub-page count, then runs the full chain per sub-page. This keeps the two-page path ready for plan 02-03 without producing a stray single-page output for a spread.
- The ctest summary line of CMake 4.4.4 reads `100% tests passed out of 1`; the plan quotes `100% tests passed, 0 tests failed out of 1`. Pass/fail was judged by the exit code and the 100% line.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] `pipeline_tests` could not find `DistortionModel.h`**
- **Found during:** Task 2 (first build)
- **Issue:** `core/filters/output/Settings.h` includes `DistortionModel.h`, which lives in `src/dewarping`; the `output` library links `dewarping` PRIVATE, so the include path does not reach the test target.
- **Fix:** added `dewarping` to the `target_link_libraries` line of `pipeline_tests` (PUBLIC include directory of that static library).
- **Files modified:** `src/pipeline_tests/CMakeLists.txt`
- **Verification:** build and link succeed; no production CMake file touched.
- **Committed in:** `6d95520`

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** one extra word on a test-only link line; no scope creep. `ld` prints a harmless "ignoring duplicate libraries" warning for the static libraries, as it does for `core_tests`.

## Issues Encountered

- None beyond the include path above. The tracer passed on its first run; the sign convention guess (negative) made in the test before measuring was confirmed by the measurement.
- Not run in this plan: the `macos-debug` build (plans 02-05 onwards record and cross-check against it).

## Known Stubs

None.

## Threat Flags

None. No new network endpoints, auth paths or file access outside the build tree; the settings and temp isolation (T-02-02, T-02-03) is asserted by `EnvironmentTestSuite`, the production-diff gate (T-02-04) exits 0.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plans 02-02 to 02-10 can rely on `Rng`, `generatePage`, the `pipeline_tests` fixture, `StageSettings` / `PipelineOptions` / `runPipeline` and the `GOLDEN_DIR` / `ARTIFACT_DIR` / `TEST_TMP_ROOT` definitions. The first task of each later plan checks `records/access-approval.md` for `Decision: approve`.
- Open for later plans: tolerances for page split and dewarp ground truth still need their own measurement (research P4).

## Self-Check: PASSED

All 12 listed files exist; commits `dccdcda`, `f1afd49` and `6d95520` are ancestors of HEAD; the five automated verify commands of Task 2 exit 0 (ctest exit 0, direct run "No errors detected" with both suites, forbidden-API grep, production-diff gate, approval-first gate).

---
*Phase: 02-golden-baselines*
*Completed: 2026-10-23*
