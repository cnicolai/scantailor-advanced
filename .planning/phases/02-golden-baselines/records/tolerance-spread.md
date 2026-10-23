# Comparator limits: Release/Debug spread and frozen values (plan 02-09, D-12)

## Method

- **Commit measured:** `147eff3` (HEAD at the start of plan 02-09; non-test `src` equals `cde6825` apart from `src/CMakeLists.txt`, as recorded by plans 02-05 to 02-07).
- **Presets:** `macos-release` and `macos-debug`, both built from that tree (`cmake --preset ... && cmake --build --preset ...`).
- **Runs:** `build/macos-release/pipeline_tests --log_level=message` and `build/macos-debug/pipeline_tests --log_level=message`, each without `UPDATE_GOLDENS`, both exit 0 (`*** No errors detected`). Each run printed 461 `GOLDEN-METRIC` lines; the two logs list the same suite, name and metric in the same order.
- **What was counted:** only comparisons against goldens that exist under `src/pipeline_tests/golden` (suites `flat`, `mixed`, `book`). The four lines of `GoldenStoreTestSuite` (`suite_a/page_one`, a throw-away golden in a temporary directory) are excluded. All goldens were recorded from `macos-release`, so the Release run compares a build with its own output (spread expected 0) and the Debug run gives the Release/Debug spread.
- **Comparisons:** 9 bi-level images (flat 2, mixed 2, book 5), 9 colour/grey images (flat 2, mixed 3, book 4) and 14 text goldens (385 key comparisons).
- **Parser:** a short `python3 -I` script in `build/scratch/tools` (not committed): it pairs the Release and Debug lines, takes the maximum of `value` per kind and metric over both runs, and checks that no line has `result=fail`.
- **Freezing rule (D-12):** frozen limit = the smallest grid value strictly greater than the measured maximum, capped at the ceiling. If a measured maximum had reached its ceiling, nothing would have been frozen and the plan would have stopped with a report. No maximum came near a ceiling.
- **Result:** every measured maximum is 0 (Release against itself and Debug against the Release goldens), so each limit is frozen at the first grid step above 0, the expected outcome from research spike S6 (Release, Debug and ASan outputs bit-identical).

## Image kinds

| Kind | Metric | Comparisons | Release max | Debug max | Ceiling | Step | Frozen limit |
|------|--------|-------------|-------------|-----------|---------|------|--------------|
| bilevel | edge_tolerant_xor_fraction | 9 | 0 | 0 | 0.001 | 0.0001 | 0.0001 |
| bilevel | component_count_relative | 9 | 0 | 0 | 0.02 | 0.001 | 0.001 |
| bilevel | shift | 9 | 0 | 0 | fixed | fixed | 0 (not tunable) |
| color | mae | 9 | 0 | 0 | 0.5 | 0.05 | 0.05 |
| color | max_abs_error | 9 | 0 | 0 | 8 | 1 | 1 |
| color | shift | 9 | 0 | 0 | fixed | fixed | 0 (not tunable) |

The `size` metric (0 when the sizes match) is 0 in all 18 image comparisons and has limit 0.

## Text goldens (D-14 limits unchanged)

| Kind | Metric | Comparisons | Release max | Debug max | Ceiling | Step | Frozen limit |
|------|--------|-------------|-------------|-----------|---------|------|--------------|
| text | keys ending `_deg` | 30 | 0 | 0 | D-14 | D-14 | 0.01 deg |
| text | keys ending `_px` | 190 | 0 | 0 | D-14 | D-14 | 1 px |
| text | keys ending `_mm` | 68 | 0 | 0 | D-14 | D-14 | 0.05 mm |
| text | all other keys (exact) | 97 | 0 | 0 | D-14 | D-14 | exact |

The measured spread of every text class is 0 and lies below its D-14 limit, so the text limits stay where they are.

## Where the values live

`src/test_support/Tolerances.h`: `kBilevelEdgeTolerantXorFraction = 0.0001`, `kBilevelComponentCountRelative = 0.001`, `kColorMeanAbsError = 0.05`, `kColorMaxAbsError = 1.0`. `kGeometryAngleDeg = 0.01`, `kGeometryPx = 1.0`, `kGeometryMm = 0.05` and `kShiftSearchRadius = 2` are unchanged.

## What the frozen limits mean in practice

A limit frozen just above a spread of 0 accepts only deterministic rounding differences of one grid step. Plan 02-09's `ComparatorSelfTestSuite` proves on the recorded goldens that a 1 px shift, 300 isolated specks and a +10 level change still fail under these values. A later change of toolchain (compiler, Qt, libpng) that produces output differing by more than these steps fails the golden tests on purpose: the right response is to look at `diff.png` and then either fix the cause or re-record with `UPDATE_GOLDENS=1` and a reason, not to widen a limit.
