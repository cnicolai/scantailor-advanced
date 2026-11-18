---
phase: 06-hardening-and-cleanup
plan: 14
subsystem: output-stage-split
tags: [cpp, clean-01, output-generator, pure-move, digest-probe, move-check, gate]

requires:
  - phase: 06-hardening-and-cleanup
    provides: "plan 06-13: Processor.h, PixelOps.h, NoDewarp.cpp, move_check.py, digest probe, helper placement plan"
provides:
  - "src/core/filters/output/OutputGeneratorDewarp.cpp: processWithDewarping, dewarper set-up, distortion-model methods and their helpers"
  - "src/core/filters/output/OutputGeneratorBinarize.cpp: illumination, binarization, threshold, despeckle, picture-zone, segmentation, posterization methods and their helpers"
  - "OutputGenerator.cpp reduced to 268 lines (set-up and dispatch only)"
  - "records/clean-01.md covering both split steps; s19 lines in records/output-digests.md"
affects: [06-16]

actuals:
  tokens: 23600
  tasks: 2
  commits: 3

plan_head_before: 596699446eefcbbc9b3af7bb382393c573f0bf3a
plan_head_after: 0dfee5c6582cba700731d081974d95d98e3e9f76

tech-stack:
  added: []
  patterns:
    - "Topic-file split by line-range cut script; chunk-multiset move check over all files against the planning base, plus decoded-pixel digests and the gate"
    - "Helpers used by one file sit in an anonymous namespace of that file, helpers used by several in the anonymous namespace of OutputGeneratorPixelOps.h"

key-files:
  created:
    - src/core/filters/output/OutputGeneratorDewarp.cpp
    - src/core/filters/output/OutputGeneratorBinarize.cpp
  modified:
    - src/core/filters/output/OutputGenerator.cpp
    - src/core/filters/output/CMakeLists.txt
    - .planning/phases/06-hardening-and-cleanup/records/clean-01.md
    - .planning/phases/06-hardening-and-cleanup/records/output-digests.md
    - .planning/phases/06-hardening-and-cleanup/records/gate-log.md

key-decisions:
  - "The two caller-less helpers calcLocalWindowSize and from300dpi are kept (D-16) in OutputGeneratorBinarize.cpp, directly before to300dpi"
  - "OutputGenerator.cpp no longer includes OutputGeneratorPixelOps.h because it calls no helper; Dewarp and Binarize include both internal headers"
  - "Placement table of plan 06-13 held unchanged: no helper moved to PixelOps.h in step 2"

requirements-completed: []

coverage:
  - id: D1
    description: "The dewarping and binarization paths left OutputGenerator.cpp as a pure move: chunk multiset of all six files equals the original at e5319cd9 (79 against 79)"
    requirement: CLEAN-01
    verification:
      - kind: other
        ref: "build/scratch/move-s19-verify.txt (RESULT: PASS); control with calcLocalWindowSize dropped gives LOST x1 and RESULT: FAIL"
        status: pass
    human_judgment: false
  - id: D2
    description: "Decoded output pixels are unchanged by the whole split: seven DIGEST lines at s19 equal those at s17"
    requirement: CLEAN-01
    verification:
      - kind: other
        ref: "build/scratch/digests-s17.txt equals build/scratch/digests-s19.txt (records/output-digests.md)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Gate s19-split-rest passes: Release and Debug ctest 100% tests passed out of 7, goldens unchanged, command diff only the two new compile lines and two archive members"
    requirement: CLEAN-01
    verification:
      - kind: other
        ref: "build/scratch/gate/s19-split-rest/summary.txt (tested 4650ec47, result PASS); records/gate-log.md row s19-split-rest"
        status: pass
    human_judgment: false
  - id: D4
    description: "All four OutputGenerator translation units compile with -ffp-contract=off; OutputGenerator.cpp is 268 lines (below 400)"
    requirement: CLEAN-01
    verification:
      - kind: other
        ref: "compile_commands.json check (4 of 4 with the flag); wc -l 268; clang-format --dry-run -Werror clean"
        status: pass
    human_judgment: false
  - id: D5
    description: "records/clean-01.md holds both steps: file list with line counts, helper placement, move-check outputs and controls, digest table with four columns, compile-flag check, gate rows s17 to s19"
    requirement: CLEAN-01
    verification:
      - kind: other
        ref: "Task 2 automated check on records/clean-01.md"
        status: pass
    human_judgment: false

duration: about 25min
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 14: CLEAN-01 second step, the dewarping and binarization paths leave OutputGenerator.cpp Summary

**`OutputGenerator.cpp` (2814 lines at the planning base) is now 268 lines of set-up and dispatch: the dewarping path (851 lines) and the binarization topics (1081 lines) moved verbatim into `OutputGeneratorDewarp.cpp` and `OutputGeneratorBinarize.cpp`, proven by chunk multiset (79 against 79 over six files), seven unchanged output digests and a passing Release and Debug gate.**

## Performance

- **Duration:** about 25 min of clock time (one Release build, one gate run of about 8 min)
- **Completed:** 2026-11-17
- **Tasks:** 2 (tracer, auto)
- **Files:** 2 created, 2 source files edited, 3 records edited

## Accomplishments

- A scratch cut script (`build/scratch/cut_s19.py`) copied the line ranges of the original for `OutputGenerator.cpp` (chunks 1, 2, 4 to 8, 51 to 54), `OutputGeneratorDewarp.cpp` (helpers 38, 39, 42 to 47; methods 56, 60 to 62, 78, 79) and `OutputGeneratorBinarize.cpp` (helpers 9, 10, 19, 25, 26, 35 to 37, 40, 41, 49, 50; methods 57 to 59 and 63 to 77). Every one of the 79 chunks is assigned once. `xcrun clang-format -i` changed no moved line.
- Proof: `move_check.py` over the six files `RESULT: PASS`, 79 against 79 chunks; a control copy of the Binarize file without `calcLocalWindowSize` gives `LOST x1` and `RESULT: FAIL`. The seven digests of the probe equal `digests-s17.txt` line for line.
- Gate `s19-split-rest` (tested `4650ec47`): Release and Debug `100% tests passed out of 7`, goldens unchanged. Command diff 4 / 4 per preset: the two new compile lines (both with `-ffp-contract=off`) and the `liboutput.a` archive line (two members added, none removed, no automoc change). Output-path guard prints nothing.
- Edge probe CLEAN-01 concurrency: the only `static` keywords in the new files are two `static uint8_t transform` member functions; the move adds no static storage, so parallel page processing behaves as before.
- `records/clean-01.md` now records both steps; `records/output-digests.md` holds the s19 lines.

## Task Commits

1. **Task 1 (tracer): the two topic files, move check, digests, gate** - `4650ec47` (refactor, gate `s19-split-rest`) and `d1ffd62e` (docs, gate record)
2. **Task 2: complete records/clean-01.md** - `0dfee5c6` (docs)

Tracer feedback gate: the tracer `<verify>` carries only automated checks, so move check, digest comparison, gate summary, gate-log row, compile-flag check and clang-format were re-run end to end after the gate record commit; they passed, and the record task followed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The gate script needs `bash`**
- **Found during:** Task 1, step 4
- **Issue:** the first run `.planning/phases/03-qt-6-and-macos-only/gate.sh check ...` failed with "permission denied" (the file is not executable; the script's own header says to run it with `bash`). No gate output was written.
- **Fix:** ran it as `bash .planning/phases/03-qt-6-and-macos-only/gate.sh check s19-split-rest` in the Bash tool's background mode.
- **Files modified:** none
- **Commit:** none

**2. [Plan wording] `OutputGenerator.cpp` does not include `OutputGeneratorPixelOps.h`**
- **Found during:** Task 1, step 2
- **Issue:** after the move `OutputGenerator.cpp` calls no helper; the plan did not say which includes it keeps.
- **Fix:** it keeps `OutputGeneratorProcessor.h` only; both new topic files include both internal headers. The build and the move check show no loss.
- **Commit:** `4650ec47`

**Total deviations:** 2 (1 blocking tool invocation, 1 plan wording). **Impact:** none on behaviour or proof.

## Issues Encountered

- A first polling loop used `until [ -f summary.txt ]`; both loops ended when the summary appeared and none outlives this run.
- The application binary was not launched and no live harness was run. Nothing outside the project was read or written.

## Known Stubs

None.

## Threat Flags

None beyond the plan's threat model. T-06-29 (changed body or flag in the second step) is mitigated: chunk multiset equal with a failing control, seven digests equal, goldens unchanged, `-ffp-contract=off` on all four units. T-06-30 (helper linkage or a dropped caller-less helper) is mitigated: all helpers sit in anonymous namespaces, `calcLocalWindowSize` and `from300dpi` are in `OutputGeneratorBinarize.cpp` and the control shows the move check catches a drop.

## Next Phase Readiness

CLEAN-01 is left open by `requirements.ready-ids` because plan 06-16 also declares it; it closes when 06-16 has its SUMMARY. Plan 06-15 (MainWindow split) can reuse `move_check.py`.

## Self-Check: PASSED

- `OutputGeneratorDewarp.cpp` and `OutputGeneratorBinarize.cpp` exist (pass).
- Commits `4650ec47`, `d1ffd62e` and `0dfee5c6` are ancestors of HEAD; `git rev-list --count` from the recorded base `59669944` gives 3 (pass).
- Gate summary `s19-split-rest` says `result: PASS` and `goldens: unchanged`; the tested commit equals the commit with the s19 subject; the gate-log row exists (pass).
- `move_check.py` ends with `RESULT: PASS`; digest sets of s17 and s19 are equal; `wc -l` of `OutputGenerator.cpp` is 268; `calcLocalWindowSize` and `from300dpi` are in `OutputGeneratorBinarize.cpp`; the touched C++ files pass `clang-format --dry-run -Werror` (pass).
