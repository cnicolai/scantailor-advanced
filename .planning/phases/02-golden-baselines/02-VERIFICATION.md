---
phase: 02-golden-baselines
verified: 2026-10-23T22:30:00Z
status: human_needed
score: 5/5 must-haves verified
covered_files:
  - .planning/phases/02-golden-baselines/02-01-PLAN.md
  - .planning/phases/02-golden-baselines/02-01-SUMMARY.md
  - .planning/phases/02-golden-baselines/02-02-PLAN.md
  - .planning/phases/02-golden-baselines/02-02-SUMMARY.md
  - .planning/phases/02-golden-baselines/02-03-PLAN.md
  - .planning/phases/02-golden-baselines/02-03-SUMMARY.md
  - .planning/phases/02-golden-baselines/02-04-PLAN.md
  - .planning/phases/02-golden-baselines/02-04-SUMMARY.md
  - .planning/phases/02-golden-baselines/02-05-PLAN.md
  - .planning/phases/02-golden-baselines/02-05-SUMMARY.md
  - .planning/phases/02-golden-baselines/02-06-PLAN.md
  - .planning/phases/02-golden-baselines/02-06-SUMMARY.md
  - .planning/phases/02-golden-baselines/02-07-PLAN.md
  - .planning/phases/02-golden-baselines/02-07-SUMMARY.md
  - .planning/phases/02-golden-baselines/02-08-PLAN.md
  - .planning/phases/02-golden-baselines/02-08-SUMMARY.md
  - .planning/phases/02-golden-baselines/02-09-PLAN.md
  - .planning/phases/02-golden-baselines/02-09-SUMMARY.md
  - .planning/phases/02-golden-baselines/02-10-PLAN.md
  - .planning/phases/02-golden-baselines/02-10-SUMMARY.md
  - src/CMakeLists.txt
  - src/dewarping/tests/TestTextLineTracer.cpp
  - src/imageproc/tests/TestBinarize.cpp
  - src/pipeline_tests/TestBookSpread.cpp
  - src/pipeline_tests/TestComparatorSelfTest.cpp
  - src/pipeline_tests/TestFlatDocument.cpp
  - src/pipeline_tests/TestMixedPage.cpp
  - src/pipeline_tests/TestProjectRoundTrip.cpp
  - src/test_support/PageGenerator.cpp
  - src/test_support/Tolerances.h
covered_digest: "v3:sha256:9db51775172b31a37145c96da0eb32d326445fbda17d71e988d1b56c51582e22"
behavior_unverified: 0
overrides_applied: 0
human_verification:
  - test: "Decide whether roadmap criterion 5 is met by the wording 'no requirement' in every XFAIL tag"
    expected: "Criterion 5 asks that each expected failure 'names the requirement that will fix it'. No requirement in Phases 3 to 6 covers F-04-1, F-04-2, F-05-1, F-06-1, F-07-1, so all five tags say 'no requirement' and each has backlog item 999.x. Accept this as meeting the criterion (then add an override), or ask for the criterion to be reworded."
    why_human: "Literal wording of the roadmap contract cannot be met because no fixing requirement exists; accepting a deviation from the contract is the user's decision."
  - test: "Accept or reject the plan 02-07 despeckle-sanity deviation (D7)"
    expected: "The plan's sanity condition (90% of specks present with despeckling off) cannot hold; the test counts the specks inside the content box (15/15 at 300 dpi, 11/11 at 600 dpi). It is open in .planning/WINDOWS.md as a deviation for the user."
    why_human: "Open deviation from a plan truth, recorded as 'for the user to accept or not'."
  - test: "Triage 02-REVIEW.md CR-01 (generator heap overflow at unused dpi values) and WR-03 (vacuous page-split check)"
    expected: "Decide whether to fix now (narrow dpiSupported to values where the text block fits, or add a bounds check in placeGlyph; assert the page-split layout value directly in the round-trip test) or carry as backlog. All 13 review items are 'open' in 02-REVIEW-DISPOSITION.md."
    why_human: "Disposition of review findings is a user decision; no scenario in the phase is affected today."
---

# Phase 2: Golden Baselines Verification Report

**Phase Goal:** A synthetic-image test suite records what the unmodified code produces for flat documents, book spreads and mixed pages, so every later change can be checked against it.
**Verified:** 2026-10-23T22:30:00Z
**Status:** human_needed
**Re-verification:** No, initial verification

The goal is achieved in the codebase. All five roadmap criteria are backed by code I read and by runs I made myself. Status is `human_needed` rather than `passed` only because three decisions belong to the user (above); no gap blocks the next phase.

## Goal Achievement

### Observable Truths (roadmap success criteria)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Generator is deterministic for flat, spread and mixed pages (and curved) with ground truth; no font rendering, no `rand()` | VERIFIED | `grep -rnE 'rand\(\|qrand\|QFont\|drawText\|QRandomGenerator\|std::mt19937\|<random>\|std::sin\|std::cos\|QPainter'` over `src/test_support`, `src/pipeline_tests`, `src/dewarping/tests`, `TestBinarize.cpp` finds nothing. `PageGenerator.h` has `GroundTruth` with skew, text blocks, gutter, curl, line tops, picture and speck rectangles. `GeneratorDeterminismTestSuite` (10 cases, includes `test_same_seed_same_pixels`, `test_different_seed_different_pixels`) passes in my Release `ctest` run. Fixture fingerprints are in `golden/*/fixtures.txt` and checked by `test_*_fixture_fingerprints`. |
| 2 | Tests check flat (deskew, content box, binarization), book (split position, dewarp straightness), mixed (picture zones, colour/grey, despeckle); revived `TestBinarize` and new dewarping tests run alongside | VERIFIED | `--list_content`: `FlatDocumentTestSuite` (6 scenarios plus deskew sweep), `BookSpreadTestSuite` (4 spread scenarios), `DewarpPageTestSuite` (AUTO dewarp residual 1.042 px at 300 dpi against bound 1.5, OFF 7.83 px; straight-page noise floor guard), `MixedPageTestSuite` (despeckle 300/600, colour 300/600, MIXED picture zone, AUTO layout). `BinarizeTestSuite` has nine per-binarizer cases plus uneven illumination; `dewarping_tests` has 10 cases (model, XML round trip, dewarper 1e-9 round trip, raster straightening, tracer). All run in `ctest` (7/7). Known-failing sub-checks are `XFAIL_CHECK`, not removed. |
| 3 | Altered output fails under per-kind tolerance and leaves actual/diff images; baselines change only with the update flag; manifest records toolchain | VERIFIED | `ComparatorSelfTestSuite` (9 cases) passes. My own mutation: I changed `deskew_angle_deg` in `golden/flat/flat_300_bw.txt` from -1.5 to -1.9; `FlatDocumentTestSuite/test_flat_300_bw_truth` failed with `deskew_angle_deg = 0.4 (limit 0.01)` and `ctest -R pipeline_tests` returned Failed (exit 200); artifacts were written to `build/macos-release/test-artifacts/flat/flat_300_bw`. File restored with `git checkout`; `git status --porcelain -- src` empty. `GoldenStore.cpp:157` reads `UPDATE_GOLDENS`; `test_normal_run_never_writes`, `test_update_twice_is_idempotent` pass. `golden/{flat,book,mixed}/MANIFEST.txt` list qt, boost, libtiff, libpng, jpeg-turbo, clang, xcode, cmake, macos, ffp_contract, build_type, source_commit. `Tolerances.h` limits frozen from a measured spread of 0 (`records/tolerance-spread.md`). |
| 4 | Baseline commit precedes every strip/cleanup commit and its source tree matches Phase 1 code | VERIFIED | `git diff --name-only cde6825 <c> -- src CMakeLists.txt CMakePresets.json cmake` excluding test paths gives only `src/CMakeLists.txt` for each of the three golden commits (`2f6df326`, `5afce635`, `a796c2ae`); that file's diff is exactly three `add_subdirectory` lines inside `if (BUILD_TESTS)`. `git diff --stat 5eaac18 cde6825 -- src` is empty (`src/` equals 1.2.1 release at `cde6825`). `git log cde6825..HEAD` has no Phase 3 to 6 commits. Top-level `CMakeLists.txt`, `CMakePresets.json`, `cmake/` unchanged since `cde6825`. |
| 5 | Round trip of non-default, special-character project is byte-identical; one `ctest` runs everything offscreen with isolated settings and passes; known defects are expected failures naming the fixing requirement | VERIFIED (with deviation, see human item 1) | `ProjectRoundTripTestSuite` (5 cases): five pages with `& < > " '`, spaces, NFC and NFD umlaut, 1-char and 200-byte names, special-character directories; non-default values in all six stages; write, read, write byte-identical (record: 18308 bytes). `ctest --preset macos-release` run by me: 7/7 passed, 54 s, `git status --porcelain -- src` empty afterwards. `pipeline_tests` CMake sets `QT_QPA_PLATFORM=offscreen`, `LC_ALL`, `TMPDIR` under `build/`; `EnvironmentTestSuite` checks settings/temp isolation. Debug 7/7 and ASan 7/7 are recorded in `records/ctest-phase2.md` and `records/sanitizer-findings.md` (not re-run by me). Seven `XFAIL_CHECK` marks exist (5 findings) and trip when the defect disappears. Deviation: tags say `no requirement`, because none of the v1 requirements covers these defects (checked against `REQUIREMENTS.md`); each finding has backlog 999.1 to 999.7 in `ROADMAP.md`. |

**Score:** 5/5 roadmap truths verified (0 behavior-unverified). The plan-level truths in the ten PLAN frontmatters were spot-checked against the same evidence; the only plan truth not met as written is the 02-07 despeckle-sanity wording (human item 2).

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/test_support/*` (Rng, PageGenerator, TruthMetrics, ImageCompare, GoldenStore, TextGolden, Manifest, ExpectedFailure, Tolerances, GoldenChecks) | seeded generator, comparator, store, trip-wire | VERIFIED | 8304 lines across test_support/pipeline_tests/dewarping tests; substantive, no stubs; no debt markers (only `XXXXXX` temp-dir templates) |
| `src/pipeline_tests/` (12 test files, HeadlessPipeline, main, CMake) | six-stage headless pipeline, scenarios | VERIFIED | 86 test cases listed, registered in `ctest` as `pipeline_tests` |
| `src/pipeline_tests/golden/{flat,book,mixed}` | goldens plus MANIFEST and fixtures | VERIFIED | 38 files, 12.4 MB; PNGs, `.txt`, `MANIFEST.txt`, `fixtures.txt` present |
| `src/dewarping/tests/*` | dewarping_tests | VERIFIED | 3 suites, 10 cases, registered in ctest |
| `src/imageproc/tests/TestBinarize.cpp`, `TestScale.cpp`, CMake | revived binarize test | VERIFIED | nine binarizers; links `test_support` |
| `records/*`, `02-FINDINGS.md`, `.planning/WINDOWS.md` | records | VERIFIED | all present; backlog 999.1 to 999.7 exist in `ROADMAP.md` |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `src/CMakeLists.txt` | `test_support`, `pipeline_tests`, `dewarping/tests` | three `add_subdirectory` in `if (BUILD_TESTS)` | WIRED | diff vs `cde6825` is exactly these lines; all tests appear in ctest |
| tests | goldens | `CHECK_IMAGE_GOLDEN` / `CHECK_TEXT_GOLDEN` via `GoldenStore` | WIRED | proven by my mutation: edited golden makes ctest fail |
| `XFAIL_CHECK` | defect conditions | trip-wire raising `BOOST_ERROR` when condition holds | WIRED | `test_expected_failure_trip_wire` passes |
| `UPDATE_GOLDENS` | golden write | `GoldenStore.cpp:157` | WIRED | `git status -- src` empty after normal runs |

### Data-Flow Trace (Level 4)

Test code only; the values compared come from the real `Task` objects of the six filters run by `HeadlessPipeline` (not mocks) and from the generator's ground truth. My mutation of a golden shows the compared value is the live pipeline output.

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Whole suite passes | `ctest --preset macos-release` | 7/7 passed, 54.2 s | PASS |
| Altered golden fails | edit `flat_300_bw.txt`, `pipeline_tests --run_test=.../test_flat_300_bw_truth`, then `ctest -R pipeline_tests` | mismatch reported with metric and limit, artifacts written, ctest Failed | PASS |
| Comparator self-test, golden store, image compare suites | `pipeline_tests --run_test=ComparatorSelfTestSuite,GoldenStoreTestSuite,ImageCompareTestSuite` | No errors detected | PASS |
| Tree untouched by normal run | `git status --porcelain -- src` after runs | empty | PASS |
| Test discovery | `pipeline_tests --list_content`, `imageproc_tests`, `dewarping_tests` | 86, 11 binarize, 10 dewarping cases | PASS |

### Probe Execution

No probes declared; none found. Skipped.

### Requirements Coverage

All 11 IDs from the ROADMAP phase line appear in at least one PLAN `requirements:` field (union of the ten plans: TEST-01 to TEST-11), all are marked Complete in `REQUIREMENTS.md`, and there are no orphans (`REQUIREMENTS.md` maps no other ID to Phase 2).

| Requirement | Source Plan(s) | Description | Status | Evidence |
|-------------|----------------|-------------|--------|----------|
| TEST-01 | 02-03 | Seeded generator with ground truth, no font rendering or `rand()` | SATISFIED | grep clean; determinism suite passes |
| TEST-02 | 02-02, 02-09 | Per-kind comparator, writes actual and diff | SATISFIED | `ImageCompareTestSuite`, `ComparatorSelfTestSuite`; my mutation |
| TEST-03 | 02-02, 02-05, 02-06, 02-07 | Update flag only, toolchain manifest | SATISFIED | `GoldenStore`, `MANIFEST.txt` x3 |
| TEST-04 | 02-05, 02-06, 02-07, 02-10 | Baselines from unmodified code before any removal | SATISFIED | git diff proof above; baseline `a796c2ae` |
| TEST-05 | 02-01, 02-05 | Flat: deskew, content box, binarization | SATISFIED (600 dpi binarization is XFAIL F-05-1; 300 dpi hard) | `FlatDocumentTestSuite` |
| TEST-06 | 02-03, 02-06 | Split position and dewarp straightening | SATISFIED (cutter mid-height 2 px bound is XFAIL F-06-1; cutter-inside-gutter hard check and dewarp 1.042 px hard check hold) | `BookSpreadTestSuite`, `DewarpPageTestSuite` |
| TEST-07 | 02-03, 02-07 | Mixed: picture zones, colour/grey, despeckle | SATISFIED (AUTO layout is XFAIL F-07-1; coverage scenarios force SINGLE_PAGE_UNCUT; despeckle sanity deviation D7) | `MixedPageTestSuite` |
| TEST-08 | 02-04 | `TestBinarize` runs again | SATISFIED | nine binarizers (Otsu/Mokji on Grayscale8 XFAIL F-04-1) |
| TEST-09 | 02-04 | Dewarping unit tests | SATISFIED | `dewarping_tests` (F-04-2 XFAIL on the tracer's first line) |
| TEST-10 | 02-08 | Project round trip, non-default, special characters | SATISFIED (see WR-03 weakness for page_split) | `ProjectRoundTripTestSuite` |
| TEST-11 | 02-01, 02-04, 02-09, 02-10 | One `ctest`, offscreen, isolated, passes | SATISFIED | 7/7 Release (my run), Debug and ASan per records |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `src/test_support/PageGenerator.cpp` | 377-400 (`placeGlyph`), 146-148 (`dpiSupported`) | No bounds check; every dpi 75..1200 accepted though the text block overshoots the raster for some values (review CR-01: 90..104 and 150..157 for spreads, etc.). Confirmed by reading: `layout.raster[layout.index(x0+dx, y0+dy)]` is unguarded | WARNING | Latent heap overflow in test support; no current scenario uses an affected dpi (75, 300, 600 only). Not a goal blocker; the header contract is wrong |
| `src/pipeline_tests/TestProjectRoundTrip.cpp` | 277-339 | `test_every_stage_carries_non_default_values` is satisfied for page_split by the stage having run in one project and not the other (review WR-03). No assertion of `single-uncut` in the file (grep finds none) | WARNING | The "non-default value carried" claim for page_split rests on the byte-identical round trip of a file that contains the layout, not on a direct check |
| `src/pipeline_tests/TestComparatorSelfTest.cpp` | 473-500 | `UPDATE_GOLDENS` not restored if an assertion throws (WR-01) | INFO/WARNING | Only matters in filtered, reordered runs |
| various | - | WR-02, WR-04, WR-05, IN-01 to IN-07 | INFO | Advisory, all open in `02-REVIEW-DISPOSITION.md` |

No `TBD`/`FIXME`/`XXX` debt markers in phase-modified files.

### Human Verification Required

#### 1. Criterion 5 wording

**Test:** Decide if "marked as an expected failure that names the requirement that will fix it" is satisfied by tags that say `no requirement` plus backlog items.
**Expected:** No v1 requirement fixes F-04-1, F-04-2, F-05-1, F-06-1 or F-07-1, so the literal wording cannot be met. If accepted, add to this file's frontmatter:

```yaml
overrides:
  - must_have: "Any known defect that the unmodified code cannot pass is marked as an expected failure that names the requirement that will fix it"
    reason: "No requirement covers these defects; tags say 'no requirement' and each defect has backlog item 999.1 to 999.7"
    accepted_by: "{name}"
    accepted_at: "{ISO timestamp}"
```

**Why human:** Deviation from the roadmap contract.

#### 2. D7 despeckle-sanity deviation

**Test:** Accept or reject plan 02-07's changed sanity condition (specks counted inside the content box: 15/15 and 11/11 present with despeckling off).
**Expected:** Entry 5 in `.planning/WINDOWS.md` is closed or the plan wording amended.
**Why human:** Open deviation awaiting the user.

#### 3. Review disposition

**Test:** Decide on CR-01 (and WR-03, WR-01) from `02-REVIEW.md`: fix now or backlog.
**Expected:** `02-REVIEW-DISPOSITION.md` entries move from `open` to a decision. Cheapest fix for CR-01: restrict `dpiSupported` to dpi values where the block fits, or throw in `placeGlyph` when `x0 + dot > width` or `y0 + dot > height`, plus a dpi-sweep test.
**Why human:** Triage is the user's decision; Phase 3 baselines (75/300/600 dpi) are unaffected.

### Gaps Summary

No gaps. The suite exists, is wired into one `ctest` command, passes (7/7 Release, re-run by me), and fails when a golden is altered (shown by my own mutation, reverted). Production code is unchanged from the Phase 1 code except three CMake lines, so the goldens are a true baseline. Noted without action: the Phase 2 line in `ROADMAP.md` is still unchecked (`- [ ] **Phase 2**`); the orchestrator updates it on completion. Debug and ASan results were taken from the records, not re-run (Debug takes about 230 s).

---

_Verified: 2026-10-23T22:30:00Z_
_Verifier: Claude (gsd-verifier)_
