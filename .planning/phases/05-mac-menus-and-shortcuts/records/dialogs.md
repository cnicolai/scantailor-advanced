# Menu bar while dialogs are open (plan 05-10)

Live measurement of the native macOS menu bar of a shown `MainWindow` while six dialogs and the native Open Project panel are open over it, taken on 2026-11-15 with the harness `mac_check` (section `dialogs`). It answers MAC-09 and roadmap criterion 1 for dialogs: does the bar keep the application menu and the File, View and Tools menus while a dialog is open (UI-SPEC E6, RESEARCH Pitfall 6), and does ⌘N still reach New Project (RESEARCH assumption A9)?

- Platform: cocoa, Qt 6.12.0, macOS 26.7.2, Apple Silicon, keyboard layout `com.apple.keylayout.German`.
- Command (from `{repo}`, run by `build/scratch/run-m14.sh`, which adds a 200 s kill watchdog): `D=build/scratch/mac/m14a; mkdir -p "$D/home"; env HOME="$PWD/$D/home" CFFIXED_USER_HOME="$PWD/$D/home" build/macos-release/mac_check "$D" --section dialogs,panels -ApplePersistenceIgnoreState YES`. The run was made once, with the committed harness (commit `efe9cbdb`), into `m14a`; its `summary.txt` was copied to `build/scratch/mac/m14/` where the plan's check reads it. No second run was made. The same run also produced `panels.md`.
- The home directory was redirected below the output directory (`home_redirected: yes`: `HOME`, `CFFIXED_USER_HOME`, `NSHomeDirectory()`, `QDir::homePath()` and the `AppDataLocation` all lie below `{repo}/build/`), so neither the native panel state nor the Default Parameters profile listing reached the user's folders. The home directory below the output directory stayed empty.
- No key or click was physically pressed. The one key (⌘N) was synthesised inside the harness process and sent to the dialog's own window (or, for the panel, to `[NSApp modalWindow]`) through `[NSApp sendEvent:]`; nothing was posted system-wide, and the pointer was not moved. The user did not interfere: every row showed its dialog, the application was active, and the panel was seen in all runs (`dialogs_open_panel_shown: yes`).
- Windows opened one after the other for a few seconds each; the New Project action's production slot was disconnected for the run (`dialogs_new_project_slot_cut: yes`) and only counted.

Verdict rule of `dialogs_verdict` (PASS when all hold): `dialogs_app_active` is yes; every row is shown (the dialog is visible, or for `open_panel` the block scheduled before the call saw a file panel); for every row the native application menu has items and the top-level menus after it contain `File`, `View` and `Tools`. The `new_item_enabled` and `cmd_n_fired` lines are findings, not part of the verdict.

## Raw lines

```
dialogs_app_active: yes
dialogs_new_project_slot_cut: yes
dialogs_cmd_n_key_code: 45
dialogs_settings_shown: yes
dialogs_settings_top_level: File | View | Tools
dialogs_settings_app_menu: yes
dialogs_settings_new_item_enabled: no
dialogs_settings_populated: yes
dialogs_settings_cmd_n_fired: 0
dialogs_about_shown: yes
dialogs_about_top_level: File | View | Tools
dialogs_about_app_menu: yes
dialogs_about_new_item_enabled: no
dialogs_about_populated: yes
dialogs_about_cmd_n_fired: 0
dialogs_fix_dpi_shown: yes
dialogs_fix_dpi_top_level: File | View | Tools
dialogs_fix_dpi_app_menu: yes
dialogs_fix_dpi_new_item_enabled: no
dialogs_fix_dpi_populated: yes
dialogs_fix_dpi_cmd_n_fired: 0
dialogs_relinking_shown: yes
dialogs_relinking_top_level: File | View | Tools
dialogs_relinking_app_menu: yes
dialogs_relinking_new_item_enabled: no
dialogs_relinking_populated: yes
dialogs_relinking_cmd_n_fired: 0
dialogs_default_params_shown: yes
dialogs_default_params_top_level: File | View | Tools
dialogs_default_params_app_menu: yes
dialogs_default_params_new_item_enabled: no
dialogs_default_params_populated: yes
dialogs_default_params_cmd_n_fired: 0
dialogs_message_box_shown: yes
dialogs_message_box_top_level: File | View | Tools
dialogs_message_box_app_menu: yes
dialogs_message_box_new_item_enabled: no
dialogs_message_box_populated: yes
dialogs_message_box_cmd_n_fired: 0
dialogs_open_panel_top_level: File | View | Tools
dialogs_open_panel_app_menu: yes
dialogs_open_panel_new_item_enabled: yes
dialogs_open_panel_populated: yes
dialogs_open_panel_shown: yes
dialogs_open_panel_cmd_n_fired: 0
dialogs_verdict: PASS
```

## Table

| Dialog | Kind | Top-level menus after the application menu | Application menu | New Project item enabled | ⌘N fired |
|--------|------|--------------------------------------------|------------------|--------------------------|----------|
| Settings | window-modal, from the action | File, View, Tools | yes | no | 0 |
| About | window-modal, from the action | File, View, Tools | yes | no | 0 |
| Fix DPI | window-modal, constructed by the harness (the main window's slot needs a project) | File, View, Tools | yes | no | 0 |
| Relinking | window-modal, constructed by the harness (the main window's slot needs a project) | File, View, Tools | yes | no | 0 |
| Default Parameters | window-modal, from the action, profile folder below the redirected home | File, View, Tools | yes | no | 0 |
| Message box | application-modal `QMessageBox` | File, View, Tools | yes | no | 0 |
| Open Project panel | native `NSOpenPanel`, cancelled by the harness | File, View, Tools | yes | yes (not updated by Qt during the panel's modal session) | 0 |

## Decision

No fallback menu bar was added. Every one of the seven rows was shown and populated, so the condition for plan step 2 (a `dialogs_<row>_populated: no` line) did not occur. `MainWindow.h` is unchanged and holds no `m_fallbackMenuBar`; this also keeps the reconciliation of RESEARCH Pitfall 6 intact: the bar is never reduced, and no parentless bar with fewer menus exists.

## Findings

None for ⌘N: it fired 0 times in all seven rows, so no dialog let ⌘N reach New Project through the native menu while it was open (assumption A9 did not materialise; Qt disables the New Project item under a window-modal or application-modal dialog, and the native panel's modal session does not route the key to the main menu). The Open Project row reads the item as enabled because Qt does not update it during AppKit's modal session; the key still did not fire it. Nothing is carried to the closing report from this record.
