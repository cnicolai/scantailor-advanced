# Live zone and content-box keys (plan 05-05)

Live measurement on the real Cocoa platform with the German keyboard layout, taken on 2026-11-15 with the harness `mac_check` (section `zone`): do the Mac delete key (Backspace, key code `kVK_Delete`) and the forward delete key (`kVK_ForwardDelete`) reach the zone and content-box shortcuts of plan 05-04, does B create the content box, and does a focused line edit or spin box keep its keys (MAC-07, D-10)?

- Command (from `{repo}`): `build/macos-release/mac_check build/scratch/mac/m08 --section keys,zone -ApplePersistenceIgnoreState YES`. The sibling record `keys.md` holds the `keys` lines of the same run.
- Platform: cocoa, Qt 6.12.0, macOS 26.7.2, Apple Silicon, keyboard layout `com.apple.keylayout.German`. Settings and temporary files stay below `{repo}/build/scratch/mac/m08`; the account's home directory is not redirected (`home_redirected: no`), as in the earlier sections.
- No key or click was physically pressed: every key and mouse-move event was synthesised inside the harness process and sent to its own windows through `[NSApp sendEvent:]`; no event went to another application and no Accessibility, Input Monitoring or screen-recording permission was used. The pointer was moved by the harness with `CGWarpMouseCursorPosition` (it posts no event and needs no permission), because the zone interaction reads the pointer from `QCursor::pos()` on every repaint (D-11, D-12).

## Pointer placement (a finding)

The plan placed the pointer with `QCursor::setPos`. In the first run it did nothing: `QCursor::pos()` stayed at the position the real pointer already had (`1608,769` before and after, 1.3 s later) although the point asked for was `1280,651`, so the zone was never under the pointer and the status tip stayed the default one. A standalone C program with `CGWarpMouseCursorPosition` moved the pointer without any permission prompt, so `cocoaprobe::warpPointer` (new in `CocoaProbe.mm`, no event posted, no event tap) now places the pointer and `zone_pointer_placed` checks `QCursor::pos()` against the target (within 12 pixels of Manhattan distance). The synthesised mouse-move event then reaches the viewport at the expected position and the interaction state shows the zone tip.

## Verdict rule of `zone_verdict`

PASS when all hold: the window is active; the pointer was placed and tracked (`zone_pointer_tracked`: the interaction's status tip contains `Press ⌫`, which proves the pointer is over the zone before a key is sent); Backspace removes the zone under the pointer; forward delete removes a zone added again under the pointer; with the line edit (text `abc`, cursor at the end) focused Backspace removes no zone and the text becomes `ab`; with the spin box focused Backspace removes no zone; in a `select_content::ImageView` without a box B creates the box, Backspace removes it, B creates it again and forward delete removes it (four `manualContentRectSet` signals).

## Raw lines of the `zone` section

```
zone_app_active: yes
zone_pointer_tracked: yes
zone_pointer_placed: yes
zone_backspace_removed: yes
zone_forward_delete_removed: yes
zone_backspace_in_line_edit_removed: no
zone_line_edit_text: ab
zone_backspace_in_spin_box_removed: no
zone_pointer_tracked_later: 2 of 2
content_app_active: yes
content_b_created: yes
content_backspace_removed: yes
content_forward_delete_removed: yes
content_events: 4
zone_verdict: PASS
```

## Interpretation

- Answer to the question in STATE.md: yes, Backspace reaches the zone and content-box shortcuts under Qt 6.12 on Cocoa. The Mac delete key (key code `kVK_Delete`, character U+007F) removed the zone under the pointer and the Select Content box, and forward delete (`kVK_ForwardDelete`) did the same, each exactly once.
- B created the content box once (`content_events: 4` for B, Backspace, B, forward delete, each a signal).
- Field protection (T-05-10). With the line edit focused, Backspace edited its text (`abc` became `ab`) and removed no zone although the pointer was still over it (`zone_pointer_tracked_later: 2 of 2` shows the pointer was over a zone for both later steps); the focused spin box removed none either. Qt's `ShortcutOverride` hands the key to the field first, so no extra guard in production code is needed (plan 05-04 decision).
- The positive controls are in the same run: the editor with focus removes the zone with both keys, so "no zone removed" in the field cases is not a missed event.
