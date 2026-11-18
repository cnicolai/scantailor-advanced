---
phase: 02-golden-baselines
verified: 2026-10-24T15:43:17Z
status: passed
score: 9/9 must-haves verified
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
  - .planning/phases/02-golden-baselines/02-11-PLAN.md
  - .planning/phases/02-golden-baselines/02-11-SUMMARY.md
  - .planning/phases/02-golden-baselines/02-12-PLAN.md
  - .planning/phases/02-golden-baselines/02-12-SUMMARY.md
  - .planning/phases/02-golden-baselines/02-13-PLAN.md
  - .planning/phases/02-golden-baselines/02-13-SUMMARY.md
  - .planning/phases/02-golden-baselines/02-14-PLAN.md
  - .planning/phases/02-golden-baselines/02-14-SUMMARY.md
  - src/CMakeLists.txt
  - src/dewarping/tests/TestTextLineTracer.cpp
  - src/imageproc/tests/TestBinarize.cpp
  - src/imageproc/tests/TestScale.cpp
  - src/pipeline_tests/TestBookSpread.cpp
  - src/pipeline_tests/TestComparatorSelfTest.cpp
  - src/pipeline_tests/TestFlatDocument.cpp
  - src/pipeline_tests/TestGeneratorDeterminism.cpp
  - src/pipeline_tests/TestGoldenStore.cpp
  - src/pipeline_tests/TestMixedPage.cpp
  - src/pipeline_tests/TestProjectRoundTrip.cpp
  - src/test_support/GoldenStore.cpp
  - src/test_support/PageGenerator.cpp
  - src/test_support/PageGenerator.h
  - src/test_support/TextGolden.cpp
  - src/test_support/TextGolden.h
  - src/test_support/Tolerances.h
covered_digest: "v3:sha256:89c0901bc4f4b43745661d6f7ea59ba4851e4e5db65e8feb8f16146d28507606"
behavior_unverified: 0
overrides_applied: 1
overrides:
  - must_have: "Any known defect that the unmodified code cannot pass is marked as an expected failure that names the requirement that will fix it"
    reason: "No v1 requirement covers F-04-1, F-04-2, F-05-1, F-06-1 or F-07-1, and Phase 2 records defects without fixing them (D-18). Each XFAIL_CHECK tag says 'no requirement' and carries its finding code; 02-FINDINGS.md maps each code to its backlog item (999.1, 999.2, 999.3, 999.4, 999.6). The intent of the criterion (known defects do not break the run, stay traceable, and trip when fixed) holds."
    accepted_by: "Claude (decision delegated by Christoph Nicolai during UAT)"
    accepted_at: "2026-10-24T13:45:30Z"
re_verification:
  previous_status: human_needed
  previous_score: 5/5
  gaps_closed:
    - "G-02-3: CR-01 (generator writes outside its raster) and WR-01 to WR-05 of 02-REVIEW.md"
  gaps_remaining: []
  regressions: []
coincidental_reliance_items:
  - truth: "WR-04 closed: a test proves a reference handed out for 300 dpi stays valid after the 600 dpi page is added"
    reason: incidental-ordering
    harden: "test_spread_page_cache_keeps_references only fails against a reallocating container when it runs alone; in the default order test_book_fixture_fingerprints fills both cache entries first, so the case passes for any container. The fix itself (std::map) is correct. Use a dpi no other case uses, or state in the comment that the test is documentation unless run alone (review IN-02)."
---

# Phase 2: Golden Baselines Verification Report

**Phase Goal:** A synthetic-image test suite records what the unmodified code produces for flat documents, book spreads and mixed pages, so every later change can be checked against it.
**Verified:** 2026-10-24T15:43:17Z
**Status:** passed
**Re-verification:** Yes, after gap closure (UAT gap G-02-3, plans 02-11 to 02-14)

The goal is achieved and gap G-02-3 is closed. The previous report (`human_needed`) stood on three human decisions; UAT resolved all three (criterion-5 override, D7 waiver, review triage), and the triage's fix-now decision has been carried out and is confirmed in the code below. No gap blocks Phase 3.

Method: no build, ctest or test program was run by me in this verification (rule of this run). Evidence is the source tree, git history, and the logs under `build/scratch/` that were produced by the orchestrator gates and by the plan executors in this run. The previous verification (2026-10-23) had run Release `ctest` and a golden mutation myself; those results are still valid because no golden and no production file changed since (checked below), and the Release gates re-ran after every fix.

## Goal Achievement

### Observable Truths: roadmap success criteria (regression check)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Generator deterministic for flat, spread, mixed, curved pages with ground truth; no font rendering, no `rand()` | VERIFIED | `GeneratorDeterminismTestSuite` now has 13 cases (log `build/scratch/cr01-asan-green.txt` lists them) including `test_same_seed_same_pixels`, `test_different_seed_different_pixels` and the new pinned-fingerprint cases. `PageGenerator.cpp` changes since the last report add only a fit check and bounds guards; no `rand`, `QPainter` or `<random>`. |
| 2 | Tests check flat, book, mixed behaviour; revived `TestBinarize` and dewarping tests run alongside | VERIFIED | Unchanged test files for these suites; `closing-random.txt` shows 93 test cases entered in `pipeline_tests` (86 before, plus the seven new cases of plans 02-12 to 02-14), `imageproc_tests` and `dewarping_tests` pass in all three gates. |
| 3 | Altered output fails under per-kind tolerance, leaves actual/diff images; baselines change only with the update flag; manifest records toolchain | VERIFIED | `git diff --quiet a796c2ae HEAD -- src/pipeline_tests/golden` exits 0: no golden file changed since the baseline commit. `GoldenStore.cpp` changes only make the update path stricter (WR-02, WR-05). Mutation proof from the previous report (edited golden fails, restored) still holds because the compare path is untouched. |
| 4 | Baseline commit precedes every strip/cleanup commit and its source tree matches Phase 1 code | VERIFIED | `git diff --name-only cde6825 HEAD -- src CMakeLists.txt CMakePresets.json cmake`, filtered of test and golden paths, lists only `src/CMakeLists.txt`; its diff is exactly `add_subdirectory(test_support)`, `add_subdirectory(pipeline_tests)`, `add_subdirectory(dewarping/tests)`. `git diff --name-only cb88abec HEAD -- src` outside `src/test_support` and `src/pipeline_tests` is empty: the gap closure touched no production code (D-16). |
| 5 | Round trip byte-identical with non-default values and special characters; one `ctest` runs everything offscreen, isolated, passes; known defects are expected failures | VERIFIED (override: tag wording) | `build/scratch/postgate-02-12/13/14-release.txt`: each `100% tests passed out of 7`. `closing-release.txt` and `closing-debug.txt` (return codes 0): 7/7 in Release (56.0 s) and Debug (238.6 s). Random order: `closing-random.txt`, return code 0, 93 cases. Override for the "names the requirement" wording carried forward (accepted in UAT test 1). |

### Observable Truths: gap-closure plans 02-11 to 02-14

| # | Truth (source) | Status | Evidence |
|---|----------------|--------|----------|
| 6 | 02-11: access answer recorded before any build; Phase 2 record unchanged; section `## Plans 02-11 to 02-14 (review fixes)` with Decision, Date, Scope, question, reply | VERIFIED (note) | `records/access-approval.md` still starts with `Decision: approve` (plan 02-01 gate), new section present with `Decision: approve`, `Date: 2026-10-24`, `Scope:`, question parts 1, 2, 4 and reply "I want you to decide all of these". The decision was taken by the orchestrator on that delegation (`Decided by: Claude (orchestrator)`), as the verification context states. Commit `8dc6d5d8` precedes the first build commit of 02-12 (`8c47d66e`). |
| 7 | 02-12 CR-01: generator never writes outside its raster at any accepted dpi; unsupported dpi gives null image / null mask / `std::invalid_argument`; guards in `placeGlyph` and `placeSpecks`; sweep and rule pinned; pixels unchanged at 75/300/600 | VERIFIED | Code: `dpiSupported(kind, dpi)` = `dpiInRange && layoutFits(computeGeometry(...))` (`PageGenerator.cpp:604`). `layoutFits` (271-301) checks each block's ink box (`inkWidth`/`inkHeight`, which equal the exact extent `placeGlyph` writes: `(cells*6-1)*dot` by `((lines-1)*11+7)*dot`) against the page and against its documented frame, and the MIXED picture against the page. `generatePage`, `renderInkMask` (both dpis) `mapFlatToImage`, `mapImageToFlat` all route through it. `placeGlyph` throws `std::logic_error` before a write with `x0+dot > width` or `y0+dot > height`; `placeSpecks` likewise. The divisibility shortcut the plan prohibited is not used. Behavioural evidence: RED `cr01-asan-red.txt` (status 201, `heap-buffer-overflow ... placeGlyph PageGenerator.cpp:394` from the sweep, then SEGV) against the unmodified generator; GREEN `cr01-asan-green.txt` (second run after `eb62abd5`, status 0, 0 `AddressSanitizer` lines, 0 `runtime error`, all 13 cases incl. `test_dpi_sweep_gives_valid_or_null_page` and `test_dpi_support_rule`). Ordering: `8c47d66e` (16 lines, `TestGeneratorDeterminism.cpp` only, no sweep) precedes `2a4ad510` (fix and sweep together), so no commit builds a binary that runs the out-of-buffer write. Pixels unchanged: the five 75/600 dpi fingerprints are pinned in `test_pinned_pages_unchanged` from the pre-fix generator, the 300/600 fixtures by `*_fixture_fingerprints`; `fixtures.txt` and all goldens are byte-identical to `a796c2ae`. |
| 8 | 02-13 WR-01, WR-02, WR-05: failed manifest write fails every golden of the suite; unserialisable text refused with the key named; `UPDATE_GOLDENS` restored by RAII guard on every exit | VERIFIED | `SuiteManifest::refresh` now inserts into `m_handled` only after the manifest is on disk or identical (`GoldenStore.cpp:188-206`). `checkText` calls `textRoundTrips(actual, &offendingKey)` before any golden or artifact is touched and names the key in the message. `textRoundTrips` checks `parseText(serializeText(entry)) == entry` per entry. `ScopedEnvironmentVariable` restores or unsets in its destructor and is the first statement of `test_golden_directory_untouched`. Tests present and executed (`closing-random.txt`): `test_failed_manifest_write_fails_every_golden_of_the_suite` (first, second, text check all fail with `MANIFEST.txt`, nothing written, retry succeeds), `test_text_that_cannot_round_trip_is_refused` (both modes, nothing written, four refused shapes with offending key), `test_update_flag_restored_when_an_assertion_throws` (set and unset starting state). RED evidence for each is in `02-13-SUMMARY.md` and `records/review-fixes.md`. No golden touched; store tests use temp roots. |
| 9 | 02-14 WR-03, WR-04 and closing run: page-split layout value asserted directly; spread cache is `std::map`; both ctest presets 7/7; random order clean; clang-format clean; goldens and scope invariants; disposition marks CR-01, WR-01 to WR-05 fixed | VERIFIED (disposition wording partly superseded, see warnings) | WR-03: `test_every_stage_carries_non_default_values` now requires `SINGLE_PAGE_UNCUT` in `makeNonDefaultParams()`, no `<image>` in the default project's `<page-split>`, and `layoutType="single-uncut"` plus `params/pages type="single-uncut"` on every `<image>` of both written files; the attribute names match `page_split/Filter.cpp:133` and `LayoutType.cpp:14`. WR-04: `spreadPage` is `std::map<int, GeneratedPage>`, plus `test_spread_page_cache_keeps_references`. Closing run in `records/review-fixes.md` matches the raw logs `closing-release.txt`, `closing-debug.txt` (7/7 each), `closing-random.txt` (rc 0). `git status --porcelain -- src` is empty. `02-REVIEW-DISPOSITION.md` lists CR-01 and WR-01 to WR-05 as `fixed` with the plan that fixed each. The plan's "IN-01 to IN-07 stay deferred, open count 0" no longer holds literally: the later re-review reported four new info findings that reuse the ids IN-01 to IN-04 (open: 4). |

**Score:** 9/9 truths verified (5 roadmap criteria, 4 gap-closure plan truth groups), 0 behavior-unverified.

Behaviour-dependent truths (state/ordering invariants): CR-01 (no out-of-raster write) is proven by the ASan RED/GREEN pair; WR-01 (flag restored after a throw) by a case that throws in a scope; WR-02 (suite stays unmarked after a failed write) by a case that makes the write fail and then retries. None is left on presence alone.

### G-02-3 closure

| Gap item | Closed by | Verified in code | Verified by test |
|----------|-----------|------------------|------------------|
| CR-01 | 02-12 (`2a4ad510`, `eb62abd5`) | `dpiSupported`/`layoutFits`, guards | sweep + rule + pins; ASan RED then GREEN |
| WR-01 | 02-13 (`6f625254`) | `ScopedEnvironmentVariable` | `test_update_flag_restored_when_an_assertion_throws` |
| WR-02 | 02-13 (`73a18e3c`) | `refresh()` order | `test_failed_manifest_write_fails_every_golden_of_the_suite` |
| WR-03 | 02-14 (`9eb6ff6f`) | direct `layoutType` assertions | the case itself (mutation check recorded in 02-14-SUMMARY) |
| WR-04 | 02-14 (`46186f71`) | `std::map` cache | `test_spread_page_cache_keeps_references` (weak in default order, see advisory) |
| WR-05 | 02-13 (`62dbc761`) | `textRoundTrips` in `checkText` | `test_text_that_cannot_round_trip_is_refused` |
| Constraint: no golden change, 75/300/600 dpi pixels identical | | `git diff --quiet a796c2ae -- golden` exit 0 | fingerprint pins pass in all gates |
| Constraint: both ctest presets 7/7, clang-format clean | | `closing-*.txt`; format list in `records/review-fixes.md` | not re-run by me |

The independent re-review (`02-REVIEW.md` at `4db72234`: 0 critical, 0 warning, 4 info) agrees that all six findings are closed, and I reached the same result from the code. **G-02-3 is closed.**

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/test_support/PageGenerator.{h,cpp}` | `dpiSupported(PageKind,int)`, fit check, guarded writes | VERIFIED | present, substantive, wired into all four entry points |
| `src/test_support/GoldenStore.cpp`, `TextGolden.{h,cpp}` | manifest ordering, `textRoundTrips` | VERIFIED | wired: `checkText` calls `textRoundTrips`; `refresh` used by both check paths |
| `src/pipeline_tests/TestGeneratorDeterminism.cpp` | pins, sweep, rule | VERIFIED | three named cases present and executed |
| `src/pipeline_tests/TestGoldenStore.cpp`, `TestComparatorSelfTest.cpp`, `TestProjectRoundTrip.cpp`, `TestBookSpread.cpp` | new cases | VERIFIED | all seven new cases entered in the 93-case random run |
| `records/review-fixes.md`, `records/access-approval.md` | evidence | VERIFIED | contain `## CR-01`, `## Closing run`, `## Plans 02-11 to 02-14 (review fixes)` |
| Phase 2 artifacts from the first report (goldens, `dewarping/tests`, `TestBinarize`, records, findings) | | VERIFIED | goldens identical to the baseline commit; the rest untouched by the gap closure |

### Key Link Verification

| From | To | Via | Status |
|------|----|-----|--------|
| `generatePage`, `renderInkMask`, both maps | `dpiSupported(kind, dpi)` | direct call at entry | WIRED |
| `GoldenStore::checkText` | `textRoundTrips` | call before golden/artifact write | WIRED |
| `test_golden_directory_untouched` | `ScopedEnvironmentVariable` | first statement | WIRED |
| round-trip test | `page_split::Filter::writeImageSettings` | reads `layoutType` and `params/pages type` | WIRED (attribute names confirmed in production code) |
| `spreadPage` | `std::map` | `find`/`emplace` | WIRED |
| 02-12/13/14 preconditions | `records/access-approval.md` | awk on section heading | WIRED (section present) |
| `src/CMakeLists.txt` | test targets | three `add_subdirectory` in `if (BUILD_TESTS)` | WIRED (unchanged) |

### Data-Flow Trace (Level 4)

Test code only. The compared values come from the real filter `Task` objects run by `HeadlessPipeline` and from generator ground truth; the gap closure did not touch that path.

### Behavioral Spot-Checks

Not run by me (rule of this run). Recorded results read from raw logs:

| Behavior | Source | Result | Status |
|----------|--------|--------|--------|
| Full suite after each fix | `postgate-02-12/13/14-release.txt` | 7/7 each | PASS |
| Closing Release / Debug | `closing-release.txt`, `closing-debug.txt` | 7/7, rc 0 | PASS |
| `pipeline_tests --random=1` | `closing-random.txt` | rc 0, 93 cases | PASS |
| Sweep against old generator under ASan | `cr01-asan-red.txt` | rc 201, heap-buffer-overflow in `placeGlyph` | PASS (RED as intended) |
| Generator suite on final tree under ASan | `cr01-asan-green.txt` | rc 0, 0 sanitizer lines | PASS |
| Goldens vs baseline | `git diff --quiet a796c2ae HEAD -- src/pipeline_tests/golden` | exit 0 | PASS |
| Tree clean | `git status --porcelain -- src` | empty | PASS |

Caveat: `postgate-02-14-release.txt` begins with `ninja: no work to do`, so it re-ran the binaries built by the executor's closing run at `46186f71`, not a fresh build; the executor's closing build log (`closing-build.txt`, rc 0) covers that commit. The generator `.cpp` has not changed since `2a4ad510` (`git log`), so the ASan GREEN run applies to the final generator.

### Probe Execution

No probes declared. Skipped.

### Requirements Coverage

Union of `requirements:` across the 14 PLAN files: TEST-01 to TEST-11, no ID missing, none extra. All eleven are `[x]` in `REQUIREMENTS.md`; `REQUIREMENTS.md` maps no other ID to Phase 2 (no orphans).

| Requirement | Source Plans | Status | Evidence |
|-------------|--------------|--------|----------|
| TEST-01 | 02-03, 02-12 | SATISFIED | seeded generator, no `rand`/font; CR-01 hardening now makes unsupported dpi refuse instead of overflow |
| TEST-02 | 02-02, 02-09, 02-13 | SATISFIED | comparator suites; update-flag restore (WR-01) |
| TEST-03 | 02-02, 02-05, 02-06, 02-07, 02-13 | SATISFIED | update flag only; manifests; failed manifest write no longer skipped (WR-02); text goldens must parse back (WR-05) |
| TEST-04 | 02-05, 02-06, 02-07, 02-10 | SATISFIED | goldens identical to `a796c2ae`; non-test diff vs `cde6825` is `src/CMakeLists.txt` only |
| TEST-05 | 02-01, 02-05 | SATISFIED | `FlatDocumentTestSuite` (600 dpi binarization is XFAIL F-05-1 with backlog 999.3) |
| TEST-06 | 02-03, 02-06, 02-14 | SATISFIED | split position and dewarp straightening (cutter mid-height bound XFAIL F-06-1); spread cache fixed (WR-04) |
| TEST-07 | 02-03, 02-07 | SATISFIED | mixed suite (AUTO layout XFAIL F-07-1); despeckle sanity deviation waived (WINDOWS.md entry 5 `waived`) |
| TEST-08 | 02-04 | SATISFIED | nine binarizers run |
| TEST-09 | 02-04 | SATISFIED | `dewarping_tests` (F-04-2 XFAIL) |
| TEST-10 | 02-08, 02-14 | SATISFIED | byte-identical round trip; page-split value now asserted directly (WR-03) |
| TEST-11 | 02-01, 02-04, 02-09, 02-10, 02-11, 02-14 | SATISFIED | 7/7 Release and Debug, random order clean; access approval recorded |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `src/test_support/PageGenerator.h` | 20-22, 56-63 | Header names 150 and 225 dpi as refused examples; 400 and 450 dpi are refused for SPREAD, MIXED, CURVED too (review IN-01) | INFO | Phases 3 to 6 use 75/300/600 only; a later scenario at 400 dpi would get a null page and must check `isNull()` |
| `src/pipeline_tests/TestBookSpread.cpp` | 417-426 | `test_spread_page_cache_keeps_references` passes for any container in the default order (review IN-02) | INFO | Regression guard is weak; the fix is correct |
| `src/pipeline_tests/TestGeneratorDeterminism.cpp` | 73-76, 508 | Test-local truncating `scaled()` differs from the generator's rounding; accepted counts come from a script not in the repo (IN-03) | INFO | Same values at the dpis used (75, 200) |
| `src/pipeline_tests/TestBookSpread.cpp` | 56-58 | Wrong arithmetic in the noise-floor comment (IN-04) | INFO | Comment only |
| `src/pipeline_tests/TestProjectRoundTrip.cpp` etc. | | IN-05 to IN-07 (plan ids in comments, unused include, Qt 5 fallback advertised) | INFO | deferred in UAT |

No `TBD`/`FIXME`/`XXX` markers in the files changed by the gap closure (the 10 diffs read in full contain none). No stubs.

### Warnings

1. **Disposition record shape (WARNING, record only).** `02-REVIEW-DISPOSITION.md` ends with `open: 4, total: 13`, not the plan 02-14 target "open 0, IN-01 to IN-07 deferred". Cause: the re-review produced four new info findings under the ids IN-01 to IN-04, which replaced the earlier deferred rows of those ids (old IN-01 "stale constants comment" is carried as new IN-04; old IN-02 `areaAverageReference`, IN-03 NFD file name and IN-04 duplicated sanity code are no longer reported or recorded). Nothing here is a warning- or critical-level code finding, and the CR-01/WR-01..05 rows are correct. Triage the four open infos (or defer them with a reason) when convenient; they do not gate Phase 3.
2. **Weak regression test for WR-04 (advisory, recorded under `coincidental_reliance_items`).** See IN-02.
3. **Access approval by delegation (note).** The plan 02-11 truth says "the user has answered one question". The recorded reply is a delegation ("I want you to decide all of these") and the orchestrator chose "approve, later phases too". This matches the instruction given for this run and the way UAT items 1 to 3 were handled, so I count the truth met; the user may wish to confirm that the approval extending to Phases 3 to 6 is what they intended.

### Human Verification Required

None. The three items of the previous report were resolved in UAT (tests 1 and 2 passed with the override and the WINDOWS.md waiver recorded; test 3 became G-02-3, now closed).

### Gaps Summary

No gaps. Phase 2 delivers a deterministic synthetic page generator, a per-kind comparator with a guarded golden store, goldens recorded from the unmodified code (identical to commit `a796c2ae`, production code unchanged apart from three CMake lines), and one `ctest` command that passes 7/7 in Release and Debug. The review findings that UAT triage chose to fix are fixed in test code only, each with a test, and the critical one (CR-01) has an ASan red/green pair. Noted without action: the Phase 2 line in `ROADMAP.md` is still unchecked (`- [ ] **Phase 2**`); the orchestrator updates it on completion. Debug and ASan results are taken from recorded logs, not re-run by me.

---

_Verified: 2026-10-24T15:43:17Z_
_Verifier: Claude (gsd-verifier)_
