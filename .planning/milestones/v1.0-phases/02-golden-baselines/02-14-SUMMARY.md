---
phase: 02-golden-baselines
plan: 14
subsystem: testing
tags: [pipeline-tests, project-round-trip, page-split, test-cache, closing-run, gap-closure, G-02-3, WR-03, WR-04]

requires:
  - phase: 02-golden-baselines
    provides: records/access-approval.md approval for build-tool reads (plan 02-11)
  - phase: 02-golden-baselines
    provides: CR-01 generator dpi fix (02-12) and WR-01/WR-02/WR-05 golden store fixes (02-13)
provides:
  - "Direct assertion of the page-split layoutType and pages type (single-uncut) in both files of the non-default round trip"
  - "Node-stable std::map cache for spread pages, with a reference-stability test that fails alone against the vector cache"
  - "Closing run of all review fixes: ctest 7 of 7 in Release and Debug, random order, formatting, golden and scope invariants"
  - "02-REVIEW-DISPOSITION.md: CR-01 and WR-01 to WR-05 fixed, open 0, IN-01 to IN-07 deferred"
affects: [phase-02-verification, phase-03, phase-04]

actuals:
  tokens: 2600
  tasks: 3
  commits: 4
plan_head_before: 42d024ce9ea00e8f77f7cc575eb1e057f1beeafa
plan_head_after: c4349a8ceaf1ba148d2a3b21c53ea77b89fea4e7

tech-stack:
  added: []
  patterns:
    - "Non-vacuity by mutation: change the expected value, confirm one failure per checked element, restore"
    - "Static test caches that hand out references use node-based containers (std::map)"

key-files:
  created: []
  modified:
    - src/pipeline_tests/TestProjectRoundTrip.cpp
    - src/pipeline_tests/TestBookSpread.cpp
    - .planning/phases/02-golden-baselines/records/review-fixes.md
    - .planning/phases/02-golden-baselines/02-REVIEW-DISPOSITION.md

key-decisions:
  - "The page-split value is compared against the literal file-format string single-uncut, after a BOOST_REQUIRE that the non-default parameter set really uses SINGLE_PAGE_UNCUT, so the check cannot silently turn into a comparison of two defaults"
  - "The stability test compares addresses only before dereferencing, since the vector cache leaves the old pointer dangling"

requirements-completed: [TEST-06, TEST-10, TEST-11]

coverage:
  - id: D1
    description: "The round trip fails if the page-split layout type stops reaching the project file (WR-03)"
    requirement: TEST-10
    verification:
      - kind: unit
        ref: "build/macos-release/pipeline_tests --run_test=ProjectRoundTripTestSuite/test_every_stage_carries_non_default_values"
        status: pass
    human_judgment: false
  - id: D2
    description: "No reference handed out by the spread page cache can dangle, and a test catches a regression (WR-04)"
    requirement: TEST-06
    verification:
      - kind: unit
        ref: "build/macos-release/pipeline_tests --run_test=BookSpreadTestSuite/test_spread_page_cache_keeps_references (alone) and --run_test=BookSpreadTestSuite"
        status: pass
    human_judgment: false
  - id: D3
    description: "All review fixes together: ctest 7 of 7 in macos-release and macos-debug, pipeline_tests in random order, formatting clean, goldens and production code unchanged"
    requirement: TEST-11
    verification:
      - kind: integration
        ref: "ctest --preset macos-release; ctest --preset macos-debug"
        status: pass
      - kind: unit
        ref: "build/macos-release/pipeline_tests --random=1 --log_level=test_suite"
        status: pass
      - kind: other
        ref: "git diff --quiet a796c2ae -- src/pipeline_tests/golden; git diff --name-only cde6825 (non-test pathspec) equals src/CMakeLists.txt"
        status: pass
    human_judgment: false
  - id: D4
    description: "Record and disposition: CR-01 and WR-01 to WR-05 fixed, IN-01 to IN-07 deferred, open 0"
    requirement: TEST-11
    verification:
      - kind: other
        ref: "grep checks on records/review-fixes.md and 02-REVIEW-DISPOSITION.md (task 3 verify, fourth command)"
        status: pass
    human_judgment: false

duration: 9min
completed: 2026-10-24
status: complete
---

# Phase 2 Plan 14: Close the review gap and prove the whole set Summary

**The round trip now asserts `layoutType="single-uncut"` and `pages type="single-uncut"` on every page-split `<image>` of both written files (WR-03), `spreadPage` caches in a `std::map` with a test that failed alone against the vector (WR-04), and the closing run shows 7 of 7 in Release and Debug, random order clean, goldens and production code untouched, with all six findings marked fixed.**

## Performance

- **Duration:** 9 min
- **Started:** 2026-10-24T15:19:31Z
- **Completed:** 2026-10-24T15:28:37Z (before this summary)
- **Tasks:** 3
- **Files modified:** 4 (two test sources, the record, the disposition; no production code, no golden)

## Accomplishments

- WR-03: after the per-stage difference loop, `test_every_stage_carries_non_default_values` requires that the non-default set uses `SINGLE_PAGE_UNCUT`, that the default project's `<page-split>` has no `<image>` (F-08-1), and that the first and the second written file each have one `<image>` per input page with `layoutType="single-uncut"` and `params/pages type="single-uncut"`. A comment explains why the element comparison alone proved presence only.
- WR-04: `spreadPage` uses `static std::map<int, test_support::GeneratedPage>` with find/emplace, the shape of `cachedPage` in `TestGeneratorDeterminism.cpp`; `<map>` is included. `test_spread_page_cache_keeps_references` keeps the address of the 300 dpi page, adds the 600 dpi page and requires the same address before it reads the remembered page.
- Closing run on `46186f71`: Release 7 of 7 (56.01 s), Debug 7 of 7 (238.57 s), `pipeline_tests --random=1` exit 0 with `*** No errors detected`, ten changed C++ files clang-format clean, no golden changed since `a796c2ae`, only `src/test_support/` and `src/pipeline_tests/` changed since `cb88abec`, and the non-test diff against `cde6825` is still only `src/CMakeLists.txt`. Recorded in `records/review-fixes.md` (`## WR-01 to WR-05`, `## Closing run`).
- `02-REVIEW-DISPOSITION.md`: CR-01, WR-01 to WR-05 `fixed` with their plan (02-12, 02-13, 02-14), `open: 0`, IN-01 to IN-07 untouched.

## TDD Gate Compliance

Plan type is `execute`; tasks 1 and 2 carry `tdd="true"`. `gsd_run check tdd-red-evidence` was not used because the reports are Boost.Test console output, for which the classifier has no adapter.

- **Task 1 (tracer), WR-03, non-vacuity instead of RED** (production already writes the value correctly, as the plan states). Both expected strings in the new block were temporarily changed to `"two-pages"`, `pipeline_tests --run_test=ProjectRoundTripTestSuite/test_every_stage_carries_non_default_values --log_level=test_suite` exited 201 with 20 failures, which is 5 images x 2 files x 2 attributes. One failing line:
  `TestProjectRoundTrip.cpp:357: error: in "ProjectRoundTripTestSuite/test_every_stage_carries_non_default_values": page-split image 0 of the first file has layoutType="single-uncut"`
  The mutation was restored from a copy kept in `build/scratch/`, the file was formatted and the grep gate confirms 0 occurrences of `"two-pages"`. Semantic assessment: the target case ran and failed on the planned attribute comparison, not on a parse or fixture fault. The tracer feedback gate (auto mode off, `human_verify_mode` end-of-phase, automated verify only) was met by re-running the verify end to end before task 2.
- **Task 2, WR-04 RED** (`b422c5dc`, test only; `pipeline_tests --run_test=BookSpreadTestSuite/test_spread_page_cache_keeps_references --log_level=test_suite`, exit 200, vector cache still in place):
  `TestBookSpread.cpp:424: fatal error: in "BookSpreadTestSuite/test_spread_page_cache_keeps_references": the 300 dpi spread page moved when the 600 dpi page was added to the cache`
  Semantic assessment: the target case executed and failed on the address check, the planned assertion; the build was clean. GREEN `46186f71`: the case passes alone and the whole `BookSpreadTestSuite` passes, fixtures fingerprints unchanged. The `git log -S` ordering check (the first commit that mentions the test still contains the vector cache) exits 0. REFACTOR: none needed.
- The "1 failure is expected" part of the Boost summary line in these runs is the deliberate `test_expected_failure_trip_wire`, not a failure of the run.

## Task Commits

1. **Task 1 (tracer): assert the page-split layout type in the round trip (WR-03)** - `9eb6ff6f` (test)
2. **Task 2: node-stable spread page cache (WR-04)**
   - `b422c5dc` (test) failing reference-stability case
   - `46186f71` (fix) `std::map` cache
3. **Task 3: closing run, record and disposition** - `c4349a8c` (docs)

**Plan metadata:** the docs commit following this summary.

## Files Created/Modified

- `src/pipeline_tests/TestProjectRoundTrip.cpp` - direct page-split assertions in `test_every_stage_carries_non_default_values`; `<utility>` include
- `src/pipeline_tests/TestBookSpread.cpp` - `std::map` cache in `spreadPage`, `<map>` include, `test_spread_page_cache_keeps_references`
- `.planning/phases/02-golden-baselines/records/review-fixes.md` - `## WR-01 to WR-05` and `## Closing run`
- `.planning/phases/02-golden-baselines/02-REVIEW-DISPOSITION.md` - six findings fixed, `open: 0`

## Decisions Made

- Compare against the file-format string rather than `layoutTypeToString(...)`, so the test fails if the production mapping ever changes what users' files contain.
- Task 3's third verify command expands an unquoted `$F` and so needs word splitting; it was run under `bash -c` (the interactive shell is zsh), and the formatting check was also run through `xargs`. Both pass.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None. Debug build and the full Debug ctest ran in the background without trouble.

## Known Stubs

None.

## Threat Flags

None. T-02-36, T-02-37 and T-02-38 are mitigated as planned; no production file, golden or build setting changed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Gap G-02-3 is closed: no open critical or warning finding remains, the suite is 7 of 7 in both gating builds, baselines untouched. Phase 2 is ready for re-verification.
- IN-01 to IN-07 stay deferred by decision (UAT 02 test 3).

## Self-Check: PASSED

- `src/pipeline_tests/TestProjectRoundTrip.cpp`, `src/pipeline_tests/TestBookSpread.cpp`, `records/review-fixes.md` and `02-REVIEW-DISPOSITION.md` exist; commits `9eb6ff6f`, `b422c5dc`, `46186f71`, `c4349a8c` are ancestors of HEAD (`git rev-list --count 42d024ce..HEAD` was 4 before this summary).
- All `<verify>` commands of the three tasks exit 0; only `src/pipeline_tests/TestProjectRoundTrip.cpp` changed under `src/` in the task 1 commit.
