---
phase: 02-golden-baselines
plan: 03
subsystem: testing
tags: [boost-test, synthetic-fixtures, splitmix64, ground-truth, sha256, headless-pipeline, dewarping, page-split]

requires:
  - phase: 02-golden-baselines
    provides: "02-01: Rng, flat page generator, pipeline_tests fixture, HeadlessPipeline (runPipeline, PipelineOptions, StageSettings)"
provides:
  - "PageGenerator: SPREAD, MIXED and CURVED page kinds with ground truth (blocks, gutter, picture, specks, curl), renderInkMask, fingerprint, mapFlatToImage, mapImageToFlat"
  - "TruthMetrics: inkFraction, nonBackgroundBox, countHorizontalBands, boxIoU, centredBoxIoU, registerMask, truthMismatchFraction, straightnessResidual, outputSanity"
  - "GeneratorDeterminismTestSuite, TruthMetricsTestSuite, PipelineSmokeTestSuite"
  - "HeadlessPipeline proven on a two-page spread, AUTO dewarping and MIXED output, and frozen for the phase"
affects: [02-04, 02-05, 02-06, 02-07, 02-08, 02-09, 02-10]

actuals:
  tokens: 22625
  tasks: 3
  commits: 6

tech-stack:
  added: []
  patterns:
    - "One renderer for every page kind: flat ink raster (0 paper, 1 text, 2 speck), per-kind inverse map, 3x3 supersampling, per-sample colour sum; the FLAT page stays byte-identical"
    - "Curl profile evaluated at the image x coordinate, so the image-to-flat map is closed form and the forward map is a fixed-point iteration"
    - "Edge-tolerant mismatch on summed-area tables; an unusable straightness measurement carries an infinite residual"
    - "TDD with interface stubs in the RED commit (as in 02-02), then GREEN, then a REFACTOR commit"

key-files:
  created:
    - src/test_support/TruthMetrics.h
    - src/test_support/TruthMetrics.cpp
    - src/pipeline_tests/TestGeneratorDeterminism.cpp
    - src/pipeline_tests/TestTruthMetrics.cpp
    - src/pipeline_tests/TestPipelineSmoke.cpp
  modified:
    - src/test_support/PageGenerator.h
    - src/test_support/PageGenerator.cpp
    - src/pipeline_tests/HeadlessPipeline.h

key-decisions:
  - "GroundTruth text boxes are tight ink boxes for every kind (SPREAD leftBlock and rightBlock, MIXED and CURVED textBlock), as in plan 02-01; the nominal 640 x 860, 420 x 850 and 700 x 800 frames only place the text"
  - "The curl profile p(u) is evaluated at the image x coordinate; mapImageToFlat is the closed-form renderer map and mapFlatToImage solves it by fixed-point iteration (exact except within 0.01 px of the spine edge)"
  - "straightnessResidual returns valid = false, linesFound 0 and an infinite residual when fewer than two strips are usable, so a forgotten valid check can never pass a tolerance"
  - "A symmetric arc of sagitta h gives a least-squares residual of 0.4375 h at 8 strips; the bent-lines test uses h = 27 px (11.8 px expected) to realise the plan's 'about 12 px'"
  - "HeadlessPipeline needed no code change for the spread, AUTO dewarp and MIXED paths; only the frozen-file comment was added"

patterns-established:
  - "Generated pages are compared by fingerprint(): SHA-256 over RGB32 scanline bytes, width x 4 per line"
  - "Tests that only read a generated page share a cached copy (Debug rendering of 8 kind/dpi pages costs about 19 s)"

requirements-completed: [TEST-01, TEST-06, TEST-07]

coverage:
  - id: D1
    description: "The generator is deterministic: the same seed gives byte-identical RGB32 pixels for flat, spread, mixed and curved pages at 300 and 600 dpi, a different seed gives different pixels, and the flat page is unchanged from plan 02-01"
    requirement: "TEST-01"
    verification:
      - kind: unit
        ref: "build/macos-release/pipeline_tests --run_test=GeneratorDeterminismTestSuite (test_same_seed_same_pixels, test_different_seed_different_pixels, test_flat_page_unchanged)"
        status: pass
      - kind: unit
        ref: "ctest --preset macos-debug -R pipeline_tests"
        status: pass
    human_judgment: false
  - id: D2
    description: "Each page kind carries its ground truth: skew, tight text blocks, gutter line and half width, picture rectangle and 40 specks, curl amplitude and line positions; invalid dpi gives a null page; forward and inverse maps agree to 1e-6 px"
    requirement: "TEST-01"
    verification:
      - kind: unit
        ref: "GeneratorDeterminismTestSuite (test_spread_truth_consistent, test_mixed_truth_consistent, test_curved_truth_consistent, test_ink_mask_inside_text_block, test_invalid_dpi_gives_null_page, test_forward_map_inverts_renderer_map)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Truth metrics for sanity, registration, edge-tolerant mismatch, IoU and line straightness, with straightnessResidual invalid rather than passing when fewer than two strips are usable"
    requirement: "TEST-06"
    verification:
      - kind: unit
        ref: "build/macos-release/pipeline_tests --run_test=TruthMetricsTestSuite (ten cases)"
        status: pass
    human_judgment: false
  - id: D4
    description: "runPipeline processes a generated spread into two sub-page results (TWO_PAGES, one cutter), runs AUTO dewarping with a valid distortion model, keeps colour in MIXED mode, and calls the configure hook per sub-page; HeadlessPipeline is frozen"
    requirement: "TEST-07"
    verification:
      - kind: integration
        ref: "build/macos-release/pipeline_tests --run_test=PipelineSmokeTestSuite --run_test=FlatDocumentTestSuite"
        status: pass
      - kind: integration
        ref: "ctest --preset macos-release -R pipeline_tests; ctest --preset macos-debug -R pipeline_tests"
        status: pass
    human_judgment: false
  - id: D5
    description: "No real scan or image file enters the repository: all inputs are generated in memory and the forbidden-API grep over src/test_support exits 0"
    requirement: "TEST-01"
    verification:
      - kind: other
        ref: "git status --porcelain -- src image-file count is 0; forbidden-API grep over src/test_support exit 0"
        status: pass
    human_judgment: false

duration: 24min
completed: 2026-10-23
status: complete
plan_head_before: b2f42901402d1c32f66be3ca9457dd09b2e46934
plan_head_after: 7a98fc1a067ea21b61353ca7c3c3b4e889f691f3
commits: 6
---

# Phase 2 Plan 3: Generator Kinds, Truth Metrics and Pipeline Smoke Summary

**Seeded font-free generator extended to book spreads (gutter shadow), mixed pages (colour value-noise picture, 40 specks) and curved pages (circular-arc curl) with full ground truth and SHA-256 fingerprints, plus tested truth metrics, and a headless pipeline that already handles spreads, AUTO dewarping and MIXED output.**

## Performance

- **Duration:** 24 min
- **Started:** 2026-10-23T18:19:00Z (approximate; the commit ledger was created at 18:26:59Z)
- **Completed:** 2026-10-23T18:43:00Z
- **Tasks:** 3 (Tasks 1 and 2 as RED, GREEN, with a REFACTOR commit for Task 1)
- **Files modified:** 8 (5 created, 3 modified)

## Accomplishments

- `PageGenerator` renders SPREAD (1654 x 1181, two blocks, 227 px gutter, quadratic shadow band up to 25 levels), MIXED (1000 x 1150, text column, 320 x 400 colour value-noise picture on a 16 px grid, 40 ink-coloured specks of 1 to 3 px at least 20 px from text, picture and edge) and CURVED (space-free 23 x 14 block, circular-arc curl with A = 25 base px, 0.15 A compression towards the spine). All kinds use one renderer; the FLAT fingerprints recorded before the change (`befa5408...a9` at 300 dpi, `fedb69f8...cf` at 600 dpi) still match.
- `renderInkMask` (text ink only, 0 = ink, rotated about the block centre), `fingerprint`, `mapFlatToImage` and `mapImageToFlat` give later plans truth lines in input coordinates. A dpi outside 75..1200 returns a null page with `truth.dpi == 0`.
- `TruthMetrics` implements the nine functions of the plan interface with plain Qt and arithmetic. The edge-tolerant mismatch uses summed-area tables, so a 4.6 Mpixel output costs one pass.
- `PipelineSmokeTestSuite` shows that `runPipeline` already returns TWO_PAGES with one cutter and two sane 600 dpi sub-pages for a generated spread, a valid distortion model for AUTO dewarping of the curved page, 411038 coloured pixels for the mixed page in MIXED mode, and one configure call per sub-page.
- `pipeline_tests` passes under `ctest` in Release (9 s) and Debug (74 s).

## Task Commits

1. **Task 1: spread, mixed and curved page kinds, ink masks, fingerprints, determinism suite**
   - RED `06689c5` (test): final `PageGenerator.h`, stub functions, ten failing cases
   - GREEN `1d137ee` (feat): implementation; the ten cases pass, flat fingerprints unchanged
   - REFACTOR `8fb81d0` (refactor): cached pages for read-only cases, Debug 87 s to 46 s for the suite
2. **Task 2: truth metrics**
   - RED `9679ff0` (test): final `TruthMetrics.h`, stub sources, ten failing cases
   - GREEN `70db21d` (feat): implementation; the ten cases pass
3. **Task 3: pipeline smoke tests and freeze** - `7a98fc1` (test): only `TestPipelineSmoke.cpp` and `HeadlessPipeline.h`

**Plan metadata:** the docs commit that follows this summary.

## Files Created/Modified

- `src/test_support/PageGenerator.h` / `.cpp` - four page kinds, ground truth, mask, fingerprint and maps
- `src/test_support/TruthMetrics.h` / `.cpp` - the measurement functions
- `src/pipeline_tests/TestGeneratorDeterminism.cpp` - `GeneratorDeterminismTestSuite`, ten cases
- `src/pipeline_tests/TestTruthMetrics.cpp` - `TruthMetricsTestSuite`, ten cases
- `src/pipeline_tests/TestPipelineSmoke.cpp` - `PipelineSmokeTestSuite`, four cases
- `src/pipeline_tests/HeadlessPipeline.h` - frozen-file comment only

## Decisions Made

See `key-decisions` above. Also: the plan's "known 12 px arc" sentence was read as the residual the metric reports, because a least-squares fit of a symmetric arc leaves only 0.4375 of the sagitta at 8 strips; the test states the derivation.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Added `mapImageToFlat` to the public interface**
- **Found during:** Task 1 (writing `test_forward_map_inverts_renderer_map`)
- **Issue:** The plan asks that `mapFlatToImage` followed by "the renderer's inverse map" returns the point within 1e-6 px, but the interface list has no way to call the renderer's map.
- **Fix:** exported `QPointF mapImageToFlat(const PageSpec&, const QPointF&)`, the exact map the renderer samples with. The names the plan lists are all kept.
- **Files modified:** `src/test_support/PageGenerator.h`, `PageGenerator.cpp`
- **Committed in:** `06689c5`, `1d137ee`

**2. [Process] Cached test pages as a separate REFACTOR commit**
- **Found during:** Task 3 (Debug run of `pipeline_tests` took 114 s, 87 s of it in the determinism suite)
- **Fix:** cases that only read a page share a cached copy. The same-seed and pinned-fingerprint cases still render their own copies. This touches a Task 1 file, so it is its own commit and Task 3's commit lists only the smoke test and `HeadlessPipeline.h`.
- **Committed in:** `8fb81d0`

---

**Total deviations:** 1 auto-fixed (Rule 2), 1 process note
**Impact on plan:** no scope creep; the added function is test-only.

## TDD Gate Compliance

The plan is `type: execute` with `tdd="true"` on Tasks 1 and 2, so the `tdd-red-evidence` classifier was not required (Boost.Test console output is not one of its report formats). Both tasks ran RED, GREEN:

- **Task 1:** RED `06689c5`, GREEN `1d137ee`, REFACTOR `8fb81d0`. All ten cases failed on their own assertions in RED (169 of 288 checks); `test_invalid_dpi_gives_null_page` failed on the stub's `std::invalid_argument` and `test_flat_page_unchanged` failed because the stub fingerprint was empty. Semantic assessment: `test_same_seed_same_pixels` still reported failures (the 64 character length check), so it did not pass vacuously.
- **Task 2:** RED `9679ff0`, GREEN `70db21d`. All ten cases failed on assertions against the stubs. A mutation check changed the tolerance rule from `||` to `&&` in `truthMismatchFraction`; `test_truth_mismatch_edge_tolerance` then failed on the interior-flip count, and the file was restored.
- **Task 3** has no `tdd` attribute and is one commit.

## Issues Encountered

- `clang-format` is not installed, so formatting was checked by hand against the 120 column limit; the `lint.yml` CI check was not run.
- `pipeline_tests` takes 74 s in Debug (9 s in Release). Later plans add scenarios to the same executable; the generator render dominates.
- The curved and mixed smoke outputs are not portrait (1872 x 1824 and 2080 x 1856 at 600 dpi, page layout margins included), so those two cases check non-blank output only; the spread sub-pages are portrait as planned.

## Known Stubs

None. The RED-commit stubs in `PageGenerator.cpp` and `TruthMetrics.cpp` were replaced in the GREEN commits (`grep "Interface stubs" src` finds nothing).

## Threat Flags

None. No network, auth or file access outside the build tree was added; T-02-09 to T-02-11 are covered by the image-file gate (0 image files under `src`), the forbidden-API grep, the same-seed suite and `test_flat_page_unchanged`.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plans 02-04 to 02-07 can use every name of the plan interface plus `mapImageToFlat`; `HeadlessPipeline` is frozen and scenarios go through `PipelineOptions::configure`.
- `straightnessResidual` reports an infinite residual with `valid = false` for an unusable measurement; scenario plans must check `valid` first. For a symmetric arc the residual is 0.4375 of the sagitta at 8 strips.
- AUTO layout splits the generated spread into two pages without help; where the cutter lies relative to `gutterCentre` is for plan 02-06 to measure.
- Debug runtime of the whole executable should be watched as scenario suites are added.

## Self-Check: PASSED

All 8 listed source files exist; commits `06689c5`, `1d137ee`, `8fb81d0`, `9679ff0`, `70db21d` and `7a98fc1` are ancestors of HEAD; the verify commands of the three tasks exit 0 (`GeneratorDeterminismTestSuite`, `TruthMetricsTestSuite`, `PipelineSmokeTestSuite` with `FlatDocumentTestSuite`, `ctest -R pipeline_tests` in Release and Debug, forbidden-API grep, image-file gate, `TruthMetrics.cpp` include gate).

---
*Phase: 02-golden-baselines*
*Completed: 2026-10-23*
