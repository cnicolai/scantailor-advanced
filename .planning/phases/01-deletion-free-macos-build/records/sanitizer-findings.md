# Sanitizer findings: ASan + UBSan on the unmodified tree (plan 01-04)

Source: `records/test-cases-asan.txt` (tested commit in its header). Table, counts and totals below are produced by script from that file (`python3 -I` parse of the `runtime error:` and `ERROR: AddressSanitizer` lines); nothing was edited by hand except the Method and closing text.

## Method

- Build: preset `macos-asan` (Debug, `SANITIZE=address,undefined`, `-fsanitize=address,undefined -fno-omit-frame-pointer -fsanitize-recover=address`, see `records/build-asan.txt`).
- Environment for every run: `ASAN_OPTIONS=detect_leaks=0:halt_on_error=0`, `UBSAN_OPTIONS=print_stacktrace=1`. Nothing else: no suppression files, no further options. `detect_leaks=0` is because Apple Clang has no LeakSanitizer.
- Runs: `ctest --preset macos-asan -V` (`records/ctest-asan.txt`) and each executable directly (`records/test-cases-asan.txt`): four Boost.Test executables with `--log_level=test_suite --report_level=detailed`, `qt_tests` as registered and with `QT_QPA_PLATFORM=offscreen`. stdout and stderr merged.
- Counting: `grep -c 'runtime error:'` for UBSan and `grep -c 'ERROR: AddressSanitizer'` for ASan on `records/test-cases-asan.txt`. The counts come from the test output, not from the ctest exit status.
- A ctest PASS does not mean zero findings: in recover mode (`halt_on_error=0`, `-fsanitize-recover=address`; UBSan recovers by default) a test can pass while printing reports. `core_tests` passes with one UBSan report in this run.
- UBSan prints each distinct source location once per process, so a count is the number of distinct reports in one process, not the number of times the undefined operation ran. A second test case that hits the same line in the same executable is not reported again; the table lists the first test case only.
- `records/ctest-asan.txt` prints the reports of the failing test (`imageproc_tests`) twice (live, and again through `outputOnFailure`), so `grep -c` on that file gives 5, not 3. The totals here use `records/test-cases-asan.txt`.

## Findings in project code

| Kind | Location | Count | Test executable | First report line | Raised in test case (reached from) |
|---|---|---|---|---|---|
| UBSan: member access within null pointer of type 'SharedData' | `src/imageproc/BinaryImage.cpp:1044:37` | 2 | core_tests, imageproc_tests | `src/imageproc/BinaryImage.cpp:1044:37: runtime error: member access within null pointer of type 'SharedData'` | BinaryImageTestSuite/test_from_to_qimage in imageproc_tests via TestBinaryImage.cpp:46; ObliqueFinderTestSuite/build_sausage_mask_preserves_size in core_tests via TestObliqueFinder.cpp:23 |
| UBSan: left shift of negative value -1 | `src/imageproc/SlicedHistogram.cpp:49:47` | 1 | imageproc_tests | `src/imageproc/SlicedHistogram.cpp:49:47: runtime error: left shift of negative value -1` | SlicedHistogramTestSuite/test_null_image in imageproc_tests via TestSlicedHistogram.cpp:35 |

## Findings in third-party code

None. No report has a first frame outside `{repo}/src/`: no `vptr` or other check fired on Qt, Boost or system code in these tests. The check set was not narrowed (no `-fno-sanitize=`), so this is the result of the full `address,undefined` set.

## Totals

- Total ASan reports: 0
- Total UBSan reports: 3
- Distinct project locations: 2
- Per executable (UBSan / ASan):
  - `core_tests`: 1 / 0
  - `foundation_tests`: 0 / 0
  - `math_tests`: 0 / 0
  - `imageproc_tests`: 2 / 0
  - `qt_tests (default)`: 0 / 0
  - `qt_tests (offscreen)`: 0 / 0

Test results in the sanitizer build match Debug (`records/ctest-debug.txt`): `80% tests passed, 1 tests failed out of 5`, the failing case is again `imageproc_tests` `ScaleTestSuite/test_random_image` (no sanitizer report is attached to it).

Findings are recorded, not fixed, in Phase 1 (success criterion 4); fixes belong to later phases (SAFE-01 to SAFE-03).
