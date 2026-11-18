---
phase: 01-deletion-free-macos-build
plan: 07
subsystem: infra
tags: [closing-check, macos, arm64, cmake, ctest, git-diff, build-01, build-07]

requires:
  - phase: 01-deletion-free-macos-build
    provides: presets and build settings (01-01, 01-02), test results (01-03), sanitizer findings (01-04), prior-state and DPR probe (01-05), BUILD-03 PASS (01-06)
provides:
  - "records/closing-check.md: one section per success criterion (1 to 5) with commands, output and verdicts, Phase 1 verdict PASS, Phase 2 baseline commit cde6825"
  - "records/src-diff.txt: full git diff 5eaac18 outside .planning/ and .claude/ (4 hunks)"
  - "records/fix-log.md: closing cross-check mapping all six code/build commits to rows 1 to 6"
affects: [phase-02-golden-baselines, phase-03-strip, verify-work]

actuals:
  tokens: 5300
  tasks: 2
  commits: 2
plan_head_before: cde682504f6a76179bdd3b5a5138df60d8cb0780
plan_head_after: 9b0aca710644b981b444ddb66ff266345f59e1c7

tech-stack:
  added: []
  patterns:
    - "Closing check from fresh configures (--fresh) and a clean rebuild, compared line by line with the first-build records"
    - "Per-hunk classification of the diff against the release commit (modified / added only / REMOVED ONLY)"

key-files:
  created:
    - .planning/phases/01-deletion-free-macos-build/records/closing-check.md
    - .planning/phases/01-deletion-free-macos-build/records/src-diff.txt
  modified:
    - .planning/phases/01-deletion-free-macos-build/records/fix-log.md

key-decisions:
  - "Phase 1 verdict PASS: all five criteria PASS on commit cde6825; the one failing test (imageproc_tests ScaleTestSuite/test_random_image) is the recorded baseline, identical to the first build"
  - "Phase 2 baseline commit is cde6825; its src/, CMakeLists.txt and CMakePresets.json equal those of 11bc573, the last commit touching code or build files"
  - "Criterion 5 treats -ffp-contract=off (fix-log row 4) as the user-decided build setting D-03, the only token that changes generated code, not as an unrequested processing change"

patterns-established:
  - "A record that closes a phase pins the commit and names which later commits touch only .planning/"

requirements-completed: [BUILD-01, BUILD-02, BUILD-03, BUILD-04, BUILD-05, BUILD-06, BUILD-07]

coverage:
  - id: D1
    description: "macos-release configures and builds from a fresh tree; binary is arm64; -Wall and -Wextra on 408 of 408 compile commands; exactly one libjpeg, libpng and libtiff in the load commands and at runtime"
    requirement: "BUILD-01"
    verification:
      - kind: other
        ref: "cmake --preset macos-release --fresh && cmake --build --preset macos-release --clean-first; file; otool -L | grep -c; DYLD_PRINT_LIBRARIES=1 core_tests --list_content; jq on compile_commands.json"
        status: pass
    human_judgment: false
  - id: D2
    description: "BUILD-03 smoke verdict PASS is carried into the closing record"
    requirement: "BUILD-03"
    verification: []
    human_judgment: true
    rationale: "The end-to-end walkthrough was driven by the user in the GUI (plan 01-06); this plan only quotes its verdict line"
  - id: D3
    description: "ctest on the final tree reproduces the first build's per-test results; first-build records tracked in git"
    requirement: "BUILD-04"
    verification:
      - kind: other
        ref: "ctest --preset macos-release; git ls-files records/ctest-release.txt records/ctest-debug.txt records/ctest-summary.md"
        status: pass
    human_judgment: false
  - id: D4
    description: "-ffp-contract=off in every compile command of all three presets, SANITIZE option present, sanitizer findings recorded and tracked"
    requirement: "BUILD-07"
    verification:
      - kind: other
        ref: "jq over build/macos-{release,debug,asan}/compile_commands.json; grep -n 'set(SANITIZE' CMakeLists.txt"
        status: pass
    human_judgment: false
  - id: D5
    description: "git diff 5eaac18 outside .planning/ and .claude/ lists only CMakeLists.txt and CMakePresets.json; four hunks, none remove-only; every code/build commit has a fix-log row"
    requirement: "BUILD-05"
    verification:
      - kind: other
        ref: "git diff --stat/--numstat 5eaac18; for c in $(git log --format=%h 5eaac18..HEAD -- . ':!.planning' ':!.claude'); grep -q $c records/fix-log.md"
        status: pass
    human_judgment: false

duration: 5 min
completed: 2026-10-23
status: complete
---

# Phase 1 Plan 07: Closing Check Summary

**All five Phase 1 success criteria re-checked from fresh Clang/Ninja trees at cde6825: arm64 with one libjpeg/libpng/libtiff, `-ffp-contract=off` on 408 of 408 compile commands in all three presets, ctest identical to the first build, and a diff against 5eaac18 with four hunks (one replaced CMake line, none remove-only); Phase 1 verdict PASS.**

## Performance

- **Duration:** 5 min (the release rebuild ran in the background)
- **Started:** 2026-10-23T13:20:00Z (approx.)
- **Completed:** 2026-10-23T13:27:00Z (approx.)
- **Tasks:** 2
- **Files modified:** 3 (two created, one extended)

## Accomplishments

- Criterion 1: `cmake --preset macos-release --fresh` and `--clean-first` rebuild (456 of 456 steps, exit 0, no `error:`); `file` reports `Mach-O 64-bit executable arm64`; `otool -L` and `DYLD_PRINT_LIBRARIES` both give 1 for each of libjpeg, libpng and libtiff, with the same dyld UUIDs as `binary-check.txt`.
- Criterion 3: `ctest --preset macos-release` gives `80% tests passed, 1 tests failed out of 5`; per-test status and case counts (55, 22, 32, 93 of 94, 8 + 7) equal the first build; no differences. The failure is `ScaleTestSuite/test_random_image` at `TestScale.cpp:68` and `:70`, the known baseline.
- Criterion 4: `-Wall`, `-Wextra` and `-ffp-contract=off` in 408 of 408 commands of each of `macos-release`, `macos-debug` and `macos-asan` after fresh configures; sanitizer flags only in the ASan tree; `SANITIZE` option at `CMakeLists.txt:72`; findings (0 ASan, 3 UBSan at two project locations) quoted from `sanitizer-findings.md`.
- Criterion 5: only `CMakeLists.txt` (modified) and `CMakePresets.json` (new) differ outside `.planning/` and `.claude/`; four hunks (`modified`, `added only` x3); the single removed line is `cmake_minimum_required(VERSION 3.9 ...)`; `src/` is identical to 5eaac18. All six code/build commits have fix-log rows.
- Criterion 2: the `PASS` verdict of `SMOKE-FINDINGS.md` (steps 1 to 11 plus re-check R1 to R5) is quoted.
- Phase 2 baseline commit named: `cde6825`.

## Task Commits

1. **Task 1: Fresh-tree checks for criteria 1, 3 and 4** - `a34313b` (docs)
2. **Task 2: Criterion 5 diff audit, fix-log completion, criterion 2 and the phase verdict** - `9b0aca7` (docs)

**Plan metadata:** recorded in the following docs commit (SUMMARY, STATE, ROADMAP, REQUIREMENTS).

## Files Created/Modified

- `.planning/phases/01-deletion-free-macos-build/records/closing-check.md` - one section per criterion, command output, verdicts, phase verdict, baseline commit
- `.planning/phases/01-deletion-free-macos-build/records/src-diff.txt` - full diff against 5eaac18 outside `.planning/` and `.claude/`
- `.planning/phases/01-deletion-free-macos-build/records/fix-log.md` - closing cross-check section with the commit-to-row table

## Decisions Made

- Phase 1 verdict PASS. The failing `imageproc_tests` case is a recorded baseline of the unmodified code (first build, Debug, ASan), not a gap in criterion 3.
- Phase 2 baseline commit is `cde6825`; commits after `11bc573` touch only `.planning/`.
- `-ffp-contract=off` (fix-log row 4) is counted as a build setting taken by the user (D-03), the one token that changes generated code, and is stated as such in criterion 5 rather than hidden under "no processing change".

## Deviations from Plan

None - plan executed exactly as written. The fix-log needed no new rows (all six commits already had one); a closing cross-check section was added to it instead.

## Issues Encountered

- Homebrew `qtbase` is now `6.12.0_1`, recorded earlier as `6.12.0`. A formula revision of the same Qt release; no test, library or flag result changed. Recorded in `closing-check.md`, not acted on.
- The shell is zsh, where a bare `=====` echo is parsed as a command; one of the first listing commands printed an error line. It only affected output and was re-run without it.

## Known Stubs

None. This plan adds records only.

## Threat Flags

None. No network, auth, file-access or schema surface was added.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 1 is complete on the evidence of all seven plans. Phase 2 (golden baselines) must record its baselines from `cde6825` (or any later commit with the same `src/`, `CMakeLists.txt` and `CMakePresets.json`), with `-ffp-contract=off` fixed.
- Known baseline defects for Phase 2 to mark as expected failures: `ScaleTestSuite/test_random_image` (`TestScale.cpp:68`, `:70`); UBSan reports at `BinaryImage.cpp:1044:37` and `SlicedHistogram.cpp:49:47`.

## Self-Check: PASSED

- `closing-check.md`, `src-diff.txt`, `fix-log.md`, `01-07-SUMMARY.md` exist on disk.
- Commits `a34313b` and `9b0aca7` are ancestors of HEAD.
- Task 1 and Task 2 `<verify>` commands re-run: arm64 file line, one of each image library, 408/408 flags in all three trees, five `verdict:` lines, `all-commits-logged`, only CMake files differ, `Phase 1 verdict: PASS` printed.

---
*Phase: 01-deletion-free-macos-build*
*Completed: 2026-10-23*
