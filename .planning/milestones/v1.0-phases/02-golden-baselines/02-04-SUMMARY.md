---
phase: 02-golden-baselines
plan: 04
subsystem: testing
tags: [boost-test, binarization, dewarping, ground-truth, expected-failure, area-average, test-support]

requires:
  - phase: 02-golden-baselines
    provides: "test_support: Rng, PageGenerator (FLAT, CURVED), renderInkMask, mapFlatToImage, truthMismatchFraction, registerMask, straightnessResidual, XFAIL_CHECK (plans 02-01 to 02-03)"
provides:
  - "BinarizeTestSuite: all nine binarizers checked against the generator's ink mask (even page), seven local ones also on an unevenly lit page, bounds tightened per D-15"
  - "ScaleTestSuite/test_random_image fixed in test code: exact area-average reference and a seeded Rng input (D-17); the last Phase 1 test failure is gone"
  - "dewarping_tests: new ctest entry with three suites and ten cases for DistortionModel, DistortionModelBuilder, CylindricalSurfaceDewarper, RasterDewarper and TextLineTracer"
  - "records/unit-tests.md: D-17 investigation numbers, binarization and dewarping measurements, findings F-04-1 and F-04-2 for plan 02-10"
affects: [02-05, 02-06, 02-07, 02-08, 02-09, 02-10]

actuals:
  tokens: 10187
  tasks: 2
  commits: 3

tech-stack:
  added: []
  patterns:
    - "Test executables below the pipeline link test_support by target name and take their fixtures from the seeded generator"
    - "Bounds are named constants at the top of the test file with the measured worst case in a comment (D-15)"
    - "A defect of the unmodified code becomes XFAIL_CHECK(condition, tag) plus a numbered finding in the records file (D-18)"
    - "A zero measurement is guarded against a vacuous metric by a case that feeds the same metric a blank and a shifted result"

key-files:
  created:
    - src/dewarping/tests/CMakeLists.txt
    - src/dewarping/tests/main.cpp
    - src/dewarping/tests/TestDistortionModel.cpp
    - src/dewarping/tests/TestCylindricalDewarper.cpp
    - src/dewarping/tests/TestTextLineTracer.cpp
    - .planning/phases/02-golden-baselines/records/unit-tests.md
  modified:
    - src/imageproc/tests/TestBinarize.cpp
    - src/imageproc/tests/TestScale.cpp
    - src/imageproc/tests/CMakeLists.txt
    - src/CMakeLists.txt

key-decisions:
  - "D-17 branch A: scaleToGray equals the exact area average (maximum difference 0 at 50x50 and 80x80); Qt 6.12 smooth scaling is about 1.1 levels darker and up to 30 levels off, so the reference changed, not the code"
  - "Every binarizer reproduces the ink mask pixel for pixel on both pages (mismatch 0, also without edge tolerance), so each bound is the D-15 minimum of 0.05% of the ink area"
  - "The global thresholds are tested on an Indexed8 grey copy of the page because BinaryImage rejects Format_Grayscale8; the rejection itself is an XFAIL_CHECK (F-04-1)"
  - "The dewarper fixture uses two arcs of unequal sagitta (30 and 10 px); an equal pair has no depth change, arc length 1 and a straight line at normalised y 0.5"
  - "The model's top curve skipping the first text line of the curved page (seed 1) is an XFAIL_CHECK (F-04-2); other seeds follow both outer lines"

patterns-established:
  - "Raster dewarp is tested by drawing the image of a normalised horizontal line through mapToWarpedSpace, dewarping it and measuring straightnessResidual before and after"

requirements-completed: [TEST-08, TEST-09, TEST-11]

coverage:
  - id: D1
    description: "TestBinarize runs on synthetic inputs: nine binarizers against the generator's ink mask on an evenly lit page, seven local ones on an unevenly lit page, mismatch bound 0.05% of the ink area, mask registration (0, 0)"
    requirement: "TEST-08"
    verification:
      - kind: unit
        ref: "build/macos-release/imageproc_tests --run_test=BinarizeTestSuite (11 cases); ctest --preset macos-debug -R imageproc_tests"
        status: pass
    human_judgment: false
  - id: D2
    description: "ScaleTestSuite/test_random_image passes against an exact area-average reference with a seeded input; the investigation is recorded and production code is unchanged"
    requirement: "TEST-11"
    verification:
      - kind: unit
        ref: "build/macos-release/imageproc_tests --run_test=ScaleTestSuite"
        status: pass
      - kind: other
        ref: "git diff --quiet cde6825 -- src/imageproc ':!src/imageproc/tests/*'"
        status: pass
    human_judgment: false
  - id: D3
    description: "dewarping_tests checks model validity and XML round trip, builder determinism, dewarper round trip (4.6e-16, bound 1e-9), unit-square corners, raster output size and straightening (0.02 px, bound 0.5 px), and the tracer's model on the curved page"
    requirement: "TEST-09"
    verification:
      - kind: unit
        ref: "ctest --preset macos-release -R dewarping_tests; ctest --preset macos-debug -R dewarping_tests"
        status: pass
    human_judgment: false
  - id: D4
    description: "Defects found in unmodified code are XFAIL_CHECK marks with findings F-04-1 (global thresholds reject Format_Grayscale8) and F-04-2 (model top curve skips the first text line of the curved page)"
    verification:
      - kind: unit
        ref: "BinarizeTestSuite/test_otsu_on_flat_page, test_mokji_on_flat_page; DewarpingTextLineTracerTestSuite/test_model_curves_follow_first_and_last_lines (log KNOWN DEFECT)"
        status: pass
    human_judgment: true
    rationale: "Whether each finding becomes a backlog item, and with which fixing requirement, is decided in plan 02-10"
  - id: D5
    description: "Wiring touches nothing in production: src/CMakeLists.txt differs from cde6825 by exactly the three add_subdirectory lines, src/dewarping/CMakeLists.txt and src/imageproc/CMakeLists.txt are unchanged"
    requirement: "TEST-11"
    verification:
      - kind: other
        ref: "wiring gate of Task 2 (git diff cde6825 on the three CMake files)"
        status: pass
    human_judgment: false

duration: 11min
completed: 2026-10-23
status: complete
plan_head_before: c93c5857d1b8b59ba64c9c9787a693f505d0ee61
plan_head_after: c5039149d473e3d87974ed2eba12303542a15fe1
commits: 3
---

# Phase 2 Plan 4: Binarization, Scale and Dewarping Unit Tests Summary

**TestBinarize revived with all nine binarizers checked pixel for pixel against the generator's ink mask (even and unevenly lit pages), TestScale fixed with an exact area-average reference after a recorded investigation showing Qt 6's smooth scaling moved and `scaleToGray` did not, and a new `dewarping_tests` executable covering the distortion model, builder, cylindrical and raster dewarper and the text-line tracer, with two findings marked as expected failures.**

## Performance

- **Duration:** 11 min
- **Started:** 2026-10-23T18:48:42Z
- **Completed:** 2026-10-23T18:59:39Z (last task commit; SUMMARY and state commits follow)
- **Tasks:** 2 (three commits: records first, then the two tasks' code)
- **Files modified:** 10 (6 created, 4 modified)

## Accomplishments

- **D-17 settled in test code.** A throw-away measurement showed `scaleToGray` equal to the exact area average at 50x50 and 80x80 on the old and on a seeded input (maximum difference 0), while `QImage::scaled` is about 1.1 levels darker with up to 30 levels difference. `TestScale.cpp` now compares against a test-local area-average helper with the old tolerance of 1 and fills its input from `test_support::Rng`. `imageproc_tests` passes in Release and Debug for the first time since Phase 1.
- **TestBinarize on ground truth.** Ten cases (nine binarizers plus the uneven-illumination case) and one guard case. Every binarizer reproduces the truth mask exactly (mismatch 0 even with edge radius 0), so every bound is the D-15 minimum 0.0005. The guard case shows a blank page scoring above 0.9 and a 3 px shifted page failing the bound, so the zeros are not a vacuous metric.
- **dewarping_tests.** Ten cases in three suites: model validity, degenerate and crossed models, XML round trip with a 5 px negative; builder from five noisy arc curves (valid, deterministic); dewarper round trip 4.6e-16 (bound 1e-9), unit-square corners exact, raster output size for three sizes, straightening of three curves (before 6.6 to 10.9 px, after at most 0.024 px, bound 0.5 px); the tracer builds a valid model from the curved page and its bottom curve follows the last line within 5.18 px (bound 10.5 px).
- **Findings for plan 02-10.** F-04-1: `binarizeOtsu` and `binarizeMokji` throw on `Format_Grayscale8`. F-04-2: on the curved page (seed 1) the model's top curve follows the second text line (64.4 px from the first); other seeds follow both outer lines, seeds 1 and 4 lose the first, seed 6 the last.
- `ctest --preset macos-release` runs all seven tests and passes; the whole Debug run of `imageproc_tests` and `dewarping_tests` also passes.

## Task Commits

1. **Records first (T-02-12):** `9e35594` (docs) - D-17 investigation numbers committed before the TestScale change
2. **Task 1: Revive TestBinarize, settle TestScale (D-17)** - `184fe85` (test)
3. **Task 2: dewarping_tests** - `c503914` (test)

**Plan metadata:** the docs commit that follows this summary.

## Files Created/Modified

- `src/imageproc/tests/TestBinarize.cpp` - `BinarizeTestSuite`: eleven cases, bounds and measured worst cases at the top
- `src/imageproc/tests/TestScale.cpp` - area-average reference helper, seeded input
- `src/imageproc/tests/CMakeLists.txt` - `test_support` added to the link list
- `src/CMakeLists.txt` - `add_subdirectory(dewarping/tests)`, the third and last wiring line of the phase
- `src/dewarping/tests/CMakeLists.txt`, `main.cpp` - the new executable and its ctest entry
- `src/dewarping/tests/TestDistortionModel.cpp`, `TestCylindricalDewarper.cpp`, `TestTextLineTracer.cpp` - the three suites
- `.planning/phases/02-golden-baselines/records/unit-tests.md` - investigation, measurements, findings

## Decisions Made

See `key-decisions`. Also: Task 1 and Task 2 are test-only changes over existing code, so there is no separate RED commit; the pre-existing `ScaleTestSuite/test_random_image` failure was the RED state of Task 1, and each new check was run against the unmodified code before its bound was set.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Global thresholds need an Indexed8 copy of the page**
- **Found during:** Task 1 (first run of `test_otsu_on_flat_page`)
- **Issue:** The plan converts the fixture to `Format_Grayscale8`, which `binarizeOtsu` and `binarizeMokji` reject with `std::runtime_error`.
- **Fix:** The two global thresholds are checked on `GrayImage(page).toQImage()` (Indexed8 grey, the format every production caller passes); the rejection itself is an `XFAIL_CHECK` and finding F-04-1, so the fixture content is unchanged and the defect is not hidden.
- **Files modified:** `src/imageproc/tests/TestBinarize.cpp`, `records/unit-tests.md`
- **Committed in:** `184fe85`

**2. [Rule 1 - Bug in plan fixture] Dewarper arcs of unequal sagitta**
- **Found during:** Task 2 (raster straightening case)
- **Issue:** Two arcs of equal sagitta give a model with no depth change (arc length 1), and the line "halfway between the directrices" is then a straight image line, so the first run reported residual 0 before and after.
- **Fix:** Top arc sagitta 30 px, bottom arc 10 px (both circular arcs, as planned); the case also checks that the drawn input line is curved (residual above 3 px) and uses three lines (normalised y 0.25, 0.5, 0.75) instead of one. The distortion-model cases keep equal sagittas, which only need validity.
- **Files modified:** `src/dewarping/tests/TestCylindricalDewarper.cpp`
- **Committed in:** `c503914`

**3. [Process] Added one case and one test-local helper beyond the plan**
- `test_ground_truth_metric_rejects_wrong_output` in `BinarizeTestSuite` (guards the zero mismatches), and a small duplicate of the arc helper in two dewarping test files (the plan lists no shared header).
- **Committed in:** `184fe85`, `c503914`

---

**Total deviations:** 2 auto-fixed (1 Rule 3, 1 Rule 1 in the plan's fixture description), 1 process note
**Impact on plan:** no scope creep; every plan gate passes. Neither deviation changes a bound or loosens a check.

## Issues Encountered

- The installed ctest prints `100% tests passed out of N` without the `, 0 tests failed` part that the plan's `fails_when` text names; the result is the same and the exit status is 0.
- A first version of the warped bounding box used `QRectF::united()`, which ignores rectangles without area and produced a 1 x 0 content rect; the extremes are now tracked by hand.
- `clang-format` is not on PATH; `xcrun clang-format --dry-run -Werror` exits 0 on every touched file.

## Known Stubs

None.

## Threat Flags

None. No network, auth or file access outside the build tree was added. T-02-12 (investigation committed before the reference changed: `9e35594` before `184fe85`), T-02-13 (D-15 rule and D-18 marks with findings) and T-02-14 (wiring gate passes: exactly three `add_subdirectory` lines, `src/dewarping/CMakeLists.txt` and `src/imageproc/CMakeLists.txt` equal `cde6825`, no production file under `src/imageproc` differs) are covered.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plans 02-05 to 02-08 can run on the same executables; `imageproc_tests` takes 0.7 s in Release and 7 s in Debug, `dewarping_tests` 0.2 s and 2 s.
- Plan 02-10 gets two findings (F-04-1, F-04-2) with fixing requirement `none`; F-04-2 should be read together with the dewarp scenario of plan 02-06, because the production stage adds the page edges with `TopBottomEdgeTracer` after the tracer.
- The single `ctest` run passes in Release (7 of 7); the Debug `pipeline_tests` run was not repeated in this plan.

## TDD Gate Compliance

Task 1 and Task 2 carry `tdd="true"` on an `execute` plan and test code that already has production code behind it: the RED state of Task 1 was the Phase 1 failure of `ScaleTestSuite/test_random_image` (two failed assertions), and the new checks have no preceding production change, so there are `test(02-04)` commits and no `feat(02-04)` commit by design. The `tdd-red-evidence` classifier does not read Boost.Test output and was not used.

## Self-Check: PASSED

All 10 listed files exist; commits `9e35594`, `184fe85` and `c503914` are ancestors of HEAD; `ctest --preset macos-release` (7 of 7) and `ctest --preset macos-debug -R 'imageproc_tests|dewarping_tests'` (2 of 2) pass; the wiring gate and the production-unchanged gate exit 0.

---
*Phase: 02-golden-baselines*
*Completed: 2026-10-23*
