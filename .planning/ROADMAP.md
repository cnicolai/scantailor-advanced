# Roadmap: ScanTailor Advanced (personal macOS fork)

## Milestones

- ✅ **v1.0 macOS** — Phases 1–6 and 06.1 (shipped 2026-11-18) — [archive](milestones/v1.0-ROADMAP.md)

## Phases

<details>
<summary>✅ v1.0 macOS (Phases 1–6, 06.1) — SHIPPED 2026-11-18</summary>

- [x] Phase 1: Deletion-Free macOS Build (7/7 plans) — completed 2026-10-23
- [x] Phase 2: Golden Baselines (14/14 plans) — completed 2026-10-24
- [x] Phase 3: Qt 6 and macOS Only (9/9 plans) — completed 2026-10-24
- [x] Phase 4: Retina Rendering (8/8 plans) — completed 2026-10-25
- [x] Phase 5: Mac Menus and Shortcuts (11/11 plans) — completed 2026-11-15
- [x] Phase 6: Hardening and Cleanup (16/16 plans) — completed 2026-11-17
- [x] Phase 06.1: Address tech debt: post-audit cleanup (INSERTED, 12/12 plans) — completed 2026-11-18

Full phase details: `milestones/v1.0-ROADMAP.md`. Phase directories: `milestones/v1.0-phases/`.

</details>

The next milestone is not defined yet. Start it with `/gsd-new-milestone`; continue phase numbering at 7.

---
*Roadmap created: 2026-10-18 · reorganised after v1.0: 2026-11-18*

## Backlog

### Phase 999.1: Global binarizers reject Format_Grayscale8 input (BACKLOG)

**Goal:** `BinaryImage::BinaryImage(const QImage&, BinaryThreshold)` (`src/imageproc/BinaryImage.cpp:142`), used by `binarizeOtsu` and `binarizeMokji`, throws `std::runtime_error: Unsupported QImage format` for `Format_Grayscale8` input; the check requires no exception (measured: throws, bound: no exception, FLAT page, 300 dpi, seed 7). No production caller passes this format, so no requirement exists. When fixed, remove the `XFAIL_CHECK` tagged `global thresholds reject Format_Grayscale8 (std::runtime_error): no requirement` in `src/imageproc/tests/TestBinarize.cpp`. Finding F-04-1 in `.planning/milestones/v1.0-phases/02-golden-baselines/02-FINDINGS.md`.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.2: Distortion model can leave out the first or last text line (BACKLOG)

**Goal:** `DistortionModelBuilder::tryBuildModel` builds a model whose top curve follows the second text line of the generated curved page (seed 1, 300 dpi) although `TextLineTracer` traced the first: the top curve lies 64.40 px from the first line against a bound of 55 px (one line pitch), and 5.73 px from the second. The production dewarping stage adds the page edges afterwards and falls back to a trivial model, which this unit test does not run. No requirement exists. When fixed, remove the `XFAIL_CHECK` tagged `model top curve skips the first text line of the curved page (seed 1): no requirement` in `src/dewarping/tests/TestTextLineTracer.cpp`. Finding F-04-2 in `.planning/milestones/v1.0-phases/02-golden-baselines/02-FINDINGS.md`.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.3: Output of the 600 dpi flat page keeps about 0.1 degrees of residual skew (BACKLOG)

**Goal:** On the generated flat page (skew 1.5 degrees, black-and-white output) the 600 dpi input leaves a residual rotation of about 0.08 to 0.12 degrees that the stored deskew angle (exactly -1.5 degrees) does not show: binarization mismatch against the truth mask 0.0829 against a bound of 0.005 (0 at 300 dpi). The cause inside the output or deskew transformation was not analysed. No requirement exists. When fixed, remove the `XFAIL_CHECK` tagged `output of the 600 dpi flat page keeps about 0.1 deg residual skew, binarization mismatch 8.3% against 0.5% (F-05-1): no requirement` in `src/pipeline_tests/TestFlatDocument.cpp` and re-record the golden `flat_600_bw` (it records this behaviour). Finding F-05-1 in `.planning/milestones/v1.0-phases/02-golden-baselines/02-FINDINGS.md`.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.4: page_split puts the cutter 28 to 30 px from the gutter centre (BACKLOG)

**Goal:** On the generated book spread (seed 1, skew 1.0 degree, AUTO layout) the page_split cutter lies 30.376 px (300 dpi) and 55.736 px (600 dpi, 27.87 px in 300 dpi pixels) from the gutter centre at mid-height against a bound of 2 px; it stays inside the gutter at every sample (margin 80.5 px at 300 dpi), so no content is cut. The cause was not analysed. No requirement exists. When fixed, remove the four `XFAIL_CHECK` tags `page_split cutter <30.4|55.7> px from the gutter centre at mid-height on the <300|600> dpi spread, bound 2 px (F-06-1): no requirement` in `src/pipeline_tests/TestBookSpread.cpp` and re-record the spread goldens. Finding F-06-1 in `.planning/milestones/v1.0-phases/02-golden-baselines/02-FINDINGS.md`.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.5: Test support: straightnessResidual cannot measure the curved book page (BACKLOG)

**Goal:** Limitation of `src/test_support`, not of the application: `straightnessResidual` finds no valid strip (0 of 8) on the output of the curved page without dewarping and has a 4 to 9 px noise floor from glyph content (dots of `i` and `j`, ascenders, descenders) on the AUTO output, so `src/pipeline_tests/TestBookSpread.cpp` measures line offsets against the truth mask (noise floor 0.583 px on a straight page) and reports the plan's measure as reference only. Goal: give `src/test_support` a glyph-independent straightness measure. No requirement exists and no expected-failure mark exists to remove. Finding F-06-2 in `.planning/milestones/v1.0-phases/02-golden-baselines/02-FINDINGS.md`.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.6: page_split AUTO cuts the mixed page and keeps only a fragment of its picture (BACKLOG)

**Goal:** On the generated mixed page (300 dpi, seed 1, skew 0) page_split AUTO chooses layout type 1 (SINGLE_PAGE_CUT) with two cutters at x = 588..603 and 893..903, and the output keeps an ink box of 386 x 542 px against 1600 x 1610 px for text and picture (0.241 and 0.337 of the box, bound 0.9), dropping the text column. The coverage scenarios therefore force SINGLE_PAGE_UNCUT. The cause was not analysed. No requirement exists. When fixed, remove the `XFAIL_CHECK` tagged `page_split AUTO cuts the mixed page and keeps only a fragment of its picture, dropping the text: no requirement` in `src/pipeline_tests/TestMixedPage.cpp` and consider the forced layout in the mixed scenarios. Finding F-07-1 in `.planning/milestones/v1.0-phases/02-golden-baselines/02-FINDINGS.md`.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.7: A layout type from the default profile is not written to the project file until page_split has run (BACKLOG)

**Goal:** After `loadDefaultSettings` alone (profile with `SINGLE_PAGE_UNCUT`) the written element is `<page-split defaultLayoutType="auto-detect"/>` with no `<image>` child, identical to the default-profile project: `page_split::Filter::writeImageSettings` writes an `<image>` element only when the page has params, and `defaultLayoutType` is a `Settings`-level value that `DefaultParams` never sets. The application loads defaults at processing time, so a project saved before page_split has run carries no layout type for its pages. Possibly by design; the decision is open. No requirement exists and no expected-failure mark exists (the test runs the stage first). Finding F-08-1 in `.planning/milestones/v1.0-phases/02-golden-baselines/02-FINDINGS.md`.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.8: RelinkablePath::normalize turns a backslash in a macOS file name into a path separator (BACKLOG)

**Goal:** `RelinkablePath::normalize` (`src/core/RelinkablePath.cpp`) replaces every backslash by a slash before splitting the path and, when resolving `..`, refuses a previous component that ends in `:` (a Windows drive letter). On macOS a backslash is a legal file-name character, so by reading the code `normalize("/Users/me/scan\\01.tif")` returns `/Users/me/scan/01.tif` (not executed). Phase 3 keeps this logic because it runs on the Mac and removing it would change behaviour (D-03). Fix it with a test in a later phase; decide first whether project files written by Windows builds must still be read. No requirement exists. Source: Phase 3 CONTEXT D-03.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)
