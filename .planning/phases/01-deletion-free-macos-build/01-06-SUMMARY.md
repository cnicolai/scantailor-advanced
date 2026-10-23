---
phase: 01-deletion-free-macos-build
plan: 06
subsystem: testing
tags: [smoke-test, macos, qt6, build-03, project-file, dpr]

requires:
  - phase: 01-deletion-free-macos-build
    provides: macos-release preset and binary (01-01, 01-02), D-02 prior-settings decision and DPR probe (01-05)
provides:
  - "records/SMOKE-FINDINGS.md: D-05 walkthrough results, output-file checks, project-file check, DPR, settings paths, logged-only Mac observations"
  - "records/smoke-stderr.txt: normalised stderr of the walkthrough"
  - "BUILD-03 verdict FAIL with two named, unresolved reasons"
affects: [01-07, phase-03-strip, phase-04-hidpi, phase-05-mac-integration]

actuals:
  tokens: 5000
  tasks: 3
  commits: 2

tech-stack:
  added: []
  patterns:
    - "Project-file check by python3 -I with the standard XML parser, printing only ids, modes and counts"

key-files:
  created:
    - .planning/phases/01-deletion-free-macos-build/records/smoke-stderr.txt
  modified:
    - .planning/phases/01-deletion-free-macos-build/records/SMOKE-FINDINGS.md

key-decisions:
  - "BUILD-03 verdict is FAIL, not PASS: the saved project file holds dewarping mode off only and 0 picture zones, so the plan's PASS rule is not met"
  - "Project-file check ran on the user-named file ~/Desktop/test.ScanTailor, because the user saved there at step 8"
  - "HIDPI-01 provisionally needs no embedded Info.plist: probe ratio 2.0 and the user reports a sharp window; no qt.highdpi line was captured to confirm"

requirements-completed: []  # BUILD-03 is declared by this plan but not met (verdict FAIL), so it is not copied here

coverage:
  - id: D1
    description: "App launches from the build directory, project is created from the three synthetic pages and batch-processed through all six stages with no error panel"
    requirement: BUILD-03
    verification: []
    human_judgment: true
    rationale: "GUI walkthrough carried out by the user (steps 1 to 3); reported ok, no automated test covers it"
  - id: D2
    description: "Four output TIFFs open in Preview and pass magick identify; depth 1 for Black and White, 8 for Color / Grayscale and Mixed"
    requirement: BUILD-03
    verification:
      - kind: other
        ref: "magick identify -format '%f %wx%h %x %y %U %[colorspace] depth=%z %[compression]\\n' {scratch}/smoke/out/*.tif"
        status: pass
    human_judgment: true
    rationale: "Opening in Preview is a user observation (step 7); depth is checked by command"
  - id: D3
    description: "Saved project reopens with the same pages and settings, and contains colorMode bw, colorOrGray, mixed, dewarping mode auto and at least one zone"
    requirement: BUILD-03
    verification:
      - kind: other
        ref: "python3 -I parse_project.py ~/Desktop/test.ScanTailor (colorMode bw, colorOrGray, mixed found; dewarping auto absent; zones 0)"
        status: fail
    human_judgment: true
    rationale: "Reopen result is user-reported ('as I have left it'); the command check fails on dewarping and zone"
  - id: D4
    description: "Device pixel ratio, settings paths and the logged-only Mac observations (shortcuts, menus, alarm, language) are on record and not fixed"
    verification: []
    human_judgment: true
    rationale: "Observations from the user's walkthrough; consumed by Phases 3 to 5"

duration: 44min
completed: 2026-10-23
status: halted
plan_head_before: 015b7a4c7058592dff06193a69c0866100a6f59c
plan_head_after: bb543b3f39b2e31368301aa53a80a81bd077bc5a
---

# Phase 1 Plan 06: Smoke Run Summary

**D-05 smoke run on the Homebrew-Qt 6 arm64 build: steps 1 to 11 reported ok and four output TIFFs pass the depth checks, but the saved project holds no dewarping `auto` and no picture zone, so the BUILD-03 verdict is FAIL**

## Performance

- **Duration:** 44 min (14:20 to 15:03 local, most of it the user's walkthrough)
- **Started:** 2026-10-23T12:19:55Z
- **Completed:** 2026-10-23T13:04:00Z
- **Tasks:** 3 (Task 2 was the user's walkthrough, presented by the orchestrator)
- **Files modified:** 2 (both in `records/`)

## Accomplishments

- Fresh release build from `015b7a4`, inputs checked against the recorded SHA-256 sums (Task 1, `8eff424`).
- The user walked steps 1 to 11, one step per message. The app opened sharp and in English, took the three pages through all six stages with no error panel, split the spread into two halves, and quit on Command+Q.
- Four output TIFFs (`01-flat-text`, `02-book-spread_1L`, `02-book-spread_2R`, `03-mixed`) passed `magick identify`: depths 8, 1, 1, 8, matching Color / Grayscale, Black and White, Black and White, Mixed.
- Project-file check on the user's Desktop file: parses; `colorMode` values `bw`, `colorOrGray`, `mixed`; but dewarping mode `off` only and 0 zone elements. The two expectations that fail are the reason for the FAIL verdict.
- Measured and logged for later phases without fixing: device pixel ratio 2.0 (probe), shortcut and menu layout under Qt's macOS mapping, the Linux `play` alarm (`sh: play: command not found` twice), English UI, an extra sample in the Mixed output TIFF, 30 `QObject::disconnect` warnings.

## Task Commits

1. **Task 1: Fresh release build, smoke inputs checked, findings file started** - `8eff424` (docs)
2. **Task 2: D-05 smoke walkthrough with the user** - no commit (user checkpoint; reports are in the findings table)
3. **Task 3: Output and project checks, findings written, verdict set** - `bb543b3` (docs)

**Plan metadata:** the commit that adds this file and the STATE and ROADMAP updates.

## Files Created/Modified

- `.planning/phases/01-deletion-free-macos-build/records/SMOKE-FINDINGS.md` - verdict, environment, walkthrough table with the user's verbatim reports, output and project-file checks, DPR, settings paths, logged-only observations, open question
- `.planning/phases/01-deletion-free-macos-build/records/smoke-stderr.txt` - 37 lines of app stderr, no home or scratchpad path in it

## Decisions Made

- Verdict FAIL rather than PASS. The plan sets PASS only when the project-file check meets every expectation; dewarping `auto` and at least one zone are both missing. The cause is not established (see Issues Encountered), so the verdict was not softened.
- BUILD-03 is not copied to `requirements-completed` and not marked complete, and the summary status is `halted`, so plan 01-07 (which depends on this plan) is reported as blocked until the question is resolved.
- HIDPI-01: provisionally no embedded Info.plist needed (ratio 2.0, sharp window), recorded as provisional because no `qt.highdpi` line reached the stderr file.

## Deviations from Plan

### User-chosen deviation

**1. Project saved to the Desktop, not the scratchpad path**
- **Found during:** Task 2, step 8
- **What happened:** the user saved the project to their Desktop and reopened it from there. Asked how to proceed, the user chose "Read the Desktop file" and named `~/Desktop/test.ScanTailor`.
- **Effect:** the project-file check in Task 2 `<verification>` and Task 3 `## Project file` ran on that file, not on `{scratch}/smoke/smoke.ScanTailor` (which does not exist). It was read only by `python3 -I` with the standard XML parser; only the allowed items are in the record. The file was not copied, hashed, moved or committed.
- **Not an error:** recorded in `SMOKE-FINDINGS.md` and here.

### Auto-fixed Issues

None. No launch failure occurred, so no D-07 launch fix was made and no `src/` file was edited. `records/fix-log.md` has no row from this plan.

**Total deviations:** 1 user-chosen, 0 auto-fixed.
**Impact on plan:** the project-file check has an uncertainty it would not have had with the scratchpad file (explanation 2 in Issues Encountered).

## Issues Encountered

**Saved project holds no dewarping `auto` and no picture zone (blocking BUILD-03).** Three explanations fit and none is excluded (details in `SMOKE-FINDINGS.md` `## What is open`):

1. The Auto dewarping and the picture zone were not applied. The user's verbatim reports for steps 4 and 6 do not say they were: "split into two halves, rows look straighter" and "colour block in colour, text black and white". Mixed mode can detect a picture block without a zone.
2. `~/Desktop/test.ScanTailor` is not the file saved in step 8 of this run. Its three colour modes and page structure match this run, which would also hold for an earlier attempt.
3. The app saved without these two settings. Code reading finds no cause (`Settings::setDewarpingOptions`, `Filter::writePageSettings`, `Filter::loadSettings`), but the running app was not tested.

Suggested resolution, for the orchestrator and the user to decide: repeat steps 4, 6 and 8 on the scratchpad path, confirm after step 4 that the Dewarping box reads `Auto` and after step 6 that the Picture Zones tab shows the zone, then re-run the project-file check. If it still fails with both settings confirmed in the GUI, explanation 3 becomes a bug to investigate (a D-07 launch-fix candidate only if platform- or toolchain-caused).

## Known Stubs

None. No source file was created or modified in this plan.

## Threat Flags

None. No new network endpoint, auth path, file access pattern or schema change. The alarm at `src/app/MainWindow.cpp:1337` is the already-registered T-01-14 (accepted, observed only).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 01-07 depends on this plan and declares BUILD-03 as well; it should wait for the decision above.
- Phase 3 (STRIP-06): UI language is English. Phase 4 (HIDPI-01): ratio 2.0, sharp, provisional. Phase 5 (MAC-10 and menus): alarm needs the system beep; Quit and Settings are moved by Qt into the app menu, shortcuts display with Command symbols.

## Self-Check: PASSED

- `records/SMOKE-FINDINGS.md` and `records/smoke-stderr.txt` exist (checked with `ls`).
- Commits `8eff424` and `bb543b3` are ancestors of HEAD (listed by `git log 015b7a4..HEAD`).
- `grep -E '^BUILD-03 verdict: (PASS|FAIL)'` prints `BUILD-03 verdict: FAIL`; all sections required by the acceptance criteria are present; `smoke-stderr.txt` has 0 `$HOME` matches; no image, `.tif` or `.ScanTailor` file is untracked or added since `5eaac18`.

---
*Phase: 01-deletion-free-macos-build*
*Completed: 2026-10-23*
