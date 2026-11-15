---
phase: 05-mac-menus-and-shortcuts
plan: 10
subsystem: ui
tags: [cocoa, nsopenpanel, live-harness, menu-bar, dialogs, relinking, qt6, macos]

requires:
  - phase: 05-mac-menus-and-shortcuts
    provides: mac_check harness and cocoaprobe API (05-02, 05-05), live-run practice (05-06), native relinking panels (05-09)
provides:
  - mac_check sections dialogs and panels, refused unless HOME and CFFIXED_USER_HOME name <output-dir>/home
  - Measurement that the native menu bar keeps the application menu and File, View, Tools during six dialogs and the native Open Project panel
  - Proof that all five reachable file panels, including both relinking panels, are NSOpenPanel and return an empty path when cancelled
affects: [05-11]

actuals:
  tokens: 10300
  tasks: 2
  commits: 3

plan_head_before: c94384d65424ee8b69e0c90bd80648f90836a028
plan_head_after: 22e86da5e7d612e5f3d5de62068e9cee585ca9d1
commits: 3

tech-stack:
  added: []
  patterns:
    - "A section that opens native panels checks the redirected home first and reports FAIL with a reason without opening anything"
    - "A block scheduled with cocoaprobe::runLater before a native panel opens inspects, observes the QFileDialog object and cancels the panel from inside its modal session"

key-files:
  created:
    - .planning/phases/05-mac-menus-and-shortcuts/records/dialogs.md
    - .planning/phases/05-mac-menus-and-shortcuts/records/panels.md
  modified:
    - src/app/mac_check/main.cpp
    - src/app/mac_check/CocoaProbe.h
    - src/app/mac_check/CocoaProbe.mm
    - .planning/phases/05-mac-menus-and-shortcuts/records/gate-log.md

key-decisions:
  - "No fallback menu bar: all seven dialog rows showed the application menu and File | View | Tools, so MainWindow is unchanged and has no m_fallbackMenuBar (the m14f gate was not needed)"
  - "main()'s home_redirected summary line now uses homeRedirected(out), which also requires both environment variables to name <out>/home"

patterns-established:
  - "Live sections that touch native panels run only with the redirected home, verified in-process"

requirements-completed: [MAC-09, MAC-11, MAC-01]

coverage:
  - id: D1
    description: "While Settings, About, Fix DPI, Relinking, Default Parameters, an application-modal message box and the native Open Project panel are open, the native menu bar shows the application menu and the File, View and Tools menus"
    requirement: MAC-09
    verification:
      - kind: other
        ref: "build/scratch/mac/m14a/summary.txt dialogs_verdict: PASS, seven *_populated: yes (records/dialogs.md)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Both relinking panels, the Open Project panel and the two new-project folder panels are native NSOpenPanel, open below the output directory, and return an empty path when cancelled without changing the relinking substitution"
    requirement: MAC-11
    verification:
      - kind: other
        ref: "build/scratch/mac/m14a/summary.txt panels_verdict: PASS, five *_native: yes, *_cancel_result: empty, two *_substitution_unchanged: yes (records/panels.md)"
        status: pass
      - kind: integration
        ref: "gate m14-mac-check-dialogs: 100% tests passed out of 7 in Release and Debug, goldens unchanged, command diff 0 / 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "The native folder panel shows files greyed out, and macOS draws the relinking panel title"
    requirement: MAC-11
    verification:
      - kind: other
        ref: "records/panels.md Observations: panel title property read, rendering and greying not observed (no screenshot)"
        status: unverified
    human_judgment: true
    rationale: "Whether the title and the greyed files are drawn is visual; no screenshot is taken because screen capture needs a privacy permission the phase excludes"

duration: not measured
completed: 2026-11-15
status: complete
---

# Phase 5 Plan 10: Dialogs and native file panels Summary

**mac_check sections dialogs and panels prove on real Cocoa that the menu bar stays populated (application menu plus File, View, Tools) during six dialogs and the Open Project panel, and that all five reachable file panels, including both relinking panels, are native NSOpenPanel that return an empty path when cancelled; no fallback bar was needed**

## Performance

- **Duration:** not measured
- **Completed:** 2026-11-15
- **Tasks:** 2
- **Live runs:** 1 full run (`m14a`, `dialogs,panels`, home redirected) plus 1 guard run without a redirected home that opened nothing (`m14-guard`). The run `m14a` was made before the Task 1 commit to validate the harness and, with the harness code unchanged since, serves as the recorded run; its `summary.txt` was copied to `build/scratch/mac/m14/` for the plan's verify path. The pointer was not moved and `cocoaprobe::warpPointer` was not used. No interference was detected: `dialogs_app_active` and `panels_app_active` were yes, every dialog row was shown and every panel was seen. Windows and native panels took focus for roughly a minute.

## Accomplishments

- `CocoaProbe`: `sendKeyToModalWindow`, `modalWindowClass`, `modalWindowIsFilePanel`, `modalPanelDirectory`, `modalPanelTitle`, `cancelModalPanel`; `sendKey` now shares one implementation with `sendKeyToModalWindow`. Events still go only through `[NSApp sendEvent:]`.
- `mac_check`: `homeRedirected(out)` (both variables name `<out>/home`; `NSHomeDirectory`, `QDir::homePath` and `AppDataLocation` below `build/`), `runDialogsSection`, `runPanelsSection`; `dialogs` and `panels` are in the known sections and in `all`. Without the redirection, `--section panels` printed `panels_reason: home not redirected` and `panels_verdict: FAIL`, exited 1 and opened no panel (`m14-guard`).
- `dialogs_verdict: PASS`: for Settings, About, Fix DPI, Relinking, Default Parameters, an application-modal message box and the native Open Project panel the bar read `File | View | Tools` with a populated application menu. The New Project item is disabled under every dialog (enabled but unfired under the native panel), and ⌘N fired 0 times in all seven rows.
- `panels_verdict: PASS`: relink file, relink folder, Open Project, input folder and output folder are `NSOpenPanel`, each starts below the output directory, each cancel finished the `QFileDialog` with `Rejected` and no `filesSelected` (empty path), and after the relinking dialog's `commitChanges` both relinking rows left the substitution of the collected path equal to the original.
- Records `dialogs.md` and `panels.md` hold the raw lines, tables, the decision and the observations, and state that no key or click was physically pressed.

## Task Commits

1. **Task 1: sections dialogs and panels, gated** - `efe9cbdb` (test); gate record `48909dbe` (`m14-mac-check-dialogs`, release and debug 100% tests passed out of 7, goldens unchanged, command diff 0 / 0)
2. **Task 2: records** - `22e86da5` (docs). No source change: no row was unpopulated, so no `m14f-menubar-fallback` commit.

**Plan metadata:** the SUMMARY commit follows this file.

## Files Created/Modified

- `src/app/mac_check/main.cpp` - `homeRedirected`, `prepareDialogFolders`, `readMenuBar`, `runDialogsSection`, `PanelObservation`, `runPanelRow`, `runPanelsSection`, section wiring and usage text
- `src/app/mac_check/CocoaProbe.h`, `CocoaProbe.mm` - the six modal-window probes
- `.planning/phases/05-mac-menus-and-shortcuts/records/dialogs.md`, `panels.md`, `gate-log.md`

## Decisions Made

- No fallback menu bar (measurement showed every row populated); `m_fallbackMenuBar` and `testFallbackMenuBarHoldsSameActions` do not exist.
- `all` includes `dialogs` and `panels` as the plan says; without the redirected home `--section all` therefore reports FAIL for those two sections (they refuse by design).

## Deviations from Plan

### Auto-fixed Issues

**1. [Ordering] Live run before the Task 1 commit**
- **Issue:** The plan has Task 1 commit and gate first, then the live run in Task 2.
- **Fix:** The first live run was made on the built, uncommitted harness to avoid gating a harness that might need fixes; it passed unchanged, so the committed code is the code that ran. Its output directory is `m14a`; the summary was copied to `m14` for the plan's verify command. Recorded in both records and above.

### Notes

- `QObject::disconnect: wildcard call disconnects from destroyed signal ...` lines appear when a `MainWindow` is destroyed; they also appear in the 05-02 and 05-05 runs and are not caused by this plan.
- `cocoaprobe::warpPointer` was not used (no pointer placement is needed in this plan).

**Total deviations:** 1 ordering deviation, no code deviations. **Impact:** none on the evidence; the recorded run is of the committed harness.

## Auth Gates

None.

## Issues Encountered

None. A scripted edit briefly duplicated the end of `runPanelsSection` in `main.cpp`; the build failed, the duplicate was removed and the file rebuilt and formatted before the commit.

## Known Stubs

None.

## Threat Flags

None. T-05-18 mitigated (home guard verified by the refused guard run, panels cancelled below `build/`, home folder stayed empty); T-05-19 not triggered (⌘N fired 0 times); T-05-20 not needed (no fallback bar).

## Next Phase Readiness

- Plan 05-11 can document both run commands; the one residual for the closing report is visual: whether the relinking panel title is drawn and whether the folder panel greys files was not observed (no screenshot), and the `Files to insert` ProxyModel (hides project files in the old Qt dialog, not consulted by a native panel) predates the phase and is not repaired.

## Self-Check: PASSED

- Files found: `records/dialogs.md`, `records/panels.md`, `src/app/mac_check/main.cpp` (`runDialogsSection`, `runPanelsSection`, `homeRedirected`), `CocoaProbe.h` (`cancelModalPanel`, `modalPanelDirectory`).
- Commits found: `efe9cbdb`, `48909dbe`, `22e86da5`.
- Gate: `m14-mac-check-dialogs` `result: PASS`, goldens unchanged, command diff 0 / 0; `grep -c m_fallbackMenuBar src/app/MainWindow.h` is 0.
