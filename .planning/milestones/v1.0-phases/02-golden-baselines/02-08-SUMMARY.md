---
phase: 02-golden-baselines
plan: 08
subsystem: testing
tags: [boost-test, project-file, round-trip, unicode-nfc-nfd, special-characters, default-params, qdom]

requires:
  - phase: 02-golden-baselines
    provides: "pipeline_tests executable with offscreen core Application, isolated settings and temp root (02-01), XFAIL_CHECK (02-02)"
provides:
  - "ProjectRoundTripTestSuite: write, read, write of a five-page project with special-character and NFC/NFD names and non-default parameters in all six stages, byte-identical"
  - "records/round-trip.md: character set, non-default values per stage, results, SAFE-02 disposition, finding F-08-1"
  - "SAFE-02 evidence: the premise was not reproduced for file and directory names on the unmodified code"
affects: [02-09, 02-10, phase-06-SAFE-02]

actuals:
  tokens: 7000
  tasks: 2
  commits: 2

tech-stack:
  added: []
  patterns:
    - "Fixture saves DefaultParamsProvider params and profile name and restores both in its destructor with a BOOST_CHECK on the name"
    - "Real StageSequence plus a PageSelectionProvider stub; page_split run through filter->createTask with fix_orientation first, so the stage is written"
    - "Numeric parse-back checked per attribute: toDouble ok, equal to the value taken from the DefaultParams used, at most two decimals"

key-files:
  created:
    - src/pipeline_tests/TestProjectRoundTrip.cpp
    - .planning/phases/02-golden-baselines/records/round-trip.md
  modified:
    - .planning/WINDOWS.md

key-decisions:
  - "No SAFE-02 expected-failure mark: every special-character case passes on the unmodified code"
  - "page_split is run for every page of the non-default project because DefaultParams alone leaves its element empty (F-08-1)"
  - "The select_content page rectangle is not checked for exact parse-back: it is stored in pixels after an integer-dpm conversion (2549.9949 px for 215.9 mm at 300 dpi)"

patterns-established:
  - "Round-trip project files are written next to the input and output directories inside a QTemporaryDir, so all paths in the file are relative and the two writes are comparable byte for byte"

requirements-completed: [TEST-10]

coverage:
  - id: D1
    description: "A five-page project with &, <, >, quotes, spaces, NFC and NFD umlaut, one-character and 200-byte names in special-character input and output directories and non-default parameters in all six stages is written, read and written again; both files are byte-identical"
    requirement: "TEST-10"
    verification:
      - kind: unit
        ref: "build/macos-release/pipeline_tests --run_test=ProjectRoundTripTestSuite/test_round_trip_is_byte_identical (Release and Debug)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Each of the six stage elements of the written project differs from the default-profile project"
    requirement: "TEST-10"
    verification:
      - kind: unit
        ref: "ProjectRoundTripTestSuite/test_every_stage_carries_non_default_values"
        status: pass
    human_judgment: false
  - id: D3
    description: "Page paths read back equal the originals as code-unit sequences, NFC and NFD stay two pages, the output directory is unchanged and the page order is the creation order"
    requirement: "TEST-10"
    verification:
      - kind: unit
        ref: "ProjectRoundTripTestSuite/test_names_and_order_survive"
        status: pass
    human_judgment: false
  - id: D4
    description: "Every non-default numeric value is UI-enterable and parses back exactly from the project file"
    requirement: "TEST-10"
    verification:
      - kind: unit
        ref: "ProjectRoundTripTestSuite/test_numeric_values_parse_back_exactly"
        status: pass
    human_judgment: false
  - id: D5
    description: "A one-page project round-trips byte-identically"
    requirement: "TEST-10"
    verification:
      - kind: unit
        ref: "ProjectRoundTripTestSuite/test_single_page_round_trip"
        status: pass
    human_judgment: false
  - id: D6
    description: "The suite leaves DefaultParamsProvider on its original profile, and works only inside its own QTemporaryDir under build/<preset>/test-tmp"
    verification:
      - kind: unit
        ref: "fixture destructor BOOST_CHECK; EnvironmentTestSuite passes after it, also with --random=1; test-tmp is empty after the runs"
        status: pass
    human_judgment: false
  - id: D7
    description: "The SAFE-02 disposition (premise not reproduced for paths) is recorded with evidence"
    requirement: "TEST-10"
    verification: []
    human_judgment: true
    rationale: "Whether SAFE-02 is still needed, and what it should then fix, is a Phase 6 decision for the user; the record only supplies the evidence"

duration: 11min
completed: 2026-10-23
status: complete
plan_head_before: 77c735595763d173bf50e03d9ac0ca8c793c7c4d
plan_head_after: a027d969b0cca6136a84944c914865a003f7beb7
commits: 2
---

# Phase 2 Plan 8: Project Round-Trip Test Summary

**Write, read, write of a five-page project (special-character and NFC/NFD names, non-default values in all six stages) gives byte-identical files on the unmodified code, so the SAFE-02 premise was not reproduced for file and directory names.**

## Performance

- **Duration:** 11 min
- **Started:** 2026-10-23T20:04:00Z
- **Completed:** 2026-10-23T20:15:00Z
- **Tasks:** 2
- **Files modified:** 3 (2 created, `.planning/WINDOWS.md` appended)

## Accomplishments

- `ProjectRoundTripTestSuite` with five cases and a fixture that generates five 64 x 64 PNGs named `a&b <c> "d" 'e'.png`, `Seite_ä_NFC.png` (U+00E4), `Seite_ä_NFD.png` (U+0061 U+0308), `x.png` and 100 x `Ü` + `.png` (204 bytes), in `in & <dir> "q" 'a'`, with output directory `out & <x> "y"`, and restores `DefaultParamsProvider` afterwards.
- Both files of the round trip are identical (18308 bytes for five pages, 4220 for one); a mutation of the second write's output directory fails both byte-identity cases and was reverted.
- Non-default values in all six stages (rotation 90, `SINGLE_PAGE_UNCUT`, manual deskew 1.25, letter-size page rectangle with page detection on and fine-tuned corners, margins 7.5 / 8.25 / 9 / 10.5 mm with top-left alignment, MIXED colour at 400 dpi with despeckle 2.0, dewarping AUTO, depth perception 2.5), all parsing back exactly from the file.
- `records/round-trip.md` states the SAFE-02 disposition with evidence; no expected-failure mark exists in the suite.

## Task Commits

1. **Task 1: Round-trip test with special-character names and non-default parameters** - `a2b0653` (test)
2. **Task 2: SAFE-02 disposition, Debug run and the round-trip record** - `a027d96` (test; record, unused include removed)

**Plan metadata:** the docs commit that follows this summary.

## Files Created/Modified

- `src/pipeline_tests/TestProjectRoundTrip.cpp` - fixture `RoundTripFixture`, `AllPagesProvider` stub, `ProjectRoundTripTestSuite` (five cases)
- `.planning/phases/02-golden-baselines/records/round-trip.md` - character set, values per stage, results, SAFE-02 disposition, finding
- `.planning/WINDOWS.md` - one `deviation` entry for the page_split run below

## Decisions Made

- No SAFE-02 mark: every special-character check passes on the unmodified code (prohibition of the plan honoured).
- The select_content page rectangle is excluded from the exact parse-back check because it is written in pixels after an integer dots-per-metre conversion.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] page_split element was identical for default and non-default profile**
- **Found during:** Task 1 (first run of `test_every_stage_carries_non_default_values`)
- **Issue:** The plan assumed `loadDefaultSettings` after `setParams` puts a non-default value into every stage element. For page_split it does not: `Filter::writeImageSettings` writes an `<image>` element only when split parameters exist, and `defaultLayoutType` is not set from `DefaultParams`. The element read `<page-split defaultLayoutType="auto-detect"/>` in both projects.
- **Fix:** The helper `runPageSplit` runs, for every page of the non-default project, the real filters' tasks (`fix_orientation` task with the `page_split` task as next task, `NullTaskStatus`, the PNG loaded with `ImageLoader`). With `SINGLE_PAGE_UNCUT` no detection runs, and the element then carries `layoutType="single-uncut"` and the split parameters. No expected-failure mark was added (the behaviour is by design); recorded as finding F-08-1 with `Fixing requirement: none`.
- **Files modified:** `src/pipeline_tests/TestProjectRoundTrip.cpp`, `records/round-trip.md`, `.planning/WINDOWS.md`
- **Verification:** `test_every_stage_carries_non_default_values` passes in Release and Debug.
- **Committed in:** `a2b0653`, `a027d96`

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** The page_split stage is exercised through its real task rather than through defaults alone; no scope creep, no change to production code.

## TDD Gate Compliance

The plan's tasks are test-only on existing production code. The RED/GREEN split of `tdd.md` does not apply: the new tests pass on the unmodified code by design (a failing case would have been an XFAIL finding), so there is no `feat(02-08)` commit and no INVALID_RED evidence to classify. The test commits are `test(02-08)` (`a2b0653`, `a027d96`).

## Issues Encountered

None beyond the deviation above.

## User Setup Required

None - no external service configuration required.

## Known Stubs

None.

## Threat Flags

None. Files are created only inside the suite's `QTemporaryDir` under `build/<preset>/test-tmp`; names contain no `/` or `..` (T-02-21); the fixture restores the process-wide default parameters and checks the profile name (T-02-22).

## Next Phase Readiness

- Ready for 02-09 (the suite runs under random order without leaving a profile behind: checked with `--random=1`) and 02-10 (finding F-08-1 for the backlog; the SAFE-02 evidence for Phase 6).
- Debug `pipeline_tests` under ctest takes about 206 s; Release about 42 s.

---
*Phase: 02-golden-baselines*
*Completed: 2026-10-23*

## Self-Check: PASSED

- `src/pipeline_tests/TestProjectRoundTrip.cpp` and `records/round-trip.md` exist; commits `a2b0653` and `a027d96` are on the branch.
- Acceptance greps over the record (all five sections) pass; `ProjectRoundTripTestSuite` passes in Release and Debug; `ctest --preset macos-release -R pipeline_tests` and `ctest --preset macos-debug -R pipeline_tests` exit 0.
