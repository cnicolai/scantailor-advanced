# ctest summary: starting state of the test suite (BUILD-04)

Release build and tests at commit `de61f9e` (preset `macos-release`); Debug build and tests at commit `a3c7076` (preset `macos-debug`). The source tree equals the one of `1985dcc` in both cases; the commits since touch only `.planning/`. Source commits since `5eaac18`: four build settings, all logged in `fix-log.md` (rows 1, 3, 4, 5); no `src/` edit. Sources of the numbers: `ctest-release.txt`, `test-cases-release.txt`, `ctest-debug.txt`, `test-cases-debug.txt`.

| Test | Release ctest | Release cases passed/failed | Debug ctest | Debug cases passed/failed |
|------|---------------|-----------------------------|-------------|---------------------------|
| core_tests | Passed | 55 / 0 | Passed | 55 / 0 |
| foundation_tests | Passed | 22 / 0 | Passed | 22 / 0 |
| math_tests | Passed | 32 / 0 | Passed | 32 / 0 |
| imageproc_tests | **Failed** (exit status 201) | 93 / 1 | **Failed** (exit status 201) | 93 / 1 |
| qt_tests | Passed | 15 / 0 (TestCoreQt 8, TestUxQt 7) | Passed | 15 / 0 (TestCoreQt 8, TestUxQt 7) |

ctest overall, Release and Debug alike: `80% tests passed, 1 tests failed out of 5`, exit status 8.

Release and Debug differ in no test: every ctest status and every case count above is the same in both presets (Debug includes `DEVELOPER_VERSION` code and keeps asserts on).

## Failed cases

- Release and Debug, `imageproc_tests`: `ScaleTestSuite/test_random_image` (`src/imageproc/tests/TestScale.cpp`), 2 of 4 assertions failed: line 68 `checkScale(img, QSize(50, 50))` and line 70 `checkScale(img, QSize(80, 80))`. The case compares `scaleToGray()` with `QImage::scaled(..., Qt::SmoothTransformation)` pixel by pixel with a tolerance of 1; a comment in the test says the two already differ too much when upscaling. The cause is not examined in this plan (nothing is fixed here, D-08 / BUILD-04); the first suspect is a change in Qt 6's smooth scaling.

Boost.Test also prints `did not check any assertions` for `SeedFillTestSuite/test_gray4_random`, `test_gray8_random` and `test_gray_vs_binary`; these cases count as passed.

## qt_tests platform

Decision branch applied: **qt_tests passes as registered in both presets, so `CMakePresets.json` is unchanged.** It runs under the default platform (Cocoa) in the logged-in session. Result in Release and in Debug: as registered, 8 + 7 cases passed, 0 failed, `exit status: 0`, no window-server error; with `QT_QPA_PLATFORM=offscreen` for reference, the same 8 + 7 passed, `exit status: 0` (the offscreen plug-in is available in Homebrew's Qt). The only difference is a Qt warning in the offscreen runs about the missing font family "Sans Serif" (`qt.qpa.fonts`), which does not affect the result.
