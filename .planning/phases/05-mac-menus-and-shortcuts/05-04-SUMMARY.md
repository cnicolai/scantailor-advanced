---
phase: 05-mac-menus-and-shortcuts
plan: 04
subsystem: ui
tags: [qt6, qshortcut, zones, select-content, shortcuts, macos]

requires:
  - phase: 05-mac-menus-and-shortcuts
    provides: TestMacQt class and gate log (05-01); emptyImageFrame helpers and stage keys (05-03)
provides:
  - Backspace (the Mac delete key) and forward Delete each remove the zone under the pointer
  - B and Insert create the Select Content box, Backspace and Delete remove it; the context menu shows B and Backspace
  - Zone and Select Content status tips with the Mac key symbols
  - Six permanent offscreen tests, including the proof that a focused spin box or line edit keeps the keys
affects: [05-05, 05-06, 05-08, 05-11]

actuals:
  tokens: 5300
  tasks: 2
  commits: 4

plan_head_before: 9c1661033159fd9cd9d11381563dbff717efb51b
plan_head_after: 309649c295b585217faee2821c08e657499a8961
commits: 4

tech-stack:
  added: []
  patterns:
    - "One QShortcut per key sequence for zones (m_removeZoneShortcut, m_removeZoneBackspaceShortcut); one setShortcuts list per QAction for the content box"
    - "Right-click tests go through the window handle (QTest::mouseClick(QWindow*, ...)) so QWidgetWindow synthesises the QContextMenuEvent"

key-files:
  created: []
  modified:
    - src/core/zones/ZoneDefaultInteraction.h
    - src/core/zones/ZoneDefaultInteraction.cpp
    - src/core/filters/select_content/ImageView.cpp
    - src/qt_tests/tst_mac_qt.h
    - src/qt_tests/tst_mac_qt.cpp
    - .planning/phases/05-mac-menus-and-shortcuts/records/gate-log.md

key-decisions:
  - "Both shortcuts stay window-context shortcuts, so a focused field takes Backspace and Delete first through ShortcutOverride; no extra guard in production code"
  - "No new guard for Backspace during a zone drag: it behaves as Delete did before this phase (backstop truth)"
  - "No Qt::RightButton special case anywhere; the platform path already reaches contextMenuEvent"

patterns-established:
  - "Test mechanics for scroll-area views: focus may sit on the view or its viewport, so tests use focusIsIn(view) and send keys to QApplication::focusWidget()"

requirements-completed: [MAC-07, MAC-08, MAC-06]

coverage:
  - id: D1
    description: "Backspace and Delete remove the zone under the pointer; a second press removes nothing; nothing under the pointer removes nothing"
    requirement: MAC-07
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testZoneRemoveKeys"
        status: pass
    human_judgment: false
  - id: D2
    description: "With a spin box or line edit focused, Backspace and Delete edit the field and remove no zone; the focused editor removes it (positive control)"
    requirement: MAC-07
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testZoneKeysIgnoredInTextFields"
        status: pass
    human_judgment: false
  - id: D3
    description: "B and Insert create, Backspace and Delete remove the content box; repeated keys and keys during a captured interaction or in a spin box change nothing"
    requirement: MAC-07
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testContentBoxKeys, #testContentMenuShowsKeys"
        status: pass
    human_judgment: false
  - id: D4
    description: "Zone and Select Content status tips use the Mac key symbols"
    requirement: MAC-06
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testZoneStatusTips"
        status: pass
    human_judgment: false
  - id: D5
    description: "A right-button click on a zone shows Properties and Delete; on the Select Content view shows Remove Content Box"
    requirement: MAC-08
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testRightClickOpensZoneMenu"
        status: pass
      - kind: integration
        ref: "gates m06-zone-keys and m07-content-box-keys: 100% tests passed out of 7 in Release and Debug, goldens unchanged, command diff 0 / 0"
        status: pass
    human_judgment: true
    rationale: "Offscreen the platform path ends in QContextMenuEvent; the native Cocoa behaviour of a right click (and Control-click) is plan 05-06's live check."

duration: resumed run (earlier interrupted run not timed)
completed: 2026-11-15
status: complete
---

# Phase 5 Plan 4: Zone and content-box keys Summary

**Backspace and fn+delete both remove the zone under the pointer (second QShortcut), B/Insert create and Backspace/Delete remove the Select Content box (one setShortcuts list per action), and the tips name the Mac keys; a focused field keeps the keys**

## Performance

- **Duration:** not measured. An earlier executor was interrupted after writing Task 1's code; this run checked it, completed Task 1 and did Task 2.
- **Completed:** 2026-11-15
- **Tasks:** 2
- **Files modified:** 6 (five under `src/`, the gate log)

## Accomplishments

- `ZoneDefaultInteraction` holds `m_removeZoneBackspaceShortcut` (Qt::Key_Backspace) next to the existing Delete shortcut; both call `removeZoneUnderMouse`. The three zone tips read exactly as the plan states (⌘, ⇧, ⇧⌘, ⌫, ⌥⌘).
- `select_content::ImageView`: create has {B, Insert}, remove has {Backspace, Delete}, so the menu draws B and the delete symbol; the default tip says "Hold ⇧ to drag a box.". The early returns for an existing box, a missing box and a captured interaction are unchanged.
- Six new `TestMacQt` tests (`testZoneRemoveKeys`, `testZoneKeysIgnoredInTextFields`, `testZoneStatusTips`, `testContentBoxKeys`, `testContentMenuShowsKeys`, `testRightClickOpensZoneMenu`); `qt_tests` now runs 22 TestMacQt tests, all passing.

## Task Commits

1. **Task 1: zone removal keys and tips** - `68e14287` (feat); gate record `f25cf8e1`
2. **Task 2: content-box keys and right-click menus** - `45495643` (feat); gate record `309649c2`

**Plan metadata:** the SUMMARY commit follows this file.

## Files Created/Modified

- `src/core/zones/ZoneDefaultInteraction.h`, `.cpp` - second removal shortcut, Mac tips
- `src/core/filters/select_content/ImageView.cpp` - two `setShortcuts` lists, Shift tip
- `src/qt_tests/tst_mac_qt.h`, `tst_mac_qt.cpp` - six tests; helpers `makeZoneEditor`, `zonePage`, `addRectZone`, `zoneCount`, `hasZoneAt`, `pointAt`, `viewportPointOf`, `keyToFocusWidget`, `focusIsIn`, `ZoneWindow`, `ContentWindow`, `visibleMenuOf`, `rightClickThroughWindow`
- `records/gate-log.md` - rows `m06-zone-keys`, `m07-content-box-keys`

## Decisions Made

- Window-context shortcuts kept; the field-focus protection comes from Qt's ShortcutOverride, proved for a spin box and a line edit.
- Backspace during a zone drag is not guarded (backstop truth); the content box already ignores keys while captured, and a test shows it.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Focus check compared the viewport instead of the scroll area**
- **Found during:** Task 1 (first run of the leftover tests)
- **Issue:** `QApplication::focusWidget()` is the editor (the scroll area), not its viewport, so `QTRY_VERIFY(focusWidget == viewport)` failed in `testZoneRemoveKeys` and `testZoneKeysIgnoredInTextFields`.
- **Fix:** helper `focusIsIn(view)` accepts the view or any descendant; keys still go to `QApplication::focusWidget()`.
- **Files modified:** `src/qt_tests/tst_mac_qt.cpp`
- **Commit:** `68e14287`

**2. [Rule 1 - Bug] Right click through the widget sent no context-menu event**
- **Found during:** Task 2
- **Issue:** `QTest::mouseClick(QWidget*, ...)` delivers the event straight to the widget and skips `QWidgetWindow`, which creates the `QContextMenuEvent`; no menu appeared.
- **Fix:** the test clicks through `windowHandle()` with window coordinates (`rightClickThroughWindow`). No production change.
- **Files modified:** `src/qt_tests/tst_mac_qt.cpp`
- **Commit:** `45495643`

---

**Total deviations:** 2 auto-fixed (test mechanics only)
**Impact on plan:** none on scope; production code is as the plan specifies.

## Issues Encountered

- The first executor's Task 1 code was complete and correct against the plan; only the test focus check needed the fix above. Both gates passed on the first run. The draft `build/scratch/gsd/imageview-task2.patch` matched the plan and was applied unchanged.
- **Open point for the user (manual):** the native right click and Control-click on a zone or the content box on Cocoa are checked by plan 05-06's harness, not here.

## User Setup Required

None.

## Known Stubs

None.

## Threat Flags

None. T-05-10 (delete while typing): `testZoneKeysIgnoredInTextFields` and `testContentBoxKeys` show a spin box and a line edit keep Backspace and Delete. T-05-11 (key during drag): content box tested with a `Captor`; zones unchanged from the Delete behaviour.

## Next Phase Readiness

- Plan 05-05's live harness can drive Backspace and fn+delete on a zone and B/Backspace on the content box.
- The zone tips and the Select Content tip no longer hold PC key names; plan 05-08's scan should find nothing in these two files.

## Self-Check: PASSED

- Files present: `ZoneDefaultInteraction.h`, `ZoneDefaultInteraction.cpp`, `select_content/ImageView.cpp`, `tst_mac_qt.h`, `tst_mac_qt.cpp`, `records/gate-log.md`.
- Commits `68e14287`, `f25cf8e1`, `45495643`, `309649c2` are ancestors of HEAD; `git rev-list --count 9c166103..HEAD` printed 4 before this file.
- Gates `m06-zone-keys` and `m07-content-box-keys`: `result: PASS`, `goldens: unchanged`, command diff 0 / 0; output-path guard prints nothing; no `RightButton` in the three view files.

---
*Phase: 05-mac-menus-and-shortcuts*
*Completed: 2026-11-15*
