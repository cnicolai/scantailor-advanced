# Sanitizer findings: ASan + UBSan on the full Phase 2 suite (plan 02-10)

Source: `records/test-cases-asan.txt` (tested commit in its header). Tables, counts and totals below are produced by script from that file (`python3 -I` parse of the `runtime error:` and `ERROR: AddressSanitizer` lines); nothing was edited by hand except the Method and closing text. Format as in Phase 1 (`.planning/phases/01-deletion-free-macos-build/records/sanitizer-findings.md`) plus a `Known from Phase 1?` column. Recorded, not fixed (D-11, D-19); the result does not gate the phase.

## Method

- Build: preset `macos-asan` (Debug, `SANITIZE=address,undefined`, `-fsanitize=address,undefined -fno-omit-frame-pointer -fsanitize-recover=address`), rebuilt from the Phase 2 tree (`cmake --preset macos-asan && cmake --build --preset macos-asan`, 70 steps, exit 0).
- Environment for every run: `ASAN_OPTIONS=detect_leaks=0:halt_on_error=0`, `UBSAN_OPTIONS=print_stacktrace=1` (the values of the `macos-asan` test preset). No suppression files, no further options. `detect_leaks=0` is because Apple Clang has no LeakSanitizer.
- Runs: `ctest --preset macos-asan --output-on-failure` once (summary below), then each of the seven test executables directly: `pipeline_tests`, `dewarping_tests`, `imageproc_tests`, `core_tests`, `foundation_tests` and `math_tests` with `--log_level=test_suite --report_level=detailed`, and `qt_tests` with its defaults (it sets `QT_QPA_PLATFORM=offscreen` and its settings location itself, plan 02-09). stdout and stderr merged. Output: `records/test-cases-asan.txt`.
- Counting: the number of `runtime error:` lines for UBSan and of `ERROR: AddressSanitizer` lines for ASan in `records/test-cases-asan.txt`; the counts come from the test output, not from the ctest exit status. A test that passes can still print reports (recover mode).
- UBSan prints each distinct source location once per process, so a count is the number of distinct reports in one process, not the number of times the undefined operation ran. A second test case that hits the same line in the same executable is not reported again; the table lists the first test case only. A location can appear in several executables because each is its own process.
- `Known from Phase 1?` is `yes` for the two locations of the Phase 1 record (`src/imageproc/BinaryImage.cpp:1044` and `src/imageproc/SlicedHistogram.cpp:49`) and `new` for every other location.
- Research P11 expected new reports from pipeline paths that Phase 1's tests never reached (`Despeckle.cpp` lines 368, 392, 418, 446 and `EstimateBackground.cpp:243`); all five appear. `src/imageproc/InfluenceMap.cpp` lines 129 and 130 were not predicted.

## ctest of the ASan build

`ctest --preset macos-asan --output-on-failure`: `100% tests passed out of 7` (exit 0), `Total Test time (real) = 661.08 sec`.

| # | Test | Result | Time |
|---|------|--------|------|
| 1 | foundation_tests | Passed | 0.12 s |
| 2 | math_tests | Passed | 0.05 s |
| 3 | imageproc_tests | Passed | 19.27 s |
| 4 | core_tests | Passed | 0.51 s |
| 5 | qt_tests | Passed | 0.71 s |
| 6 | pipeline_tests | Passed | 632.31 s |
| 7 | dewarping_tests | Passed | 8.10 s |

## Findings in project code

| Kind | Location | Count | Test executable | First report line | Known from Phase 1? | Raised in test case (reached from) |
|---|---|---|---|---|---|---|
| UBSan: member access within null pointer of type 'SharedData' | `src/imageproc/BinaryImage.cpp:1044:37` | 4 | pipeline_tests, dewarping_tests, imageproc_tests, core_tests | `src/imageproc/BinaryImage.cpp:1044:37: runtime error: member access within null pointer of type 'SharedData'` | yes | BookSpreadTestSuite/test_spread_300_bw_split in pipeline_tests via TestBookSpread.cpp:191; DewarpingTextLineTracerTestSuite/test_tracer_builds_valid_model_from_curved_page in dewarping_tests via TestTextLineTracer.cpp:95; BinaryImageTestSuite/test_from_to_qimage in imageproc_tests via TestBinaryImage.cpp:46; ObliqueFinderTestSuite/build_sausage_mask_preserves_size in core_tests via TestObliqueFinder.cpp:23 |
| UBSan: left shift of negative value -1 | `src/core/Despeckle.cpp:368:46` | 1 | pipeline_tests | `src/core/Despeckle.cpp:368:46: runtime error: left shift of negative value -1` | new | BookSpreadTestSuite/test_spread_300_bw_split in pipeline_tests via TestBookSpread.cpp:191 |
| UBSan: left shift of negative value -125 | `src/core/Despeckle.cpp:392:48` | 1 | pipeline_tests | `src/core/Despeckle.cpp:392:48: runtime error: left shift of negative value -125` | new | BookSpreadTestSuite/test_spread_300_bw_split in pipeline_tests via TestBookSpread.cpp:191 |
| UBSan: left shift of negative value -60 | `src/core/Despeckle.cpp:418:48` | 1 | pipeline_tests | `src/core/Despeckle.cpp:418:48: runtime error: left shift of negative value -60` | new | BookSpreadTestSuite/test_spread_300_bw_split in pipeline_tests via TestBookSpread.cpp:191 |
| UBSan: left shift of negative value -1 | `src/core/Despeckle.cpp:446:46` | 1 | pipeline_tests | `src/core/Despeckle.cpp:446:46: runtime error: left shift of negative value -1` | new | BookSpreadTestSuite/test_spread_300_bw_split in pipeline_tests via TestBookSpread.cpp:191 |
| UBSan: left shift of negative value -1 | `src/imageproc/InfluenceMap.cpp:130:37` | 1 | pipeline_tests | `src/imageproc/InfluenceMap.cpp:130:37: runtime error: left shift of negative value -1` | new | BookSpreadTestSuite/test_spread_300_bw_split in pipeline_tests via TestBookSpread.cpp:191 |
| UBSan: left shift of negative value -1 | `src/imageproc/InfluenceMap.cpp:129:37` | 1 | pipeline_tests | `src/imageproc/InfluenceMap.cpp:129:37: runtime error: left shift of negative value -1` | new | BookSpreadTestSuite/test_spread_300_bw_split in pipeline_tests via TestBookSpread.cpp:191 |
| UBSan: shift exponent -431 is negative | `src/core/EstimateBackground.cpp:243:46` | 1 | pipeline_tests | `src/core/EstimateBackground.cpp:243:46: runtime error: shift exponent -431 is negative` | new | BookSpreadTestSuite/test_spread_300_bw_split in pipeline_tests via TestBookSpread.cpp:191 |
| UBSan: left shift of negative value -1 | `src/imageproc/SlicedHistogram.cpp:49:47` | 1 | imageproc_tests | `src/imageproc/SlicedHistogram.cpp:49:47: runtime error: left shift of negative value -1` | yes | SlicedHistogramTestSuite/test_null_image in imageproc_tests via TestSlicedHistogram.cpp:35 |

## Findings in third-party code

None.

## Totals

- Total ASan reports: 0
- Total UBSan reports: 12
- Distinct project locations: 9
- Of these known from Phase 1: 2; new: 7
- Per executable (UBSan / ASan):
  - `pipeline_tests`: 8 / 0
  - `dewarping_tests`: 1 / 0
  - `imageproc_tests`: 2 / 0
  - `core_tests`: 1 / 0
  - `foundation_tests`: 0 / 0
  - `math_tests`: 0 / 0
  - `qt_tests`: 0 / 0

## Reading of the result

- Every test of the full suite passes under ASan and UBSan, in `ctest` and when started directly (exit status 0 for all seven executables). In recover mode the UBSan reports do not change a result.
- All nine reported locations are undefined behaviour in production code that the tests exercise: a member access through a null pointer in the placement `operator new` of `BinaryImage::SharedData`, and left shifts of negative values (and one negative shift exponent) in `Despeckle`, `InfluenceMap`, `SlicedHistogram` and `EstimateBackground`. None is a Phase 2 defect and none is fixed here.
- Seven of the nine locations are new relative to Phase 1. They are reported in `pipeline_tests`, first by `BookSpreadTestSuite/test_spread_300_bw_split`, the first test case of that executable, and once per process, so later pipeline test cases that pass the same lines are not listed. The unit-test executables add no new location.
- No requirement of the roadmap (Phases 3 to 6) names these locations (`REQUIREMENTS.md` has no sanitizer fix requirement); whether and when to fix them is a decision for the user. The list is complete for the Phase 2 suite. Third-party code produced no report.
- Findings are recorded, not fixed, in Phase 2 (D-11, D-19); no production file was changed by this plan.
