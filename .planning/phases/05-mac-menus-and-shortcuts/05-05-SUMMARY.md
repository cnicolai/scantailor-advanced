---
phase: 05-mac-menus-and-shortcuts
plan: 05
subsystem: ui
tags: [cocoa, nsevent, live-harness, shortcuts, zones, qt6, macos]

requires:
  - phase: 05-mac-menus-and-shortcuts
    provides: mac_check harness and cocoaprobe API (05-02), stage, zoom and thumbnail keys (05-03), zone and content-box keys (05-04)
provides:
  - mac_check sections keys and zone, proving every window key, the Output tab keys, the zoom and thumbnail keys, the native View menu, Cmd-Q, and the delete keys on zones and the content box on real Cocoa with the German layout
  - cocoaprobe::warpPointer, a pointer placement that works without any privacy permission
  - Answer to the STATE.md question: Backspace reaches the zone and content-box shortcuts under Qt 6.12 on Cocoa
affects: [05-06, 05-10, 05-11]

actuals:
  tokens: 9500
  tasks: 2
  commits: 3

plan_head_before: 97b7f5f1d2ce276e96fc03795e95bf6581d2aaae
plan_head_after: c9c923ddd28fff61ba222359a4d474ffc529065c
commits: 3

tech-stack:
  added: []
  patterns:
    - "A counter per watched QAction on a deque (stable addresses), production slots disconnected before counting"
    - "A window event filter that ignores and counts QEvent::Close makes Cmd-Q observable without ending the run"
    - "Pointer placed with CGWarpMouseCursorPosition, checked against QCursor::pos(), then one synthesised mouse-move"

key-files:
  created:
    - .planning/phases/05-mac-menus-and-shortcuts/records/keys.md
    - .planning/phases/05-mac-menus-and-shortcuts/records/zone.md
  modified:
    - src/app/mac_check/main.cpp
    - src/app/mac_check/CocoaProbe.h
    - src/app/mac_check/CocoaProbe.mm
    - .planning/phases/05-mac-menus-and-shortcuts/records/gate-log.md

key-decisions:
  - "The pointer is placed with CGWarpMouseCursorPosition (cocoaprobe::warpPointer) instead of QCursor::setPos, which left the pointer where it was; no event is posted and no permission is needed"
  - "The zone section runs before keys and Cmd-Q is the last key of keys, with the summary flushed first, in case a termination request ends the process"
  - "No binding changed: every key fired on the first complete run"

patterns-established:
  - "Live sections record a positive control in the same run (editor focused removes the zone) next to every 'removes nothing' claim"

requirements-completed: [MAC-02, MAC-03, MAC-04, MAC-05, MAC-07]

coverage:
  - id: D1
    description: "Cmd-1 to Cmd-6, the letters O E N V K P, Cmd-N, Cmd-O, Cmd-S, Shift-Cmd-S, Cmd-W and Cmd-comma each trigger exactly their own action once on Cocoa with the German layout; Cmd-Q reaches actionQuit once"
    requirement: MAC-02
    verification:
      - kind: other
        ref: "build/scratch/mac/m08/summary.txt keys_verdict: PASS (records/keys.md)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Option-Command 1 to 5 switch the Output tabs 1 to 5 and trigger no stage action; Cmd-1 triggers the stage once and changes no tab"
    requirement: MAC-04
    verification:
      - kind: other
        ref: "build/scratch/mac/m08/summary.txt keys_output_tab_1..5 = 0..4, keys_output_tab_stage_fires 0, keys_cmd1_in_output_stage 1 (records/keys.md)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Cmd-plus and Cmd-minus zoom the page view by exactly 2^(1/6) and 2^(-1/6); Option-Command plus and minus trigger the thumbnail actions once each; the native View menu shows all keys"
    requirement: MAC-04
    verification:
      - kind: other
        ref: "build/scratch/mac/m08/summary.txt keys_zoom_plus_ratio 1.122462, keys_zoom_minus_ratio 0.890899, keys_view_menu (records/keys.md)"
        status: pass
    human_judgment: true
    rationale: "Option-Command plus and minus are taken by macOS before the app if Accessibility Zoom keyboard shortcuts are on; the harness bypasses that and cannot see the setting"
  - id: D4
    description: "Backspace and forward delete remove the zone under the pointer and the content box, B creates the box, and a focused line edit or spin box keeps the keys"
    requirement: MAC-07
    verification:
      - kind: other
        ref: "build/scratch/mac/m08/summary.txt zone_verdict: PASS (records/zone.md)"
        status: pass
      - kind: integration
        ref: "gate m08-mac-check-keys: 100% tests passed out of 7 in Release and Debug, goldens unchanged, command diff 0 / 0"
        status: pass
    human_judgment: false

duration: 12min
completed: 2026-11-15
status: complete
---

# Phase 5 Plan 5: Live window and zone keys Summary

**mac_check sections keys and zone prove on real Cocoa with the German layout that every window key, the Output tab and zoom chords, the native View menu, Cmd-Q, and Backspace and forward delete on zones and the content box work, with the pointer placed by CGWarpMouseCursorPosition because QCursor::setPos did not move it**

## Performance

- **Duration:** 12 min (measured from a start-time file)
- **Started:** 2026-11-15T09:38:18Z
- **Completed:** 2026-11-15T09:50Z
- **Tasks:** 2
- **Files modified:** 7 (three harness sources, two new records, the gate log, this summary)

## Accomplishments

- Section `keys`: a shown `MainWindow` with the production slots of the File, Settings, stage and thumbnail actions disconnected and one counter per action. Every stage key (Cmd-1 to Cmd-6 and the letters), every File and Settings key fired its own action exactly once with no cross fire; Cmd-Q reached `actionQuit` once and produced no close event. The Output tabs answer to Option-Command 1 to 5 (indices 0 to 4, no stage trigger) while Cmd-1 triggers the stage once and keeps the tab. Cmd-plus and Cmd-minus zoom by 1.122462 and 0.890899; Option-Command plus and minus trigger the thumbnail actions once; Cmd-equals also zooms on the German layout (Shift-0). The native View menu lists the six stages with Cmd-1 to Cmd-6, Zoom In, Zoom Out and the two thumbnail items with their glyphs.
- Section `zone`: Backspace and forward delete each removed the zone under the pointer (status tip `Press ⌫` proved the pointer was over it first); with the line edit `abc` focused Backspace made it `ab` and removed no zone, the spin box likewise; B created the Select Content box and Backspace and forward delete removed it. The STATE.md question is answered: Backspace reaches the shortcuts under Qt 6.12 on Cocoa.
- Records `keys.md` and `zone.md` hold the raw lines, the verdict rules, the no-physical-input statement and the findings; one gated harness commit (`m08-mac-check-keys`, Release and Debug 100% of 7, goldens unchanged, command diff 0 / 0).

## Task Commits

1. **Task 1: sections keys and zone, run live, gated** - `b240dcd1` (test); gate record `c57fdd79`
2. **Task 2: records keys.md and zone.md** - `c9c923dd` (docs)

**Plan metadata:** the SUMMARY commit follows this file.

## Files Created/Modified

- `src/app/mac_check/main.cpp` - sections `keys` and `zone`, `ActionCounter`, `CloseSwallower`, key and window helpers, copies of the zone and content-box helpers of `tst_mac_qt.cpp`
- `src/app/mac_check/CocoaProbe.h`, `CocoaProbe.mm` - `warpPointer`
- `records/keys.md`, `records/zone.md`, `records/gate-log.md`

## Decisions Made

- Pointer placement by `CGWarpMouseCursorPosition` (see deviation 1).
- Section order `menubar`, `layout`, `zone`, `keys`; Cmd-Q last, summary flushed before it.
- No production binding changed; the first complete run passed every key row.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] QCursor::setPos did not move the pointer on Cocoa**
- **Found during:** Task 1 (first live run of `zone`)
- **Issue:** the plan places the pointer with `QCursor::setPos` ("no privacy permission"). In the harness `QCursor::pos()` stayed at `1608,769` before and after the call (target `1280,651`, read again after 300 ms and 1.3 s), so the zone was never under the pointer: the interaction recomputes proximity from `QCursor::pos()` on every repaint, the status tip stayed the default one and the keys removed nothing. A standalone C program using `CGWarpMouseCursorPosition` moved the pointer with no permission prompt.
- **Fix:** new `cocoaprobe::warpPointer(QPoint)` (declared in `CocoaProbe.h`, implemented in `CocoaProbe.mm`) calls `CGWarpMouseCursorPosition` and `CGAssociateMouseAndMouseCursorPosition(true)`; the zone section checks `QCursor::pos()` against the target (`zone_pointer_placed`). This departs from the prohibition's wording "the pointer is placed with QCursor::setPos only" but keeps its intent: no event is posted (no `CGEventPost`, no event tap in `CocoaProbe.mm`, count 0), no event goes to another application, no permission is needed. Reported here so the orchestrator can accept or reject the wording change; plan 05-06 (context menus) needs the same placement.
- **Files modified:** `src/app/mac_check/CocoaProbe.h`, `CocoaProbe.mm`, `main.cpp`
- **Verification:** `zone_pointer_placed: yes`, `zone_pointer_tracked: yes`, `zone_verdict: PASS`; gate `m08-mac-check-keys` PASS
- **Committed in:** `b240dcd1`

**2. [Rule 3 - Blocking] CocoaProbe API gained one function**
- **Found during:** Task 1
- **Issue:** the interface list says "none required", but deviation 1 needs a probe function.
- **Fix:** `warpPointer`, nothing else.
- **Committed in:** `b240dcd1`

---

**Total deviations:** 2 auto-fixed (1 bug in the plan's pointer method, 1 blocking API gap)
**Impact on plan:** none on scope or on any production binding; the harness commit is the single gated commit the plan asks for.

## Issues Encountered

- The first complete run printed `qt.qpa.keymapper: Could not map key to character` lines for AppKit housekeeping events (`MouseEntered`, `KitDefined`); they come from Qt's key mapper and change no result.
- `home_redirected: no` in these sections, as in the `menubar` and `layout` runs of 05-02: the section settings stay under the output directory, the account home is not redirected. Only `identity` needs the redirect.
- The user's pointer was moved by the harness during the `zone` section (planned, D-11).

## User Setup Required

None.

## Known Stubs

None.

## Threat Flags

None. T-05-12 (events only through `[NSApp sendEvent:]`, no `CGEventPost` or event tap: count 0 in `CocoaProbe.mm`, the pointer warp posts no event), T-05-13 (production slots disconnected, close events swallowed, settings below `build/scratch/mac/m08`) and T-05-24 (`grep -c "$HOME"` prints 0 in both records) are in place.

## Next Phase Readiness

- Plan 05-06 can reuse `warpPointer`, the copied helpers `makeZoneEditor`, `addRectZone`, `zoneCount` and the content-box window in `main.cpp`, and the `showAndActivate` helper.
- Open point for the user: Option-Command plus and minus for the thumbnails are intercepted by macOS if Accessibility Zoom keyboard shortcuts are on (`records/keys.md`); the thumbnail buttons stay available.

## Self-Check: PASSED

- Files present: `src/app/mac_check/main.cpp`, `CocoaProbe.h`, `CocoaProbe.mm`, `records/keys.md`, `records/zone.md`, `records/gate-log.md`.
- Commits `b240dcd1`, `c57fdd79`, `c9c923dd` are ancestors of HEAD; `git rev-list --count 97b7f5f1..HEAD` printed 3 before this file.
- Gate `m08-mac-check-keys`: `result: PASS`, `goldens: unchanged`, command diff 0 / 0, tested commit `b240dcd1`; output-path guard against `f6cc4457` prints nothing; clang-format dry run clean on the three harness files.

---
*Phase: 05-mac-menus-and-shortcuts*
*Completed: 2026-11-15*
