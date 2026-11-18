---
phase: 01-deletion-free-macos-build
plan: 05
subsystem: infra
tags: [qt6, devicePixelRatio, qsettings, imagemagick, smoke-test, macos]

requires:
  - phase: 01-deletion-free-macos-build
    provides: "Homebrew Qt 6 and the build-tool access approval (plan 01-01)"
provides:
  - "Measured device pixel ratio (2 on the Studio Display, window also 2) from a bare Qt 6.12.0 executable, for Phase 4 HIDPI-01 scope"
  - "Real settings paths: INI ~/.config/scantailor-advanced/scantailor-advanced.ini, AppData ~/Library/Application Support/scantailor-advanced/scantailor-advanced (+ /profiles)"
  - "Three synthetic 300 DPI smoke pages in the scratchpad with regeneration script and SHA-256 checksums"
  - "Prior-state record: no ScanTailor settings exist in any approved location, so the first launch starts from true defaults"
affects: [01-06-smoke-run, 01-07, phase-02-goldens, phase-04-retina]

actuals:
  tokens: 1800
  tasks: 3
  commits: 3
plan_head_before: ac065cdb4eef35f001e5627b57a9f32220d78718
plan_head_after: 4f620b9dc3471f9af8845f71bd1071a0db7af92a

tech-stack:
  added: []
  patterns:
    - "Throwaway probes live in the session scratchpad and construct no QSettings"
    - "Records normalise paths to ~, {repo} and {scratch}"

key-files:
  created:
    - .planning/phases/01-deletion-free-macos-build/records/probe-output.txt
    - .planning/phases/01-deletion-free-macos-build/records/smoke-pages.md
    - .planning/phases/01-deletion-free-macos-build/records/prior-state.md
  modified: []

key-decisions:
  - "D-02 answered list-corrected: only the INI directory, the AppData directories and scantailor-named Preferences entries were listed (names, sizes, dates)"
  - "Nothing exists in any approved location, so no keep or move-aside question was needed and nothing was moved"

requirements-completed: []  # BUILD-03 is shared with plans 01-06 and 01-07; left open on purpose

coverage:
  - id: D1
    description: "probe-output.txt records per-screen and window devicePixelRatio, Qt build string, derived INI path and AppData path, and that no QSettings was constructed"
    requirement: BUILD-03
    verification:
      - kind: other
        ref: "grep devicePixelRatio / 'probe constructs QSettings: no' / scantailor-advanced.ini in records/probe-output.txt"
        status: pass
    human_judgment: false
  - id: D2
    description: "Three synthetic 300 DPI pages (flat text, book spread, mixed) exist in the scratchpad, documented with commands and SHA-256, none committed"
    requirement: BUILD-03
    verification:
      - kind: other
        ref: "grep -c '300 300 PixelsPerInch' records/smoke-pages.md >= 3; no image files added or untracked in git"
        status: pass
    human_judgment: false
  - id: D3
    description: "prior-state.md records the D-02 decision, approved paths and the listing result before the first launch"
    requirement: BUILD-03
    verification:
      - kind: other
        ref: "grep Decision line and outcome line in records/prior-state.md; grep -r $HOME prints nothing"
        status: pass
    human_judgment: false

duration: 3min
completed: 2026-10-23
status: complete
---

# Phase 1 Plan 05: Smoke Preparation Summary

**Pixel ratio 2 measured with a bare Qt 6.12.0 probe, real INI/AppData settings paths named, three deterministic 300 DPI test pages generated in the scratchpad, and a clean (empty) prior-settings state confirmed by the user-approved listing**

## Performance

- **Duration:** 3 min for the continuation (Task 3); Task 1 ran in the earlier session (committed 2026-10-18), Task 2 was the user's decision
- **Started:** 2026-10-23T12:00Z (continuation)
- **Completed:** 2026-10-23T12:05Z
- **Tasks:** 3 (Task 2 was a blocking-human decision, answered `list-corrected`)
- **Files modified:** 3 (all records, all new)

## Accomplishments

- The probe (unbundled Qt 6.12.0 executable, `platform: cocoa`) reports `devicePixelRatio` 2 for the Studio Display and 2 for the window. This is the HIDPI-01 measurement D-06 asked for, taken without any committed code change.
- The probe derived and printed the real settings paths. The app uses an INI file (`QSettings::IniFormat`, `src/app/main.cpp:47`), not a plist: `~/.config/scantailor-advanced/scantailor-advanced.ini`; profiles go to `~/Library/Application Support/scantailor-advanced/scantailor-advanced/profiles`. The probe constructed no QSettings.
- Three synthetic pages (`01-flat-text.tif` 2480x3508, `02-book-spread.tif` 4960x3508, `03-mixed.tif` 2480x3508, 300 PixelsPerInch) exist only in the scratchpad; `smoke-pages.md` holds the generator script, identify output, SHA-256 checksums and a regeneration note.
- With `list-corrected`, the approved listings found nothing: none of `~/.config/scantailor-advanced/`, `~/Library/Application Support/scantailor-advanced/` (and its subdirectories) exist, and no entry in `~/Library/Preferences` contains "scantailor". The first launch in plan 01-06 therefore starts from true defaults and no move-aside was needed.

## Task Commits

1. **Task 1: Scratchpad probe and three throwaway pages** - `6392688` (docs)
2. **Task 2: D-02 permission (checkpoint:decision, blocking-human)** - no commit; answer `list-corrected`
3. **Task 3: List approved locations, record prior state** - `4f620b9` (docs)

Between them, `a07e4b8` is the orchestrator's pause commit (`wip: phase 01 paused at plan 01-05 task 2/3`); it is counted in `actuals.commits` because the count is measured from the plan's base `ac065cd`.

**Plan metadata:** committed with this SUMMARY (docs: complete plan)

## Files Created/Modified

- `.planning/phases/01-deletion-free-macos-build/records/probe-output.txt` - DPR per screen and window, Qt build string, derived INI and AppData paths
- `.planning/phases/01-deletion-free-macos-build/records/smoke-pages.md` - page specifications, generator script, identify output, checksums, regeneration note
- `.planning/phases/01-deletion-free-macos-build/records/prior-state.md` - D-02 decision, approved paths, listing result (nothing found)

## Decisions Made

- D-02: the user chose `list-corrected`; only the approved listings were run, and nothing outside them was searched.
- No keep or move-aside decision arose because no ScanTailor entry exists anywhere approved. If the app is launched and later closes, it will create the INI and AppData directories; plan 01-06 should remember that a second smoke run no longer starts from empty settings.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None. The `a07e4b8` pause commit sits inside the plan's commit range, so the measured commit count is 3 rather than the 2 task commits.

## Authentication Gates

None. Task 2 was a planned human decision gate, not an authentication gate.

## Known Stubs

None.

## Threat Flags

None. No new network, auth, file-access or schema surface was added; all outputs are records.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 01-06 can use the pages in the scratchpad (regenerate with the script in `smoke-pages.md` if the scratchpad was cleared) and knows the INI and AppData paths to expect after the first launch.
- BUILD-03 stays open: it is shared with plans 01-06 and 01-07 and is marked complete only after the smoke run.

## Self-Check: PASSED

- Records exist: probe-output.txt, smoke-pages.md, prior-state.md (checked with `[ -f ]`).
- Commits `6392688` and `4f620b9` are ancestors of HEAD.
- Verify commands for Tasks 1 and 3 pass; no image file added to git or untracked; `grep -r "$HOME"` on the records prints nothing.

---
*Phase: 01-deletion-free-macos-build*
*Completed: 2026-10-23*
