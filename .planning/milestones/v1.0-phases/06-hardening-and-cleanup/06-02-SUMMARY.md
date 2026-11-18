---
phase: 06-hardening-and-cleanup
plan: 02
subsystem: build-hygiene
tags: [cpp, includes, boost, gate, records]

requires:
  - phase: 06-hardening-and-cleanup
    provides: "gate-log.md, output-path-allowlist.txt and the gate conventions (plan 06-01)"
provides:
  - "No file under src/core, src/foundation or src/math includes the Boost foreach header (first half of CLEAN-04)"
  - "Gate-log row s02a-foreach"
affects: [06-03]

actuals:
  tokens: 120
  tasks: 1
  commits: 2

plan_head_before: 60da8f10bf05e460860622ad8ad5dfc8eedb4e55
plan_head_after: e9a1cb0665dc57a11a33cb295e2d40a1aa58904a

tech-stack:
  added: []
  patterns:
    - "Include removal proven by a full Release and Debug build plus gate.sh (empty command diff), no direct include needed"

key-files:
  created: []
  modified:
    - src/foundation/FastQueue.h
    - src/math/XSpline.cpp
    - src/math/spfit/ConstraintSet.cpp
    - src/math/spfit/Optimizer.cpp
    - src/math/spfit/SplineFitter.cpp
    - src/core/FileNameDisambiguator.cpp
    - src/core/ProjectPages.cpp
    - src/core/ThumbnailPixmapCache.cpp
    - src/core/filters/page_layout/Settings.cpp
    - src/core/filters/page_split/ImageView.cpp
    - src/core/filters/page_split/PageLayoutEstimator.cpp
    - src/core/zones/EditableZoneSet.h
    - src/core/zones/SerializableSpline.cpp
    - .planning/phases/06-hardening-and-cleanup/records/gate-log.md

key-decisions:
  - "No direct include was needed: the foreach header supplied nothing the 13 files (or the files that include FastQueue.h and EditableZoneSet.h) use, so the output-path allowlist is unchanged"

requirements-completed: [CLEAN-04]

coverage:
  - id: D1
    description: "The Boost foreach include is gone from all 13 files under src/core, src/foundation and src/math; the 13 files of plan 06-03 keep theirs"
    requirement: CLEAN-04
    verification:
      - kind: other
        ref: "git grep -l 'boost/foreach' -- src/core src/foundation src/math prints nothing (13 files at e5319cd9, positive control passes); git grep for src/app src/dewarping src/imageproc still lists 13 files"
        status: pass
    human_judgment: false
  - id: D2
    description: "The commit passes the gate: 100% tests passed out of 7 in Release and Debug, goldens unchanged, empty command diff"
    requirement: CLEAN-04
    verification:
      - kind: other
        ref: "bash .planning/phases/03-qt-6-and-macos-only/gate.sh check s02a-foreach -> result: PASS, tested commit 9d65d428"
        status: pass
    human_judgment: false

duration: 15min
completed: 2026-11-16
status: complete
---

# Phase 6 Plan 02: Unused Boost foreach includes, foundation, math and core Summary

**The unused Boost foreach include is removed from the 13 files under `src/core`, `src/foundation` and `src/math`; both presets build without any replacement include and the gate passes 7 of 7 with unchanged goldens.**

## Performance

- **Duration:** about 15 min (two full builds and the gate run)
- **Completed:** 2026-11-16
- **Tasks:** 1 (tracer)
- **Files modified:** 13 source files, 1 record

## Accomplishments

- Deleted the one `#include <boost/foreach.hpp>` line from each of the 13 files (13 deletions, no other change in the diff).
- Built Release and Debug in full before committing: both exit 0, no compile error, so no file needed a direct include. The two headers that other files include (`src/foundation/FastQueue.h`, `src/core/zones/EditableZoneSet.h`) caused no missing name in any includer.
- Gate `s02a-foreach` on `9d65d428`: Release and Debug `100% tests passed out of 7`, goldens unchanged against a796c2ae, command diff 0 / 0.
- `git grep -l 'boost/foreach' -- src/core src/foundation src/math` prints nothing (13 files at `e5319cd9`); the 13 files under `src/app`, `src/dewarping`, `src/imageproc` for plan 06-03 are untouched. The output-path guard of convention 13 prints nothing; `xcrun clang-format --dry-run -Werror` is clean on all 13 files.

## Task Commits

1. **Task 1 (tracer): remove the unused Boost foreach includes from foundation, math and core** - `9d65d428` (refactor), gated as `s02a-foreach`
2. **Gate record** - `e9a1cb06` (docs): row `s02a-foreach` in `records/gate-log.md`

## Deviations from Plan

None - plan executed exactly as written. No direct include was added, so no file outside the plan was touched and the allowlist did not change. The tracer feedback gate was met by the task `<verify>` checks, which passed.

## Issues Encountered

None.

## Known Stubs

None.

## Threat Flags

None - include removal only, no new surface. T-06-06 mitigated: full Release and Debug build in the gate, the removed header was not restored anywhere.

## Next Phase Readiness

Plan 06-03 removes the remaining 13 foreach includes under `src/imageproc`, `src/dewarping` and `src/app`, and appends row `s02b-foreach`.

## Self-Check: PASSED

- Commits `9d65d428` and `e9a1cb06` are ancestors of HEAD (FOUND); gate `summary.txt` has `result: PASS`, `goldens: unchanged`, both command diffs empty, tested commit `9d65d428` equals the commit with the plan's subject (pass).
- Acceptance criteria re-run: no foreach include under the three directories (pass); 13 files still include it under `src/app`, `src/dewarping`, `src/imageproc` (pass); gate-log row `| s02a-foreach |` names "13 includes removed" (pass); output-path guard prints nothing (pass).
