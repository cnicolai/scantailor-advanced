---
phase: 02-golden-baselines
plan: 06
subsystem: testing
tags: [boost-test, golden-images, ground-truth, page-split, dewarping, text-goldens, synthetic-fixtures, expected-failure]

requires:
  - phase: 02-golden-baselines
    provides: "02-01 generator and pipeline_tests fixture; 02-02 GoldenStore, comparator, text goldens, XFAIL_CHECK; 02-03 TruthMetrics, frozen HeadlessPipeline; 02-05 scenario runner pattern and records conventions"
provides:
  - "BookSpreadTestSuite: four spread scenarios (300/600 dpi, black-and-white and colour/greyscale) checking TWO_PAGES layout, cutter inside the gutter at every sampled height, the D-15 mid-height offset, per-sub-page deskew and content-box IoU, plus a fixture fingerprint case"
  - "DewarpPageTestSuite: AUTO dewarping against the generator's straight lines on the curved page (AUTO 2.08 px against 7.83 px without dewarping, 1.04 px at 300 dpi against the 1.5 px bound) and a noise-floor guard for the measure"
  - "golden/book: eight sub-page images, four spread text goldens, the AUTO-dewarped curved page with its text golden, fixtures.txt (3 keys) and MANIFEST.txt, recorded from macos-release"
  - "records/scenario-book.md: split and dewarp measurements, tolerance decisions, strip-count sweep of the plan's straightness measure, golden list, findings F-06-1 and F-06-2"
affects: [02-09, 02-10]

actuals:
  tokens: 14058
  tasks: 3
  commits: 5

tech-stack:
  added: []
  patterns:
    - "Spread truth frames: the centre of a sub-page's bounding box in the flat frame is found by mapping the sub-page polygon (page corners and the found cutter) with mapImageToFlat, so content IoU does not depend on the cutter error"
    - "Truth-registered line offsets: the row profile of the truth mask (same glyphs, flat, at the output dpi) is matched per line and strip against the output; the residual is the deviation of the shifts from a straight line"
    - "A measure gets a guard case on a page whose answer is known (flat page, noise floor 0.58 px) next to the case that uses it"

key-files:
  created:
    - src/pipeline_tests/TestBookSpread.cpp
    - src/pipeline_tests/golden/book/ (16 files)
    - .planning/phases/02-golden-baselines/records/scenario-book.md
  modified:
    - .planning/WINDOWS.md

key-decisions:
  - "D-15 outcomes: cutter outside the gutter tightens to 0.5 px (worst 0, 80 px inside), sub-page deskew tightens to 0.01 deg (worst 0), content-box IoU stays 0.95 (worst 1 - IoU 0.0319), dewarp bound stays 1.5 px at 300 dpi (measured 1.04 px)"
  - "F-06-1: page_split puts the cutter 30.4 px (300 dpi) and 55.7 px (600 dpi) from the gutter centre at mid-height; the D-15 point check is an XFAIL_CHECK in all four spread scenarios, the hard 'inside the gutter' check holds"
  - "F-06-2: TruthMetrics::straightnessResidual cannot carry the dewarp check on this page (no valid strip for the OFF output at 8 strips, 4 to 9 px noise); the checks use a truth-registered line offset measure local to TestBookSpread.cpp and the plan's call is only reported"
  - "Spread content IoU uses the centre of the mapped sub-page polygon as the truth frame centre; the first version (middle of each half) gave 0.918 to 0.949 on five of eight sub-pages purely because of the cutter error"
  - "Orientation sanity is checked on the glyph ink box, as in plan 02-05"

patterns-established:
  - "TRUTH-METRIC lines for suite=book carry non-bounded context values with bound=none, and the plan's reference measure as *_plan_straightness_* lines"
  - "Goldens are recorded in two steps: test-code commit, then the update run on the committed tree, then the golden files alone in their own commit (manifest source_commit equals the test-code commit)"

requirements-completed: [TEST-06, TEST-03, TEST-04]

coverage:
  - id: D1
    description: "On the generated spread at 300 and 600 dpi in black-and-white and colour/greyscale, page_split stores TWO_PAGES with one cutter that stays inside the gutter at every sampled height (at least 80.5 px inside), each sub-page deskew is exact and its content box has IoU at least 0.968 against the truth block; the mid-height offset check is an expected failure (F-06-1)"
    requirement: "TEST-06"
    verification:
      - kind: integration
        ref: "build/macos-release/pipeline_tests --run_test=BookSpreadTestSuite (test_spread_300_bw_split, test_spread_300_color_split, test_spread_600_bw_split, test_spread_600_color_split)"
        status: pass
      - kind: integration
        ref: "ctest --preset macos-debug -R pipeline_tests"
        status: pass
    human_judgment: false
  - id: D2
    description: "AUTO dewarping of the generated curved page leaves text lines straighter than dewarping off (residual 2.08 px against 7.83 px at 600 dpi) and within the D-15 bound (1.04 px at 300 dpi against 1.5 px), measured against the generator's truth mask with a noise floor of 0.58 px"
    requirement: "TEST-06"
    verification:
      - kind: integration
        ref: "build/macos-release/pipeline_tests --run_test=DewarpPageTestSuite (test_curved_300_auto_dewarp_straightens, test_line_offset_measure_noise_floor_on_flat_page)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Eight sub-page image goldens, four spread text goldens, one dewarp image and text golden and a fixtures.txt of three input fingerprints were recorded from macos-release after each sanity check passed; macos-release and macos-debug both pass them"
    requirement: "TEST-03"
    verification:
      - kind: integration
        ref: "build/macos-release/pipeline_tests --run_test=BookSpreadTestSuite --run_test=DewarpPageTestSuite (no UPDATE_GOLDENS, 211 GOLDEN-METRIC lines, none failing)"
        status: pass
      - kind: integration
        ref: "ctest --preset macos-debug -R pipeline_tests"
        status: pass
    human_judgment: false
  - id: D4
    description: "The goldens are in their own commit 5afce63 with the reason in the body, from a tree whose non-test src equals cde6825 apart from src/CMakeLists.txt; MANIFEST.txt says build_type = Release and ffp_contract = off"
    requirement: "TEST-04"
    verification:
      - kind: other
        ref: "git diff --name-only cde6825 non-test path gate (prints src/CMakeLists.txt only); manifest grep; git show --name-only of the golden commit; git status --porcelain golden/book empty after a normal run"
        status: pass
    human_judgment: false
  - id: D5
    description: "Whether the cutter position finding (F-06-1) and the straightness-measure limitation (F-06-2) become backlog items is a decision for plan 02-10"
    verification: []
    human_judgment: true
    rationale: "Both findings are recorded with fixing requirement none; promoting them to the backlog is a project decision"

duration: 29min
completed: 2026-10-23
status: complete
plan_head_before: fdd2e9808c5aed96ea87a80a6078cd079e0e2458
plan_head_after: 9e2cc4481a6d59fad92041c8e3574e832e064d11
commits: 5
---

# Phase 2 Plan 6: Book-Page Scenarios and Goldens Summary

**Page-split position checked against the gutter on a generated spread at 300 and 600 dpi in both output modes (cutter always inside the gutter, but 30 to 56 px off its centre: expected failure F-06-1), AUTO dewarping shown to straighten a generated curved page (2.08 px against 7.83 px without, 1.04 px at 300 dpi against the 1.5 px bound) with a truth-registered line measure, and the `book` golden suite (sixteen files) recorded from macos-release and passed by macos-debug.**

## Performance

- **Duration:** 29 min
- **Started:** 2026-10-23T19:16:42Z
- **Completed:** 2026-10-23T19:45:00Z
- **Tasks:** 3
- **Files modified:** 19 (18 created, 1 modified: `.planning/WINDOWS.md`)

## Accomplishments

- **Split.** On all four spread scenarios page_split stores `TWO_PAGES` with one cutter. The cutter stays at least 80.5 px (300 dpi) or 171.3 px (600 dpi) inside the gutter at every one of the 120 or 238 sampled heights, so no text is cut. Each sub-page's stored deskew angle is exactly -1.0 degrees and its content box has IoU 0.968 to 0.983 against the truth block. The D-15 point check fails on the unmodified code: the cutter is 30.4 px (300 dpi) and 55.7 px (600 dpi) from the gutter centre at mid-height against 2 px. That is F-06-1, an `XFAIL_CHECK` in all four scenarios.
- **Dewarp.** With the truth-registered line offset measure, the curved page's lines drift by up to 7.83 px (600 dpi) without dewarping and stay within 2.08 px with AUTO dewarping (ratio 0.27, 1.04 px in 300 dpi pixels, under the 1.5 px bound). The measure's own noise floor on a straight page is 0.58 px, and its OFF value agrees with the generator's closed form (6.87 px for the top line). The AUTO run stores a valid distortion model and finds all 14 lines. F-04-2 (the tracer's top curve following the second line) leaves no visible curvature residual on this fixture.
- **Goldens.** `golden/book` holds both sub-pages of the four spread scenarios (bi-level 12 to 17 KB, colour 1.3 to 1.5 MB), four 41-key stage-decision text files, the AUTO-dewarped curved page (bi-level, 19 KB) with its 8-key text file, `fixtures.txt` and `MANIFEST.txt` (`build_type = Release`, `source_commit = 235e5a7`). Release passes without `UPDATE_GOLDENS` and leaves the directory untouched; `ctest --preset macos-debug -R pipeline_tests` passes in 160 s.

## Task Commits

1. **Task 1: Spread scenarios, split checks, measurements** - `887a6c9` (test); includes the first version of `records/scenario-book.md` with split measurements, tolerance decisions and F-06-1
2. **Task 2: Curved page, AUTO dewarping against ground truth** - `cfe8971` (test); records the dewarp measurements, the strip sweep of the plan's measure and F-06-2
3. **Task 3: Record the book goldens from macos-release**
   - `235e5a7` (test) golden checks and `test_book_fixture_fingerprints`
   - `5afce63` (test) the sixteen golden files alone, reason in the body
   - `9e2cc44` (docs) `## Goldens` section and the two `.planning/WINDOWS.md` entries

**Plan metadata:** the docs commits that follow this summary.

## Files Created/Modified

- `src/pipeline_tests/TestBookSpread.cpp` - spread scenario runner, split truth, truth frames, line offset measure, dewarp and guard cases, golden checks, fingerprint case
- `src/pipeline_tests/golden/book/*` - the book golden suite and its manifest
- `.planning/phases/02-golden-baselines/records/scenario-book.md` - measurements, tolerance decisions, sweep, golden list, findings
- `.planning/WINDOWS.md` - F-06-1 (`unmet-truth`) and F-06-2 (`deviation`) added to the ledger

## Decisions Made

See `key-decisions`. Tolerance constants in the test equal the `## Tolerance decisions` rows: `kMaxOutsideGutterPx = 0.5`, `kMaxMidHeightOffsetPx = 2.0` (marked), `kAngleToleranceDeg = 0.01`, `kMinContentIou = 0.95`, `kMaxDewarpResidual300Px = 1.5`, `kMaxNoiseFloorPx = 1.5`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug in plan specification] `straightnessResidual` with 8 strips cannot measure the curved page**
- **Found during:** Task 2 (first run)
- **Issue:** The plan asks for `straightnessResidual(output, nonBackgroundBox(output), lineCount, 8)` on both outputs and requires both results `valid`. On the output without dewarping no strip is usable (seven strips have 15 and one has 16 bands instead of 14: the dots of `i` and `j` form a band of their own). A sweep of the strip count (2 to 16) shows the ordering of OFF and AUTO flipping with the count and values of 4 to 9 px for outputs that are straight (AUTO) and curved (OFF) by the images and by the truth-registered measure. Using it would have reported "AUTO not straighter" for a page that is visibly straight, a false finding against D-04.
- **Fix:** `TestBookSpread.cpp` measures line offsets against the truth mask (row profile of `renderInkMask`, per line and strip, then the deviation of the shifts from a straight line), validated by a guard case on the straight flat page (0.58 px) and by the closed form of the OFF curl (6.87 px against 7.83 px measured). The plan's call stays in the test and is reported as `*_plan_straightness_*` lines. `src/test_support` is frozen and unchanged; plan 02-10 may want a backlog item (F-06-2).
- **Files modified:** `src/pipeline_tests/TestBookSpread.cpp`, `records/scenario-book.md`
- **Verification:** `DewarpPageTestSuite` passes in Release and Debug; the numbers are in `## Dewarp measurements`
- **Committed in:** `cfe8971`

**2. [Rule 1 - Plan ambiguity] Truth frame centre for the spread content IoU**
- **Found during:** Task 1 (first run)
- **Issue:** The plan says "per-sub-page content IoU against `leftBlock` and `rightBlock` (centred)" without naming the truth frame centre. Taking the middle of each half of the page gave IoU 0.918 to 0.949 on five of eight sub-pages, because the real sub-pages are cut at the found cutter, which is 28 to 30 px off the gutter centre.
- **Fix:** The truth centre is the centre of the bounding box of the sub-page polygon (page corners and the found cutter) mapped to the flat frame with `mapImageToFlat`; the cutter error is measured by its own check. Chosen before any bound was set; fixture and options untouched.
- **Files modified:** `src/pipeline_tests/TestBookSpread.cpp`
- **Committed in:** `887a6c9`

**3. [Rule 1 - Plan inconsistency] "Portrait" sanity condition on the ink box**
- **Found during:** Task 1
- **Issue:** As in plan 02-05, the default hard margins can make a whole sub-page output wider than high, so `outputSanity` with `portrait = true` would fail although the content is upright.
- **Fix:** orientation is checked on the glyph ink box; size, ink fraction, line count (within one of 15) and margin position (within 5 mm) are as planned. Width bounds: minimum from the sub-page's truth block, maximum 1.2 x half the spread width at the output dpi.
- **Committed in:** `887a6c9`

**4. [Process] Two test-support-free notes**
- The test code was committed before the recording run (`235e5a7`), so the manifest's `source_commit` is that commit and no manifest refresh was needed (plan 02-05 needed one).
- One throw-away diagnostic (`DBG-TEMP`, images in the session scratchpad) was used to understand the band counts and the AUTO line offsets and was removed before `cfe8971`.

---

**Total deviations:** 3 auto-fixed (all Rule 1, plan specification points, none a production change) and 1 process note
**Impact on plan:** Interfaces and tolerances are as planned; the D-15 and D-18 rules were applied unchanged. Deviation 1 changes which instrument carries the dewarp checks; it makes the result more trustworthy, not weaker (the noise floor is measured and guarded).

## Issues Encountered

- A shell command of mine that began with a bare `cat > /tmp/...` blocked on standard input and ran into the tool timeout; it created an empty file `/tmp/dbg_unused` outside the project, which I deleted. No project file was affected.
- ctest 4.x prints `100% tests passed out of 1`; the plan quotes the older `, 0 tests failed` wording, so the exit code and the 100% line were used.
- `pipeline_tests` takes about 160 s in Debug with this suite (114 s before), 24 s in Release.
- The cause of F-06-1 (cutter off the gutter centre) and the 4.5% vertical stretch of the AUTO output were not analysed; only the effects are recorded.

## Known Stubs

None.

## Threat Flags

None. No new network, auth or file access outside the build tree; T-02-17 is mitigated by the sanity gate, Release-only recording with `build_type = Release` in the manifest, the production-diff gate and the separate golden commit; T-02-18 by the strict hard checks (inside the gutter, straighter than OFF), the D-15 marks with F-06-1 and the unchanged fixture.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 02-09 gets the `GOLDEN-METRIC suite=book` and `TRUTH-METRIC suite=book` lines (211 golden metric lines per run); Release reproduces every colour golden with `max_abs_error` 0, Debug passed within the comparator ceilings.
- Plan 02-10 gets F-06-1 (no requirement, also in `.planning/WINDOWS.md` as `unmet-truth`) and F-06-2 (test-support limitation, `deviation`), plus the observations that F-04-2 shows no residual on this fixture and that the AUTO output is stretched vertically by about 4.5%.
- Plans 02-07 and 02-08 can copy the truth-frame technique (`mapImageToFlat` of the sub-page polygon) and the guard-case pattern for any measure of their own.

## Self-Check: PASSED

All listed files exist (`TestBookSpread.cpp`, the 16 files under `golden/book/`, `scenario-book.md`); commits `887a6c9`, `cfe8971`, `235e5a7`, `5afce63` and `9e2cc44` are ancestors of HEAD; the four automated verify commands of Task 3 and the two of Tasks 1 and 2 exit 0 (production-diff gate prints `src/CMakeLists.txt` only, Release suites pass without update and leave `golden/book` clean, Debug ctest passes, manifest and golden-commit gate hold); `xcrun clang-format --dry-run -Werror` exits 0 on `TestBookSpread.cpp`.

---
*Phase: 02-golden-baselines*
*Completed: 2026-10-23*
