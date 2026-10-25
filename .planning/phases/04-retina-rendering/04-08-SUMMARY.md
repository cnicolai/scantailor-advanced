---
phase: 04-retina-rendering
plan: 08
subsystem: testing
tags: [qt6, hidpi, retina, documentation, traceability, sanitizers, closing-record]

requires:
  - phase: 04-retina-rendering
    provides: "plans 04-01 to 04-07: the Retina code, TestHiDpiQt (26 tests), retina_check, gate-log.md and the live records"
provides:
  - "TESTING.md: two-screen offscreen platform of qt_tests, what TestHiDpiQt covers, section 'Live Retina check'"
  - "REQUIREMENTS.md: TESTX-03 traced to Phase 4 (D-04)"
  - "records/closing-check.md: one verdict per success criterion, requirement mapping, decisions D-00a to D-09, static checks, ASan/UBSan run of qt_tests, gate log; Phase 4 verdict PASS"
affects: [verify-work for phase 04, phase 05, phase 06]

plan_head_before: 5fdec0e6f1e55b95c21498a9082f97b90a1b3c82
plan_head_after: ff5941a8089512dc00e8d784cc7dda6a9e868442

actuals:
  tokens: 4800
  tasks: 2
  commits: 3

tech-stack:
  added: []
  patterns:
    - "Closing record cites every claim to a gate label, a test name or a live record, and lists what measurement cannot decide"

key-files:
  created:
    - .planning/phases/04-retina-rendering/records/closing-check.md
  modified:
    - TESTING.md
    - .planning/REQUIREMENTS.md
    - .planning/phases/04-retina-rendering/records/gate-log.md

key-decisions:
  - "The one UBSan report of the qt_tests run (BinaryImage.cpp:1044, a known Phase 1 and Phase 2 location) is attributed to old code and recorded, not fixed: it is raised by a Phase 4 test helper calling BinaryImage, but no file Phase 4 created or changed holds a report"
  - "Criterion 1 is recorded as measured with the retina_check binary, which has the same Mach-O properties as the app; the app binary itself was not launched because it touches ~/.config and ~/Library"

patterns-established:
  - "A closing record ends with a section of what measurement cannot decide, so the verdict line does not over-claim"

requirements-completed: [HIDPI-01, HIDPI-02, HIDPI-03, HIDPI-04, HIDPI-05, HIDPI-06, TESTX-03]

coverage:
  - id: D1
    description: "TESTING.md documents the two-screen offscreen platform of qt_tests (OneX ratio 1, TwoX ratio 2, a test aid), the Retina tests and the live check retina_check with its exit codes"
    requirement: TESTX-03
    verification:
      - kind: other
        ref: "bash .planning/phases/03-qt-6-and-macos-only/gate.sh check r13-docs (result PASS, goldens unchanged, empty command diff)"
        status: pass
    human_judgment: false
  - id: D2
    description: "TESTX-03 has the traceability row '| TESTX-03 | Phase 4 | Pending |' and the v2 entry says it was brought into Phase 4 by D-04"
    requirement: TESTX-03
    verification:
      - kind: other
        ref: "grep -qF '| TESTX-03 | Phase 4 | Pending |' .planning/REQUIREMENTS.md"
        status: pass
    human_judgment: false
  - id: D3
    description: "Output code and goldens did not change since the Phase 4 base, scale-factor names appear only in the two allowed files, 84 devicePixelRatio uses exist, 7 ctest suites"
    requirement: HIDPI-02
    verification:
      - kind: other
        ref: "records/closing-check.md section 'Static checks' (outputs in build/scratch/closing/)"
        status: pass
    human_judgment: false
  - id: D4
    description: "qt_tests under ASan and UBSan: 0 AddressSanitizer errors, 1 UBSan report in the known BinaryImage.cpp:1044, none in a file Phase 4 created or changed"
    requirement: TESTX-03
    verification:
      - kind: other
        ref: "build/macos-asan/qt_tests (build/scratch/closing/asan-qt-tests.txt), 41 tests passed"
        status: pass
    human_judgment: false
  - id: D5
    description: "Closing record with a verdict and evidence for each of the five success criteria; criterion 4 stated as resting on the offscreen two-screen simulation (D-03)"
    verification:
      - kind: other
        ref: "records/closing-check.md, last line 'Phase 4 verdict: PASS'"
        status: pass
    human_judgment: true
    rationale: "Criterion 4 against real displays (Cocoa delivery of DevicePixelRatioChange, assumption A1) and the perceptual 'as sharp as Preview' comparison cannot be decided by this Mac with one display; the record lists both"

duration: 10min
completed: 2026-10-25
status: complete
---

# Phase 4 Plan 8: Close Phase 4 Summary

**TESTING.md now explains the 1x/2x offscreen test screens and the live `retina_check`, TESTX-03 is traced to Phase 4, the static checks (no output code or golden change, scale-factor names only in the test aid and the harness) and an ASan/UBSan run of `qt_tests` came out clean for Phase 4 files, and `records/closing-check.md` closes the phase with `Phase 4 verdict: PASS`**

## Performance

- **Duration:** 10 min (after reading the plans, summaries and records; the long ASan and gate builds ran in the background)
- **Started:** 2026-10-25T15:27:12Z
- **Completed:** 2026-10-25T15:37Z
- **Tasks:** 2
- **Files modified:** 4 (TESTING.md, REQUIREMENTS.md, gate-log.md, closing-check.md created)

## Accomplishments

- TESTING.md: the sentence about the two Qt suites is replaced by one for `pipeline_tests` (single-screen offscreen) and one for `qt_tests` (screens `OneX` ratio 1 and `TwoX` ratio 2, `offscreen-screens.json`, `QT_SCREEN_SCALE_FACTORS`, a test aid only); the `qt_tests` row lists `TestHiDpiQt`; the new section `## Live Retina check` gives the command `build/macos-release/retina_check build/scratch/retina/all --section all -ApplePersistenceIgnoreState YES`, the sections, the output rule (below `build/`), the cleared variables, `summary.txt` and the exit codes 0 to 3 (checked against `src/app/retina_check/main.cpp`).
- REQUIREMENTS.md: the TESTX-03 bullet notes D-04, the traceability table has `| TESTX-03 | Phase 4 | Pending |` after HIDPI-06, the coverage block has `v2 brought forward: 1`, and the footer is updated.
- Static checks: `git diff --stat` since the Phase 4 base `059f4800` is empty for `src/pipeline_tests/golden`, `src/imageproc`, `src/dewarping`, every filter `Task.cpp` and `OutputGenerator.cpp` (positive control: 33 files changed in `src`), and `git diff a796c2ae -- src/pipeline_tests/golden` is empty. The scale-factor search lists `src/qt_tests/main.cpp` (a `qputenv`, test aid) and `src/app/retina_check/main.cpp` (four `qunsetenv` calls) only. 84 `devicePixelRatio` uses (0 before the phase), `Total Tests: 7`, and 13 gate-log rows each with Release and Debug `100% tests passed out of 7` and unchanged goldens.
- ASan and UBSan run of `qt_tests`: 41 tests passed (8 + 7 + 26), 0 AddressSanitizer errors, 1 UBSan report at `src/imageproc/BinaryImage.cpp:1044:37`, the location known since Phase 1. It is raised through the Phase 4 test helper `makeZoneMask` calling old code; no file Phase 4 created or changed has a report, so no `r14-sanitizer-fix` was needed.
- `records/closing-check.md`: verdict and evidence for the five success criteria (all PASS, criterion 4 by simulation), the seven requirements mapped to plans, tests and records, decisions D-00a to D-09, assumptions A1 and A7, and a closing section of what measurement cannot decide.

## Task Commits

1. **Task 1: document the Retina tests and the live check in TESTING.md** - `28b70784` (docs); gate `r13-docs` PASS (Release and Debug 100% out of 7, goldens unchanged, command diff 0 / 0), gate record `0f57be04`
2. **Task 2: traceability, final static and sanitizer checks, closing record** - `ff5941a8` (docs); `.planning/` only, no gate

**Plan metadata:** the commit that adds this summary and the state and roadmap update.

## Files Created/Modified

- `TESTING.md` - two-screen platform of `qt_tests`, `TestHiDpiQt` row, section `Live Retina check`
- `.planning/REQUIREMENTS.md` - TESTX-03 note, traceability row, coverage line, footer
- `.planning/phases/04-retina-rendering/records/closing-check.md` - the closing record (new)
- `.planning/phases/04-retina-rendering/records/gate-log.md` - row `r13-docs`

## Decisions Made

- The sanitizer report in `BinaryImage.cpp` is old code (Phase 2 rule: recorded, not fixed), although a Phase 4 test is a new caller of it; the record says so in plain words, including that `qt_tests` had no report in the baseline.
- HIDPI-01 is recorded as measured with the harness binary that shares the app's Mach-O properties, because launching the app itself would read and write `~/.config` and `~/Library`.

## Deviations from Plan

None - plan executed exactly as written. The `r14-sanitizer-fix` branch of step 3 was not taken (no Phase 4 file has a report). The REQUIREMENTS.md edit was done with a short script of exact string replacements instead of the Edit tool, with the same result.

## Issues Encountered

- The `macos-asan` build directory was already configured and mostly built, so the `qt_tests` target built in one pass; the build prints the pre-existing duplicate-library linker warning and compiler warnings of old code, none from Phase 4 files that were new in this plan.
- Uncommitted `.planning/config.json`, `.planning/milestone.lock` and `.planning/state.json` predate this plan and were left out of every commit.
- The plan-commit ledger file (`gsd-plan-head-before-04-08`) was written after the first task commit, from that commit's parent `5fdec0e6`; the value is the same as if it had been written before.

## Known Stubs

None.

## Threat Flags

None. This plan changed documentation and records only (T-04-25: criterion 4 is stated as a simulation with A1 and the paint-time check named; T-04-26: output code and goldens diff empty and every gate row unchanged; T-04-27: the sanitizer run found nothing in a Phase 4 file).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 4 is complete and ready for `/gsd-verify-work 04`. What stays with the user: whether thumbnails, page and overlays look as sharp as other macOS apps, and the Cocoa `DevicePixelRatioChange` on a real drag between a 1x and a 2x display (needs a second display).
- Phase 6 (STRIP-07) can remove the OpenGL branch of the colour picker: `ColorPickupInteraction::medianColor` is tested independently of it.
- The sanitizer baseline now has a `qt_tests` entry at the known `BinaryImage.cpp:1044` location (1 report, via `makeZoneMask`).

## Short summary for the user (D-01)

- **Sharp now:** the page view at fit, 100% and 200%; thumbnails (500 px files at the default Quality 250 on this display); the picture-zone mask, page badges, deskew handles, the stage-list animation and list icons; the colour picker samples the right area on a 2x grab. Output files and goldens did not change.
- **Measured live on the Studio Display (ratio 2):** the page view equals a device-resolution reference pixel for pixel at three zooms; thumbnails, mask, badge, handles, stage animation, list decoration and picker all passed in one `retina_check` run. No Info.plist was needed, and the 0.5 mm smoothing switch already counted millimetres correctly (ratio 0.9978).
- **Simulated:** moving between a 1x and a 2x display, using two offscreen screens in `qt_tests`. No 1x display was attached, so Cocoa's own event on a real drag is not observed (assumption A1; every rebuilt buffer also checks its ratio at paint time).
- **Finding:** one UBSan report in the known `BinaryImage.cpp:1044` location now also appears when `qt_tests` runs under the sanitizers (through a Phase 4 test helper); it is old code and left alone.

## Self-Check: PASSED

- Created and modified files exist: `TESTING.md`, `.planning/REQUIREMENTS.md`, `records/closing-check.md`, `records/gate-log.md`.
- Commits `28b70784`, `0f57be04`, `ff5941a8` are ancestors of HEAD.
- `build/scratch/gate/r13-docs/summary.txt` reads `result: PASS`, `goldens: unchanged`, `diff_lines=0` for both presets; `records/closing-check.md` ends with `Phase 4 verdict: PASS` and `grep -c "$HOME"` prints 0.

---
*Phase: 04-retina-rendering*
*Completed: 2026-10-25*
