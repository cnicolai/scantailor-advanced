# Phase 05 — UI Review

**Audited:** 2026-11-15
**Baseline:** `05-UI-SPEC.md` (approved 2026-11-14), `05-CONTEXT.md` decisions D-00a to D-12
**Screenshots:** not captured (native Qt 6 desktop app, no dev server, no URL; the user is working at the Mac, so no window was opened). Visual pillars are scored from the `.ui` files, C++ string literals, the live harness records in `records/`, and the closing check.
**Interaction captures:** off (`workflow.ui_interaction_capture` is false; the app is not a web target). Interaction findings are code-derived and harness-derived, and are labelled that way below.

**Scope note:** this phase changed menus, key bindings, key-name text, context-menu timing, the batch alarm and the file panels. It added no colour, no font, no spacing token and no screen element. Pillars 3, 4 and 5 are therefore scored on what the phase touched, and the findings say so.

---

## Pillar Scores

| Pillar | Score | Key Finding |
|--------|-------|-------------|
| 1. Copywriting | 3/4 | Key names are Mac-style and no user-visible "Ctrl/Alt/Del/Ins" survives. The native Settings item still reads "Preferences..." (three ASCII dots), not the contracted "Settings…" (known open item). |
| 2. Visuals | 3/4 | Menu bar structure matches the contract (application menu, File, View, Tools). The Help menu was removed rather than kept empty, a deviation from the contract's "Claude's choice" row, and the record accepts it. |
| 3. Color | 4/4 | Phase added no colour. No colour literal was added to app code; the only `QColor` lines are test fixtures in `mac_check`/`qt_tests`. Scored on scope, not on a 60/30/10 split. |
| 4. Typography | 3/4 | No font size or weight was changed. The 7 pt bold glyphs in the Margins help are unchanged and were never rendered. |
| 5. Spacing | 3/4 | No spacing token changed. The nine rewritten apply-dialog hints and the Margins help were checked as markup and string length only, never drawn, so overflow is unproven. |
| 6. Experience Design | 3/4 | Strong measured coverage: menu roles, shortcuts, ⌫/⌦, context menus, panels, dialogs, German layout. Gaps: the Accessibility Zoom conflict on ⌥⌘+/⌥⌘-, the beep was never heard, two-finger click is unverified, and the Files-to-insert panel no longer filters already-imported files. |

**Overall: 19/24**

No pillar scored 1. No defect in the record breaks a user task, so there is no BLOCKER. All scores are WARNING-level findings.

---

## Top 3 Priority Fixes

1. **Settings item reads "Preferences..." instead of "Settings…"** — The contract's primary secondary CTA (`Settings…`, ⌘,) is shown as "Preferences..." with three ASCII dots in the native application menu, because Qt merges the `PreferencesRole` text from its own strings. Users searching for "Settings" in the menu will not find the word. Concrete fix: install a small `QTranslator` in `src/app/main.cpp` (before `MainWindow` is constructed) that maps the Qt source string `Preferences...` to `Settings…` in the Cocoa menu context, then confirm with `mac_check --section menubar` that the item reads `Settings…`. This is copy only and needs no change to the role, the shortcut or `actionSettings`. Known open item, recorded in `records/menubar.md`, still open.

2. **⌥⌘+ / ⌥⌘- (Larger/Smaller Thumbnails) can be taken by macOS Accessibility Zoom** — When "Use keyboard shortcuts to zoom" is on, macOS consumes these chords before the app sees them. The app cannot detect this, and the harness cannot read the setting. The thumbnail resize then silently does nothing. Concrete fix: add ⌃⌘+ and ⌃⌘- as second bindings on `actionMagnifyThumbnails` and `actionDiminishThumbnails` in `src/app/MainWindow.cpp` (around lines 1642-1644) via `setShortcuts({...})`, with ⌥⌘ listed first so the menu still draws ⌥⌘. The harness already measured ⌃⌘+ and ⌃⌘- firing once each on the German layout (`records/layout.md`), so the fallback is proven. Tooltips stay on ⌥⌘ as contracted. Known open item (Accessibility Zoom), but this is the cheapest mitigation.

3. **Files to insert shows already-imported files again** — The native panel (`src/app/MainWindow.cpp`, Files-to-insert call near line 1862) no longer applies the old `ProxyModel` that hid files already in the project, so a user can select them again. Recorded in `records/panels.md` as pre-existing, not introduced by Phase 5, and not repaired. Concrete fix: after the native panel returns, drop every returned path that is already in the project's file list and show a `QMessageBox::information` with the count of skipped files. This keeps the native panel (MAC-11) and restores the filtering behaviour.

---

## Detailed Findings

### Pillar 1: Copywriting (3/4)

**Passing evidence:**
- `src/app/MainWindow.ui` action texts follow Title Case with a single-character ellipsis: "Save Project As…", "Default Parameters…", "Fix DPI…", "Reverse Two-Page Spread Order", "Larger Thumbnails", "Zoom In".
- Stage actions use the real stage names ("Fix Orientation", "Split Pages", "Deskew", "Select Content", "Margins", "Output") with tooltips "Switch to <name>" (`MainWindow.ui` lines 1338-1426).
- Rewritten tooltips match the contract: `MainWindow.ui:434` "Larger thumbnails.  (⌥⌘+)", `:491` "Smaller thumbnails.  (⌥⌘-)", `:558` "Go To Page…  (⌘G)".
- Output tab status tip: `src/core/filters/output/TabbedImageView.cpp:17` "Use ⌥⌘1 to ⌥⌘5 to switch the tabs."
- Zone tips: `src/core/zones/ZoneDefaultInteraction.cpp:20,23-24,30` use ⌘, ⇧, ⇧⌘, ⌫, ⌥⌘-click. `src/core/interaction/InteractiveXSpline.cpp:58` and `src/core/filters/deskew/ImageView.cpp:33` match the contract.
- All nine apply-dialog hints read "Use ⌘-click / ⇧-click to select multiple pages." (grep, nine hits).
- A grep of `src/app` and `src/core` `.ui` and `.cpp`/`.h` literals found no user-visible "Ctrl", "Alt", "Control", "Option", "Del " or "Ins " key names. The remaining "Ctrl" and "Shift+PgUp" hits are `notr="true"` shortcut properties (not visible text) or code comments.

**Findings:**
- **WARNING — Settings item text.** `MainWindow.ui:1240` says "Settings…", but the native application menu reads "Preferences..." (`records/menubar.md`, `settings_item_title: Preferences...`). It is three ASCII dots, and the word differs from the contract's CTA row. Known open item; the fix is in the Top 3 list.
- **Minor — tooltip case.** The toolbar tooltips use sentence case ("Larger thumbnails.") while the menu items use Title Case ("Larger Thumbnails"). The contract prescribes both forms, so this is a contract-level inconsistency, not a defect. No action needed.

### Pillar 2: Visuals (3/4)

**Passing evidence:**
- Native bar measured with the Help-menu fix and the plist applied: application menu, File, View, Tools (`records/menubar.md` `m03p`, `records/dialogs.md` for View present under dialogs). Settings, About and Quit appear once each, in the application menu only (`role_items_outside_app_menu: 0`).
- The application menu now reads "About ScanTailor Advanced", "Hide ScanTailor Advanced", "Quit ScanTailor Advanced" (E15 resolved by the embedded `src/app/Info.plist`).
- Tools never ends on a separator: `src/app/MainWindow.cpp:327-331` inserts the separator only under `ENABLE_DEBUG_FEATURES`. The default build has `DEVELOPER_VERSION:BOOL=OFF` (`build/macos-release/CMakeCache.txt`), so Debug Mode is hidden and no separator remains after "Units". Checked, passes.
- Settings is not in `menuDebug` (`MainWindow.ui` addaction list ends at `actionDebug`).

**Findings:**
- **WARNING — Help menu removed.** UI-SPEC section 1 lists Help as an empty menu the contract keeps "in the `.ui`". The implementation removed `menuHelp` (commit `7db348da`) because macOS showed an empty Help menu. The record accepts this and the contract's pass criterion ("About only in the application menu") still holds. Recorded as a deviation from the written table, not a defect.
- **WARNING — Settings label.** See Pillar 1; the visual result is the same item.
- **Unverified — dark scheme.** The native menu bar follows the system, not the QSS scheme (contract section Color). Not rendered in either scheme in this audit.

### Pillar 3: Color (4/4, scoped)

- Phase added no colour, accent or destructive colour. `git diff f6cc4457 -- src | grep '^+'` finds `QColor(128, 128, 128)` only in test fixtures (`src/app/mac_check` and `src/qt_tests`), no colour literal in app code.
- Destructive context-menu items keep the default text colour (contract). No code change.
- 60/30/10 split: not applicable, no surface added. Scored on scope, not on a distribution.

### Pillar 4: Typography (3/4)

- No font size, weight or family changed. The phase only changed glyph characters inside the existing rich text.
- **WARNING — 7 pt bold Margins help.** `src/core/filters/page_layout/OptionsWidget.ui:905-907` keeps 7 pt spans with bold `‣ ⌥⌘-drag`, `⇧/⌘-drag` and `⇧⌘`. The contract keeps them as they are, but they were checked as markup only and never rendered (`records/*` and closing check). Legibility of ⌥ and ⇧ glyphs at 7 pt in the system font on Retina is unproven.

### Pillar 5: Spacing (3/4)

- No spacing token, margin or padding value changed. `.ui` geometry changed only by adding the View menu, which is a native menu and has no `.ui` geometry.
- **WARNING — overflow not observed.** The rewritten hints in the nine apply dialogs and the Margins help are shorter or equal in length to the text they replace (contract E13, E14), but no label was drawn to confirm it fits. Known open item.

### Pillar 6: Experience Design (3/4)

**Passing evidence (measured by the live harness, no key pressed by a person):**
- Stage keys ⌘1 to ⌘6 and letters O, E, N, V, K, P each fire once, no cross-firing (`records/keys.md`, `records/layout.md`, `chord_cmd_*: 1`, `cross_fires: 0`).
- Output tabs ⌥⌘1 to ⌥⌘5 fire on the German layout (`output_tabs_group: option-command`); ⌘1 in the Output stage switches stage, not tab (`keys_cmd1_in_output_tab_changed: no`).
- ⌫ and ⌦ remove the zone under the pointer and the content box; B creates the box; ⌫ in a focused line edit or spin box does not remove a zone (`records/zone.md`, PASS).
- Right-click and ⌃-click open the zone menu, first click works, Escape dismisses without change; the 200 ms deferral was removed after 20 of 20 confirmed runs (`records/context.md`, PASS). `grep` confirms no `200` delay remains in `ZoneContextMenuInteraction.cpp`.
- Dialogs (Settings, About, Fix DPI, Relinking, Default Parameters, message box, Open Project panel): menu bar stays populated, no fallback bar needed (`records/dialogs.md`, PASS).
- Native file panels for relinking (both calls), Open Project and folder choosers: native, cancel returns empty, substitution unchanged (`records/panels.md`, PASS). No `DontUseNativeDialog` or `AA_DontUseNativeMenuBar` in `src` outside the test scan lists.
- Batch-finished path: `src/app/MainWindow.cpp:1343-1347` calls `QApplication::alert(this)` then `QApplication::beep()`; no `std::system`, `QProcess`, `popen` or `external_alarm` remains in `src` (grep verified).

**Findings:**
- **WARNING — Accessibility Zoom conflict.** ⌥⌘+ and ⌥⌘- (thumbnail size) can be silently swallowed by macOS when Accessibility Zoom keyboard shortcuts are on. The app cannot detect this. See Top 3 fix 2.
- **WARNING — beep not heard.** MAC-10 is proven only by the source check `testBatchAlarmUsesBeep` and the code path above. Nobody listened for the system alert. The contract accepts this under D-11 (no manual step), but the user-facing outcome is unobserved.
- **WARNING — two-finger click unverified.** It arrives as the same `rightMouseDown` as a right-click, so the measured right-click row stands in for it (`records/context.md`). No separate two-finger event was injected.
- **WARNING — Files to insert (pre-existing).** The native panel does not hide already-imported files. Recorded in `records/panels.md` as predating the phase. See Top 3 fix 3.
- **Note — ⌘N under a modal dialog does nothing.** Measured 0 of 7 rows (`records/dialogs.md`). This is standard modal behaviour and matches the contract (E7). No defect.
- **Note — relinking folder chooser greys out files.** Accepted by contract E12 (partial). Not photographed; the mode is the documented behaviour of the folder panel.

---

## Deviations from the contract (recorded, not new defects)

| Contract item | Implementation | Source |
|---------------|----------------|--------|
| Help menu kept empty in `.ui` (section 1) | Help menu removed; About moved to File and the application menu | `records/menubar.md`, commit `7db348da` |
| Settings item reads "Settings…" (section 1, CTA) | Reads "Preferences..." | `records/menubar.md`, known open item |
| Bar keeps menus during dialogs, fallback bar if reduced (MAC-09) | Bar stayed populated in all seven rows, so no fallback was added (`m_fallbackMenuBar` absent) | `records/dialogs.md` |
| Relinking title drawn in panel (E12 long-text) | Title property read; drawing not observed | `records/panels.md` |

## Known open items (listed as the phase recorded them, not as new findings)

- Shorter apply-dialog hints and the 7 pt bold Margins help glyphs: never rendered.
- Native Settings item reads "Preferences..." rather than "Settings…".
- ⌥⌘+ / ⌥⌘- can be taken by macOS Accessibility Zoom shortcuts.
- Two-finger click and the system beep: unverified residuals.

---

## Files Audited

- `src/app/MainWindow.ui` (menus, action texts, shortcut properties, tooltips, status tips, menuRole values)
- `src/app/MainWindow.cpp` (lines 318-335 Debug Mode separator, 1340-1350 batch alarm, 1630-1644 shortcut bindings, Files-to-insert region near 1862)
- `src/app/Info.plist` (referenced by the record; not re-read)
- `src/core/filters/output/TabbedImageView.cpp` (lines 17-23)
- `src/core/zones/ZoneDefaultInteraction.cpp` (lines 20-30)
- `src/core/zones/ZoneContextMenuInteraction.cpp` (line 112; 200 ms delay absent)
- `src/core/filters/select_content/ImageView.cpp` (lines 150-155)
- `src/core/interaction/InteractiveXSpline.cpp` (line 58)
- `src/core/filters/deskew/ImageView.cpp` (line 33)
- `src/core/filters/page_layout/OptionsWidget.ui` (lines 905-907)
- Nine apply dialogs: `select_content/ApplyDialog.ui:141`, `fix_orientation/ApplyDialog.ui:97`, `output/ApplyColorsDialog.ui:86`, `output/ChangeDewarpingDialog.ui:166`, `page_layout/ApplyMarginsDialog.ui:94`, `page_split/SplitModeDialog.ui:199`, `output/ChangeDpiDialog.ui:149`, `page_layout/ApplyDialog.ui:97`, `deskew/ApplyDialog.ui:94`
- `build/macos-release/CMakeCache.txt` (DEVELOPER_VERSION value)
- Repo-wide greps: PC key names in user-visible text; `std::system`, `QProcess`, `popen`, `external_alarm`, `DontUseNativeDialog`, `AA_DontUseNative*`; colour literals added in the phase diff
- Records: `records/menubar.md`, `keys.md`, `layout.md`, `zone.md`, `context.md`, `dialogs.md`, `panels.md`, `gate-log.md`, `closing-check.md`
- Contract and context: `05-UI-SPEC.md`, `05-CONTEXT.md`

**Not read in full:** the 22 `05-NN-PLAN.md` and `05-NN-SUMMARY.md` files. Their objectives and outcomes are covered by the records and the closing check, which were read in full; the plan bodies were not re-audited.
