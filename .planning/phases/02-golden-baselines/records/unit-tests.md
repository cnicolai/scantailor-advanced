# Unit tests: D-17 investigation, binarization and dewarping measurements, findings (plan 02-04)

Measured with Qt 6.12.0 (Homebrew), preset `macos-release` unless stated, on the unmodified production code of commit `cde6825` (`src/imageproc` and `src/dewarping` are not touched by this plan).

## D-17 TestScale investigation

Question: `ScaleTestSuite/test_random_image` failed at its two checks (50x50 and 80x80) since the Qt 6 port. Which side moved, `scaleToGray()` or the `QImage::scaled(..., Qt::SmoothTransformation)` reference?

Method: a throw-away test case (not committed) scaled one 100x100 grey image to 50x50 and to 80x80 with `scaleToGray()`, with `QImage::scaled()` (converted back through `GrayImage`; Qt returns `Format_ARGB32` here) and with an exact area-average reference computed in double precision (each output pixel is the area-weighted mean of the input pixels its footprint covers, rounded half up). Two inputs: the one the old test used (unseeded C library random, run with its default state) and a `test_support::Rng` input (seed 7, `below(256)`). "over 1" counts output pixels whose absolute difference exceeds 1, the tolerance of the test. Means are the mean grey values of the two images of the pair.

| Input | Size | Pair | Max abs diff | Pixels over 1 (of) | Mean A | Mean B |
|-------|------|------|--------------|--------------------|--------|--------|
| C library random | 50x50 | `scaleToGray` vs area average | 0 | 0 (2500) | 127.9224 | 127.9224 |
| C library random | 50x50 | `QImage::scaled` vs area average | 3 | 613 (2500) | 126.8064 | 127.9224 |
| C library random | 80x80 | `scaleToGray` vs area average | 0 | 0 (6400) | 127.8047 | 127.8047 |
| C library random | 80x80 | `QImage::scaled` vs area average | 28 | 5131 (6400) | 126.8448 | 127.8047 |
| Rng seed 7 | 50x50 | `scaleToGray` vs area average | 0 | 0 (2500) | 128.7156 | 128.7156 |
| Rng seed 7 | 50x50 | `QImage::scaled` vs area average | 3 | 588 (2500) | 127.6020 | 128.7156 |
| Rng seed 7 | 80x80 | `scaleToGray` vs area average | 0 | 0 (6400) | 128.6030 | 128.6030 |
| Rng seed 7 | 80x80 | `QImage::scaled` vs area average | 30 | 5131 (6400) | 127.6250 | 128.6030 |

`scaleToGray` against `QImage::scaled` is the same pair as the second row of each group (the first row of a group is identical to the area average, so the differences equal).

Result: `scaleToGray()` equals the exact area average at both sizes on both inputs (maximum difference 0, same mean). Qt's smooth scaling is about 1.1 grey levels darker on average and up to 30 levels away on the 80x80 downscale. The reference moved, `scaleToGray()` did not (Branch A, as predicted by research P12). Decision: `TestScale.cpp` compares against the area-average reference (a static helper in the test) with the old tolerance of 1, and the input comes from the seeded `Rng`. Production code is unchanged.

## Binarization measurements

Fixture: `PageSpec{FLAT, 300 dpi, seed 7, skew 0}` converted to grey. "even" is the page as generated; "uneven" scales every pixel by target(x) / 233 in integer arithmetic with target(x) = 235 - 85 x / (width - 1), so the paper falls linearly from 235 to 150 grey levels from left to right (ink from 31 to 20). Truth: `renderInkMask(spec, 300, 0)`, offset (0, 0). Metric: `truthMismatchFraction` (share of truth ink pixels the output gets wrong). The starting bound of D-15 is 0.5% of the ink area (0.005).

The global thresholds (Otsu, Mokji) get the page as an Indexed8 grey image, because `BinaryImage(QImage, threshold)` throws `std::runtime_error("Unsupported QImage format")` for `Format_Grayscale8`; see F-04-1. The local binarizers take `Format_Grayscale8` as the plan names it.

| Method | Fixture | Parameters | Mismatch, edge radius 1 (Release) | Mismatch, edge radius 1 (Debug) | Mismatch, edge radius 0 (Release, reference only) | Bound set |
|--------|---------|------------|-----------------------------------|---------------------------------|---------------------------------------------------|-----------|
| Otsu | even | global | 0 | 0 | 0 | 0.0005 |
| Mokji | even | global, defaults 3 / 20 | 0 | 0 | 0 | 0.0005 |
| Sauvola | even | window 200, k 0.34 | 0 | 0 | 0 | 0.0005 |
| Wolf | even | window 200, bounds 1 / 254, k 0.3 | 0 | 0 | 0 | 0.0005 |
| Fox | even | window 200, bounds 1 / 254, k 0.3 | 0 | 0 | 0 | 0.0005 |
| Window | even | window 200, bounds 1 / 254, k 0.33 | 0 | 0 | 0 | 0.0005 |
| Bradley | even | window 200, k 0.34 | 0 | 0 | 0 | 0.0005 |
| Grad | even | window 200, bounds 1 / 254, k 0.3 | 0 | 0 | 0 | 0.0005 |
| EdgeDiv | even | window 200, kep 0, kdb 0 | 0 | 0 | 0 | 0.0005 |
| Sauvola | uneven | window 200, k 0.34 | 0 | 0 | 0 | 0.0005 |
| Wolf | uneven | window 200, bounds 1 / 254, k 0.3 | 0 | 0 | 0 | 0.0005 |
| Fox | uneven | window 200, bounds 1 / 254, k 0.3 | 0 | 0 | 0 | 0.0005 |
| Window | uneven | window 200, bounds 1 / 254, k 0.33 | 0 | 0 | 0 | 0.0005 |
| Bradley | uneven | window 200, k 0.34 | 0 | 0 | 0 | 0.0005 |
| Grad | uneven | window 200, bounds 1 / 254, k 0.3 | 0 | 0 | 0 | 0.0005 |
| EdgeDiv | uneven | window 200, kep 0, kdb 0 | 0 | 0 | 0 | 0.0005 |

Every binarizer reproduces the truth mask pixel for pixel, with and without the edge tolerance. The likely reason is the fixture, not luck: a glyph edge pixel with at least 5 of its 9 samples inked has grey at most about 121 and one with 4 at least about 143 (ink 31, paper 233), so the mask's 5-of-9 rule is a threshold at the middle of the ink-paper range, and every binarizer here lands on the same side of it for these two-level pages. This was not examined per binarizer. The zero is not a vacuous metric: `test_ground_truth_metric_rejects_wrong_output` shows a blank page scoring above 0.9 and a copy shifted by 3 px scoring above the bound, and `registerMask(..., 4)` finds offset (0, 0) for all nine binarizers on the even page.

D-15 tightening (worst case 0, at most half the starting bound): twice the worst case is 0, rounded up to the step of 0.05 percentage points and never below one step, so every bound is 0.0005 (0.05% of the ink area). No binarizer exceeds the starting bound, so D-18 marks none of the mismatch checks.

## Findings

### F-04-1: global thresholds reject Format_Grayscale8
- Check: `BinarizeTestSuite/test_otsu_on_flat_page` and `test_mokji_on_flat_page`, first assertion (`binarizeOtsu` / `binarizeMokji` on the generated page as `Format_Grayscale8` must not throw)
- Measured: both throw `std::runtime_error: Unsupported QImage format` from `BinaryImage::BinaryImage(const QImage&, BinaryThreshold)` (`src/imageproc/BinaryImage.cpp:142`)
- Bound: no exception
- Fixture: FLAT, 300 dpi, seed 7, converted to `Format_Grayscale8`
- Mark: `XFAIL_CHECK(..., "global thresholds reject Format_Grayscale8 (std::runtime_error): no requirement")`
- Fixing requirement: none (no production caller passes this format: `BlackOnWhiteEstimator` and `PageFinder` pass the Indexed8 result of `transformToGray`; the local binarizers accept the format)
- Backlog: pending (plan 02-10)
