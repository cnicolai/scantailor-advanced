---
phase: 02-golden-baselines
plan: 13
subsystem: testing
tags: [test-support, golden-store, update-mode, manifest, text-golden, raii, gap-closure, G-02-3, WR-01, WR-02, WR-05]

requires:
  - phase: 02-golden-baselines
    provides: records/access-approval.md approval for build-tool reads (plan 02-11)
  - phase: 02-golden-baselines
    provides: page generator dpi fit check (plan 02-12), same pipeline_tests executable
provides:
  - "SuiteManifest::refresh marks a suite handled only after its manifest is on disk"
  - "test_support::textRoundTrips(values, &offendingKey) and its enforcement in GoldenStore::checkText"
  - "ScopedEnvironmentVariable RAII guard in the comparator self-test, with a test for the restore after a throw"
affects: [02-14, phase-03, phase-04]

actuals:
  tokens: 3370
  tasks: 3
  commits: 5
plan_head_before: eb1fc9c20563a6fb92781a8da49fefe4e06164c8
plan_head_after: 6f6252545c5ff101f2f3b0ec6d51c5d87d58b809

tech-stack:
  added: []
  patterns:
    - "A shared 'handled' set is updated only after the side effect it guards has succeeded"
    - "Input validation of stored text values by round trip (serialize, parse, compare) instead of a character blacklist"
    - "RAII guard for process environment variables in tests"

key-files:
  created: []
  modified:
    - src/test_support/GoldenStore.cpp
    - src/test_support/TextGolden.h
    - src/test_support/TextGolden.cpp
    - src/pipeline_tests/TestGoldenStore.cpp
    - src/pipeline_tests/TestComparatorSelfTest.cpp

key-decisions:
  - "textRoundTrips compares each entry on its own (one-entry map through serializeText and parseText), so the offending key is named exactly and a value with an embedded 'x = y' line is caught by the size mismatch"
  - "The refusal message is identical in update and normal mode and is produced before the golden is read, so neither mode writes an artifact"

requirements-completed: [TEST-02, TEST-03]

coverage:
  - id: D1
    description: "A failed manifest write fails every later golden of the suite in the same store, writes nothing, and the first check after the obstacle is gone writes the manifest (WR-02)"
    requirement: TEST-03
    verification:
      - kind: unit
        ref: "src/pipeline_tests/TestGoldenStore.cpp#test_failed_manifest_write_fails_every_golden_of_the_suite"
        status: pass
    human_judgment: false
  - id: D2
    description: "GoldenStore::checkText refuses text values that would not parse back, in both modes, before any write, naming the key (WR-05)"
    requirement: TEST-03
    verification:
      - kind: unit
        ref: "src/pipeline_tests/TestGoldenStore.cpp#test_text_that_cannot_round_trip_is_refused"
        status: pass
    human_judgment: false
  - id: D3
    description: "UPDATE_GOLDENS is restored (or unset) by an RAII guard on every exit of test_golden_directory_untouched, proven after a throw for a set and an unset start state (WR-01)"
    requirement: TEST-03
    verification:
      - kind: unit
        ref: "src/pipeline_tests/TestComparatorSelfTest.cpp#test_update_flag_restored_when_an_assertion_throws"
        status: pass
      - kind: unit
        ref: "src/pipeline_tests/TestComparatorSelfTest.cpp#test_golden_directory_untouched"
        status: pass
    human_judgment: false
  - id: D4
    description: "All earlier store and comparator cases pass and no golden changed"
    requirement: TEST-02
    verification:
      - kind: integration
        ref: "ctest --preset macos-release -R pipeline_tests (100% of 1, 55 s)"
        status: pass
      - kind: other
        ref: "git diff --quiet a796c2ae -- src/pipeline_tests/golden"
        status: pass
    human_judgment: false

duration: 4min
completed: 2026-10-24
status: complete
---

# Phase 2 Plan 13: Golden store update-mode defects Summary

**A failed manifest write now keeps the suite unmarked so every later golden of it fails too (WR-02), `textRoundTrips` makes `checkText` refuse text values that would not parse back and name the key (WR-05), and an RAII `ScopedEnvironmentVariable` restores `UPDATE_GOLDENS` on every exit of the comparator self-test (WR-01), each with a test that was red before the fix.**

## Performance

- **Duration:** 4 min
- **Started:** 2026-10-24T15:12:49Z
- **Completed:** 2026-10-24T15:16:12Z (last task commit; SUMMARY follows)
- **Tasks:** 3
- **Files modified:** 5 (three under `src/test_support`, two test files under `src/pipeline_tests`; no production code, no golden)

## Accomplishments

- WR-02: `SuiteManifest::refresh` no longer inserts the suite into the handled set before the write. It returns true at once for a handled suite, accepts an identical manifest on disk, and otherwise marks the suite only after `saveAtomically` succeeded. A directory standing where `MANIFEST.txt` belongs now fails the first image check, the second image check and the text check of the suite, writes none of them, and the retry after removing the directory writes golden and manifest (the manifest parses back to the current toolchain plus `source_commit`).
- WR-05: `textRoundTrips(values, &offendingKey)` serialises and parses each entry on its own. `checkText` calls it directly after the suite/name validation, so neither update nor normal mode reads a golden, leaves an artifact, touches the manifest or writes a file for a key containing " = " or a line break, an empty key, or a value with a line break. The message names the key and the rule.
- WR-01: `ScopedEnvironmentVariable` (copy deleted) restores the old value or unsets the variable in its destructor. `test_golden_directory_untouched` starts with it, and no direct `qputenv("UPDATE_GOLDENS", ...)` remains in the file (the two `qputenv` calls left are inside the guard). `test_update_flag_restored_when_an_assertion_throws` throws `std::runtime_error` out of an inner guard and checks the value is back to "0" and then, from an unset start, unset again.
- Full run: `ctest --preset macos-release -R pipeline_tests` passes; `git status --porcelain -- src/pipeline_tests/golden` is empty and `git diff --quiet a796c2ae -- src/pipeline_tests/golden` exits 0.

## TDD Gate Compliance

All three tasks carry `tdd="true"`; the plan type is `execute`, so `gsd_run check tdd-red-evidence` was not used (the RED reports are Boost.Test console output, which the classifier has no adapter for). Semantic assessment of each RED: the target test executed and failed on the planned assertion, not on a build or fixture fault.

- **WR-02 RED** (`7cba3f91`, test only; run alone, exit 201): the first check passed its assertions (the directory obstacle does make `QSaveFile` refuse the path on this Mac, so the read-only-directory fallback was not needed) and the test failed at the second check:
  `TestGoldenStore.cpp:449: error: ... second check: recorded .../suite_a/page_two.png`, then `TestGoldenStore.cpp:458: ... page_two.png was written without a manifest` and `decisions.txt was written without a manifest`.
  GREEN `73a18e3c`.
- **WR-05 RED** (`009f6688`, test only; exit 201, 8 failures in the case): update mode `recorded .../suite_a/decisions.txt` (line 482/483) and recorded the multi-line `note` value (485); `update mode wrote something for refused values` (487); `artifact folder left behind` (488); the normal store reported `golden mismatch for suite_a/decisions` and `golden suite_a/notes cannot be parsed` instead of the refusal (493, 495) and `normal mode left an artifact for refused values` (497). GREEN `62dbc761`, which also adds the direct `textRoundTrips` assertions to the case.
- **WR-01:** the guard class and its test cannot be committed apart (the test does not compile without the class), so they enter history in one `fix` commit (`6f625254`), as the plan's single commit for this task has it. To still show the test can fail, the guard destructor was emptied temporarily (not committed): the run then failed four checks (exit 201): `qgetenv("UPDATE_GOLDENS") == "0" has failed` (line 511), `!GoldenStore::updateRequestedFromEnvironment() has failed` (512, 517) and `the variable was left set after the throw` (516). The file was restored from a copy and rebuilt before the real runs.
- Ordering check of the plan (`git log -S` test commit precedes and is separate from the commit that last touched the fixed file, and the test commit touches nothing under `src/test_support`) exits 0 for both WR-02 and WR-05.
- The tracer gate of Task 1 was passed by re-running its `<verify>` end to end (all three automated checks exit 0) before the expansion tasks started.

## Task Commits

1. **Task 1 (tracer): failed manifest write fails every golden of the suite (WR-02)**
   - `7cba3f91` test: failed manifest write must fail every golden of the suite
   - `73a18e3c` fix: mark a suite's manifest handled only after it is written
2. **Task 2: text goldens refused unless they parse back (WR-05)**
   - `009f6688` test: unserialisable text values must be refused
   - `62dbc761` fix: refuse text goldens that would not parse back
3. **Task 3: UPDATE_GOLDENS restored by an RAII guard (WR-01)**
   - `6f625254` fix: restore UPDATE_GOLDENS with an RAII guard in the comparator self-test

**Plan metadata:** the docs commit following this summary.

## Files Created/Modified

- `src/test_support/GoldenStore.cpp` - `SuiteManifest::refresh` marks the suite handled after the write; `checkText` calls `textRoundTrips` and refuses with a message naming the key
- `src/test_support/TextGolden.h` - `textRoundTrips` declaration; the key/value precondition comment now states that `checkText` enforces it
- `src/test_support/TextGolden.cpp` - `textRoundTrips`, entry by entry
- `src/pipeline_tests/TestGoldenStore.cpp` - `test_failed_manifest_write_fails_every_golden_of_the_suite`, `test_text_that_cannot_round_trip_is_refused`
- `src/pipeline_tests/TestComparatorSelfTest.cpp` - `ScopedEnvironmentVariable`, `test_update_flag_restored_when_an_assertion_throws`, guard in `test_golden_directory_untouched`

## Decisions Made

- `textRoundTrips` checks each entry as a one-entry map, so the named key is the real offender even when other keys are fine; an embedded line "x = y" inside a value is caught because the parse yields two entries.
- The refusal sits before the golden is read, so a refused value never produces an artifact folder, in either mode. An empty key is refused too (the parser rejects it), which the header comment now says.
- The obstacle for WR-02 is a directory at the manifest path; the plan's read-only-directory fallback proved unnecessary.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None. The `GoldenStoreTestSuite` log shows one Boost "error" line from `test_expected_failure_trip_wire`; that is the intended expected failure of the existing trip-wire case and the run reports `*** No errors detected`.

## Known Stubs

None.

## Threat Flags

None. T-02-33, T-02-34 and T-02-35 are mitigated as planned; the change adds no network, auth or file access surface beyond the store's own temporary directories.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 02-14 can proceed; `GoldenStore` behaviour for passing checks, identical manifests and normal-mode comparisons is unchanged, and no golden changed.
- Callers of `checkText` must keep keys free of " = " and line breaks, and non-empty; a violation is now a failed check with a message naming the key instead of a golden that cannot be parsed later.

## Self-Check: PASSED

- `src/test_support/GoldenStore.cpp`, `src/test_support/TextGolden.h`, `src/test_support/TextGolden.cpp`, `src/pipeline_tests/TestGoldenStore.cpp` and `src/pipeline_tests/TestComparatorSelfTest.cpp` exist and are the only files changed between `plan_head_before` and `plan_head_after`.
- Commits `7cba3f91`, `73a18e3c`, `009f6688`, `62dbc761`, `6f625254` are ancestors of HEAD; `git rev-list --count eb1fc9c2..HEAD` was 5 before this summary.
- All `<verify>` commands of the three tasks exit 0; `xcrun clang-format --dry-run -Werror` is clean on the five touched files; `ctest --preset macos-release -R pipeline_tests` passes; no file under `src/pipeline_tests/golden` changed.
