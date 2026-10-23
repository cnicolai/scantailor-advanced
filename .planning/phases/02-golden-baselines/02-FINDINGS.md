# Phase 2 findings: what the golden-baseline tests found in the unmodified code

The Phase 2 tests ran against production code that equals commit `cde6825` apart from the three `add_subdirectory` lines in `src/CMakeLists.txt` (proof in `records/closing-check.md`). They found the seven items below. Each is recorded, marked in the tests where a check cannot pass, and not fixed (D-18). Each has an entry `### Phase 999.N` in the `## Backlog` section of `.planning/ROADMAP.md`, and its number is written back into the record it came from. Six items are behaviours of the application code; F-06-2 is a limitation of the test support (`src/test_support`), not of the application. F-04-1, F-04-2, F-05-1, F-06-1 and F-07-1 carry an `XFAIL_CHECK`: it records the failure without failing the run and turns into an error as soon as the defect is fixed, so whoever fixes the defect has to remove the mark. F-06-2 and F-08-1 carry no mark.

None of the marks names a requirement that will fix it, because none of the roadmap's requirements (Phases 3 to 6) covers these defects; each tag says `no requirement`.

| ID | Title | Check | Measured | Bound | XFAIL tag | Fixing requirement | Backlog |
|----|-------|-------|----------|-------|-----------|--------------------|---------|
| F-04-1 | Global binarizers reject `Format_Grayscale8` | `BinarizeTestSuite/test_otsu_on_flat_page`, `test_mokji_on_flat_page`: first assertion | `std::runtime_error: Unsupported QImage format` (`BinaryImage.cpp:142`) | no exception | `global thresholds reject Format_Grayscale8 (std::runtime_error): no requirement` | none | 999.1 |
| F-04-2 | The distortion model can leave out the first or last text line | `DewarpingTextLineTracerTestSuite/test_model_curves_follow_first_and_last_lines`, top curve | 64.40 px from the first line (5.73 px from the second) | 55 px | `model top curve skips the first text line of the curved page (seed 1): no requirement` | none | 999.2 |
| F-05-1 | Output of the 600 dpi flat page keeps about 0.1 degrees of residual skew | `FlatDocumentTestSuite/test_flat_600_bw_truth`, binarization mismatch | 0.0829038 (0 at 300 dpi) | 0.005 | `output of the 600 dpi flat page keeps about 0.1 deg residual skew, binarization mismatch 8.3% against 0.5% (F-05-1): no requirement` | none | 999.3 |
| F-06-1 | page_split puts the cutter 28 to 30 px from the gutter centre | `BookSpreadTestSuite/test_spread_{300,600}_{bw,color}_split`, cutter distance at mid-height | 30.376 px at 300 dpi, 55.736 px at 600 dpi | 2 px | `page_split cutter 30.4 px from the gutter centre at mid-height on the 300 dpi spread, bound 2 px (F-06-1): no requirement` (and the 55.7 px / 600 dpi variant) | none | 999.4 |
| F-06-2 | Test support: `straightnessResidual` cannot measure the curved book page | the plan's straightness bound on the OFF and AUTO outputs | no valid result without dewarping (0 of 8 strips); 4.64 px for AUTO; 4 to 9 px noise floor | D-15 bound of the plan | none (test-support limitation) | none | 999.5 |
| F-07-1 | page_split AUTO cuts the mixed page and keeps only a fragment of its picture | `MixedPageTestSuite/test_mixed_auto_layout_keeps_single_page` | layout type 1 with 2 cutters; ink box 0.241 x 0.337 of the text and picture box | layout type 0; ink box at least 0.9 in both directions | `page_split AUTO cuts the mixed page and keeps only a fragment of its picture, dropping the text: no requirement` | none | 999.6 |
| F-08-1 | A layout type from the default profile is not written to the project file until page_split has run | `ProjectRoundTripTestSuite/test_every_stage_carries_non_default_values`, element `<page-split>` | `<page-split defaultLayoutType="auto-detect"/>`, identical to the default-profile project | element differs from the default-profile project | none (the test runs the stage first) | none | 999.7 |

### F-04-1: global thresholds reject Format_Grayscale8

- Check: `BinarizeTestSuite/test_otsu_on_flat_page` and `test_mokji_on_flat_page`, first assertion (`binarizeOtsu` / `binarizeMokji` on the generated page as `Format_Grayscale8` must not throw).
- Measured: both throw `std::runtime_error: Unsupported QImage format` from `BinaryImage::BinaryImage(const QImage&, BinaryThreshold)` (`src/imageproc/BinaryImage.cpp:142`).
- Bound: no exception.
- Fixture: FLAT, 300 dpi, seed 7, converted to `Format_Grayscale8`.
- Mark: `XFAIL_CHECK(..., "global thresholds reject Format_Grayscale8 (std::runtime_error): no requirement")` in `src/imageproc/tests/TestBinarize.cpp`.
- Fixing requirement: none. No production caller passes this format: `BlackOnWhiteEstimator` and `PageFinder` pass the Indexed8 result of `transformToGray`; the local binarizers accept the format.
- Backlog: 999.1

### F-04-2: the distortion model can leave out the first or last text line

- Check: `DewarpingTextLineTracerTestSuite/test_model_curves_follow_first_and_last_lines`, top curve against the centre of the first text line.
- Measured: 64.40 px at the worst of nine x positions (at 845.8 px, the right end); the same curve is within 5.73 px of the second text line. `TextLineTracer` traces the first line, but `DistortionModelBuilder::tryBuildModel` builds a model whose top curve is the second line.
- Bound: 55 px (one text line pitch at 300 dpi).
- Fixture: CURVED, 300 dpi, seed 1. A survey with seeds 2 to 6 (not part of the test, Release): seeds 1 and 4 lose the first line, seed 6 the last, seeds 2, 3 and 5 follow both.
- Mark: `XFAIL_CHECK(topDistance <= kStartBoundPx, "model top curve skips the first text line of the curved page (seed 1): no requirement")` in `src/dewarping/tests/TestTextLineTracer.cpp`.
- Fixing requirement: none. The production dewarping stage adds the page edges with `TopBottomEdgeTracer` after the tracer and falls back to a trivial model when the result is invalid, which this unit test does not run; whether the first line then lies outside the model domain is not shown by the unit test.
- Backlog: 999.2

### F-05-1: Output of the 600 dpi flat page keeps about 0.1 degrees of residual skew

- Check: `FlatDocumentTestSuite/test_flat_600_bw_truth`, binarization mismatch of the output against `renderInkMask(spec, 600, netRotation)` after `registerMask`.
- Measured: 0.0829038 (edge-tolerant mismatch, edge radius 1) at 600 dpi input; 0 at 300 dpi input (edge radius 2).
- Bound: 0.005.
- Fixture: FLAT, 600 dpi, seed 1, skew 1.5 degrees, black-and-white output at 600 dpi.
- Mark: `XFAIL_CHECK(binarizationOk, "output of the 600 dpi flat page keeps about 0.1 deg residual skew, binarization mismatch 8.3% against 0.5% (F-05-1): no requirement")` in `src/pipeline_tests/TestFlatDocument.cpp`.
- Evidence: the stored deskew angle is exactly -1.5 degrees and the generated input matches the truth mask rotated by 1.5 degrees; in the output the best-registered local offset changes with the row (-1, 0 and +2 px in three bands at 600 dpi), and a search over the truth mask rotation gives the best match at -0.08 to -0.12 degrees (mismatch 0.0070 at -0.12). The cause inside the output or deskew transformation was not analysed. The golden `flat_600_bw` records this behaviour (D-18).
- Fixing requirement: none.
- Backlog: 999.3

### F-06-1: page_split puts the cutter 28 to 30 px (300 dpi) from the gutter centre

- Check: `BookSpreadTestSuite/test_spread_{300,600}_{bw,color}_split`, distance of the cutter from `gutterCentre` at mid-height.
- Measured: 30.376 px on the 300 dpi spread, 55.736 px on the 600 dpi spread (27.87 px in 300 dpi pixels). The cutter stays inside the gutter at every sample (margin 80.5 px at 300 dpi, 171.3 px at 600 dpi), so the hard check holds and no page content is cut.
- Bound: 2 px (D-15).
- Fixture: SPREAD, seed 1, skew 1.0 degrees, AUTO layout.
- Mark: `XFAIL_CHECK(midHeightOk, <tag>)` in `src/pipeline_tests/TestBookSpread.cpp`, four scenarios; tag `page_split cutter <30.4|55.7> px from the gutter centre at mid-height on the <300|600> dpi spread, bound 2 px (F-06-1): no requirement`.
- Note: the cutter runs from (783.2, 1182) to (809.0, 0), a slant of 1.25 degrees for an applied tilt of 1.0 degrees, and the gutter centre line lies about 30 px to the right of it along the whole height. The cause was not analysed.
- Fixing requirement: none. No Phase 3 to 6 requirement concerns the page split position; the cutter stays in the gutter, so the output is not visibly wrong on this fixture.
- Backlog: 999.4

### F-06-2: test-support limitation, not a production defect: `straightnessResidual` cannot measure the curved book page

- Check: the plan's hard truth and D-15 bound for the curved page, as `straightnessResidual(output, nonBackgroundBox(output), lineCount, 8)` on the OFF and AUTO outputs.
- Measured: no valid result for the output without dewarping (0 of 8 strips usable); 4.64 px for AUTO; the order of OFF and AUTO depends on the strip count; values of 4 to 9 px for a page whose lines are straight in the AUTO output by the truth-registered measure. The cause is the font: the dots of `i` and `j` form a band of their own in a strip without ascender, and the ink-weighted centre of a band moves by several pixels with the ascenders and descenders in the strip.
- Consequence: `src/pipeline_tests/TestBookSpread.cpp` measures the line offsets against the truth mask (noise floor 0.583 px on a straight page) and reports the plan's measure as reference only. `src/test_support` is frozen in Phase 2 and was not changed.
- Mark: none.
- Fixing requirement: none. The backlog item is to give `src/test_support` a glyph-independent straightness measure.
- Backlog: 999.5

### F-07-1: page_split AUTO cuts the mixed page and keeps only a fragment of its picture

- Check: `MixedPageTestSuite/test_mixed_auto_layout_keeps_single_page`, layout type and ink box of the output against the box of text and picture.
- Measured: layout type 1 (SINGLE_PAGE_CUT) with 2 cutters at x = 588..603 and 893..903; ink box 386 x 542 px against 1600 x 1610 px (0.241 and 0.337); no text column in the output.
- Bound: layout type 0, ink box at least 0.9 of the box of text and picture in both directions, ink share inside the mapped picture rectangle at least 0.05.
- Fixture: MIXED, 300 dpi, seed 1, skew 0, `BLACK_AND_WHITE`, AUTO layout. The coverage scenarios force SINGLE_PAGE_UNCUT.
- Mark: `XFAIL_CHECK(keepsSinglePage && keepsPage, "page_split AUTO cuts the mixed page and keeps only a fragment of its picture, dropping the text: no requirement")` in `src/pipeline_tests/TestMixedPage.cpp`.
- Fixing requirement: none. The cause was not analysed.
- Backlog: 999.6

### F-08-1: a layout type from the default profile is not written to the project file until page_split has run

- Check: `ProjectRoundTripTestSuite/test_every_stage_carries_non_default_values`, element `<page-split>` of the non-default project against the default one.
- Measured: after `loadDefaultSettings` alone (profile with `SINGLE_PAGE_UNCUT`) the written element is `<page-split defaultLayoutType="auto-detect"/>` with no `<image>` child, identical to the default-profile project. `page_split::Filter::writeImageSettings` appends an `<image>` element only inside `if (const Params* params = record.params())`, and `defaultLayoutType` is a `Settings`-level value that `DefaultParams` never sets. The application loads defaults at processing time, so a project saved after the stage ran carries the layout; a project saved before that does not.
- Bound: the element differs from the default-profile project.
- Fixture: five pages, 64 x 64 px, 300 dpi, layout `SINGLE_PAGE_UNCUT` in the profile.
- Mark: none. The test runs the page_split stage for every page first, after which the element carries `layoutType="single-uncut"` and the check passes. The behaviour may be by design for a stage whose output is the parameters; the backlog item holds the question open.
- Fixing requirement: none.
- Backlog: 999.7

## Not defects of the application, recorded for completeness

- **D7, despeckle fixture sanity (plan 02-07).** The plan's sanity condition "at least 90% of the specks present with despeckling off" cannot hold for the generated mixed page: the output keeps only the content box, and 24 of 40 (300 dpi) and 28 of 40 (600 dpi) specks lie outside it. The test counts the specks inside the content box (15 of 15 and 11 of 11 present with despeckling off) and reports the raw share as `specks_present_despeckle_off_share_of_all`. This is a deviation from the plan's wording, not a defect, and it removes nothing from the despeckle truth. It is in `.planning/WINDOWS.md` as an open deviation and is for the user to accept or not. No backlog item.
- **Comparator self-test and the frozen limits (plan 02-09).** The self-test (`ComparatorSelfTestSuite`) fails its altered goldens under the frozen limits, but it fails them equally under the D-12 ceilings (300 specks: `component_count_relative` 1.21; +10 levels: `max_abs_error` 10 against ceiling 8), so it does not distinguish the frozen limits from the ceilings. That distinction rests on the measurement in `records/tolerance-spread.md` (spread 0 over all goldens). An additional alteration of +2 grey levels (fails at limit 1, passes at ceiling 8) would close this; it was not in the plan. No backlog item.
- **Expected-failure self-tests.** `src/pipeline_tests/TestGoldenStore.cpp` raises two marks on purpose to test the trip-wire of `XFAIL_CHECK`: `XFAIL_CHECK(false, "no requirement: defect still present (logs only)")` and `XFAIL_CHECK(true, "no requirement: raised on purpose by the test of the trip-wire")`. They test the mechanism and do not refer to a defect. `src/test_support/ExpectedFailure.h` shows the form `XFAIL_CHECK(condition, "tag")` in a comment.
- **Sanitizer reports.** Twelve UBSan reports at nine locations (two known from Phase 1, seven new) are in `records/sanitizer-findings.md`; they are recorded, not fixed (D-19), and not repeated here.
