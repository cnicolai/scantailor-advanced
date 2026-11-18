---
phase: 02-golden-baselines
plan: 12
subsystem: testing
tags: [test-support, page-generator, asan, dpi, gap-closure, G-02-3, CR-01]

requires:
  - phase: 02-golden-baselines
    provides: records/access-approval.md approval for build-tool reads (plan 02-11)
provides:
  - "test_support::dpiSupported(PageKind, int): fit check of the scaled geometry against page and frames"
  - "Guarded raster writes in the page generator (std::logic_error instead of a heap overflow)"
  - "dpi sweep over all four page kinds, green under ASan, red against the unmodified generator"
  - "Pixels pinned at 75, 300 and 600 dpi for every kind"
affects: [02-13, 02-14, phase-03, phase-04]

actuals:
  tokens: 7300
  tasks: 2
  commits: 4
plan_head_before: ad3b239f071c9df28a8a15edbe49dbb2356a5ac2
plan_head_after: a8c498a9497a97c679fc47809eb46693e7ec0248

tech-stack:
  added: []
  patterns:
    - "Public support predicate computed from the rounded geometry, checked by every entry point of the generator"
    - "Pin commit with the old pixels precedes the generator change; the sweep enters history together with the fix"

key-files:
  created:
    - .planning/phases/02-golden-baselines/records/review-fixes.md
  modified:
    - src/test_support/PageGenerator.cpp
    - src/test_support/PageGenerator.h
    - src/pipeline_tests/TestGeneratorDeterminism.cpp

key-decisions:
  - "Fit rule from the scaled geometry (ink box inside page and documented frame, MIXED picture inside page), not a divisibility rule: 150 and 225 dpi are multiples of 75 and still overflow"
  - "Raster guards test coordinates, not QRect::contains: a speck of side 0 at 75 dpi is a null rectangle that writes nothing and must stay allowed"

requirements-completed: [TEST-01]

coverage:
  - id: D1
    description: "The generator never writes outside its raster at any dpi it accepts; dpiSupported refuses every other dpi and all entry points follow it"
    requirement: TEST-01
    verification:
      - kind: unit
        ref: "src/pipeline_tests/TestGeneratorDeterminism.cpp#test_dpi_sweep_gives_valid_or_null_page (Release and macos-asan, no AddressSanitizer line)"
        status: pass
      - kind: unit
        ref: "src/pipeline_tests/TestGeneratorDeterminism.cpp#test_dpi_support_rule"
        status: pass
    human_judgment: false
  - id: D2
    description: "Pixels are unchanged at 75, 300 and 600 dpi for every kind; no golden file changed"
    requirement: TEST-01
    verification:
      - kind: unit
        ref: "build/macos-release/pipeline_tests --run_test=GeneratorDeterminismTestSuite/test_pinned_pages_unchanged (and the four other pin cases, one run)"
        status: pass
      - kind: other
        ref: "git diff --quiet a796c2ae -- src/pipeline_tests/golden"
        status: pass
    human_judgment: false
  - id: D3
    description: "CR-01 evidence on record: rule, counts, sweep table, pinned fingerprints, ASan RED and GREEN"
    requirement: TEST-01
    verification:
      - kind: other
        ref: "grep checks on records/review-fixes.md (section, heap-buffer-overflow, placeGlyph, 5 fingerprints, no home path)"
        status: pass
    human_judgment: false

duration: 12min
completed: 2026-10-24
status: complete
---

# Phase 2 Plan 12: Page generator dpi fit check Summary

**`dpiSupported(PageKind, int)` refuses every dpi whose rounded page geometry does not fit page and frames, the glyph and speck writes throw instead of overflowing, and a dpi sweep over all four page kinds ran red under ASan (heap-buffer-overflow in `placeGlyph`) before the fix and green after it, with the pixels at 75, 300 and 600 dpi unchanged.**

## Performance

- **Duration:** 12 min
- **Started:** 2026-10-24T14:57:24Z
- **Completed:** 2026-10-24T15:09:45Z
- **Tasks:** 2
- **Files modified:** 4 (three source files, one record)

## Accomplishments

- Closed CR-01 (gap G-02-3): the old range check accepted 75..1200, so FLAT 90, SPREAD 90..104 and 150..157, MIXED 90..96 and CURVED 90..97 wrote past the heap buffer. The new rule accepts FLAT 1125, SPREAD 845, MIXED 702 and CURVED 697 of 1126 dpi values; 75 and every multiple of 60 are accepted for all kinds. `test_dpi_support_rule` pins every dpi 75..1200 and passed on the first run against the planning-time reproduction.
- `generatePage`, `renderInkMask`, `mapFlatToImage` and `mapImageToFlat` all follow `dpiSupported`; `placeGlyph` and `placeSpecks` throw `std::logic_error` before an out-of-raster write as a backstop.
- `test_dpi_sweep_gives_valid_or_null_page` covers 75, 90, 100, 150, 200, 225 and 450 dpi for all kinds: RED under `macos-asan` against the unmodified generator (exit 201, `ERROR: AddressSanitizer: heap-buffer-overflow`, stack through `placeGlyph`), GREEN on the final tree (exit 0, `*** No errors detected`, 0 `AddressSanitizer` and 0 `runtime error:` lines).
- Byte identity at 75, 300 and 600 dpi shown case by case: the five pin cases passed in one run, the three `fixtures.txt` goldens equal the baseline a796c2ae, `ctest --preset macos-release -R 'pipeline_tests|imageproc_tests|dewarping_tests'` reports 100% of 3.
- Header contract: `PageGenerator.h` documents the rule at `dpiSupported` and names it in the comments of `PageSpec::dpi`, `generatePage`, `renderInkMask` and both maps.

## TDD Gate Compliance

- RED: `test_dpi_sweep_gives_valid_or_null_page`, run under macos-asan against the unmodified generator. Target test executed and failed on the planned defect (heap-buffer-overflow in `placeGlyph`, FLAT 90 dpi first in sweep order). Per the plan, the RED evidence is the ASan log (`build/scratch/cr01-asan-red.txt`), not a commit: committing the sweep before the fix would make a commit that builds a binary executing the out-of-buffer write. The plan's gate checks that the sweep and the fix are one commit.
- Pin commit first: `8c47d66e` `test(02-12)` (test file only) precedes the fix.
- GREEN: `2a4ad510` `fix(02-12)`, sweep and fix together.
- Follow-up: `eb62abd5` `test(02-12)` adds `test_dpi_support_rule` (passed on first run, since the rule already existed; it pins the rule, it does not drive new behaviour).
- The `classifier` command `gsd_run check tdd-red-evidence` was not used: the plan type is `execute`, and the RED report is a sanitizer log in a format the classifier has no adapter for.

## Task Commits

1. **Task 1 (tracer): pin pixels, sweep under ASan, fit check and guards**
   - `8c47d66e` test: pin generator fingerprints at 75 dpi and of the curved page at 600 dpi
   - `2a4ad510` fix: refuse dpi values whose page geometry does not fit, guard the raster writes, add the dpi sweep
2. **Task 2: contract in the header, the rule pinned, record**
   - `eb62abd5` test: pin the dpi support rule and document the dpi contract
   - `a8c498a9` docs: record CR-01 evidence

**Plan metadata:** the docs commit following this summary.

## Files Created/Modified

- `src/test_support/PageGenerator.cpp` - `dpiInRange`, `layoutFits`, public `dpiSupported`, frame constants, raster guards, entry points on the new rule
- `src/test_support/PageGenerator.h` - `dpiSupported` declaration with the rule, updated dpi contract comments
- `src/pipeline_tests/TestGeneratorDeterminism.cpp` - `test_pinned_pages_unchanged`, `test_dpi_sweep_gives_valid_or_null_page`, `test_dpi_support_rule`
- `.planning/phases/02-golden-baselines/records/review-fixes.md` - CR-01 evidence

## Decisions Made

- Fit rule computed from the scaled geometry; a divisibility rule was rejected (150 and 225 dpi are multiples of 75 and still overflow).
- Raster guards compare coordinates instead of calling `QRect::contains`, because a MIXED speck of side 0 at 75 dpi is a null rectangle and `contains` would reject it, throwing for a page whose pinned fingerprint must not change.
- The sweep test uses an `insidePage` helper with the same coordinate comparison for the same reason.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Maps moved to the new predicate in Task 1 instead of Task 2**
- **Found during:** Task 1, Step C
- **Issue:** Renaming the range check to `dpiInRange` removed `dpiSupported(int)`, which `mapFlatToImage` and `mapImageToFlat` still called; the Release target did not compile. The plan assigns the map change to Task 2.
- **Fix:** Changed both maps to `dpiSupported(spec.kind, spec.dpi)` in the Task 1 fix commit. Task 2 then added only the map checks in the sweep and the header comments.
- **Files modified:** src/test_support/PageGenerator.cpp
- **Verification:** Release build and suite green; map checks of Task 2 pass.
- **Committed in:** 2a4ad510

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** None on behaviour or commit order; the sweep still enters history together with the fit check and guards.

## Issues Encountered

The first Release build after the generator edit failed to compile (the deviation above). The suite run that followed the failed build used the Release binary from before the sweep existed, so no binary containing both the sweep and the unmodified generator was ever run outside ASan.

## Known Stubs

None.

## Threat Flags

None. The change adds no network, auth or file access surface; T-02-30 to T-02-32 are mitigated as planned.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Later plans may add a scan resolution such as 150 dpi to a sweep only after checking `dpiSupported(kind, 150)`: it is false for SPREAD, MIXED and CURVED.
- Plan 02-13 and 02-14 can proceed; no golden changed.

## Self-Check: PASSED

- `src/test_support/PageGenerator.cpp`, `src/test_support/PageGenerator.h`, `src/pipeline_tests/TestGeneratorDeterminism.cpp` and `.planning/phases/02-golden-baselines/records/review-fixes.md` exist.
- Commits `8c47d66e`, `2a4ad510`, `eb62abd5`, `a8c498a9` are ancestors of HEAD; `git rev-list --count ad3b239f..HEAD` was 4 before this summary.
- All `<verify>` commands of both tasks exit 0; `xcrun clang-format --dry-run -Werror` clean on the three source files.
