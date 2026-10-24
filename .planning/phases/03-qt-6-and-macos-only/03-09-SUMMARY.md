---
phase: 03-qt-6-and-macos-only
plan: 09
subsystem: docs
tags: [codebase-maps, claude-md, closing-check, phase-verdict]

requires:
  - phase: 03-qt-6-and-macos-only
    provides: plans 03-01 to 03-08 (strip commits c00 to c11, gate log, leftover sweeps, fresh-clone proof, sanitizer comparison)
provides:
  - seven .planning/codebase maps patched to the stripped tree (D-10)
  - regenerated GSD sections of .claude/CLAUDE.md with the profile section untouched
  - records/closing-check.md with the Phase 3 verdict (five criteria PASS), the STRIP and D-01 to D-15 mapping, and the summary for the user
affects: [phase-04-retina-rendering, every later agent that reads CLAUDE.md or the maps]

actuals:
  tokens: 11400
  tasks: 2
  commits: 2
plan_head_before: 521b58306978667abf82a39ac50fb2b98f35ce46
plan_head_after: 3b17e133e68373a8c3d78b61297f2fc49e2df4e0

tech-stack:
  added: []
  patterns: ["patch stale facts in the codebase maps, never remap or stamp", "regenerate CLAUDE.md only through generate-claude-md"]

key-files:
  created:
    - .planning/phases/03-qt-6-and-macos-only/records/closing-check.md
  modified:
    - .planning/codebase/STACK.md
    - .planning/codebase/INTEGRATIONS.md
    - .planning/codebase/STRUCTURE.md
    - .planning/codebase/CONCERNS.md
    - .planning/codebase/CONVENTIONS.md
    - .planning/codebase/TESTING.md
    - .planning/codebase/ARCHITECTURE.md
    - .claude/CLAUDE.md

key-decisions:
  - "The one D-10 line without a Phase 3 marker (generator boilerplate listing .github/skills/ as a place for skills) stays, because CLAUDE.md is edited only through the generator"
  - "A CONCERNS.md entry for the kept path normaliser points to backlog item 999.8"
  - "The correction of the 03-01 cmake record is made in closing-check.md; before-state.md stays as measured"

requirements-completed: [STRIP-01, STRIP-02, STRIP-03, STRIP-04, STRIP-05, STRIP-06, STRIP-08]

coverage:
  - id: D1
    description: "The seven codebase maps state the current facts (Qt 6 only, macOS only, presets, seven suites, ui_snapshot, no CI, packaging, translations or portable mode) and mark every removed thing with Phase 3"
    requirement: "STRIP-08"
    verification:
      - kind: other
        ref: "D-10 grep over .planning/codebase/*.md and .claude/CLAUDE.md: 40 matching lines, 39 with a Phase 3 or 999.8 marker, the 40th is generator boilerplate (see Deviations)"
        status: pass
    human_judgment: false
  - id: D2
    description: "GSD sections of .claude/CLAUDE.md regenerated; GSD:profile section byte-identical to the parent commit; no map frontmatter, build input or stamp changed"
    verification:
      - kind: other
        ref: "diff of the GSD:profile block against the parent commit; git diff of last_mapped_commit and last_mapped_at; git diff --quiet over src CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in"
        status: pass
    human_judgment: false
  - id: D3
    description: "records/closing-check.md gives PASS for the five ROADMAP Phase 3 criteria, maps STRIP-01 to STRIP-06, STRIP-08 and D-01 to D-15 to commits and records, lists the exceptions, and ends with a summary of at most 15 lines that asks for no manual step"
    verification:
      - kind: other
        ref: "section, criterion, requirement and decision grep of Task 2 plus summary length 10, home path count 0, records directory clean"
        status: pass
    human_judgment: true
    rationale: "Whether the summary is clear and asks nothing of the user is a judgment; the structure is checked by command"

duration: 7min
completed: 2026-10-24
status: complete
---

# Phase 3 Plan 09: Agent docs and closing check Summary

**The seven codebase maps and the GSD sections of CLAUDE.md now describe the stripped Qt 6, macOS-only tree, and `records/closing-check.md` closes Phase 3 with PASS on all five ROADMAP criteria and a traceable map of every STRIP requirement and decision D-01 to D-15.**

## Performance

- **Duration:** 7 min
- **Started:** 2026-10-24T21:36:18Z
- **Completed:** 2026-10-24T21:43:02Z
- **Tasks:** 2
- **Files modified:** 9 (eight in Task 1, one new record in Task 2)

## Accomplishments

- Patched all seven `.planning/codebase/*.md` maps by hand to the current facts: Qt 6 only with the 6.5 floor, macOS on Apple Silicon only, `CMakePresets.json` with `macos-release`, `macos-debug` and `macos-asan`, the Homebrew package list, seven ctest suites, goldens, `src/test_support`, `src/app/ui_snapshot`, INI settings and the Application Support profile path, no CI, packaging, translations or portable mode. Every removed thing is named with `Phase 3`; the kept `RelinkablePath::normalize` has a CONCERNS entry that cites 999.8. Phase 2 facts that were stale (TestBinarize active, pipeline tests exist, no dewarping tests missing) were corrected too. Frontmatter untouched; no remap, no stamp.
- Regenerated the managed sections of `.claude/CLAUDE.md` with `generate-claude-md` (`action: updated`, five sections generated, `skills` fallback). The `GSD:profile` block is byte-identical to its parent; the generated `project` section did not change.
- Wrote `records/closing-check.md`: toolchain at the checked commit equals the golden manifests, five criteria PASS with their records, STRIP-01 to STRIP-06 and STRIP-08 mapped to commits c00 to c11, D-01 to D-15 mapped, the four resolved edges, the five flagged readings, the prohibitions of all nine plans, the exceptions, the gate cost, and a 10-line summary for the user. It records the correction of the 03-01 cmake note (Homebrew formula `cmake 4.4.4`, toolchain unchanged during the phase).

## Task Commits

1. **Task 1: Patch the maps, regenerate CLAUDE.md (D-10)** - `e1451ad1` (docs)
2. **Task 2: Closing check of Phase 3** - `3b17e133` (docs)

**Plan metadata:** the docs commit of this SUMMARY with STATE.md, ROADMAP.md and REQUIREMENTS.md follows.

## Files Created/Modified

- `.planning/codebase/{STACK,INTEGRATIONS,STRUCTURE,CONCERNS,CONVENTIONS,TESTING,ARCHITECTURE}.md` - stale facts patched; each gets a "patched, not re-mapped" note in its analysis-date line
- `.claude/CLAUDE.md` - managed sections regenerated from the patched maps
- `.planning/phases/03-qt-6-and-macos-only/records/closing-check.md` - phase verdict and user summary

## Decisions Made

- The D-10 search has one remaining line without a marker, `.claude/CLAUDE.md:298`, which the generator writes into its `skills` fallback (`No project skills found. Add skills to any of: ... .github/skills/ ...`). It lists places where skills may live and makes no claim about the removed workflows. The file is edited only through the generator and a hand edit would be overwritten, so the line stays and the closing check says so.
- Test, CI and formatting facts in the maps point to `TESTING.md` instead of repeating counts that go stale (no Boost case count any more).
- The correction of `before-state.md` is made in the closing check; the record itself is not rewritten.

## Deviations from Plan

### Accepted exception

**1. [Check cannot pass literally] D-10 search leaves one generator-owned line**
- **Found during:** Task 1, step 4 (D-10 search after regeneration)
- **Issue:** The plan's first automated check and acceptance criterion require every line of the search to carry `Phase 3` or `999.8`. `.claude/CLAUDE.md:298` (generated `skills` fallback text) matches `.github` and cannot carry a marker without a hand edit inside a generated section.
- **Fix:** None to the file. The 39 other lines carry the marker. Recorded in `closing-check.md` ("Exceptions and deviations") so a later verifier skips this line; `build/scratch/d10-after.txt` (not committed) holds the 40 lines.
- **Files modified:** none
- **Verification:** `grep -v -e 'Phase 3' -e '999\.8'` over the search output prints exactly that one line. The other two automated checks of Task 1 pass.
- **Committed in:** n/a (`e1451ad1` documents the maps; the exception is in `3b17e133`)

---

**Total deviations:** 1 accepted exception (generator boilerplate)
**Impact on plan:** None on content; a verifier that re-runs the plan's first Task 1 check literally will see one line and should read the closing check.

## Issues Encountered

- A first Python edit of `STRUCTURE.md` put the `dewarping/tests` line under `foundation/`; it was corrected before the commit.
- Four phrases in the maps matched the D-10 search by accident ("translation units", "Inconsistent", "renders two windows", a bare Wayland line); they were reworded so the search reflects real facts only.
- The ugrep environment forced `-e` patterns and `grep -F` for the checks; no check was weakened.

## User Setup Required

None - no external service configuration required.

## Summary for the user

Phase 3 removed everything the Mac build does not need: Qt 5, the Windows and Linux code and files, Flatpak, Debian and NSIS packaging, the install rules, all GitHub workflows and Dependabot, the portable mode and the translations.
Qt 6 is now the only supported version, and the UI is English only (the Language row is gone from Settings and the text strings stay in the code).
Every one of the twelve strip commits passed Release and Debug 7 of 7 with the golden images unchanged since the Phase 2 baseline `a796c2ae`, so the output did not change.
The one sanitizer run after the strip found the same 12 UBSan reports at the same 9 places as in Phase 2 and no ASan report; nothing is new.
Settings are in `~/.config/scantailor-advanced/scantailor-advanced.ini`, and profiles are in `~/Library/Application Support/scantailor-advanced/scantailor-advanced/profiles`.
The README alone is enough to build the app and run the tests: a fresh clone passed with only its commands.
The codebase maps and the GSD part of `.claude/CLAUDE.md` now describe the stripped tree.
One new backlog item, 999.8, records that the path normaliser still treats a backslash in a macOS file name as a separator; it was kept on purpose and is not fixed yet.
There is no CI any more, so the local Release and Debug test run is the gate before a commit.
Phase 4 starts with a measurement of the device pixel ratio the build-directory binary reports on the Retina display, and then makes the page view, thumbnails and overlays sharp.

## Next Phase Readiness

- Phase 3 is complete: all nine plans have a SUMMARY, the five criteria are PASS, and the phase can go to verification.
- Phase 4 (Retina Rendering) depends on the baselines being unchanged, which `a796c2ae` and the gate log show.
- No blocker.

## Self-Check: PASSED

- All eight Task 1 files and `records/closing-check.md` exist.
- Commits `e1451ad1` and `3b17e133` are ancestors of HEAD; `git rev-list --count 521b5830..HEAD` was 2 at write time.
- `git diff --quiet 521b5830 HEAD -- src CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in` exits 0: no build input changed.
- Task 1 checks 2 and 3 and all Task 2 checks pass; Task 1 check 1 passes except for the one accepted line above.

---
*Phase: 03-qt-6-and-macos-only*
*Completed: 2026-10-24*
