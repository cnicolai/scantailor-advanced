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
