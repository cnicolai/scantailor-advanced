---
phase: 06-hardening-and-cleanup
plan: 13
subsystem: output-stage-split
tags: [cpp, clean-01, output-generator, pure-move, digest-probe, move-check, gate]

requires:
  - phase: 06-hardening-and-cleanup
    provides: "plan 06-01 conventions (gate, labels, output-path guard); plan 06-12 gate-log state"
provides:
  - "src/pipeline_tests/TestOutputDigestProbe.cpp: disabled-by-default probe that prints seven DIGEST lines (SHA-1 of decoded output pixels)"
  - "records/move_check.py: pure-move proof by chunk multiset (reused by plans 06-14 and 06-15)"
  - "OutputGeneratorProcessor.h, OutputGeneratorPixelOps.h, OutputGeneratorNoDewarp.cpp: first split step of OutputGenerator.cpp"
  - "records/clean-01.md: step 1 and the helper placement plan for plan 06-14; records/output-digests.md"
affects: [06-14]

actuals:
  tokens: 14600
  tasks: 2
  commits: 5

plan_head_before: 4998317ab2be5742d8b1c3d11ed158ffaf23fe36
plan_head_after: c3fa2f1a32faaeb90c396af51dd73cff28bec96f

tech-stack:
  added: []
  patterns:
    - "Pure-move proof: chunk multiset (move_check.py) plus decoded-pixel digests plus gate, with a failing positive control for the script"
    - "Shared helpers of a split file live in a header-only internal header inside an anonymous namespace, so each translation unit keeps its own internal copy"

key-files:
  created:
    - src/pipeline_tests/TestOutputDigestProbe.cpp
    - src/core/filters/output/OutputGeneratorProcessor.h
    - src/core/filters/output/OutputGeneratorPixelOps.h
    - src/core/filters/output/OutputGeneratorNoDewarp.cpp
    - .planning/phases/06-hardening-and-cleanup/records/move_check.py
    - .planning/phases/06-hardening-and-cleanup/records/output-digests.md
    - .planning/phases/06-hardening-and-cleanup/records/clean-01.md
  modified:
    - src/core/filters/output/OutputGenerator.cpp
    - src/core/filters/output/CMakeLists.txt
    - .planning/phases/06-hardening-and-cleanup/records/gate-log.md

key-decisions:
  - "Shared helpers go into the header-only OutputGeneratorPixelOps.h in an anonymous namespace (not a .cpp with external linkage), so internal linkage is kept literally and inlining is as before"
  - "The files were produced by a scratch script that copies line ranges of the original, never by retyping code; move_check.py is the committed proof"
  - "Only helpers used by both dewarping paths moved in step 1 (reserveBlackAndWhite, fillExcept, clipContentPolyToRect, fillMarginsInPlace, applyFillZones*, smoothToGrayscale); NoDewarp has no helper of its own; Binarize-only and Dewarp-only helpers stay in OutputGenerator.cpp until plan 06-14"

requirements-completed: []

coverage:
  - id: D1
    description: "An opt-in probe prints seven DIGEST lines (SHA-1 of decoded output pixels) and reproduces the research harness digests exactly"
    requirement: CLEAN-01
    verification:
      - kind: other
        ref: "pipeline_tests --run_test=OutputDigestProbeTestSuite: seven lines equal to build/scratch/phase6/m4/digests-pre-split.txt (diff empty); case carries boost::unit_test::disabled()"
        status: pass
    human_judgment: false
  - id: D2
    description: "move_check.py proves a pure move: PASS on the unchanged file, FAIL on a copy with one character changed in a function body"
    requirement: CLEAN-01
    verification:
      - kind: other
        ref: "records/clean-01.md positive controls (build/scratch/move-control-pass.txt, move-control-fail.txt)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The Processor class, the helpers both dewarping paths need and processWithoutDewarping left OutputGenerator.cpp unchanged; chunk multiset equal (79 against 79), digests identical before and after, goldens unchanged, -ffp-contract=off on both units"
    requirement: CLEAN-01
    verification:
      - kind: other
        ref: "build/scratch/move-s18-verify.txt (RESULT: PASS); build/scratch/digests-s17.txt equals digests-s18.txt; gate.sh check s17-digest-probe (tested f899b433) and s18-split-nodewarp (tested ff354702): result PASS, goldens unchanged"
        status: pass
    human_judgment: false
  - id: D4
    description: "records/clean-01.md hands plan 06-14 the helper-to-user map with a target file for every helper, the two caller-less helpers, the digests of research, s17 and s18, and the gate rows"
    requirement: CLEAN-01
    verification:
      - kind: other
        ref: "Task 2 automated check on records/clean-01.md (REC_OK)"
        status: pass
    human_judgment: false

duration: about 40min
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 13: CLEAN-01 first step, digest probe, move check and the first OutputGenerator split Summary

**An opt-in probe that hashes decoded output pixels and a chunk-multiset move check (with a failing positive control) now prove that moving the `Processor` class, the shared pixel helpers and `processWithoutDewarping` out of `OutputGenerator.cpp` changed nothing: 79 chunks equal, seven digests equal, goldens unchanged, gated in Release and Debug.**

## Performance

- **Duration:** about 40 min of clock time (two gate runs of about 7 min each, one Release and one Debug build of `pipeline_tests`)
- **Completed:** 2026-11-17
- **Tasks:** 2 (tracer, auto)
- **Files:** 7 created, 3 edited; `OutputGenerator.cpp` 2814 to 2041 lines, `OutputGeneratorProcessor.h` 239, `OutputGeneratorPixelOps.h` 443, `OutputGeneratorNoDewarp.cpp` 336

## Accomplishments

- `TestOutputDigestProbe.cpp`: suite `OutputDigestProbeTestSuite`, case `test_print_output_digests` (disabled by default), prints `DIGEST <kind> <dpi> <colour> <dewarping> - <sha1>` for flat 300 bw, flat 600 gray, mixed 300 mixed, mixed 600 mixed, curved 300 gray auto, curved 600 mixed auto and curved 600 bw auto. Its seven lines equal the research harness lines of e0b632f7 exactly.
- `records/move_check.py`: reads the original with `git show`, drops structural lines, cuts column-0 `}` chunks and compares the multisets (`Counter`), with `--allow-changed` and `--allow-line` for later steps. Positive controls: PASS (`chunks: orig=79 new=79`) on the unchanged file; FAIL (`LOST x1` and `ADDED x1` for the constructor chunk, exit 1) when line 82 `QPoint(0, 0)` became `QPoint(0, 1)`.
- Split step 1 by a scratch cut script copying line ranges of the original: `OutputGeneratorProcessor.h` (class verbatim), `OutputGeneratorPixelOps.h` (22 helper chunks both dewarping paths need, anonymous namespace, `clipContentPolyToRect` in its nested one), `OutputGeneratorNoDewarp.cpp` (`processWithoutDewarping`). All three joined `sources`. `xcrun clang-format -i` changed no moved line.
- Proof: `move_check.py` on the four files `RESULT: PASS` (79 against 79 chunks); the digest lines at s17 and s18 identical; `-ffp-contract=off` on both `OutputGenerator*.cpp` compile lines; output-path guard prints nothing; no file deleted.
- Gates: `s17-digest-probe` (tested `f899b433`) and `s18-split-nodewarp` (tested `ff354702`): Release and Debug `100% tests passed out of 7`, goldens unchanged. Command diffs 3 / 3 each: s17 the probe compile line and the `pipeline_tests` link line; s18 the `OutputGeneratorNoDewarp.cpp` compile line and the `liboutput.a` archive line (one member added, none removed, no automoc change).
- `records/clean-01.md` records step 1 with the re-derived helper-to-user map (transitive closure), the target file of every helper for plan 06-14, the two helpers without callers (`calcLocalWindowSize`, `from300dpi`; they move next to `to300dpi`), the move-check output with both controls, the digest table and the gate rows.

## Task Commits

1. **Task 1 (tracer): probe, move check, first split step** - `f899b433` (test, gate `s17-digest-probe`, record `2fc9dfda`) and `ff354702` (refactor, gate `s18-split-nodewarp`, record `94baa2df`)
2. **Task 2: record the first split step and the placement plan** - `c3fa2f1a` (docs)

Tracer feedback gate: the tracer `<verify>` carries only automated checks, so the digest comparison, the move check and the gate evidence were re-run end to end before the record task; they passed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] `OutputGeneratorProcessor.h` carries the two `using namespace` lines**
- **Found during:** Task 1, step 4 (cut)
- **Issue:** the plan lists the header's content without `using namespace imageproc;` and `using namespace dewarping;`, but the class members use `GrayImage`, `BinaryImage`, `DistortionModel` and others unqualified, so the header would not compile on its own.
- **Fix:** the header carries the two lines after its include block, as `OutputGeneratorPixelOps.h` does. They are structural lines for the move check, and no code line changed.
- **Files modified:** `src/core/filters/output/OutputGeneratorProcessor.h`
- **Commit:** `ff354702`

**2. [Plan wording] 79 chunks, not 82**
- **Found during:** Task 1, step 2 (first positive control)
- **Issue:** the plan's 82 counts the three `}  // namespace` closers, which the script drops as structural lines.
- **Fix:** none needed; the controls and the s18 check report 79 against 79, and `clean-01.md` says so.
- **Commit:** `c3fa2f1a`

**3. [Plan wording] `OutputGeneratorNoDewarp.cpp` has no helper of its own**
- **Found during:** Task 1, step 4 (helper map)
- **Issue:** the plan allows "any helper only it needs"; the map shows every helper `processWithoutDewarping` calls is also needed by the dewarping path, so none is private to it.
- **Fix:** the file holds `processWithoutDewarping` only, with no anonymous namespace.
- **Commit:** `ff354702`

**Total deviations:** 3 (1 blocking in the plan's file description, 2 plan wording). **Impact:** none on behaviour or proof.

## Issues Encountered

- The first mutation line number (85) of the failing positive control did not hit the intended text, so the script printed PASS for an unchanged copy; the control was redone on line 82 (the `QPoint(0, 0)` assert), which gives FAIL. The record shows the run that changed a line.
- The scratch cut script imported `move_check` and left a `__pycache__` directory in `records/`; it was removed before any commit.
- The application binary was not launched and no live harness was run.

## Known Stubs

None.

## Threat Flags

None beyond the plan's threat model. T-06-18 (changed body or flag in the split) is mitigated: chunk multiset equal with a failing control, seven digests equal before and after, goldens unchanged, `-ffp-contract=off` on both units. T-06-19 (helpers losing internal linkage) is mitigated: every moved helper is still in an anonymous namespace, in `OutputGeneratorPixelOps.h`.

## Next Phase Readiness

CLEAN-01 stays open: plan 06-14 moves the dewarping and binarization paths and appends step 2 to `records/clean-01.md` using the placement table and `move_check.py`.

## Self-Check: PASSED

- The seven created files exist (pass).
- Commits `f899b433`, `2fc9dfda`, `ff354702`, `94baa2df` and `c3fa2f1a` are ancestors of HEAD; `git rev-list --count` from the recorded base `4998317a` gives 5 (pass).
- Both gate summaries say `result: PASS` and `goldens: unchanged`; the tested commits equal the commits with the plan subjects; both gate-log rows exist (pass).
- `grep -cF 'class OutputGenerator::Processor' OutputGeneratorProcessor.h` prints 1; `grep -cF 'class OutputGenerator::Processor {' OutputGenerator.cpp` prints 0; `move_check.py` ends with `RESULT: PASS`; the digest hashes of s17 and s18 are equal; all touched C++ files pass `clang-format --dry-run -Werror` (pass).
