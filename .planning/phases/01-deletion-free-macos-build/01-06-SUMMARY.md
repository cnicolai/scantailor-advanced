---
phase: 01-deletion-free-macos-build
plan: 06
subsystem: testing
tags: [smoke-test, macos, qt6, build-03, project-file, dpr]

requires:
  - phase: 01-deletion-free-macos-build
    provides: macos-release preset and binary (01-01, 01-02), D-02 prior-settings decision and DPR probe (01-05)
provides:
  - "records/SMOKE-FINDINGS.md: D-05 walkthrough results, re-check of steps 4, 6, 8, output-file checks, project-file checks, DPR, settings paths, logged-only Mac observations"
  - "records/smoke-stderr.txt: normalised stderr of the first walkthrough (the re-check has no stderr capture)"
  - "BUILD-03 verdict PASS after the re-check of steps 4, 6, 8 (first run FAIL, kept in the record)"
affects: [01-07, phase-03-strip, phase-04-hidpi, phase-05-mac-integration]

actuals:
  tokens: 9000
  tasks: 3
  commits: 5

tech-stack:
  added: []
  patterns:
    - "Project-file check by python3 -I with the standard XML parser, printing only ids, modes and counts"
    - "Verdict repair by re-check: append the re-check to the record, keep the failed first run"

key-files:
  created:
    - .planning/phases/01-deletion-free-macos-build/records/smoke-stderr.txt
  modified:
    - .planning/phases/01-deletion-free-macos-build/records/SMOKE-FINDINGS.md

key-decisions:
  - "BUILD-03 verdict is PASS after the re-check of steps 4, 6, 8: the re-saved project holds dewarping auto on page 02 left and one picture zone on page 03; the first run's FAIL stays in the record"
  - "The first run's Auto dewarping and picture zone never took effect (its output-params also said off, no zone existed); no app defect was found in the save and load path"
  - "The macOS save dialog cannot take the scratchpad path, so the project file stays at the user-named Desktop path; recorded as a dialog constraint, not a user error"
  - "HIDPI-01 provisionally needs no embedded Info.plist: probe ratio 2.0 and the user reports a sharp window; still no qt.highdpi line from the app (no stderr capture in the re-check)"

requirements-completed: [BUILD-03]

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
        ref: "magick identify -format '%f %wx%h %x %y %U %[colorspace] depth=%z %[compression]\\n' {scratch}/smoke/out/*.tif (run after the first walkthrough and again after the re-check)"
        status: pass
    human_judgment: true
    rationale: "Opening in Preview is a user observation (step 7, first-run files only); depth is checked by command"
  - id: D3
    description: "Saved project reopens with the same pages and settings, and contains colorMode bw, colorOrGray, mixed, dewarping mode auto and at least one zone"
    requirement: BUILD-03
    verification:
      - kind: other
        ref: "python3 -I parse_project2.py ~/Desktop/test.ScanTailor (re-check: colorMode bw, colorOrGray, mixed; dewarping auto and off; zones 1; first run: auto absent, zones 0)"
        status: pass
    human_judgment: true
    rationale: "Reopen result is user-reported (R5 pass); the command check passes after the re-check"
  - id: D4
    description: "Device pixel ratio, settings paths and the logged-only Mac observations (shortcuts, menus, alarm, language) are on record and not fixed"
    verification: []
    human_judgment: true
    rationale: "Observations from the user's walkthrough; consumed by Phases 3 to 5"

duration: 44min + re-check
completed: 2026-10-23
status: complete
plan_head_before: 015b7a4c7058592dff06193a69c0866100a6f59c
plan_head_after: b75c54a35edc232602f23f40e9a4c49801adf112
---

# Phase 1 Plan 06: Smoke Run Summary

**D-05 smoke run on the Homebrew-Qt 6 arm64 build: BUILD-03 PASS after a re-check of steps 4, 6 and 8 (the first run failed because Auto dewarping and the picture zone never took effect); the re-saved project holds dewarping `auto` and one zone, and four output TIFFs pass the depth checks**

## Performance

- **Duration:** 44 min for the first run (14:20 to 15:03 local, most of it the user's walkthrough), plus the user's re-check (about 15:05 to 15:15 local) and the record update
- **Started:** 2026-10-23T12:19:55Z
- **Completed:** 2026-10-23T13:20:00Z (re-check recorded)
- **Tasks:** 3 (Task 2 was the user's walkthrough, presented by the orchestrator; re-check continuation of Task 3)
- **Files modified:** 2 (both in `records/`)

## Accomplishments

- Fresh release build from `015b7a4`, inputs checked against the recorded SHA-256 sums (Task 1, `8eff424`).
- The user walked steps 1 to 11, one step per message. The app opened sharp and in English, took the three pages through all six stages with no error panel, split the spread into two halves, and quit on Command+Q.
- First run, project-file check on the user's Desktop file: dewarping `off` only and 0 zone elements, so the verdict was FAIL (`bb543b3`).
- Re-check of steps 4, 6 and 8 (R1 to R5, one step per message, GUI state confirmed by the user): the re-saved file parses, holds `bw`, `colorOrGray`, `mixed`, dewarping `auto` on page 02 left (params and output-params) and one zone on page 03. R5 reopened the project and both settings survived; Command+Q quit the app.
- Four output TIFFs (`01-flat-text`, `02-book-spread_1L`, `02-book-spread_2R`, `03-mixed`) passed `magick identify` after the first run and again after the re-check: depths 8, 1, 1, 8, matching Color / Grayscale, Black and White, Black and White, Mixed. `02-book-spread_1L` and `03-mixed` were re-rendered during the re-check.
- Measured and logged for later phases without fixing: device pixel ratio 2.0 (probe), shortcut and menu layout under Qt's macOS mapping, the Linux `play` alarm (`sh: play: command not found` twice), English UI, an extra sample in the Mixed output TIFF, 30 `QObject::disconnect` warnings.

## Task Commits

1. **Task 1: Fresh release build, smoke inputs checked, findings file started** - `8eff424` (docs)
2. **Task 2: D-05 smoke walkthrough with the user** - no commit (user checkpoint; reports are in the findings table)
3. **Task 3: Output and project checks, findings written, verdict set** - `bb543b3` (docs, first run, FAIL), then `b75c54a` (docs, re-check recorded, PASS)

**Plan metadata:** `7de8359` (first summary, status halted), `936b4bf` (state and roadmap after the FAIL), and the commits that update this file and the STATE and ROADMAP entries after the re-check.

## Files Created/Modified

- `.planning/phases/01-deletion-free-macos-build/records/SMOKE-FINDINGS.md` - verdict, environment, walkthrough table with the user's verbatim reports, output and project-file checks, DPR, settings paths, logged-only observations, re-check section
- `.planning/phases/01-deletion-free-macos-build/records/smoke-stderr.txt` - 37 lines of app stderr from the first run, no home or scratchpad path in it

## Decisions Made

- Verdict PASS after the re-check. The plan sets PASS only when steps 1 to 10 succeeded, the app quit, every TIFF opened, the depth lines are `ok` and the project file meets every expectation; after the re-check all of them hold. The first run's FAIL, its reasons and its analysis stay in the record.
- The cause of the first-run FAIL is the settings not taking effect in the first walkthrough (first-file `output-params` also `off`, no zone), not the save or load path; no app defect was found. Why they did not take effect is not established beyond that.
- BUILD-03 is listed under `requirements-completed` for this plan. Plan 01-07 also declares it, so the requirement is marked complete in REQUIREMENTS.md only when the shared-ID gate allows it.
- HIDPI-01: provisionally no embedded Info.plist needed (ratio 2.0, sharp window), recorded as provisional because no `qt.highdpi` line reached the stderr file and the re-check had no stderr capture.

## Deviations from Plan

### User-chosen deviation

**1. Project saved to the Desktop, not the scratchpad path**
- **Found during:** Task 2, step 8 (and again at R4 of the re-check)
- **What happened:** the user saved the project to their Desktop and reopened it from there. In the re-check the user explained why: the macOS save dialog cannot take the scratchpad path ("No I can't paste that. You know, when I saved on the Desktop earlier, then there was a reason for that"). Asked how to proceed, the user chose "Read the Desktop file" and named `~/Desktop/test.ScanTailor`.
- **Effect:** the project-file checks ran on that file, not on `{scratch}/smoke/smoke.ScanTailor` (which does not exist). It was read only by `python3 -I` with the standard XML parser; only the allowed items are in the record. The file was not copied, hashed, moved or committed.
- **Nature:** a practical constraint of the file dialog and the long scratchpad path, not a user error. A later walkthrough should name a path the dialog can reach.

**2. No stderr capture for the re-check**
- **Found during:** re-check, after R5
- **What happened:** the user apparently launched without the redirect; `{scratch}/smoke/smoke-stderr-recheck.txt` does not exist.
- **Effect:** `records/smoke-stderr.txt` covers the first run only. The app's `qt.highdpi` confirmation is therefore still missing.

### Auto-fixed Issues

None. No launch failure occurred, so no D-07 launch fix was made and no `src/` file was edited. `records/fix-log.md` has no row from this plan.

**Total deviations:** 2 user-side or environment-side (Desktop save path, missing re-check stderr), 0 auto-fixed.
**Impact on plan:** none on the verdict; the project-file check rests on a user-named Desktop file whose modification time (15:13:17 local) shows it was written during the re-check.

## Issues Encountered

**First run: saved project held no dewarping `auto` and no picture zone (resolved by the re-check).** Three explanations were open; the outcome of each is in `SMOKE-FINDINGS.md` `## What is open`:

1. The Auto dewarping and the picture zone were not applied in the first walkthrough. Confirmed by the re-check.
2. `~/Desktop/test.ScanTailor` might not have been the file saved at step 8. Not needed to explain the result; the re-saved file is dated during the re-check.
3. The app saved without these settings. Excluded: with both settings confirmed in the GUI, the app saved and reloaded them correctly.

Remaining small items, not blocking: the two TIFFs re-rendered in the re-check were checked by `magick identify` only (not re-opened in Preview), and the app's own `qt.highdpi` output is still missing.

## Known Stubs

None. No source file was created or modified in this plan.

## Threat Flags

None. No new network endpoint, auth path, file access pattern or schema change. The alarm at `src/app/MainWindow.cpp:1337` is the already-registered T-01-14 (accepted, observed only).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 01-07 depends on this plan and is no longer blocked: the BUILD-03 verdict is PASS.
- Phase 3 (STRIP-06): UI language is English. Phase 4 (HIDPI-01): ratio 2.0, sharp, provisional. Phase 5 (MAC-10 and menus): alarm needs the system beep; Quit and Settings are moved by Qt into the app menu, shortcuts display with Command symbols.
- For plan 01-07 and later: `03-mixed.tif` carries an extra sample (libtiff `ExtraSamples` warning), logged and not fixed.

## Self-Check: PASSED

- `records/SMOKE-FINDINGS.md` and `records/smoke-stderr.txt` exist.
- Commits `8eff424`, `bb543b3` and `b75c54a` are ancestors of HEAD (`git log 015b7a4..HEAD` lists them; 5 commits measured before this file was committed).
- `grep -E '^BUILD-03 verdict: (PASS|FAIL)'` prints `BUILD-03 verdict: PASS (after re-check of steps 4, 6, 8; first run FAIL, see below)`; all sections required by the acceptance criteria are present; `smoke-stderr.txt` has 0 `$HOME` matches; no `$HOME` or `/Users/` string in `records/`; no image, `.tif` or `.ScanTailor` file is untracked or added since `5eaac18`.
- The plan's three automated `<verify>` commands pass.

---
*Phase: 01-deletion-free-macos-build*
*Completed: 2026-10-23*
