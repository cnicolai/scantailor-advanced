# Book-page scenarios: page split and dewarping against ground truth, tolerances, goldens (plan 02-06)

Measured with Qt 6.12.0 (Homebrew), preset `macos-release`, on the unmodified production code of commit `cde6825` (the only non-test path that differs is `src/CMakeLists.txt`, the two test `add_subdirectory` lines). Every value below is a `TRUTH-METRIC suite=book` line of `build/macos-release/pipeline_tests --run_test=BookSpreadTestSuite --log_level=message` (spreads) or `--run_test=DewarpPageTestSuite` (curved page).

## Split measurements

Fixture: `PageSpec{SPREAD, dpi, seed 1, skewDeg 1.0}` for dpi 300 and 600 (1654 x 1181 px at 300 dpi, 3308 x 2362 px at 600 dpi), pipeline defaults (AUTO layout) plus `outputDpi` 600, despeckle 1.0; `BLACK_AND_WHITE` for `_bw`, `COLOR_GRAYSCALE` for `_color`. The cutter is the single line in `PipelineResult::cutters` (input coordinates, y runs 1182 to 0). The gutter is the ground truth `gutterCentre` line with half-width `gutterHalfWidthPx` (113.5 px at 300 dpi, 227 px at 600 dpi). The cutter is sampled every 10 input px from y = 0 to the image height (and at the height itself).

| Scenario | Layout type | Cutters | Cutter (x1,y1) to (x2,y2) | Worst distance outside the gutter | Worst margin to the nearer gutter edge | Offset from gutter centre at mid-height |
|----------|-------------|---------|----------------------------|-----------------------------------|----------------------------------------|-----------------------------------------|
| spread_300_bw | 2 (TWO_PAGES) | 1 | (783.217, 1182) to (809.009, 0) | 0 px | 80.55 px inside | 30.376 px |
| spread_300_color | 2 | 1 | same as `_bw` | 0 px | 80.55 px inside | 30.376 px |
| spread_600_bw | 2 | 1 | (1576.615, 2364) to (1617.878, 0) | 0 px | 171.26 px inside | 55.736 px (27.87 px in 300 dpi pixels) |
| spread_600_color | 2 | 1 | same as `_bw` | 0 px | 171.26 px inside | 55.736 px |

Split decisions are identical for black-and-white and colour/greyscale output at a given input dpi (the page_split stage runs before the output stage).

Per sub-page (deskew error is |stored angle + applied angle| in degrees; content IoU is `centredBoxIoU` of the select_content box against the truth block, each relative to the centre of its own sub-page frame, see "Truth frames" below):

| Scenario | Sub-page | Stored deskew (deg) | Deskew error (deg) | Content-box IoU | Content box (px) | Truth block (px) | Output (px) |
|----------|----------|---------------------|--------------------|-----------------|------------------|------------------|-------------|
| spread_300_* | left | -1 | 0 | 0.96814 | 640 x 812 | 625 x 805 | 1752 x 1860 |
| spread_300_* | right | -1 | 0 | 0.97972 | 634 x 810 | 625 x 805 | 1752 x 1860 |
| spread_600_* | left | -1 | 0 | 0.97731 | 1268 x 1624 | 1250 x 1610 | 1740 x 1860 |
| spread_600_* | right | -1 | 0 | 0.98282 | 1264 x 1620 | 1250 x 1610 | 1740 x 1860 |

Sanity (D-05): all eight sub-page outputs pass `outputSanity` (size from the sub-page's truth block up to 1.2 x half the spread width, ink fraction 0.5% to 50%), the text-line count within one of 15, and the left/top glyph position within 5 mm of the stored hard margins. As for the flat page (plan 02-05), the orientation is checked on the glyph ink box, not on the output with its hard margins.

### Truth frames

select_content reports the content box relative to the centre of the sub-page's bounding box in the deskewed frame. A first version of the test took that centre as the middle of each half of the page in the flat frame; that gave IoU 0.918 to 0.949 on five of eight sub-pages, because the found cutter is 28 to 30 px off the gutter centre (F-06-1) and the real sub-pages are therefore not the two halves. The truth centre is now the centre of the bounding box of the sub-page polygon (page corners and the found cutter) mapped to the flat frame with `mapImageToFlat`. That makes the IoU independent of the cutter error, which the mid-height check measures on its own. This is a change to the test's measurement, made before any bound was set; the fixture and the pipeline options were not touched.

## Tolerance decisions

Rule: D-15 as written in plan 02-04 (worst case over all fixtures of the check; at most half the starting tolerance gives twice the worst case rounded up to the step, never below one step; otherwise the starting tolerance stays; above the starting tolerance D-18 applies).

| Check | Starting tolerance | Worst case | Outcome | Constant in `TestBookSpread.cpp` |
|-------|--------------------|------------|---------|----------------------------------|
| Cutter outside the gutter, 4 scenarios, 94 samples at 300 dpi and 237 at 600 dpi each | 2 px | 0 px (at least 80.5 px inside) | tightened to one step, 0.5 px | `kMaxOutsideGutterPx = 0.5` |
| Cutter offset from the gutter centre at mid-height (D-15 point check) | 2 px | 30.376 px (300 dpi), 55.736 px (600 dpi) | kept; the worst case exceeds the starting tolerance, so D-18 applies: `XFAIL_CHECK` in all four scenarios (F-06-1) | `kMaxMidHeightOffsetPx = 2.0` |
| Sub-page deskew error, 8 sub-pages | 0.15 deg | 0 deg | tightened to one step, 0.01 deg (the fixture has one applied angle, 1.0 deg; plan 02-05's sweep shows other angles with errors up to 0.125 deg, which this check does not cover) | `kAngleToleranceDeg = 0.01` |
| Content-box IoU, 8 sub-pages | 0.95 (1 - IoU 0.05) | 1 - IoU = 0.03186 (spread_300 left) | kept: 0.03186 is above half of 0.05 | `kMinContentIou = 0.95` |

## Findings

### F-06-1: page_split puts the cutter 28 to 30 px (300 dpi) from the gutter centre
- Check: `BookSpreadTestSuite/test_spread_{300,600}_{bw,color}_split`, distance of the cutter from `gutterCentre` at mid-height.
- Measured: 30.376 px on the 300 dpi spread, 55.736 px on the 600 dpi spread (27.87 px in 300 dpi pixels); the cutter stays inside the gutter at every sample (margin 80.5 px at 300 dpi, 171.3 px at 600 dpi), so the hard check holds and no page content is cut.
- Bound: 2 px (D-15).
- Fixture: SPREAD, seed 1, skew 1.0 degrees, AUTO layout.
- Mark: `XFAIL_CHECK(midHeightOk, "page_split cutter <30.4|55.7> px from the gutter centre at mid-height on the <300|600> dpi spread, bound 2 px (F-06-1): no requirement")`
- Fixing requirement: none (no Phase 3 to 6 requirement concerns the page split position; the cutter stays in the gutter, so the output is not visibly wrong on this fixture)
- Backlog: pending (plan 02-10)
- Note: the cutter runs from (783.2, 1182) to (809.0, 0), a slant of 1.25 degrees for an applied tilt of 1.0 degrees, and the gutter centre line lies about 30 px to the right of it along the whole height (top 28 px, bottom 33 px). Research P4 measured a different cutter (795.5,0 to 873.0,1182) on a first version of the fixture. The cause was not analysed.
