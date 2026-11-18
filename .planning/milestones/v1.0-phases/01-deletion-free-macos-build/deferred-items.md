## Deferred Items

- imageproc_tests fails `ScaleTestSuite/test_random_image` in Release and Debug (found by plan 01-03)
  status: acknowledged
  **Fixed:** by plan 02-04 (commit 184fe855): TestScale compares against an exact area-average reference; the case passes in Release and Debug.
  **What:** `src/imageproc/tests/TestScale.cpp:68` and `:70` (`checkScale` at 50x50 and 80x80) fail on the first state that builds: `scaleToGray()` and `QImage::scaled(..., Qt::SmoothTransformation)` differ by more than 1 grey level somewhere. The case uses an unseeded `rand()`, so the input is the same on every run.
  **Why deferred:** BUILD-04 records the starting test state and Phase 1 edits only build and launch fixes (D-07, D-08). Whether this is a Qt 6 behaviour change in `QImage::scaled`, a test expectation that never held outside Qt 5, or a real difference in `scaleToGray()` is not examined. The Phase 2 baselines must treat this case as a known failure.
  **Records:** `records/ctest-summary.md`, `records/test-cases-release.txt`, `records/test-cases-debug.txt`.
