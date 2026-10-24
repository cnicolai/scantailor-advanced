---
phase: 03-qt-6-and-macos-only
plan: 08
subsystem: testing
tags: [asan, ubsan, sanitizer, leftover-grep, gate-log, d-14, d-11]

requires:
  - phase: 03-qt-6-and-macos-only
    provides: the strip commits c00 to c11 (plans 03-01 to 03-07), gate-log.md, fresh-clone.md, ui-screenshots.md
  - phase: 02-golden-baselines
    provides: sanitizer-findings.md (0 ASan, 12 UBSan at 9 project locations) as the D-14 reference
provides:
  - records/test-cases-asan.txt (raw output of the seven executables under ASan+UBSan on cf4da61c)
  - records/sanitizer-compare.md (D-14 comparison, Verdict PASS, no new and no missing location)
  - records/leftover-grep.md (final sweeps for ROADMAP criteria 1 to 4 and D-11, all PASS)
affects: [03-09]

actuals:
  tokens: 73200
  tasks: 2
  commits: 2
plan_head_before: 6a03388b99fae0dd4f36acec1f21550a78c40243
plan_head_after: 2cecffabf7c5038884605b8445c200ac347663d4

tech-stack:
  added: []
  patterns:
    - "D-14: sanitizer state compared by running the executables directly and parsing runtime-error lines by script against a fixed location list"

key-files:
  created:
    - .planning/phases/03-qt-6-and-macos-only/records/test-cases-asan.txt
    - .planning/phases/03-qt-6-and-macos-only/records/sanitizer-compare.md
    - .planning/phases/03-qt-6-and-macos-only/records/leftover-grep.md
  modified: []

key-decisions:
  - "Each executable ran with the environment its ctest entry sets (offscreen Qt, LC_ALL, TMPDIR inside the build tree), so settings stay in the build tree as in Phase 2"
  - "The packaging search uses grep -F so that 'install(' is read literally under ugrep; patterns and files are those of plan 03-03"

requirements-completed: [STRIP-01, STRIP-02, STRIP-03, STRIP-04, STRIP-05, STRIP-06]

coverage:
  - id: D1
    description: "ASan+UBSan run of all seven executables on the last strip commit: exit 0 each, 0 ASan and 12 UBSan reports at the same 9 project locations as Phase 2, new locations 0, missing 0"
    verification:
      - kind: other
        ref: "python3 -I build/scratch/asan/parse.py on records/test-cases-asan.txt; records/sanitizer-compare.md ends with Verdict: PASS"
        status: pass
    human_judgment: false
  - id: D2
    description: "The five files that carry the known reports are unchanged since a796c2ae and no source or build file was edited in this plan"
    verification:
      - kind: other
        ref: "git diff --quiet a796c2ae -- <five files> exits 0; git status --porcelain over src, CMakeLists.txt, CMakePresets.json, cmake is empty"
        status: pass
    human_judgment: false
  - id: D3
    description: "Final sweeps for ROADMAP criteria 1 to 4 are empty or as expected on the last strip commit (Qt5/QT_VERSION, platform and translation words, packaging files and search, .github, src/translations, 249 tr() calls, 573 .ui strings, two files in cmake/)"
    requirement: STRIP-01
    verification:
      - kind: other
        ref: "records/leftover-grep.md checks 1 to 8 and 11; Result table PASS for criteria 1 to 4"
        status: pass
    human_judgment: false
  - id: D4
    description: "Every Phase 3 commit that changed a path outside .planning/ and .claude/ (twelve, c00 to c11) has a gate-log row with two ctest summaries; goldens equal a796c2ae and no commit touched them since"
    verification:
      - kind: other
        ref: "records/leftover-grep.md checks 9a, 9b and 10; Result row D-11 PASS"
        status: pass
    human_judgment: false

duration: 14min
completed: 2026-10-24
status: complete
---

# Phase 3 Plan 08: Sanitizer comparison and final sweeps Summary

**After the strip, ASan+UBSan on all seven executables report exactly what Phase 2 reported (0 ASan, 12 UBSan, the same 9 project locations, every exit status 0), and the final leftover sweeps on the last strip commit are empty or as expected, with all twelve strip commits gated and the goldens untouched**

## Performance

- **Duration:** about 14 min (ASan configure and build about 3 min, the direct run of the seven executables about 10 min)
- **Started:** 2026-10-24T21:20:31Z
- **Completed:** 2026-10-24T21:34:29Z
- **Tasks:** 2
- **Files modified:** 3 new records (no source file)

## Accomplishments

- D-14: `macos-asan` rebuilt on `cf4da61c` (201 steps) and the seven executables started directly. All exit 0. Parse by script: 0 ASan, 12 UBSan (`pipeline_tests` 8, `dewarping_tests` 1, `imageproc_tests` 2, `core_tests` 1, the other three 0), 9 distinct project locations, all in the Phase 2 list. `New project locations: 0`, `Missing Phase 2 locations: 0`, `Verdict: PASS`. Nothing fixed; the five files with the reports have no commit since `a796c2ae`.
- `pipeline_tests` now runs 93 test cases against 86 in Phase 2; the seven extra come from Phase 2 plans 02-13 and 02-14 (written after the Phase 2 sanitizer run) and add no location.
- Final sweeps on the last strip commit: criterion 1 and 2 searches, packaging search, the two translation searches print nothing; packaging files, `.github` and `src/translations` are gone; `cmake/` holds exactly `ListItemsPrepend.cmake` and `SetDefaultBuildType.cmake`; 249 `tr()`/`translate()` calls and 573 `.ui` strings; Clang and SANITIZE blocks equal `a796c2ae`; the alarm command line (D-04) and the `RelinkablePath` backslash and drive-letter code (D-03) are still there; both `ctest -N` presets say `Total Tests: 7`.
- D-11 completeness: the twelve strip commits `8d0105ab` to `cf4da61c` (c00 to c11) each have a gate-log row with `100% tests passed out of 7` for Release and Debug; `a796c2ae` is an ancestor of the oldest; `src/pipeline_tests/golden` equals `a796c2ae` and has no later commit.

## Task Commits

1. **Task 1: ASan+UBSan run compared with Phase 2** - `f346b5e4` (docs)
2. **Task 2: final leftover sweeps and gate-log completeness** - `2cecffab` (docs)

**Plan metadata:** added by the final docs commit of this plan.

## Files Created/Modified

- `.planning/phases/03-qt-6-and-macos-only/records/test-cases-asan.txt` - merged output of the seven executables, normalised
- `.planning/phases/03-qt-6-and-macos-only/records/sanitizer-compare.md` - D-14 comparison
- `.planning/phases/03-qt-6-and-macos-only/records/leftover-grep.md` - sweeps for criteria 1 to 4 and D-11

## Decisions Made

- Executables ran with their ctest environment (including `TMPDIR` inside `build/macos-asan/test-tmp`) rather than the bare Phase 2 command line, so that no settings or temporary file can leave the build tree. The recorded `$` line of each section shows the exact environment.
- Check 3b of the sweeps uses `grep -F` so that `install(` is read as text; the patterns and files are unchanged from plan 03-03.

## Deviations from Plan

None - plan executed exactly as written. (Only a typo in my own scratch script for check 6e was corrected before the final run; it is not a plan deviation.)

## Issues Encountered

- The `build/macos-asan` directory still holds seven stale `.qm` files from the pre-strip build; the reconfigured build produces none. They are untracked build output and do not affect any record (the clean-directory proof is `records/fresh-clone.md`).

## Known Stubs

None.

## Threat Flags

None. T-03-18 mitigated (executables run directly, counts by script from the committed raw output, the five reporting files proven unchanged). T-03-19 mitigated (`grep -c "$HOME"` prints 0 for all three records; no ANSI codes). T-03-SC: no package installed, upgraded or removed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 03-09 can start. The code part of the phase is closed: the sanitizer state equals Phase 2 and the final tree is free of everything the phase removes.
- Scratch files are in `build/scratch/` (ignored by git).

## Self-Check: PASSED

- `records/test-cases-asan.txt`, `records/sanitizer-compare.md`, `records/leftover-grep.md` exist; `f346b5e4` and `2cecffab` are ancestors of HEAD (`git rev-list --count 6a03388b..HEAD` = 2 before this SUMMARY).
- All automated checks of both tasks exit 0; `git status --porcelain` over the records directory is empty.

---
*Phase: 03-qt-6-and-macos-only*
*Completed: 2026-10-24*
