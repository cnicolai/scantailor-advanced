# Sanitizer comparison: ASan + UBSan on the full suite after the strip, against Phase 2 (plan 03-08, D-14)

Source: `records/test-cases-asan.txt` (tested commit in its header: `cf4da61c76195f4f23740f2a726b235e80186848`, the last strip commit c11-docs; HEAD at the run added only `.planning/` records). Counts and tables below are produced by script (`python3 -I build/scratch/asan/parse.py`) from that committed file. Reference: `.planning/phases/02-golden-baselines/records/sanitizer-findings.md` (0 ASan and 12 UBSan reports at 9 project locations). Nothing was fixed in this plan (D-14).

## Method

- Build: `cmake --preset macos-asan && cmake --build --preset macos-asan` on the tree of `cf4da61c` (preset `macos-asan`: Debug, `SANITIZE=address,undefined`, Apple Clang; 201 build steps, exit 0).
- Environment for every run: `ASAN_OPTIONS=detect_leaks=0:halt_on_error=0`, `UBSAN_OPTIONS=print_stacktrace=1` (the values of the `macos-asan` test preset), plus the environment the executable's own ctest entry sets (`QT_QPA_PLATFORM=offscreen`, `LC_ALL=en_US.UTF-8`, `QT_LOGGING_RULES=qt.svg=false` and `TMPDIR=build/macos-asan/test-tmp` for `pipeline_tests`; `QT_QPA_PLATFORM`, `LC_ALL` and `TMPDIR` for `qt_tests`; `LC_ALL` for `dewarping_tests`), so settings and temporary files stay in the build tree. No suppression files, no further options.
- Commands: the seven executables started directly from the repository root, in the order `pipeline_tests`, `dewarping_tests`, `imageproc_tests`, `core_tests`, `foundation_tests`, `math_tests` with `--log_level=test_suite --report_level=detailed`, then `qt_tests` with its defaults, stdout and stderr merged. `ctest --preset macos-asan` was not used for the counts because it hides the UBSan output of passing tests (RESEARCH Pitfall 7). The full command line of each run is the `$` line of its section in `records/test-cases-asan.txt`.
- Counting rule as in Phase 2: the number of `runtime error:` lines for UBSan and of `ERROR: AddressSanitizer` lines for ASan per executable. UBSan prints each source location once per process, so a count is the number of distinct reports in one process. A location can appear in several executables because each is its own process.
- Test case counts of this run: `pipeline_tests` 93, `dewarping_tests` 10, `imageproc_tests` 105, `core_tests` 55, `foundation_tests` 22, `math_tests` 32 (Phase 2 recorded 86 for `pipeline_tests` and identical numbers for the others; the seven added `pipeline_tests` cases come from the later Phase 2 plans 02-13 and 02-14, not from Phase 3).

## Exit status per executable

| Executable | Exit status |
|------------|-------------|
| `pipeline_tests` | 0 |
| `dewarping_tests` | 0 |
| `imageproc_tests` | 0 |
| `core_tests` | 0 |
| `foundation_tests` | 0 |
| `math_tests` | 0 |
| `qt_tests` | 0 |

## Findings in project code

| Kind | Location | Count | Test executable | In Phase 2 list? |
|---|---|---|---|---|
| UBSan: member access within null pointer of type 'SharedData' | `src/imageproc/BinaryImage.cpp:1044:37` | 4 | pipeline_tests, dewarping_tests, imageproc_tests, core_tests | yes |
| UBSan: left shift of negative value -1 | `src/core/Despeckle.cpp:368:46` | 1 | pipeline_tests | yes |
| UBSan: left shift of negative value -125 | `src/core/Despeckle.cpp:392:48` | 1 | pipeline_tests | yes |
| UBSan: left shift of negative value -60 | `src/core/Despeckle.cpp:418:48` | 1 | pipeline_tests | yes |
| UBSan: left shift of negative value -1 | `src/core/Despeckle.cpp:446:46` | 1 | pipeline_tests | yes |
| UBSan: left shift of negative value -1 | `src/imageproc/InfluenceMap.cpp:130:37` | 1 | pipeline_tests | yes |
| UBSan: left shift of negative value -1 | `src/imageproc/InfluenceMap.cpp:129:37` | 1 | pipeline_tests | yes |
| UBSan: shift exponent -431 is negative | `src/core/EstimateBackground.cpp:243:46` | 1 | pipeline_tests | yes |
| UBSan: left shift of negative value -1 | `src/imageproc/SlicedHistogram.cpp:49:47` | 1 | imageproc_tests | yes |

The shifted values and the first test case per executable are those of Phase 2: `BookSpreadTestSuite/test_spread_300_bw_split` (pipeline_tests), `DewarpingTextLineTracerTestSuite/test_tracer_builds_valid_model_from_curved_page` (dewarping_tests), `BinaryImageTestSuite/test_from_to_qimage` and `SlicedHistogramTestSuite/test_null_image` (imageproc_tests), `ObliqueFinderTestSuite/build_sausage_mask_preserves_size` (core_tests).

## Findings in third-party code

None.

## Totals

- Total ASan reports: 0
- Total UBSan reports: 12
- Distinct project locations: 9
- Of these in the Phase 2 list: 9; new: 0
- Per executable (UBSan / ASan):
  - `pipeline_tests`: 8 / 0
  - `dewarping_tests`: 1 / 0
  - `imageproc_tests`: 2 / 0
  - `core_tests`: 1 / 0
  - `foundation_tests`: 0 / 0
  - `math_tests`: 0 / 0
  - `qt_tests`: 0 / 0

## Comparison with Phase 2

| | ASan | UBSan | Distinct project locations |
|---|---|---|---|
| Phase 2 (`9859d1bd`, before the strip) | 0 | 12 | 9 |
| Phase 3 (`cf4da61c`, after the strip) | 0 | 12 | 9 |

The per-executable counts (8/0, 1/0, 2/0, 1/0, 0/0, 0/0, 0/0) and the nine file:line:col locations are identical, including the line numbers.

New project locations: 0
Missing Phase 2 locations: 0

The five files that carry the reports (`src/imageproc/BinaryImage.cpp`, `src/core/Despeckle.cpp`, `src/imageproc/InfluenceMap.cpp`, `src/core/EstimateBackground.cpp`, `src/imageproc/SlicedHistogram.cpp`) have no commit after `a796c2ae` (`git log a796c2ae..HEAD -- <file>` is empty for each; `git diff --quiet a796c2ae -- <the five files>` exits 0), so the line numbers compare. No source or build file was edited in this plan, and no report was fixed.

Verdict: PASS
