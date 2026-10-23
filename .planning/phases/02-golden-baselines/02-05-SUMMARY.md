---
phase: 02-golden-baselines
plan: 05
subsystem: testing
tags: [boost-test, golden-images, ground-truth, deskew, binarization, text-goldens, synthetic-fixtures, expected-failure]

requires:
  - phase: 02-golden-baselines
    provides: "02-01 generator and pipeline_tests fixture; 02-02 GoldenStore, comparator, text goldens, XFAIL_CHECK; 02-03 TruthMetrics, frozen HeadlessPipeline"
provides:
  - "FlatDocumentTestSuite: four scenario cases (300/600 dpi, black-and-white and colour/greyscale) with deskew, content-box and binarization or contrast checks against ground truth, a 14-run deskew sweep and a fixture fingerprint case"
  - "First committed goldens of the phase: src/pipeline_tests/golden/flat (four images, four stage-decision text files, fixtures.txt with 18 keys, MANIFEST.txt from macos-release)"
  - "records/scenario-flat.md: measurements, sweep, tolerance decisions, golden list, finding F-05-1"
  - "TRUTH-METRIC log line format: TRUTH-METRIC suite=flat scenario=<n> metric=<m> value=<v> bound=<b>"
affects: [02-06, 02-07, 02-08, 02-09, 02-10]

actuals:
  tokens: 8000
  tasks: 2
  commits: 5

tech-stack:
  added: []
  patterns:
    - "One Measured struct per scenario (page, pipeline result, every truth value, sanity) feeds the TRUTH-METRIC report, the assertions and the golden checks"
    - "Generated pages are cached per (dpi, skew) so the scenario cases, the sweep and the fingerprint case render each input once"
    - "A known defect is an XFAIL_CHECK passed to the shared scenario runner as a tag; the golden still records current behaviour (D-18)"
    - "Goldens are recorded in two steps: test-code commit, a manifest-refreshing update run on the committed tree, then the golden files alone in their own commit"

key-files:
  created:
    - .planning/phases/02-golden-baselines/records/scenario-flat.md
    - src/pipeline_tests/golden/flat/flat_300_bw.png
    - src/pipeline_tests/golden/flat/flat_300_bw.txt
    - src/pipeline_tests/golden/flat/flat_300_color.png
    - src/pipeline_tests/golden/flat/flat_300_color.txt
    - src/pipeline_tests/golden/flat/flat_600_bw.png
    - src/pipeline_tests/golden/flat/flat_600_bw.txt
    - src/pipeline_tests/golden/flat/flat_600_color.png
    - src/pipeline_tests/golden/flat/flat_600_color.txt
    - src/pipeline_tests/golden/flat/fixtures.txt
    - src/pipeline_tests/golden/flat/MANIFEST.txt
  modified:
    - src/pipeline_tests/TestFlatDocument.cpp

key-decisions:
  - "D-15 outcomes: deskew tolerance stays 0.15 deg (worst case 0.125 in the sweep), content-box IoU tightens to 0.96 (worst 1 - IoU 0.0187), binarization stays 0.005 and flat_600_bw becomes an XFAIL_CHECK"
  - "F-05-1: the unmodified output stage leaves about 0.1 deg of residual skew on a 600 dpi input although the stored angle is exactly -1.5; the golden records that behaviour"
  - "Orientation sanity is checked on the glyph ink box, because the default hard margins make the whole 1676 x 1636 px output wider than high"
  - "fixtures.txt holds 18 keys: 14 sweep inputs under the plan's key scheme plus 4 scenario keys, since the four scenario inputs coincide with two of the sweep inputs"

patterns-established:
  - "TRUTH-METRIC lines also carry non-bounded context values (bound=none): stored angle, output size, margins, registration offset"
  - "Text goldens of a scenario use the 20 keys of the plan; ints go through QString::number, reals through formatNumber"

requirements-completed: [TEST-05, TEST-03, TEST-04]

coverage:
  - id: D1
    description: "Deskew angle, content box and binarization (black-and-white) or ink/paper contrast (colour/greyscale) of the generated flat page are checked against ground truth at 300 and 600 dpi; the 600 dpi binarization check is an expected failure (F-05-1)"
    requirement: "TEST-05"
    verification:
      - kind: integration
        ref: "build/macos-release/pipeline_tests --run_test=FlatDocumentTestSuite (test_flat_300_bw_truth, test_flat_300_color_truth, test_flat_600_bw_truth, test_flat_600_color_truth)"
        status: pass
      - kind: integration
        ref: "ctest --preset macos-debug -R pipeline_tests"
        status: pass
    human_judgment: false
  - id: D2
    description: "A deskew sweep over -3.0, -1.5, -0.5, 0.5, 1.5, 2.0 and 3.0 degrees at 300 and 600 dpi keeps every stored angle within 0.15 degrees of the negated applied angle (worst 0.125)"
    requirement: "TEST-05"
    verification:
      - kind: integration
        ref: "FlatDocumentTestSuite/test_deskew_angle_sweep"
        status: pass
    human_judgment: false
  - id: D3
    description: "Four image goldens, four stage-decision text goldens and a fixtures.txt of 18 input fingerprints were recorded from macos-release after each sanity check passed; macos-release and macos-debug both pass them"
    requirement: "TEST-03"
    verification:
      - kind: integration
        ref: "build/macos-release/pipeline_tests --run_test=FlatDocumentTestSuite (no UPDATE_GOLDENS)"
        status: pass
      - kind: integration
        ref: "ctest --preset macos-debug -R pipeline_tests"
        status: pass
    human_judgment: false
  - id: D4
    description: "The goldens are in their own commit 2f6df32 with the reason in the body, from a tree whose non-test src equals cde6825 apart from src/CMakeLists.txt; MANIFEST.txt says build_type = Release and ffp_contract = off"
    requirement: "TEST-04"
    verification:
      - kind: other
        ref: "git diff --name-only cde6825 non-test path gate (prints src/CMakeLists.txt only); manifest grep; git show --name-only of the golden commit"
        status: pass
    human_judgment: false
  - id: D5
    description: "A run of FlatDocumentTestSuite without UPDATE_GOLDENS leaves src/pipeline_tests/golden/flat unchanged"
    requirement: "TEST-03"
    verification:
      - kind: other
        ref: "build/macos-release/pipeline_tests --run_test=FlatDocumentTestSuite && git status --porcelain -- src/pipeline_tests/golden/flat (empty)"
        status: pass
    human_judgment: false

duration: 12min
completed: 2026-10-23
status: complete
plan_head_before: 9cceae72f556b03155555b4d8ebd35131ca5eb8b
plan_head_after: c2e57775c553f9739aac56c4a00dd5585b970c01
commits: 4
---

# Phase 2 Plan 5: Flat-Document Scenarios and First Goldens Summary

**Flat-page ground-truth checks (deskew, content box, binarization or contrast) at 300 and 600 dpi in black-and-white and colour/greyscale, a 14-run deskew sweep, and the first committed goldens (four images, four stage-decision text files, 18 input fingerprints) recorded from macos-release and passed by macos-debug; one defect found: the 600 dpi output keeps about 0.1 degrees of residual skew.**

## Performance

- **Duration:** 12 min
- **Started:** 2026-10-23T19:03:05Z
- **Completed:** 2026-10-23T19:15:00Z
- **Tasks:** 2
- **Files modified:** 12 (11 created, 1 modified)

## Accomplishments

- `FlatDocumentTestSuite` checks each of the four scenarios against the generator's ground truth: stored deskew error 0 at 1.5 degrees, content-box IoU 0.9813 (tolerance tightened to 0.96 by the D-15 rule), binarization mismatch 0 at 300 dpi, ink/paper contrast 178 to 180 grey levels for the colour outputs. The deskew sweep (7 angles x 2 dpi) has worst case 0.125 degrees at 300 dpi and -0.5 degrees, as research P4 predicted.
- The flat goldens are committed in their own commit `2f6df32`: `flat_300_bw`, `flat_300_color`, `flat_600_bw`, `flat_600_color` (1676 x 1636 px, bi-level 11 to 14 KB, colour 1.1 to 1.2 MB), four 20-key text goldens, `fixtures.txt` and `MANIFEST.txt`. Recording logged `RECORDED` for every golden and no `sanity failed`.
- Release passes without `UPDATE_GOLDENS` and leaves `golden/flat` untouched; `ctest --preset macos-debug -R pipeline_tests` passes against the Release goldens (114 s for the whole executable).
- Finding F-05-1: for the 600 dpi input the output has a shear that grows with the row (local offsets -1, 0, +2 px in three bands) and the best truth-mask rotation is about -0.1 degrees, so the edge-tolerant binarization mismatch is 8.3% against the 0.5% bound. The check is an `XFAIL_CHECK` and the golden records the behaviour.

## Task Commits

1. **Task 1: Flat scenarios, ground-truth checks, deskew sweep and measurements** - `4f1e017` (test); includes `records/scenario-flat.md` with measurements, sweep, tolerance decisions and F-05-1
2. **Task 2: Record the flat goldens from macos-release**
   - `24d18b6` (test) golden checks and `test_flat_fixture_fingerprints`
   - `2f6df32` (test) the ten golden files alone, reason in the body
   - `c2e5777` (docs) `## Goldens` section of the records file

**Plan metadata:** the docs commit that follows this summary.

## Files Created/Modified

- `src/pipeline_tests/TestFlatDocument.cpp` - scenario runner, truth checks, sweep, golden checks, fingerprint case
- `src/pipeline_tests/golden/flat/*` - the flat golden suite and its manifest
- `.planning/phases/02-golden-baselines/records/scenario-flat.md` - measurements, tolerance decisions, golden list, finding

## Decisions Made

See `key-decisions`. Tolerance constants in the test equal the `## Tolerance decisions` rows: `kAngleToleranceDeg = 0.15`, `kMinContentIou = 0.96`, `kMaxBinarizationMismatch = 0.005`, `kMinInkPaperContrast = 100`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug in plan specification] "Portrait" sanity condition cannot hold for this output**
- **Found during:** Task 1 (first measurement run)
- **Issue:** `outputSanity` with `portrait = true` failed on all four scenarios ("output 1676x1636 is landscape"). The stored hard margins are 10 mm left and right and 5 mm top and bottom, so the whole page of this fixture is 40 px wider than high, although the content (1204 x 1400 px) is upright. Plan 02-03 met the same effect for the curved and mixed smoke outputs.
- **Fix:** the orientation is checked on the glyph ink box (`nonBackgroundBox`, width not above height) instead of on the page with margins; all other sanity conditions are as planned. A 90 degree rotation would still fail. Nothing was loosened: size, ink fraction, line count and margin position checks are unchanged.
- **Files modified:** `src/pipeline_tests/TestFlatDocument.cpp`
- **Verification:** all four scenarios pass the sanity check; `RECORDED` for every golden, no `sanity failed`.
- **Committed in:** `4f1e017`

**2. [Rule 1 - Plan ambiguity] Width bounds scale with output dpi over input dpi**
- **Found during:** Task 1 (writing the sanity expectation)
- **Issue:** "2 x truth text-block width" is right for the 300 dpi input only; a 600 dpi input has a text block of 1190 px and an output of the same scale.
- **Fix:** both width bounds use the factor `600 / input dpi` (2 at 300 dpi, 1 at 600 dpi). Same intent, applied to both inputs.
- **Committed in:** `4f1e017`

**3. [Rule 1 - Plan inconsistency] fixtures.txt has 18 keys, not the 14 of the key scheme**
- **Found during:** Task 2
- **Issue:** the key scheme `flat_<dpi>_skew_<angle>` gives 14 distinct keys (the four scenario inputs coincide with the two 1.5 degree sweep inputs), but the acceptance criterion asks for 18 keys.
- **Fix:** 14 sweep keys under the scheme plus 4 `scenario_<name>` keys with the fingerprint of the input each scenario really uses.
- **Committed in:** `24d18b6`

**4. [Process] Manifest refreshed after the test-code commit**
- The first recording run happened before the test-code commit, so `source_commit` in the manifest was `4f1e017`. After committing the test code, one more `UPDATE_GOLDENS=1` run refreshed only the manifest (no golden file was rewritten) and the golden commit then held the final manifest with `source_commit = 24d18b6`.

---

**Total deviations:** 3 auto-fixed (plan specification points, none a production change) and 1 process note
**Impact on plan:** none on interfaces or tolerances; sanity strictness is the same in substance.

## Issues Encountered

- The `portrait` sanity condition (above) and the 600 dpi binarization mismatch (F-05-1) were the only surprises. For F-05-1 a throw-away diagnostic (not committed) rendered the truth mask at rotation steps of 0.04 degrees and per-band offsets; the cause inside the output or deskew transformation was not analysed.
- The plan quotes ctest's summary as "100% tests passed, 0 tests failed out of 1"; CMake 4.4.4 prints `100% tests passed out of 1`, so the exit code and the 100% line were used.
- The image-file gate of plan 02-03 (0 image files under `src`) no longer applies: the five tracked PNGs are the synthetic goldens of this plan.

## Known Stubs

None.

## Threat Flags

None. No new network, auth or file access outside the build tree; T-02-15 is mitigated by the sanity gate, Release-only recording with `build_type = Release` in the manifest, the production-diff gate and the separate golden commit; T-02-16 by the `## Tolerance decisions` table and F-05-1.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plans 02-06 to 02-08 can copy the scenario runner pattern (Measured struct, TRUTH-METRIC lines, XFAIL tag, two-step golden recording). Plan 02-09 gets the `GOLDEN-METRIC` and `TRUTH-METRIC` lines of the flat suite; Release and Debug agree on all four goldens (comparator limits not needed beyond the ceilings). Plan 02-10 gets F-05-1 for the backlog (also appended to `.planning/WINDOWS.md` as `unmet-truth`).
- `pipeline_tests` takes about 114 s in Debug and 24 s in Release with this suite.

## Self-Check: PASSED

All 12 listed files exist; commits `4f1e017`, `24d18b6`, `2f6df32` and `c2e5777` are ancestors of HEAD; the four automated verify commands of Task 2 and the two of Task 1 exit 0 (production-diff gate prints `src/CMakeLists.txt` only, Release suite passes without update and leaves the golden directory clean, Debug ctest passes, manifest and golden-commit gate hold).

---
*Phase: 02-golden-baselines*
*Completed: 2026-10-23*
