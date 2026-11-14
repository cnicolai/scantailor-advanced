# German keyboard layout chords (plan 05-02)

Live measurement of which key chords fire on the German keyboard layout, taken on 2026-11-14 with the harness `mac_check` (section `layout`). It decides the chord groups that plan 05-03 binds for the Output tabs and the thumbnail keys (D-02, D-06, D-07).

- Command (from `{repo}`): `build/macos-release/mac_check build/scratch/mac/m03 --section menubar,layout -ApplePersistenceIgnoreState YES`
- Platform: cocoa, Qt 6.12.0, macOS 26.7.2, Apple Silicon, one Studio Display (window device pixel ratio 2); the only enabled keyboard layout is `com.apple.keylayout.German`.
- The harness process stands in for `scantailor-advanced`, which is not launched (home-folder rule). Settings, temporary files and the summary stay below `build/scratch/mac/m03`.
- No key or click was physically pressed: every event was synthesised inside the harness process and sent to its own probe window through `[NSApp sendEvent:]`. No event went to another application, no `CGEventPost`, no event tap, and no Accessibility, Input Monitoring or screen-recording permission was used (D-11).

## How the events were built

A probe `QWidget` (300 by 200, strong focus, no editable child) holds one `QAction` per chord with the sequence shown in the table (window shortcut context). For every chord the harness
1. looked up the virtual key code in the current layout with `UCKeyTranslate` (`keycode_*` lines; the keypad block is skipped, see below),
2. built the characters the way AppKit reports them (`UCKeyTranslate` with the Shift, Option and Control part of the modifiers; Command ignored; Backspace as U+007F, Forward Delete as the function-key character),
3. sent key down and key up with the chord's modifiers to the probe window with `[NSApp sendEvent:]`, waited 300 ms with event processing, and counted how often the chord's own action and every other probe action triggered.

Qt's own `QTest::keyClick` bypasses the Cocoa key mapper that matches Option chords, so only this NSEvent-level input answers the German-layout question.

## First attempt, not a measurement

The first run of this section started while the screen was locked (`frontmost_app: loginwindow`, `app_active: no`); every chord counted 0. It says nothing about the layout. The same run also showed that `keyCodeFor` found the keypad `=` key (code 81) for `=` before the real key, so the lookup was fixed to scan the main keyboard only (commit `fix(05-02): skip the keypad in the layout key lookup and report the frontmost app`). The run below is the one after that fix with an unlocked session (`frontmost_app: VLC`, `app_active: yes`).

## Raw lines of the layout section

```
keyboard_layout: com.apple.keylayout.German
keycode_1: 18
keycode_2: 19
keycode_3: 20
keycode_4: 21
keycode_5: 23
keycode_6: 22
keycode_plus: 30
keycode_minus: 44
keycode_equal: 29
keycode_equal_shift: yes
keycode_b: 11
chord_cmd_1: 1
chord_cmd_2: 1
chord_cmd_3: 1
chord_cmd_4: 1
chord_cmd_5: 1
chord_cmd_6: 1
chord_opt_cmd_1: 1
chord_opt_cmd_2: 1
chord_opt_cmd_3: 1
chord_opt_cmd_4: 1
chord_opt_cmd_5: 1
chord_ctrl_cmd_1: 1
chord_ctrl_cmd_2: 1
chord_ctrl_cmd_3: 1
chord_ctrl_cmd_4: 1
chord_ctrl_cmd_5: 1
chord_cmd_plus: 1
chord_cmd_minus: 1
chord_cmd_equal: 1
chord_opt_cmd_plus: 1
chord_opt_cmd_minus: 1
chord_opt_cmd_equal: 1
chord_ctrl_cmd_plus: 1
chord_ctrl_cmd_minus: 1
chord_backspace: 1
chord_forward_delete: 1
chord_b: 1
cross_fires: 0
cross_fires_required: 0
option_command_digits: works
option_command_plus_minus: works
control_command_digits: works
control_command_plus_minus: works
output_tabs_group: option-command
thumbnails_group: option-command
command_equal_note: cmd_equal fired 1 time(s); on the German layout = is Shift-0, so Cmd-= is a US-layout binding (D-06) and not part of the verdict
layout_verdict: PASS
```

(The `sequence_<name>` lines of the summary are the second column of the table; the section order of the summary puts `keyboard_layout` in the header.)

## Chords

| Chord | Sequence (native text) | Fired (own action) |
|-------|------------------------|--------------------|
| cmd_1 | ⌘1 | 1 |
| cmd_2 | ⌘2 | 1 |
| cmd_3 | ⌘3 | 1 |
| cmd_4 | ⌘4 | 1 |
| cmd_5 | ⌘5 | 1 |
| cmd_6 | ⌘6 | 1 |
| opt_cmd_1 | ⌥⌘1 | 1 |
| opt_cmd_2 | ⌥⌘2 | 1 |
| opt_cmd_3 | ⌥⌘3 | 1 |
| opt_cmd_4 | ⌥⌘4 | 1 |
| opt_cmd_5 | ⌥⌘5 | 1 |
| ctrl_cmd_1 | ⌃⌘1 | 1 |
| ctrl_cmd_2 | ⌃⌘2 | 1 |
| ctrl_cmd_3 | ⌃⌘3 | 1 |
| ctrl_cmd_4 | ⌃⌘4 | 1 |
| ctrl_cmd_5 | ⌃⌘5 | 1 |
| cmd_plus | ⌘+ | 1 |
| cmd_minus | ⌘- | 1 |
| cmd_equal | ⌘= | 1 |
| opt_cmd_plus | ⌥⌘+ | 1 |
| opt_cmd_minus | ⌥⌘- | 1 |
| opt_cmd_equal | ⌥⌘= | 1 |
| ctrl_cmd_plus | ⌃⌘+ | 1 |
| ctrl_cmd_minus | ⌃⌘- | 1 |
| backspace | ⌫ | 1 |
| forward_delete | ⌦ | 1 |
| b | B | 1 |

Every chord fired its own action exactly once and no other probe action fired (`cross_fires: 0`).

## Decisions

- `output_tabs_group: option-command`. ⌥⌘1 to ⌥⌘5 fire exactly once each on the German layout, so plan 05-03 binds the Output tabs with ⌥⌘1 to ⌥⌘5 (D-02). The ⌃⌘ digits also fire; they stay the UI-SPEC fallback and are not needed.
- `thumbnails_group: option-command`. ⌥⌘+ and ⌥⌘- fire exactly once each, so plan 05-03 binds the thumbnail keys with ⌥⌘+ and ⌥⌘- (D-07).
- ⌘1 to ⌘6, ⌘+, ⌘-, Backspace, Forward Delete and B all fire once each; the stage keys, View menu zoom keys and the zone and content-box keys of plans 05-03 and 05-04 work on this layout.
- ⌘= note: on the German layout the `=` character is Shift-0 (key code 29 with Shift). ⌘= fired once (Qt matches `Ctrl+=` against ⇧⌘0), but the key is a US-layout binding (D-06); it is not part of the verdict.
- Accessibility Zoom caveat (RESEARCH Pitfall 10): with "Use keyboard shortcuts to zoom" on in System Settings > Accessibility > Zoom, macOS takes ⌥⌘+ and ⌥⌘- before any application sees them. The harness sends its events to its own window, which bypasses that, and cannot see the setting; reading it would mean reading `~/Library`, which is not allowed. The measurement therefore says the chords reach the application when they are delivered, not that the setting is off. If ⌥⌘+ and ⌥⌘- do nothing in daily use, check that setting first.
