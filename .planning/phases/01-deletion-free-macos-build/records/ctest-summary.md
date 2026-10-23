# ctest summary: starting state of the test suite (BUILD-04)

Build and test commit: `de61f9e` (preset `macos-release`; the source tree equals the one of `1985dcc`, the commits since touch only `.planning/`). Source commits since `5eaac18`: four build settings, all logged in `fix-log.md` (rows 1, 3, 4, 5); no `src/` edit. Sources of the numbers: `ctest-release.txt`, `test-cases-release.txt` (and the Debug counterparts, filled in with the Debug run).

| Test | Release ctest | Release cases passed/failed | Debug ctest | Debug cases passed/failed |
|------|---------------|-----------------------------|-------------|---------------------------|
| core_tests | Passed | 55 / 0 | (Debug run pending) | (Debug run pending) |
| foundation_tests | Passed | 22 / 0 | (Debug run pending) | (Debug run pending) |
| math_tests | Passed | 32 / 0 | (Debug run pending) | (Debug run pending) |
| imageproc_tests | **Failed** (exit status 201) | 93 / 1 | (Debug run pending) | (Debug run pending) |
| qt_tests | Passed | 15 / 0 (TestCoreQt 8, TestUxQt 7) | (Debug run pending) | (Debug run pending) |

ctest overall, Release: `80% tests passed, 1 tests failed out of 5`, exit status 8.

## Failed cases

- Release, `imageproc_tests`: `ScaleTestSuite/test_random_image` (`src/imageproc/tests/TestScale.cpp`), 2 of 4 assertions failed: line 68 `checkScale(img, QSize(50, 50))` and line 70 `checkScale(img, QSize(80, 80))`. The case compares `scaleToGray()` with `QImage::scaled(..., Qt::SmoothTransformation)` pixel by pixel with a tolerance of 1; a comment in the test says the two already differ too much when upscaling. The cause is not examined in this plan (nothing is fixed here, D-08 / BUILD-04); the first suspect is a change in Qt 6's smooth scaling.

Boost.Test also prints `did not check any assertions` for `SeedFillTestSuite/test_gray4_random`, `test_gray8_random` and `test_gray_vs_binary`; these cases count as passed.

## qt_tests platform

(Decision pending the Debug run.) Release result: both runs pass with 8 + 7 cases passed and 0 failed, `exit status: 0` each: as registered (default Cocoa platform, from the executor's shell) and with `QT_QPA_PLATFORM=offscreen`. The Cocoa run started without a window-server error.
