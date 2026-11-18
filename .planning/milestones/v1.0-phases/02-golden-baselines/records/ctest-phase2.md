# Full-suite runs, Phase 2 (plan 02-09, TEST-11)

## What was run

- **Commit tested:** `59143f5` (the plan's last source commit). The runs below were made on the working tree that this commit then recorded: `src/qt_tests/main.cpp` and `src/qt_tests/CMakeLists.txt` were the only uncommitted source changes, and `git status --porcelain -- src` was empty after the commit.
- **Commands:** `cmake --preset <p> && cmake --build --preset <p> && ctest --preset <p> --output-on-failure` for `macos-release` and `macos-debug`, then `build/macos-release/pipeline_tests --random=1 --log_level=test_suite`, then `git status --porcelain -- src`.
- **Environment:** the two executables that create a Qt application (`pipeline_tests`, `qt_tests`) set `QT_QPA_PLATFORM=offscreen`, `LC_ALL=en_US.UTF-8` and `TMPDIR` themselves before the application exists and keep `QSettings` and temporary files under `build/<preset>/test-tmp`; the ctest entries of both carry the same environment. The other five test programs create no Qt application.
- **Settings isolation of qt_tests (T-02-25):** with the `QSettings::setPath` call commented out, `qt_tests` printed `settings file .../.config/scantailor-tests/qt_tests.ini is not under .../build/macos-release/test-tmp` and returned 2 before running a test; with the call restored it passes.

## macos-release

ctest summary: `100% tests passed out of 7` (exit 0).

| # | Test | Result | Time |
|---|------|--------|------|
| 1 | foundation_tests | Passed | 0.02 s |
| 2 | math_tests | Passed | 0.02 s |
| 3 | imageproc_tests | Passed | 0.93 s |
| 4 | core_tests | Passed | 0.36 s |
| 5 | qt_tests | Passed | 0.42 s |
| 6 | pipeline_tests | Passed | 53.31 s |
| 7 | dewarping_tests | Passed | 0.54 s |

`Total Test time (real) =  55.61 sec`. Budget from CONTEXT ("a minute or two"): met. (An earlier run of `pipeline_tests` alone, before the comparator self-test was added, took 42 s; the self-test adds about 12 s.)

## macos-debug

ctest summary: `100% tests passed out of 7` (exit 0).

| # | Test | Result | Time |
|---|------|--------|------|
| 1 | foundation_tests | Passed | 0.02 s |
| 2 | math_tests | Passed | 0.02 s |
| 3 | imageproc_tests | Passed | 6.63 s |
| 4 | core_tests | Passed | 0.36 s |
| 5 | qt_tests | Passed | 0.57 s |
| 6 | pipeline_tests | Passed | 220.21 s |
| 7 | dewarping_tests | Passed | 2.40 s |

`Total Test time (real) = 230.21 sec`. Debug is the slow gating build; the ctest `TIMEOUT` of `pipeline_tests` is 3600 s.

## Random test order

`build/macos-release/pipeline_tests --random=1 --log_level=test_suite`: exit 0, `*** No errors detected`. The suites ran in a different order than the declaration order (first suite `DewarpPageTestSuite`, then `ComparatorSelfTestSuite`, `GoldenStoreTestSuite`, ...), so no suite depends on another having run first (research P8). The log contains the one deliberate `error: ... expected failure now passes, remove the mark` line of `test_expected_failure_trip_wire`, counted by `expected_failures(1)`.

## Source tree after the normal runs

`git status --porcelain -- src` after the Release ctest, the Debug ctest and the random-order run, and after the plan's last commit: empty. The only entries before that commit were the two intended `src/qt_tests` edits. No normal run writes into the source tree (D-06). The self-test with `UPDATE_GOLDENS=1` also left `git status --porcelain -- src/pipeline_tests/golden` empty.

## Comparison with Phase 1

| | Tests | Result | Failing |
|---|-------|--------|---------|
| Phase 1 (`records/ctest-summary.md`, Release and Debug) | 5 | `80% tests passed, 1 tests failed out of 5`, exit 8 | imageproc |
| Phase 2, macos-release | 7 | `100% tests passed out of 7`, exit 0 | none |
| Phase 2, macos-debug | 7 | `100% tests passed out of 7`, exit 0 | none |

ctest 4.x prints `100% tests passed out of N` without the `0 tests failed` clause of older versions; the exit status is the judgement.
