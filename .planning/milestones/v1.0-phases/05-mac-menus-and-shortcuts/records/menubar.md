# Live application menu (plan 05-02)

Live measurement of the native macOS menu bar of a shown `MainWindow`, taken on 2026-11-14 and 2026-11-15 with the harness `mac_check` (sections `menubar` and `identity`). It answers three questions: does the application menu hold About, Settings… ⌘, and Quit ⌘Q exactly once (MAC-01, MAC-02, UI-SPEC E1); does macOS show an empty Help menu once About has moved to the application menu (UI-SPEC E5, RESEARCH Pitfall 3); and does the About item name the application or the bare process (UI-SPEC E15, CONTEXT discretion "App menu title").

- Platform: cocoa, Qt 6.12.0, macOS 26.7.2, Apple Silicon, one Studio Display (window device pixel ratio 2), keyboard layout `com.apple.keylayout.German`.
- The harness process `mac_check` stands in for `scantailor-advanced`, which is not launched (home-folder rule). It links the same `app_gui` library, builds a real `MainWindow` and, once the plist is embedded, carries the same `CFBundleName` section as the app. Settings, temporary files and summaries stay below `build/scratch/mac/`.
- No key or click was physically pressed: the menu bar was only read. Every event of the sibling record `layout.md` was synthesised inside the harness process and sent to its own window (D-11).
- Commands (from `{repo}`):
  - `build/macos-release/mac_check build/scratch/mac/m03 --section menubar,layout -ApplePersistenceIgnoreState YES` (first run)
  - `build/macos-release/mac_check build/scratch/mac/m03h --section menubar -ApplePersistenceIgnoreState YES` (after the Help fix)
  - `build/macos-release/mac_check build/scratch/mac/m03p --section menubar -ApplePersistenceIgnoreState YES` (with the embedded plist)
  - the identity runs below, each with `HOME` and `CFFIXED_USER_HOME` set to `{repo}/<output-dir>/home`.

Verdict rule of `menubar_verdict` (PASS when all hold): `app_active` is yes; the native top-level menus after the application menu equal the Qt top-level menus that keep a visible item once the role items have moved; `about_items`, `settings_items` and `quit_items` are 1; the keys are `⌘,` and `⌘q`; `settings_item_enabled` is yes; `role_items_outside_app_menu` is 0; `standard_keys_match` is yes (`keyBindings(Preferences)` holds Ctrl+comma, `keyBindings(Quit)` Ctrl+Q, `keyBindings(SaveAs)` Ctrl+Shift+S).

## First attempt, not a measurement

The very first start of the harness found the screen locked and the display asleep (`frontmost_app: loginwindow`, `app_active: no`, the native bar held only the Qt window menu). Nothing was measured; the harness gained the `frontmost_app` line so that a locked session is named, and the run was repeated with an unlocked session.

## First run (unlocked session)

```
app_active: yes
window_dpr: 2
localized_name: mac_check
bundle_name: none
menubar_top_level: File | Tools | Help
qt_top_level: File | Tools
app_menu_items: About mac_check | Preferences... | Services | Hide mac_check | Hide Others | Show All | Quit mac_check
about_item_title: About mac_check
settings_item_title: Preferences...
quit_item_title: Quit mac_check
settings_item_key: ⌘,
quit_item_key: ⌘q
settings_item_enabled: no
about_items: 1
settings_items: 1
quit_items: 1
role_items_outside_app_menu: 0
help_menu_shown: yes
standard_keys_match: yes
native_menu_0: mac_check: About mac_check | [About Qt] | -- | Preferences... (⌘,) | -- | Services | -- | Hide mac_check (⌘h) | Hide Others (⌥⌘h) | Show All | -- | Quit mac_check (⌘q)
native_menu_1: File: New Project… (⌘n) | Open Project… (⌘o) | -- | Save Project (⌘s) | Save Project As… (⇧⌘s) | -- | Close Project (⌘w)
native_menu_2: Tools: Fix DPI… | Relinking… | Reverse Two-Page Spread Order | -- | Default Parameters… | Units | [Debug Mode]
native_menu_3: Help: 
menubar_verdict: FAIL
```

Interpretation:

- The native bar is application menu, File, Tools, Help. About, Settings and Quit each appear exactly once in the application menu and none of them appears in File, Tools or Help (`role_items_outside_app_menu: 0`); `⌘,` and `⌘q` are the key equivalents; `standard_keys_match: yes` shows that on Cocoa `QKeySequence::Preferences`, `Quit` and `SaveAs` contain the explicit sequences of plan 05-01 (the Cocoa half of edge probe MAC-03). Settings and Quit are gone from File, as the roles require.
- Help showed with no items ("Help: "). Qt keeps the menu although its only action moved; that is the empty Help menu of RESEARCH Pitfall 3. `menubar_top_level` therefore differed from `qt_top_level`. Decision below.
- `settings_item_enabled: no` is a reading artefact, not a defect: Qt's merged About and Settings items carry their initial disabled flag until AppKit validates the menu, which it does when the menu opens or a key equivalent is performed. In an extra experiment (not committed) `performKeyEquivalent:` with ⌘, on the main menu returned yes and triggered the Settings action, and `performActionForItemAtIndex:` on the item triggered it too. After the harness validates each native menu with `-[NSMenu update]` before reading (the same step AppKit performs when the user opens the menu) the items read enabled; see the Help re-run.
- The item is titled "Preferences..." (three ASCII dots) although the action text is "Settings…": Qt takes the merged text from its own strings. UI-SPEC section 1 expects "Settings…" ("because macOS names it"). Not changed in this plan; see the decisions.
- The About and Quit items read "About mac_check" and "Quit mac_check": the bare process name. Decision below.

## Help menu decision

The empty Help menu shows, so About moved into File (role unchanged) and `menuHelp` was removed from `MainWindow.ui` (commit `fix(05-02): move About out of the empty Help menu`, gate `m03h-help-menu`). File now reads New Project…, Open Project…, separator, Save Project, Save Project As…, separator, Close Project, About, Settings…, Quit in Qt; the native File menu keeps New Project…, Open Project…, Save Project, Save Project As… and Close Project with their two separators, because About, Settings and Quit move to the application menu. UI-SPEC's pass criterion "About only in the application menu" still holds. The same commit made the harness validate the native menus before reading and record the enabled state of the three role items.

Re-run after the fix (`m03h`, no plist):

```
app_active: yes
bundle_name: none
menubar_top_level: File | Tools
qt_top_level: File | Tools
about_item_title: About mac_check
settings_item_title: Preferences...
quit_item_title: Quit mac_check
settings_item_key: ⌘,
quit_item_key: ⌘q
settings_item_enabled: yes
about_item_enabled: yes
quit_item_enabled: yes
about_items: 1
settings_items: 1
quit_items: 1
role_items_outside_app_menu: 0
help_menu_shown: no
standard_keys_match: yes
menubar_verdict: PASS
```

No Help menu shows; the native bar is application menu, File, Tools.

## Application name decision

Without an embedded plist the About and Quit items name the process (`About mac_check`, and `NSRunningApplication.localizedName` is `mac_check`), exactly where the app would show `About scantailor-advanced`. Qt names these items after `CFBundleName` and falls back to the process name (RESEARCH Pitfall 2). So `src/app/Info.plist` was added with `CFBundleName` = `ScanTailor Advanced` only (no `CFBundleIdentifier`, no other key; `NSHighResolutionCapable` was not needed because the window ratio stayed 2) and linked into `scantailor-advanced` and `mac_check` as the section `__TEXT,__info_plist` (`-sectcreate`). The file stays in the source tree; an `Info.plist` next to the executable would fake a bundle.

Check that the app binary carries the section: `xcrun segedit build/macos-release/scantailor-advanced -extract __TEXT __info_plist build/scratch/mac/app-info.plist` writes a plist that contains `ScanTailor Advanced`, and `otool -l build/macos-release/scantailor-advanced` lists one `sectname __info_plist`.

Re-run with the plist (`m03p`):

```
app_active: yes
window_dpr: 2
localized_name: mac_check
bundle_name: ScanTailor Advanced
menubar_top_level: File | Tools
qt_top_level: File | Tools
app_menu_items: About ScanTailor Advanced | Preferences... | Services | Hide ScanTailor Advanced | Hide Others | Show All | Quit ScanTailor Advanced
about_item_title: About ScanTailor Advanced
settings_item_title: Preferences...
quit_item_title: Quit ScanTailor Advanced
settings_item_key: ⌘,
quit_item_key: ⌘q
settings_item_enabled: yes
about_item_enabled: yes
quit_item_enabled: yes
about_items: 1
settings_items: 1
quit_items: 1
role_items_outside_app_menu: 0
help_menu_shown: no
standard_keys_match: yes
menubar_verdict: PASS
```

The window device pixel ratio is 2 in all runs, so the plist did not change it. `localized_name` stays `mac_check` (that name comes from the process, not from `CFBundleName`; the menu items follow `CFBundleName`).

## Identity comparison (settings and profiles paths)

CONTEXT discretion allows the plist only if the settings and profile paths stay unchanged. A path computed after the harness's own `setPath` could not change, so the section `identity` repeats the order of `src/app/main.cpp` (application object, then `APPLICATION_NAME` and `ORGANIZATION_NAME`, then `QSettings::setDefaultFormat(IniFormat)` and a default-constructed `QSettings`, no `setPath`) with the home redirected below the output directory. It runs alone and refuses to run unless `HOME` and `CFFIXED_USER_HOME` name `<output-dir>/home`. Commands (from `{repo}`, `D` being the output directory):

```
D=build/scratch/mac/m03i  && mkdir -p "$D/home" && env HOME="$PWD/$D/home" CFFIXED_USER_HOME="$PWD/$D/home" build/macos-release/mac_check "$D" --section identity -ApplePersistenceIgnoreState YES   # without the plist
D=build/scratch/mac/m03pi && mkdir -p "$D/home" && env HOME="$PWD/$D/home" CFFIXED_USER_HOME="$PWD/$D/home" build/macos-release/mac_check "$D" --section identity -ApplePersistenceIgnoreState YES   # with the plist
```

| Line | m03i (without plist) | m03pi (with plist) | Same |
|------|----------------------|--------------------|------|
| `bundle_name` | none | ScanTailor Advanced | no (this is the plist) |
| `default_application_name` | mac_check | ScanTailor Advanced | no (Qt's own default before main.cpp sets the names; the explicit names override it) |
| `application_name` | scantailor-advanced | scantailor-advanced | yes |
| `organization_name` | scantailor-advanced | scantailor-advanced | yes |
| `organization_domain` | none | none | yes |
| `app_settings_path` | `{out}/home/.config/scantailor-advanced/scantailor-advanced.ini` | `{out}/home/.config/scantailor-advanced/scantailor-advanced.ini` | yes |
| `app_profiles_path` | `{out}/home/Library/Application Support/scantailor-advanced/scantailor-advanced/profiles` | `{out}/home/Library/Application Support/scantailor-advanced/scantailor-advanced/profiles` | yes |
| `identity_verdict` | PASS | PASS | yes |

`home_redirected` is yes in both runs. The plist is therefore kept: the measured process carries it (`bundle_name`), the five identity lines are byte-identical, and the user's settings file `~/.config/scantailor-advanced/scantailor-advanced.ini` and the profiles directory keep their paths. The commit is `build(05-02): embed the application name for the Mac menu`, gate `m03p-app-name`.

## Decisions

- Keep the Help fix: About lives in File, there is no Help menu.
- Keep the embedded plist: the application menu reads "About ScanTailor Advanced", "Hide ScanTailor Advanced" and "Quit ScanTailor Advanced".
- Open point for the orchestrator (not changed here): the native Settings item reads "Preferences..." while UI-SPEC expects "Settings…". It comes from Qt's merged-item text for `PreferencesRole`. A later plan can override it with a `QTranslator` for the context `QMenuBar` if the text matters; the key `⌘,`, the single item and its function are as required.
