# Mixed-page scenarios: despeckling, colour/greyscale and MIXED output against ground truth, tolerances, goldens (plan 02-07)

Measured with Qt 6.12.0 (Homebrew), preset `macos-release`, on the unmodified production code of commit `cde6825` (the only non-test path that differs is `src/CMakeLists.txt`, the two test `add_subdirectory` lines). Fixture: `PageSpec{MIXED, dpi, seed 1, skewDeg 0}` for dpi 300 (1000 x 1150 base px, text column 415 x 805 at (100, 150), picture 580..900 x 300..700, 40 specks) and 600; layout `SINGLE_PAGE_UNCUT` for the coverage scenarios, `outputDpi` 600, `BLACK_AND_WHITE` for `_bw`. Every value below is a `TRUTH-METRIC suite=mixed` line of `build/macos-release/pipeline_tests --run_test=MixedPageTestSuite --log_level=message`.

Mapping of truth to output: `renderInkMask(spec, 600, 0)` with a 3 px ink bar added at the right edge of the picture rectangle, then `registerMask(output, mask, 4)`; a flat-frame rectangle at the input dpi maps to the output as the rectangle scaled by 600 / input dpi and moved by the offset. The bar is needed because `registerMask` first aligns the centre of the output's non-background box (text and picture) with the centre of the mask's ink box (text only); without it the two centres differ by about 200 px and the +-4 px search cannot recover. The offset is the same for 300 and 600 dpi input and for both despeckle runs: (41, -178) px.

## Despeckle measurements

Two runs per input dpi, despeckle level 1.0 (app default) and 0.0 (off, truth check only). A speck is present when its mapped rectangle grown by 1 px holds an ink pixel (grey < 128). Glyph components are the 8-connected ink components inside the mapped text block grown by 4 px (every speck is at least 40 output px from the text).

| Input dpi | Specks in the generator | Present, despeckle off (all) | Inside the content box (text and picture box) | Of those present, despeckle off | Present, default level | Removed share | Glyph components off / default | Component change |
|-----------|-------------------------|------------------------------|-----------------------------------------------|---------------------------------|------------------------|---------------|--------------------------------|------------------|
| 300 | 40 | 16 (0.40) | 15 | 15 (1.00) | 0 | 1 | 193 / 193 | 0 |
| 600 | 40 | 12 (0.30) | 11 | 11 (1.00) | 0 | 1 | 193 / 193 | 0 |

Output (identical for both runs and both dpi): 2080 x 1856 px; content box 804 x 810 px at 300 dpi and 1608 x 1620 px at 600 dpi (truth box of text and picture 800 x 805 px at 300 dpi). One speck at 300 dpi (x = 900, one pixel right of the truth box) lies in the 2 to 4 px of slack the content box has; it is present, counted in "present (all)" and in the removed share, and not in the "inside the content box" column.

Why only 40% and 30% of the specks are present (deviation from the plan's fixture sanity, see the plan summary): the output stage keeps the content box (text and picture) and fills the rest of the page with paper. 24 of 40 specks at 300 dpi and 28 of 40 at 600 dpi lie outside that box, so they are not in the output whatever the despeckle level is. Every speck inside the box is present with despeckling off (15 of 15 and 11 of 11), so the page does test despeckling; the plan's sanity condition "at least 90% of all specks present" cannot hold for this fixture and the stop condition it comes with was therefore not applied literally. The sanity condition in the test is "at least 10 specks inside the content box and at least 90% of those present with despeckling off". The generator, its specks and the despeckle level are unchanged.

## AUTO layout observation

Fixture: 300 dpi, `BLACK_AND_WHITE`, despeckle 1.0, layout AUTO (no layout update). Case `test_mixed_auto_layout_keeps_single_page`; no golden.

| Item | Value |
|------|-------|
| Layout type | 1 (SINGLE_PAGE_CUT), expected 0 |
| Cutters | 2: (587.888, 0) to (602.929, 1149) and (892.845, 1149) to (902.872, 0), input pixels |
| Output | 860 x 780 px |
| Ink box of the output | 386 x 542 px, against 1600 x 1610 px (text and picture box at 600 dpi) and a text block of 830 px width: 0.241 of the width and 0.337 of the height of the box of text and picture; narrower than the text block, so no text column is in it |
| Ink share at the registered offset inside the mapped picture rectangle | 0 (not meaningful for a cut page, the registration is then unreliable) |
| Picture in the output | A fragment: the two cutters sit at x = 588..603 and x = 893..903, around the picture (x = 580..900); the output shows 386 x 542 px of black-and-white picture noise and no text |

This differs from research P3, which saw the first text column cropped and the picture dropped. With these generator and seed values the cut keeps part of the picture and drops the text. In both observations page_split AUTO does not leave the mixed page uncut. The case asserts `layoutType == 0`, an ink box of at least 0.9 of text-and-picture box in both directions and ink in the mapped picture rectangle, as an `XFAIL_CHECK` (D-18), so the mark fails as soon as the behaviour is fixed. The coverage scenarios force `SINGLE_PAGE_UNCUT` (research P3), so no golden depends on this behaviour.

## Tolerance decisions

Rule: D-15 as written in plan 02-04. Share steps are 0.05 percentage points (0.0005). Worst case is the shortfall from the ideal value (removed share ideal 1, glyph component change ideal 0).

| Check | Starting tolerance | Worst case | Outcome | Constant in `TestMixedPage.cpp` |
|-------|--------------------|------------|---------|---------------------------------|
| Despeckle removed share, `_bw` 300 and 600 dpi | at least 0.90 | shortfall 0 | tightened: twice 0 is 0, one step of 0.0005 gives at least 0.9995, that is every speck present with despeckling off | `kMinSpeckRemovedShare = 0.9995` |
| Glyph component change between the two despeckle runs | at most 1% | 0 | tightened: one step, at most 0.0005 (193 components, so no component may change) | `kMaxGlyphComponentChange = 0.0005` |
| Specks inside the content box present with despeckling off (fixture sanity) | at least 0.90 | 1.00 | not a D-15 value (a property of the fixture, not of the code under test), kept | `kMinSpecksPresentOff = 0.90` |
| Specks inside the content box (fixture sanity) | none in the plan | 15 and 11 | own floor of 10 | `kMinReachableSpecks = 10` |

## Findings

### F-07-1: page_split AUTO cuts the mixed page and keeps only a fragment of its picture
- Check: `MixedPageTestSuite/test_mixed_auto_layout_keeps_single_page`, layout type and ink box of the output against the box of text and picture.
- Measured: layout type 1 (SINGLE_PAGE_CUT) with 2 cutters at x = 588..603 and 893..903; ink box 386 x 542 px against 1600 x 1610 px (0.241 and 0.337); no text column in the output.
- Bound: layout type 0, ink box at least 0.9 of the box of text and picture in both directions, ink share inside the mapped picture rectangle at least 0.05.
- Fixture: MIXED, 300 dpi, seed 1, skew 0, `BLACK_AND_WHITE`, AUTO layout.
- Mark: `XFAIL_CHECK(keepsSinglePage && keepsPage, "page_split AUTO cuts the mixed page and keeps only a fragment of its picture, dropping the text: no requirement")`
- Fixing requirement: none
- Backlog: pending (plan 02-10)
