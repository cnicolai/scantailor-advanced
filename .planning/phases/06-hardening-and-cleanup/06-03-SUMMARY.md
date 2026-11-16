---
phase: 06-hardening-and-cleanup
plan: 03
subsystem: build-hygiene
tags: [cpp, includes, boost, gate, records]

requires:
  - phase: 06-hardening-and-cleanup
    provides: "gate-log.md, output-path-allowlist.txt and the gate conventions (plan 06-01); first half of CLEAN-04 (plan 06-02)"
provides:
  - "No file under src/ includes the Boost foreach header (CLEAN-04 complete)"
  - "Gate-log row s02b-foreach"
affects: [06-04]

actuals:
  tokens: 120
  tasks: 1
  commits: 2

plan_head_before: d8f69c77b342bfad6853f0c4527bd5a2887fd324
plan_head_after: 66dc76812cd413a99ea51153b27cbd5f2bd7f094

tech-stack:
  added: []
  patterns:
    - "Include removal proven by a full Release and Debug build plus gate.sh (empty command diff), no direct include needed"

key-files:
  created: []
  modified:
    - src/imageproc/PolygonRasterizer.cpp
    - src/imageproc/RastLineFinder.cpp
    - src/imageproc/tests/TestRastLineFinder.cpp
    - src/dewarping/CylindricalSurfaceDewarper.cpp
    - src/dewarping/DetectVertContentBounds.cpp
    - src/dewarping/DistortionModelBuilder.cpp
    - src/dewarping/TextLineRefiner.cpp
    - src/dewarping/TextLineTracer.cpp
    - src/dewarping/TopBottomEdgeTracer.cpp
    - src/dewarping/TowardsLineTracer.cpp
    - src/app/FixDpiDialog.cpp
    - src/app/RelinkingModel.cpp
    - src/app/ThumbnailSequence.cpp
    - .planning/phases/06-hardening-and-cleanup/records/gate-log.md

key-decisions:
  - "No direct include was needed: the foreach header supplied nothing the 13 files use, so the output-path allowlist is unchanged"

requirements-completed: [CLEAN-04]

coverage:
  - id: D1
    description: "The Boost foreach include is gone from all 13 files under src/imageproc, src/dewarping and src/app; no file under src/ includes it any more"
    requirement: CLEAN-04
    verification:
      - kind: other
        ref: "git grep -l 'boost/foreach' -- src prints nothing (26 files at e5319cd9, positive control passes)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The commit passes the gate: 100% tests passed out of 7 in Release and Debug, goldens unchanged, empty command diff"
    requirement: CLEAN-04
    verification:
      - kind: other
        ref: "bash .planning/phases/03-qt-6-and-macos-only/gate.sh check s02b-foreach -> result: PASS, tested commit 90d99a27"
        status: pass
    human_judgment: false

duration: 20min
completed: 2026-11-16
status: complete
---

# Phase 6 Plan 03: Unused Boost foreach includes, imageproc, dewarping and app Summary

**The unused Boost foreach include is removed from the last 13 files under `src/imageproc`, `src/dewarping` and `src/app`, so no file under `src/` includes it; both presets build without any replacement include and the gate passes 7 of 7 with unchanged goldens (CLEAN-04 complete).**

## Performance

- **Duration:** about 20 min (two full builds and the gate run)
- **Completed:** 2026-11-16
- **Tasks:** 1 (tracer)
- **Files modified:** 13 source files, 1 record

## Accomplishments

- Deleted the one `#include <boost/foreach.hpp>` line from each of the 13 files (13 deletions, no other change in the diff).
- Built Release and Debug in full before committing: both exit 0 with no `error:`, so no file needed a direct include and the output-path allowlist did not change.
- Gate `s02b-foreach` on `90d99a27`: Release and Debug `100% tests passed out of 7`, goldens unchanged, command diff 0 / 0.
- `git grep -l 'boost/foreach' -- src` prints nothing (26 files at `e5319cd9`). The output-path guard of convention 13 prints nothing. `xcrun clang-format --dry-run -Werror` is clean on the 12 files other than `src/dewarping/TextLineTracer.cpp`, which still reports exactly 2 warnings (it was edited by deleting one line, never reformatted).

## Task Commits

1. **Task 1 (tracer): remove the unused Boost foreach includes from imageproc, dewarping and app** - `90d99a27` (refactor), gated as `s02b-foreach`
2. **Gate record** - `66dc7681` (docs): row `s02b-foreach` in `records/gate-log.md`

## Deviations from Plan

None - plan executed exactly as written. One slip of my own, not a plan deviation: the first deletion loop used an unquoted zsh variable and edited nothing, so the first pair of builds ran on an unchanged tree; I redid the deletion and rebuilt both presets (28 compile steps each) before committing.

## Issues Encountered

None.

## Known Stubs

None.

## Threat Flags

None - include removal only, no new surface. T-06-25 mitigated: full Release and Debug build and ctest with golden comparison in the gate; only allowlisted files under `src/imageproc` and `src/dewarping` differ from `e5319cd9`; the removed header was not restored anywhere.

## Next Phase Readiness

CLEAN-04 is complete. Plan 06-04 (SAFE-01, image size limit) is next.

## Self-Check: PASSED

- Commits `90d99a27` and `66dc7681` are ancestors of HEAD; gate `summary.txt` has `result: PASS`, `goldens: unchanged`, both command diffs 0, tested commit `90d99a27` equals the commit with the plan's subject (pass).
- Acceptance criteria re-run: no foreach include under `src/` (pass, positive control at `e5319cd9` pass); gate-log row `| s02b-foreach |` names "13 includes removed" (pass); output-path guard prints nothing (pass); TextLineTracer.cpp still 2 clang-format warnings (pass).
