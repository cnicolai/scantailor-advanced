# Live window keys (plan 05-05)

Live measurement of the window keys of a shown `MainWindow` on the real Cocoa platform with the German keyboard layout, taken on 2026-11-15 with the harness `mac_check` (section `keys`). It answers what the offscreen tests of plans 05-01, 05-03 and 05-04 cannot: does every window key reach exactly its action through Qt's Cocoa key mapper and the native menu key equivalents (MAC-02, MAC-03, MAC-04, MAC-05; D-01 to D-08)?

- Command (from `{repo}`): `build/macos-release/mac_check build/scratch/mac/m08 --section keys,zone -ApplePersistenceIgnoreState YES`. The sibling record `zone.md` holds the `zone` lines of the same run.
- Platform: cocoa, Qt 6.12.0, macOS 26.7.2, Apple Silicon, keyboard layout `com.apple.keylayout.German` (the harness refuses to call the section PASS on another layout).
- The harness process `mac_check` stands in for `scantailor-advanced`, which is not launched (home-folder rule). Its `QSettings` file and temporary files stay below `{repo}/build/scratch/mac/m08`; the account's home directory itself is not redirected in this section, as in the `menubar` and `layout` runs of plan 05-02 (`home_redirected: no`).
- No key or click was physically pressed. Every key event was synthesised inside the harness process and sent to its own `MainWindow` through `[NSApp sendEvent:]`; no event went to another application, nothing was posted system-wide and no Accessibility, Input Monitoring or screen-recording permission was used. In the `keys` section the harness did not move the pointer at all; in the `zone` section it moved the pointer with `CGWarpMouseCursorPosition` (see `zone.md`) (D-11, D-12).
- Production slots of New, Open, Save, Save As, Close Project, Quit, Settings, the six stage actions and the two thumbnail actions were disconnected before any key was counted (they would open dialogs or panels, change the stage or close the window); the two zoom actions stay connected because their effect on the page view is measured. A close-event filter on the window counts `QEvent::Close` and ignores it, so Cmd-Q can be observed without ending the run. Cmd-Q is the last key of the section.

## Verdict rule of `keys_verdict`

PASS when all hold: the application is active and the layout is German; each of Cmd-1 to Cmd-6 and each of the letters O, E, N, V, K, P (stage keys), Cmd-N, Cmd-O, Cmd-S, Shift-Cmd-S, Cmd-W and Cmd-comma trigger their own action exactly once and no other counted action; Cmd-Q gave `keys_quit_triggered: 1` or at least one close event; with an Output `TabbedImageView` of five tabs in the main area the digits with Option-Command (the chord group of `records/layout.md`) set the tab index to 0 to 4 and trigger no stage action, and Cmd-1 triggers the stage action once and changes no tab; Cmd-plus multiplies the zoom of the page view by 2^(1/6) (1.122462) and Cmd-minus by 2^(-1/6) (0.890899), both within 0.000002; Option-Command plus and minus trigger Larger Thumbnails and Smaller Thumbnails once each and leave the page zoom alone; the native View menu lists the six stage names with Cmd-1 to Cmd-6, Zoom In Cmd-plus, Zoom Out Cmd-minus, Larger Thumbnails Option-Command-plus and Smaller Thumbnails Option-Command-minus.

## Raw lines of the `keys` section

```
keys_app_active: yes
keys_layout_german: yes
keys_stage_cmd_1: 1
keys_stage_cmd_2: 1
keys_stage_cmd_3: 1
keys_stage_cmd_4: 1
keys_stage_cmd_5: 1
keys_stage_cmd_6: 1
keys_stage_letter_O: 1
keys_stage_letter_E: 1
keys_stage_letter_N: 1
keys_stage_letter_V: 1
keys_stage_letter_K: 1
keys_stage_letter_P: 1
keys_new: 1
keys_open: 1
keys_save: 1
keys_save_as: 1
keys_close: 1
keys_settings: 1
keys_close_events_before_quit: 0
keys_output_tab_1: 0
keys_output_tab_2: 1
keys_output_tab_3: 2
keys_output_tab_4: 3
keys_output_tab_5: 4
keys_output_tab_stage_fires: 0
keys_cmd1_in_output_stage: 1
keys_cmd1_in_output_tab_changed: no
keys_zoom_plus_ratio: 1.122462
keys_zoom_minus_ratio: 0.890899
keys_zoom_equal_ratio: 1.122462
keys_thumbs_larger: 1
keys_thumbs_smaller: 1
keys_thumbs_larger_equal: 1
keys_thumbs_zoom_moved: no
keys_view_menu: Fix Orientation ⌘1 | Split Pages ⌘2 | Deskew ⌘3 | Select Content ⌘4 | Margins ⌘5 | Output ⌘6 | Zoom In ⌘+ | Zoom Out ⌘- | Larger Thumbnails ⌥⌘+ | Smaller Thumbnails ⌥⌘-
keys_quit_triggered: 1
keys_quit_close_events: 0
keys_verdict: PASS
```

No line of the section carries a `_cross` or `_note` suffix: no key fired a second action, and every event was sent. Before the Output tab keys the harness set the current tab to the last one for every press, so a key that did nothing could not pass for "stays at tab 1".

## Interpretation

- Stage keys. Cmd-1 to Cmd-6 and the letters O, E, N, V, K, P each fired their own stage action once and nothing else, with the native menu carrying the Cmd digits as key equivalents: the native menu bar and Qt's shortcut map did not double-fire (D-01, D-03, MAC-04).
- File and Settings keys. Cmd-N, Cmd-O, Cmd-S, Shift-Cmd-S, Cmd-W and Cmd-comma fired New, Open, Save, Save As, Close Project and Settings once each. Cmd-comma reached `actionSettings` once (MAC-02, MAC-03); which of the two routes, the native application-menu item or Qt's shortcut map, carried it is not distinguished here, only that exactly one trigger arrived.
- Cmd-Q. `keys_quit_triggered: 1` and `keys_quit_close_events: 0`: on this Qt and macOS Cmd-Q reached `actionQuit` (the `QuitRole` action) once, and no close event arrived through the application's termination request, so the swallowed close path was not needed. The run ended normally afterwards, so the application was not terminated behind the swallower (MAC-05). The production slot of `actionQuit` then calls `close()`, which `testQuitClosesWindow` covers offscreen.
- Output tabs (D-02, Pitfall 4 live). Option-Command 1 to 5 select tabs 0 to 4 and trigger no stage action (`keys_output_tab_stage_fires: 0`); Cmd-1 with the Output tabs in the main area triggers the stage action once and keeps the tab (`keys_cmd1_in_output_tab_changed: no`).
- Zoom (D-05). Cmd-plus and Cmd-minus change the page zoom by exactly one step of 2^(1/6) in each direction (`keys_zoom_plus_ratio: 1.122462`, `keys_zoom_minus_ratio: 0.890899`).
- Cmd-equals on the German layout (informational, D-06). Cmd-equals zoomed in by the same step (`keys_zoom_equal_ratio: 1.122462`): on this layout the equals sign is Shift-0, the harness sent Shift-Command-0, and Qt matched the second sequence `Ctrl+=` of `actionZoomIn`. Option-Command-equals likewise triggered Larger Thumbnails once (`keys_thumbs_larger_equal: 1`). The US-layout bindings therefore also work here; the menu draws the plus form.
- Thumbnails (D-07). The secondary group Option-Command plus and minus triggers Larger and Smaller Thumbnails once each and does not move the page zoom (`keys_thumbs_zoom_moved: no`).
- Native View menu (D-04, D-08). The bar shows the six stage names with Cmd-1 to Cmd-6, Zoom In Cmd-plus, Zoom Out Cmd-minus and the two thumbnail items with Option-Command plus and minus, in that order; the menu draws the first sequence of each action.
- Accessibility Zoom caveat for the Option-Command plus and minus pair (`records/layout.md`, RESEARCH Pitfall 10). With "Use keyboard shortcuts to zoom" on in System Settings > Accessibility > Zoom, macOS takes Option-Command plus and minus before any application sees them. The harness sends its events to its own window, which bypasses that, and cannot see the setting (reading it would mean reading `~/Library`, which is not allowed). The measurement therefore says the chords work when they are delivered, not that the setting is off. If Larger and Smaller Thumbnails do nothing in daily use, check that setting first; the buttons next to the thumbnail list do the same.
