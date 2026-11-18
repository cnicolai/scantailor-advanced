---
phase: 05-mac-menus-and-shortcuts
plan: 07
subsystem: ui
tags: [qt6, uic, macos, copy, apply-dialogs]

requires:
  - phase: 05-mac-menus-and-shortcuts
    provides: gate procedure and gate log (05-01 to 05-06), UI-SPEC row E13 (key-name style)
provides:
  - The multi-page hint of all nine apply dialogs reads "Use ⌘-click / ⇧-click to select multiple pages."
affects: [05-08]

actuals:
  tokens: 1500
  tasks: 2
  commits: 4

plan_head_before: e6246028bcc131a3044396176a5497cb6e220e9a
plan_head_after: db95ee96ca800929875883c499f8923b0f440395
commits: 4

tech-stack:
  added: []
  patterns:
    - "Mac key names in user-visible text: ⌘ for Ctrl, ⇧ for Shift, lower-case 'click'"

key-files:
  created: []
  modified:
    - src/core/filters/fix_orientation/ApplyDialog.ui
    - src/core/filters/page_split/SplitModeDialog.ui
    - src/core/filters/deskew/ApplyDialog.ui
    - src/core/filters/select_content/ApplyDialog.ui
    - src/core/filters/page_layout/ApplyDialog.ui
    - src/core/filters/page_layout/ApplyMarginsDialog.ui
    - src/core/filters/output/ApplyColorsDialog.ui
    - src/core/filters/output/ChangeDewarpingDialog.ui
    - src/core/filters/output/ChangeDpiDialog.ui
    - .planning/phases/05-mac-menus-and-shortcuts/records/gate-log.md

key-decisions:
  - "Only the hint line of each of the nine .ui files changed (one insertion and one deletion per file); no layout, property, object name or behaviour changed, and no translation catalogue exists to update"

patterns-established:
  - "Text-only rewrites are checked with git show --numstat per commit (one line per file) plus a gate run with an empty build-command diff"

requirements-completed: [MAC-06]

coverage:
  - id: D1
    description: "Nine apply dialogs show 'Use ⌘-click / ⇧-click to select multiple pages.' and no old Ctrl+Click / Shift+Click hint remains under src outside the tests"
    requirement: MAC-06
    verification:
      - kind: other
        ref: "grep -rlF over src/core/filters finds the new hint in 9 files; the old hint is found in 0 files outside src/qt_tests"
        status: pass
      - kind: integration
        ref: "gates m10a-dialog-hints and m10b-dialog-hints: uic builds all nine dialogs, 100% tests passed out of 7 in Release and Debug, goldens unchanged, command diff 0 / 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "The shorter hint fits each dialog label without layout change"
    requirement: MAC-06
    verification:
      - kind: other
        ref: "47 code points against 54 of the old hint (UI-SPEC E13); not rendered or looked at in this plan"
        status: unverified
    human_judgment: true
    rationale: "Fit was argued from the shorter string length; no dialog was rendered in this plan"

duration: not measured
completed: 2026-11-15
status: complete
---

# Phase 5 Plan 7: Mac key names in the apply dialogs Summary

**The multi-page hint of all nine apply dialogs now reads "Use ⌘-click / ⇧-click to select multiple pages." in two gated one-line-per-file commits; uic builds every dialog and the gate passes with an empty build-command diff**

## Performance

- **Duration:** not measured
- **Completed:** 2026-11-15
- **Tasks:** 2
- **Files modified:** 9 `.ui` files plus the gate log

## Accomplishments

- Fix Orientation, Split Pages, Deskew and Select Content dialogs (commit `10e50d98`) and the two Margins and three Output dialogs (commit `31de6ead`) show the Mac key names.
- Each commit has `git show --numstat` totals of 4 and 5 files with one insertion and one deletion per file; nothing else under `src/` changed.
- The old hint is found in 0 files under `src/` outside `src/qt_tests`; the new hint in 9 files under `src/core/filters`.
- The output-path guard (convention 14) prints nothing: no diff against `f6cc4457` under the goldens, `imageproc`, `dewarping`, `OutputGenerator.cpp` or any filter `Task`.

## Task Commits

1. **Task 1: four stages** - `10e50d98` (fix); gate record `ca90a9bf` (`m10a-dialog-hints`)
2. **Task 2: Margins and Output** - `31de6ead` (fix); gate record `db95ee96` (`m10b-dialog-hints`)

**Plan metadata:** the SUMMARY commit follows this file.

## Files Created/Modified

- The nine `.ui` files listed in the frontmatter: the hint `<string>` only.
- `records/gate-log.md`: rows `m10a-dialog-hints` and `m10b-dialog-hints`.

## Decisions Made

- None beyond the plan. The characters are stored as UTF-8 (`file` reports UTF-8 text) and uic accepted them in every build.

## Deviations from Plan

None - plan executed exactly as written.

One bookkeeping note, not a deviation: the parent of the first gated commit (`e6246028`) is a docs-only commit that was never gated. Its build-input key equals that of the gated `91737869`, so `gate.sh` found the reference and no copy was needed; the gate-log row of `m10a-dialog-hints` says so.

## Auth Gates

None.

## Issues Encountered

None. `qt_tests` (Release) passed before each commit; both gates reported `result: PASS`, `goldens: unchanged`, command diff 0 / 0.

## Known Stubs

None.

## Threat Flags

None. Text-only edits; no new endpoint, auth path, file access or schema.

## Next Phase Readiness

- Plan 05-08 rewrites the remaining PC key names (Margins help, main window texts, three C++ tips) and adds the permanent scan test; the old hint no longer appears in any `.ui` file, so the scan only has the remaining strings to cover.
- Open item for the user's closing check (05-11): the shorter hint is argued to fit each label, but no dialog was rendered here.

## Self-Check: PASSED

- Files found: all nine `.ui` files contain the new hint; `records/gate-log.md` has both rows.
- Commits found: `10e50d98`, `ca90a9bf`, `31de6ead`, `db95ee96`.
- Gates: `m10a-dialog-hints` and `m10b-dialog-hints` both `result: PASS`, goldens unchanged, command diff 0 / 0, tested commits match.
