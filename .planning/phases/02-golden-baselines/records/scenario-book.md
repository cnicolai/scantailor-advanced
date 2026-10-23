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
| Cutter outside the gutter, 4 scenarios, 120 samples at 300 dpi and 238 at 600 dpi each | 2 px | 0 px (at least 80.5 px inside) | tightened to one step, 0.5 px | `kMaxOutsideGutterPx = 0.5` |
| Cutter offset from the gutter centre at mid-height (D-15 point check) | 2 px | 30.376 px (300 dpi), 55.736 px (600 dpi) | kept; the worst case exceeds the starting tolerance, so D-18 applies: `XFAIL_CHECK` in all four scenarios (F-06-1) | `kMaxMidHeightOffsetPx = 2.0` |
| Sub-page deskew error, 8 sub-pages | 0.15 deg | 0 deg | tightened to one step, 0.01 deg (the fixture has one applied angle, 1.0 deg; plan 02-05's sweep shows other angles with errors up to 0.125 deg, which this check does not cover) | `kAngleToleranceDeg = 0.01` |
| Content-box IoU, 8 sub-pages | 0.95 (1 - IoU 0.05) | 1 - IoU = 0.03186 (spread_300 left) | kept: 0.03186 is above half of 0.05 | `kMinContentIou = 0.95` |
| Dewarped line residual at 300 dpi (D-15), curved page | 1.5 px | 1.042 px (2.083 px at 600 dpi) | kept: 1.042 is above half of 1.5 (0.75); the worst case is below the starting tolerance, so the check is hard and no mark is needed | `kMaxDewarpResidual300Px = 1.5` |
| Noise floor of the line offset measure on a straight page (own guard, not D-15) | none | 0.583 px at 600 dpi | twice the worst case rounded up to 0.5 px | `kMaxNoiseFloorPx = 1.5` |

## Dewarp measurements

Fixture: `PageSpec{CURVED, 300, seed 1, skewDeg 0}` (1000 x 1150 px, 14 lines of 23 cells, curl amplitude A = 25 base px, circular-arc profile), layout `SINGLE_PAGE_UNCUT`, `BLACK_AND_WHITE`, `outputDpi` 600, despeckle 1.0; run once with `output::OFF` and once with `output::AUTO`. Both outputs are 1872 x 1824 px; the AUTO run stores a valid distortion model (`distortion_model_valid` 1) and finds all 14 text lines (14 bands over the whole ink box, 13 without dewarping).

Residual measure: for every text line and each of 8 vertical strips of the ink box, the vertical shift of the truth mask's row profile (`renderInkMask`, same glyphs, flat, at 600 dpi) that best matches the output's row profile; the residual of a line is the largest deviation of its 8 shifts from their least-squares straight line, and the page residual is the largest over the 14 lines (output pixels at 600 dpi; the D-15 bound is in 300 dpi pixels, so the value is divided by 2). A straight tilted line has residual 0. Control: the same measure on the output of the generated flat page (`FLAT`, 300 dpi, seed 1, skew 0) gives 0.583 px, so the measure's own noise is below 1 px at 600 dpi.

| Output | Line offset residual (600 dpi px) | In 300 dpi px | Worst line, strip |
|--------|-----------------------------------|---------------|-------------------|
| dewarping OFF | 7.833 | 3.917 | line 0, strip 7 |
| dewarping AUTO | 2.083 | 1.042 | line 0, strip 7 |
| ratio AUTO / OFF | 0.266 | | |

Cross-check of the OFF value against the generator's closed form: the top line is displaced by -A p(u) with A = 50 px at 600 dpi and p(u) = 1 - sqrt(1 - u^2); its least-squares straight-line residual over eight strips at u = (i + 0.5) / 8 is 6.87 px, the measure gives 7.83 px (strip quantisation and the compression of x towards the spine, which the closed form leaves out). Without dewarping the lines drift by 22 px (line 0) to 42 px (line 13) across the strips; with AUTO dewarping each line stays within 2.1 px of its straight line. In the AUTO output the line pitch is about 5 px (4.5%) larger than the truth's 110 px (shift of line 0 against line 13 in the first strip: 13 px against 78 px, 65 px over 13 pitches); that is a vertical stretch by the dewarper, not a curvature, and the measure's per-line straight-line fit removes it.

Hard truth (D-04): residual(AUTO) 2.083 px is below residual(OFF) 7.833 px. D-15 point check: 1.042 px at 300 dpi is within 1.5 px. Neither check needs an `XFAIL_CHECK`.

Reading F-04-2 (the tracer's top curve follows the second text line of this page): the AUTO output contains all 14 lines and straightens them within 2.1 px, so the tracer's top curve and the `TopBottomEdgeTracer` step of the output stage do not leave a visible curvature residual on this fixture.

### The plan's measure on the same outputs

`straightnessResidual(output, nonBackgroundBox(output), 14, 8)` as named in the plan: dewarping OFF gives no valid result (0 usable strips: seven strips have 15 and one has 16 horizontal bands instead of 14), AUTO gives 4.64 px from 3 usable strips. A sweep of the strip count (2 to 16) shows why it cannot carry the check on this page (residual in 600 dpi px, usable strips in brackets; "invalid" means fewer than two usable strips):

| Strips | OFF | AUTO |
|--------|-----|------|
| 4 | 4.11 (3) | 4.13 (4) |
| 5 | about 0 (2) | 5.73 (3) |
| 6 | 6.91 (4) | 5.33 (4) |
| 7 | about 0 (2) | 5.65 (4) |
| 8 | invalid (0) | 4.64 (3) |
| 9 | 5.41 (3) | about 0 (2) |
| 10 | 3.73 (3) | 6.69 (4) |
| 12 | invalid (1) | 4.76 (4) |
| 13 | 7.38 (4) | 6.46 (4) |
| 14 | 6.69 (4) | 7.22 (3) |
| 15 | 6.39 (5) | 6.35 (4) |
| 16 | 6.07 (4) | 8.79 (6) |

The ordering of OFF and AUTO flips with the strip count, a two-strip fit is exact by construction (residual about 0), and the values sit at 4 to 9 px for a page whose lines are, by the images and by the truth-registered measure, straight in the AUTO output and curved in the OFF output. The cause is the font: the dots of `i` and `j` form a band of their own in a strip that has no ascender, and the ink-weighted centre of a band moves by several pixels with the ascenders and descenders in the strip. The plan's call is therefore kept in the test as a reported reference (`*_plan_straightness_*` lines) and the checks use the truth-registered measure above; see deviation 1 of the plan summary. `src/test_support` is frozen and was not changed.

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

### F-06-2 (test-support limitation, not a production defect): `straightnessResidual` cannot measure this page
- Check: the plan's hard truth and D-15 bound for the curved page, as `straightnessResidual(output, nonBackgroundBox(output), lineCount, 8)` on the OFF and AUTO outputs.
- Measured: no valid result for the output without dewarping (0 of 8 strips usable); 4.64 px for AUTO; ordering of OFF and AUTO depends on the strip count (table above).
- Consequence: `src/pipeline_tests/TestBookSpread.cpp` measures the line offsets against the truth mask (noise floor 0.583 px on a straight page) and reports the plan's measure as reference only.
- Fixing requirement: none; plan 02-10 may want a backlog item to give `src/test_support` a glyph-independent straightness measure.
- Backlog: pending (plan 02-10)

## Goldens

Recorded with `UPDATE_GOLDENS=1 build/macos-release/pipeline_tests --run_test=BookSpreadTestSuite --run_test=DewarpPageTestSuite` from a tree whose non-test paths equal `cde6825` apart from `src/CMakeLists.txt` (the plan's gate command printed `src/CMakeLists.txt` only; `src/test_support` and `HeadlessPipeline.*` are unchanged by this plan). Every golden logged `RECORDED book/<name>` and no `sanity failed`. The suite directory is `src/pipeline_tests/golden/book/`. The test code was committed first (`235e5a7`, `test(02-06): check book-spread and dewarp goldens`), the recording ran on that committed tree, so the manifest names `source_commit = 235e5a7`, `build_type = Release` and `ffp_contract = off`. Golden commit: `5afce63` (`test(02-06): record book-spread and dewarp goldens from macos-release`, sixteen files, all under `golden/book/`, body starts with `Reason: initial baseline from unmodified production code`).

| File | Kind | Bytes | Pixels |
|------|------|-------|--------|
| spread_300_bw_left.png | bi-level (1 bit) | 16543 | 1752 x 1860 |
| spread_300_bw_right.png | bi-level (1 bit) | 15525 | 1752 x 1860 |
| spread_300_bw.txt | text, 41 keys | 1112 | n/a |
| spread_300_color_left.png | colour (8 bit RGB) | 1351465 | 1752 x 1860 |
| spread_300_color_right.png | colour (8 bit RGB) | 1315723 | 1752 x 1860 |
| spread_300_color.txt | text, 41 keys | 1112 | n/a |
| spread_600_bw_left.png | bi-level (1 bit) | 12686 | 1740 x 1860 |
| spread_600_bw_right.png | bi-level (1 bit) | 13361 | 1740 x 1860 |
| spread_600_bw.txt | text, 41 keys | 1121 | n/a |
| spread_600_color_left.png | colour (8 bit RGB) | 1468229 | 1740 x 1860 |
| spread_600_color_right.png | colour (8 bit RGB) | 1451636 | 1740 x 1860 |
| spread_600_color.txt | text, 41 keys | 1121 | n/a |
| curved_300_dewarp.png | bi-level (1 bit), AUTO output | 19134 | 1872 x 1824 |
| curved_300_dewarp.txt | text, 8 keys (`residual_off_px`, `residual_auto_px`, `distortion_model_valid`, size, dpi, seed, input dpi) | 158 | n/a |
| fixtures.txt | text, 3 keys (`spread_300`, `spread_600`, `curved_300`) | 234 | n/a |
| MANIFEST.txt | toolchain | 333 | n/a |

The spread goldens record the behaviour of F-06-1 (cutter off the gutter centre) and the dewarp golden the AUTO output of the unmodified code, as D-18 requires. Spread text goldens: `layout_type = 2`, `cutter_count = 1`.

Checks after recording: both suites pass again in Release without `UPDATE_GOLDENS` (211 `GOLDEN-METRIC suite=book` lines, none with `result=fail`, `git status` shows nothing under `golden/book`); `ctest --preset macos-debug -R pipeline_tests` passes (160 s for the whole executable) against the Release goldens; in Release every colour golden reproduces with `max_abs_error` 0.
