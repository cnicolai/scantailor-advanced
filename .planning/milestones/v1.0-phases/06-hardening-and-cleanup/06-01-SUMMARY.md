---
phase: 06-hardening-and-cleanup
plan: 01
subsystem: build-hygiene
tags: [cpp, preprocessor, dead-code, gate, records]

requires:
  - phase: 03-qt-6-and-macos-only
    provides: gate.sh (per-commit gate, build-command diff)
provides:
  - "No #if 0 block under src/ (CLEAN-03)"
  - "records/pp_equiv.py: preprocessor-equivalence proof for edits that must not change object code"
  - "records/gate-log.md, records/output-path-allowlist.txt and records/clean-03.md for the rest of Phase 6"
affects: [06-02, 06-03, 06-04, 06-05, 06-06, 06-07, 06-08, 06-09, 06-10, 06-11, 06-12, 06-13, 06-14, 06-15, 06-16]

actuals:
  tokens: 4900
  tasks: 1
  commits: 2

plan_head_before: e26b3da973630bae7c44899daed89452a214079c
plan_head_after: 0268c18357e9c962b3056e0d4f0e4b5ceeed641e

tech-stack:
  added: []
  patterns:
    - "Preprocessor equivalence (Release compile command with -E -P, base revision against working file) as the proof for edits that must not change object code"

key-files:
  created:
    - .planning/phases/06-hardening-and-cleanup/records/pp_equiv.py
    - .planning/phases/06-hardening-and-cleanup/records/gate-log.md
    - .planning/phases/06-hardening-and-cleanup/records/output-path-allowlist.txt
    - .planning/phases/06-hardening-and-cleanup/records/clean-03.md
  modified:
    - src/core/IncompleteThumbnail.cpp
    - src/core/filters/output/DewarpingView.cpp
    - src/core/filters/page_split/PageLayoutEstimator.cpp
    - src/core/filters/page_split/VertLineFinder.cpp
    - src/imageproc/SavGolFilter.cpp
    - src/dewarping/Curve.cpp
    - src/dewarping/DistortionModelBuilder.cpp

key-decisions:
  - "SavGolFilter.cpp gets no extra comment line: the kept #else body already opens with 'Take advantage of Savitzky-Golay filter being separable.', which names the replaced alternative"
  - "Curve.cpp and DistortionModelBuilder.cpp get no comment: their dead blocks were an old variant of a test and an unused helper, not documented algorithms"

requirements-completed: [CLEAN-03]

coverage:
  - id: D1
    description: "All eight dead #if 0 blocks are removed from seven files; the five live #else branches stay as plain code"
    requirement: CLEAN-03
    verification:
      - kind: other
        ref: "git grep -n -E '^[[:space:]]*#[[:space:]]*if[[:space:]]+0' -- src prints nothing (8 lines at e5319cd9)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The Release preprocessor output of all seven files is identical before and after, so the object code cannot change"
    requirement: CLEAN-03
    verification:
      - kind: other
        ref: "python3 -I .planning/phases/06-hardening-and-cleanup/records/pp_equiv.py e5319cd9 <seven files>: 7 EQUAL, 0 DIFFERENT, with a positive control"
        status: pass
    human_judgment: false
  - id: D3
    description: "The commit passes the gate: 100% tests passed out of 7 in Release and Debug, goldens unchanged, empty command diff"
    requirement: CLEAN-03
    verification:
      - kind: other
        ref: "bash .planning/phases/03-qt-6-and-macos-only/gate.sh check s01-if0 -> result: PASS"
        status: pass
    human_judgment: false

duration: 30min
completed: 2026-11-16
status: complete
---

# Phase 6 Plan 01: Dead code removal and Phase 6 conventions Summary

**All eight dead `#if 0` blocks are gone from seven files, proven by equal Release preprocessor output (new `pp_equiv.py`) and a passing gate, and the gate log, output-path allowlist and CLEAN-03 record that the later Phase 6 plans use now exist.**

## Performance

- **Duration:** about 30 min (the gate run took about 8 min of it)
- **Completed:** 2026-11-16
- **Tasks:** 1 (tracer)
- **Files modified:** 7 source files, 4 records

## Accomplishments

- Removed the three dead-only blocks (`DewarpingView.cpp`, `Curve.cpp`, `DistortionModelBuilder.cpp`) and the five two-branch blocks (`IncompleteThumbnail.cpp`, `SavGolFilter.cpp`, `PageLayoutEstimator.cpp`, `VertLineFinder.cpp` twice), keeping every live `#else` body as plain code. `git grep` for an `#if 0` opener under `src/` prints nothing; it found 8 lines at `e5319cd9`.
- Wrote `records/pp_equiv.py`. It runs the real Release compile command with `-E -P` on the base revision and the working file and compares them. Result: 7 `EQUAL`, 0 `DIFFERENT`. Positive control on `src/dewarping/Curve.cpp`: `EQUAL`, exit 0 on the untouched file; `DIFFERENT`, exit 1 with a temporary `static int ppEquivControl = 1;` line (removed again, `git diff --quiet` succeeded).
- Gate `s01-if0` on `dc138113`: Release and Debug `100% tests passed out of 7`, goldens unchanged against a796c2ae, command diff 0 / 0 (empty, as expected for edits only).
- Created the gate log with its first row, the allowlist of 12 CLEAN-03/04 files under the output-generation paths (the convention-13 guard prints nothing), and `records/clean-03.md` with the eight blocks, the proof and the gate result.
- Ran `gate.sh snapshot` first, because no reference existed for build-input key `5cca9111...` (the precondition).

## Task Commits

1. **Task 1 (tracer): remove the eight dead #if 0 blocks** - `dc138113` (refactor), gated as `s01-if0`
2. **Gate record** - `0268c183` (docs): gate log, `pp_equiv.py`, allowlist, `clean-03.md`

## Deviations from Plan

None - plan executed exactly as written. Two small choices inside the plan's freedom: no new comment line in `SavGolFilter.cpp` (the surviving comment already names the alternative) and none for the two dead-only blocks that documented no algorithm. The tracer feedback gate was met by the same `<verify>` checks, which passed (7 EQUAL, gate PASS), so expansion was not blocked.

## Issues Encountered

- `$F` word-splitting differs in zsh: a first `pp_equiv.py` call with a variable holding the file list returned `NOENTRY` for the joined string. Re-run with explicit arguments; no effect on the result.
- `clang-format` left two blank lines after the removed helper in `DistortionModelBuilder.cpp` (Chromium style allows two); reduced to one by hand. `clang-format --dry-run -Werror` is clean on all seven files.

## Known Stubs

None.

## Threat Flags

None - dead-code removal only, no new surface.

## Next Phase Readiness

Plans 06-02 and 06-03 (CLEAN-04, unused `boost/foreach.hpp` includes) can start. They append rows to `records/gate-log.md` and add any direct include they need to `records/output-path-allowlist.txt` in the same commit.

## Self-Check: PASSED

- Created files exist: `pp_equiv.py`, `gate-log.md`, `output-path-allowlist.txt`, `clean-03.md` under `.planning/phases/06-hardening-and-cleanup/records/` (FOUND).
- Commits `dc138113` and `0268c183` are ancestors of HEAD (FOUND).
- Acceptance criteria re-run: no `#if 0` under `src` (pass); 7 `EQUAL` / 0 `DIFFERENT` (pass); kept-body greps print 1, 1, 1 and the dead `removeDarkVertBorders(gray100)` prints 0 (pass); `summary.txt` has `result: PASS`, `goldens: unchanged`, both command diffs empty, tested commit is `dc138113` (pass); gate-log row `| s01-if0 |` present (pass); `clean-03.md` present, names `pp_equiv` and `SavGolFilter.cpp`, no home path (pass); clang-format clean on all seven files and the output-path guard prints nothing (pass).
