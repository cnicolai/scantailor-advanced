# Live context-menu triggers (plan 05-06)

Live measurement on the real Cocoa platform, taken on 2026-11-15 with the harness `mac_check` (section `context`): do a right-click and a Control-click on a zone each deliver exactly one context-menu event and show the zone menu, does the first click on `Delete` remove the zone, does Escape dismiss the menu without changing anything, and is the 200 ms deferral in `ZoneContextMenuInteraction::menuAboutToHide` still needed (MAC-08, UI-SPEC E9)?

- Command (from `{repo}`): `build/macos-release/mac_check build/scratch/mac/m09 --section context -ApplePersistenceIgnoreState YES` (the recorded run was written to `build/scratch/mac/m09b` and copied to `m09`; see "Runs"). After the deferral was removed the same section was run into `build/scratch/mac/m09d` (the run `m09f`, copied).
- Platform: cocoa, Qt 6.12.0, macOS 26.7.2, Apple Silicon, keyboard layout `com.apple.keylayout.German`. The account's home directory is not redirected (`home_redirected: no`).
- No key or click was physically pressed: every mouse and key event was synthesised inside the harness process and sent to its own windows through `[NSApp sendEvent:]`; no event went to another application and no Accessibility, Input Monitoring or screen-recording permission was used. The pointer was moved by the harness with `cocoaprobe::warpPointer` (`CGWarpMouseCursorPosition`, no event posted, no permission) in place of the `QCursor::setPos` the plan names, because `QCursor::setPos` did not move the pointer (finding in `zone.md`); the zone menu is looked up through `QCursor::pos()`, so the pointer has to be where the event says (D-11, D-12).

## Verdict rule of `context_verdict`

PASS when all hold: the window is active and the pointer was placed; the zone was tracked (status tip `Press ⌫`) before each trigger; the right-click and the Control-click each delivered exactly one `QEvent::ContextMenu` to the editor's viewport and showed a visible `QMenu` on an attempt that started with the pointer confirmed on the target; the first click on `Delete` removed the zone for both; Escape hid a menu opened the same way, left the zone count unchanged and the default interaction came back (the zone tip showed again). The `context_two_finger_click` line is not part of the verdict.

An attempt is sent only when `QCursor::pos()` is within 3 pixels of the warped target (the harness waits after the warp, re-warps every 100 ms for at most 1 s, and checks again right before the click). A trigger gets at most three attempts. A miss with the pointer off the target is counted in `context_pointer_interference`; a miss with the pointer on the target is a real failure, counted in `context_failed_attempts_pointer_confirmed`. The verdict and the deferral decision rest on confirmed attempts only: the 20-run order below counts a run only when its attempt was confirmed (up to five unconfirmed runs are repeated).

## Raw lines of the `context` section (run `m09`, commit `26486819`)

This run predates the interference counters; it had the post-warp wait (`context_cursor_lagged: 0`) and a single retry (`context_retries: 0`), so no trigger was repeated.

```
platform: cocoa
qt_runtime: 6.12.0
section: context
keyboard_layout: com.apple.keylayout.German
home_directory: ~
home_redirected: no
context_app_active: yes
context_right_click_tracked: yes
context_right_click_events: 1
context_right_click_menu_shown: yes
context_right_click_first_click_deleted: yes
context_right_click_triggered_before_queued_hide: yes
context_control_click_tracked: yes
context_control_click_events: 1
context_control_click_menu_shown: yes
context_control_click_first_click_deleted: yes
context_control_click_triggered_before_queued_hide: yes
context_menu_kind: qt
context_dismiss_menu_shown: yes
context_dismiss_via: menu window
context_dismiss_menu_hidden: yes
context_dismiss_zones_unchanged: yes
context_dismiss_default_restored: yes
context_order_runs: 20
context_order_hide_ran: 20
context_order_deleted: 20
context_triggered_before_queued_hide_count: 20
context_two_finger_click: same rightMouseDown event as right_click; not distinguishable in-process; unverified residual
context_pointer_placed: yes
context_cursor_lagged: 0
context_cursor_unconfirmed: 0
context_retries: 0
context_delay_decision: remove
context_verdict: PASS
verdict: PASS
```

## Raw lines of the re-run after the deferral was removed (run `m09d`)

Harness of the commit `test(05-06): detect pointer interference in the live context check`, built together with the code without the deferral.

```
platform: cocoa
qt_runtime: 6.12.0
section: context
keyboard_layout: com.apple.keylayout.German
home_directory: ~
home_redirected: no
context_app_active: yes
context_right_click_tracked: yes
context_right_click_events: 1
context_right_click_menu_shown: yes
context_right_click_pointer_confirmed: yes
context_right_click_retries: 0
context_right_click_first_click_deleted: yes
context_right_click_triggered_before_queued_hide: yes
context_control_click_tracked: yes
context_control_click_events: 1
context_control_click_menu_shown: yes
context_control_click_pointer_confirmed: yes
context_control_click_retries: 0
context_control_click_first_click_deleted: yes
context_control_click_triggered_before_queued_hide: yes
context_menu_kind: qt
context_dismiss_menu_shown: yes
context_dismiss_pointer_confirmed: yes
context_dismiss_retries: 0
context_dismiss_via: menu window
context_dismiss_menu_hidden: yes
context_dismiss_zones_unchanged: yes
context_dismiss_default_restored: yes
context_order_runs: 20
context_order_attempted: 20
context_order_hide_ran: 20
context_order_deleted: 20
context_triggered_before_queued_hide_count: 20
context_two_finger_click: same rightMouseDown event as right_click; not distinguishable in-process; unverified residual
context_pointer_placed: yes
context_cursor_lagged: 0
context_cursor_unconfirmed: 0
context_retries: 0
context_pointer_interference: 0
context_failed_attempts_pointer_confirmed: 0
context_delay_decision: remove
context_verdict: PASS
verdict: PASS
```

## Order of `triggered()` and the queued `aboutToHide` slot

The menu is a Qt-drawn `QMenu` (`context_menu_kind: qt`) on Cocoa with Qt 6.12.0. In 20 runs of right-click plus a click on `Delete` the `Delete` action's `triggered()` had fired before the queued `aboutToHide` slot ran in 20 of 20 (`context_triggered_before_queued_hide_count: 20`, hide slot ran 20, zone deleted 20), in the recorded run and again in the re-run (20 confirmed of 20 attempted, no interference). Earlier diagnostic runs agree where a menu showed (10 of 10 and 18 of 18) but are contaminated, see below, and are not used for the decision.

## `context_delay_decision: remove`

The rule of the plan (20 of 20 with a Qt-drawn menu) is met on confirmed runs, so the 200 ms deferral and its member `m_extraDelaysDone` were removed in `fix(05-06): drop the context menu hide delay`, with the offscreen test `testZoneMenuDismissRestoresDefaultAtOnce` (the menu object is gone within 100 ms of hiding it and the zone count is unchanged). The re-run `m09d` has verdict PASS, 20 of 20 again, pointer interference 0, and the dismissal restored the default interaction. The comment about OS X in the old code described a native menu; the menu here is not native.

## Runs and the contaminated diagnostic runs

Six live runs of the section were made in total; the person at the Mac was using the mouse and keyboard during the two earliest ones.

- `m09` and `m09dbg` (diagnostic, made before the pointer checks existed, verdict FAIL): exactly one `QEvent::ContextMenu` reached the viewport for every trigger, but for some triggers (10 of 20 order runs in one, 2 of 20 in the other, plus the first right-click and the Control-click) no `QMenu` was created: `ZoneContextMenuInteraction::create` found no zone under `QCursor::pos()`, which it reads when the menu is built. The user reports having used the mouse and keyboard during these runs, so a pointer taken off the zone by the user explains the misses; **these two runs are treated as contaminated, not as evidence of an application defect, and are not recorded as results.** (Their raw output is in `build/scratch/mac/m09-run.txt` and `m09dbg-run.txt`, outside the repository history.)
- `m09b` (recorded as `m09`): PASS, lagged 0, retries 0.
- first re-run after the removal: the dismissal step, which then had no retry, saw no menu: FAIL; `QCursor::pos()` had lagged the warp in 2 placements, and the person may have moved the pointer. Not recorded as a result (kept as `m09d-fail`).
- second re-run after the removal (`m09e`): PASS, lagged 0, retries 0.
- third re-run after the removal (`m09f`, recorded as `m09d`), with interference detection: PASS, `context_pointer_interference: 0`, `context_failed_attempts_pointer_confirmed: 0`, every trigger `pointer_confirmed: yes`, retries 0.

The application code was not changed for the misses (the zone lookup through `QCursor::pos()` is untouched). Whether a real miss exists with an undisturbed pointer is not shown: no run with an undisturbed pointer produced one (3 of 3 passing runs), and the failing runs are explained by the pointer being disturbed or not yet where it was warped. A change of `zonesUnderMouse` to use the event position would be the application remedy and would go back to plan 05-04 as a deviation.

## Unverified residual: two-finger click

A two-finger trackpad click reaches the app as the same `rightMouseDown` event as a right-click, so the right-click row stands for it. No separate two-finger event was injected and nobody clicked; this is the one MAC-08 row that rests on that equivalence, not on its own measurement.
