# Phase 5: Mac Menus and Shortcuts - Context

**Gathered:** 2026-11-14
**Status:** Ready for planning

<domain>
## Phase Boundary

The app behaves like a Mac app on the one target machine. In scope: About, Settings and Quit in the application menu with explicit menu roles, Settings out of the Tools menu (object name `menuDebug`), Cmd shortcuts (New, Open, Save, Save As, Close, Quit, zoom, Cmd+1 to Cmd+6 for the stages), Mac key names (⌘, ⌥) in every tooltip, status tip and help text, zone and content-box editing from an Apple keyboard, context menus from a right-click, a two-finger click and a Control-click, a populated menu bar while a dialog is open, the system beep after a batch run instead of a shell command, and native file dialogs everywhere, including the relinking dialog. Requirements MAC-01 to MAC-11.

Not in scope: the v2 Mac polish items (Dock icon, Open Recent, proxy icon, Dock bounce, `.app` bundle), trackpad gestures and system dark mode (REQUIREMENTS Out of Scope), the `MainWindow.cpp` split and the shared "Save project as" helper (CLEAN-02 and CLEAN-05, Phase 6), the OpenGL viewer (Phase 6), and any change to output generation.

</domain>

<decisions>
## Implementation Decisions

### Carried forward (not re-asked)
- **D-00a:** Every commit passes the Phase 3 gate (`.planning/phases/03-qt-6-and-macos-only/gate.sh`): it builds, `ctest --preset macos-release` and `ctest --preset macos-debug` pass 7/7, and no golden or manifest file changes. `UPDATE_GOLDENS` is never set. Phase 5 is GUI-only: no diff under `src/imageproc/`, `src/dewarping/`, any `Task::process` or `OutputGenerator.cpp`.
- **D-00b:** Cmd stays the primary modifier. Qt's `Ctrl` / `Qt::ControlModifier` already is Cmd on macOS, so the existing `ControlModifier` gesture logic stays untouched (PITFALLS Pitfall 9). `Qt::AA_MacDontSwapCtrlAndMeta`, `AA_DontUseNativeMenuBar` / `setNativeMenuBar(false)`, `Meta` as the main modifier, parallel Ctrl and Cmd shortcut sets, and Objective-C++ in the app to build menus are all excluded (REQUIREMENTS Out of Scope, FEATURES Area 3 Anti-Features). Fusion and the colour-scheme QSS stay.
- **D-00c:** The UI is English only (Phase 3). Changed `tr()` source strings need no translation update.
- **D-00d:** Values already stored in the user's settings file are left alone; nothing in this phase rewrites or deletes them (same rule as Phase 3 D-07 for `language`).

### Cmd+number keys (stages and Output tabs)
- **D-01:** Cmd+1 to Cmd+6 switch the six stages everywhere, in pipeline order (Fix Orientation, Split Pages, Deskew, Select Content, Margins, Output). This fulfils roadmap criterion 2 and MAC-05.
- **D-02:** The Output stage tabs, which use Cmd+1 to Cmd+5 today (`src/core/filters/output/TabbedImageView.cpp:19-23`, `Qt::CTRL | Qt::Key_1..5`), move to Option+Cmd+1 to Option+Cmd+5 (⌥⌘1–⌥⌘5). Their status tip ("Use Ctrl+1..5 to switch the tabs.") changes with them.
- **D-03:** The single-letter stage keys O, E, N, V, K and P (`actionSwitchFilter1..6` in `src/app/MainWindow.ui`) stay as they are.
- **D-04:** A new **View** menu lists the six stages with ⌘1–⌘6 shown next to them, so the keys can be found in the menu bar. The menu entries carry the stage names (the action texts in `MainWindow.ui` currently all say "Switch filter to orientation" and need the real names).

### Zoom keys
- **D-05:** The user types on a **German keyboard layout**. On it, + is its own key and = is Shift+0.
- **D-06:** In the page view, ⌘+ and ⌘= zoom in and ⌘- zooms out. Both zoom-in bindings exist so the keys work on German and US layouts. Help text and menus name ⌘+. On the German layout ⌘+ is what roadmap criterion 2 calls "Cmd+=". The plain + and - keys in the page view (`src/core/interaction/ZoomHandler.cpp:18-19`) keep working.
- **D-07:** Thumbnail resizing, which uses Cmd++ and Cmd+- today (`actionMagnifyThumbnails` / `actionDiminishThumbnails` in `MainWindow.ui`), moves to ⌥⌘+ and ⌥⌘- (plus ⌥⌘= for a US layout). The toolbar buttons and Option+wheel stay. The toolbar tooltips "(Ctrl++)" and "(Ctrl+-)" change with it.
- **D-08:** The View menu also lists the four zoom commands with their keys: Zoom In (⌘+), Zoom Out (⌘-), Larger Thumbnails (⌥⌘+), Smaller Thumbnails (⌥⌘-).

### Zone keys on a Mac keyboard
- **D-09:** **B** ("box") creates the content box in Select Content. Insert (`QKeySequence("Ins")`, `src/core/filters/select_content/ImageView.cpp:152`) stays as a second binding for PC keyboards. B is free everywhere in the app (single-letter keys in use: O E N V K P Q W globally, Z X C D in the zone and dewarping editors).
- **D-10:** The Mac delete key (⌫, Qt's `Key_Backspace`) and forward delete (⌦, fn+⌫, `Key_Delete`) both remove the zone under the mouse pointer (`ZoneDefaultInteraction.cpp:33`, today `Key_Delete` only) and the content box (`select_content/ImageView.cpp:153`). The semantics stay "under the pointer", as today; there is no undo, as today. Bind both keys at once (`setShortcuts`, not `setShortcut` twice). ⌫ must not fire while a text or spin-box field in the options panel has focus (window-context shortcut; PITFALLS Pitfall 9). STATE.md's open question, whether Backspace reaches the shortcut under Qt 6 (QTBUG-33015 history), is answered by the harness check in D-12.

### How Phase 5 is checked
- **D-11:** Claude verifies everything, as in Phases 2 to 4. There is no GUI walkthrough and no human-verify checkpoint. The end-of-phase report is a short written summary, and the records state that no key or click was physically pressed.
- **D-12:** Two layers of proof:
  - **`ctest` (offscreen, permanent, in `qt_tests`):** menu roles (About, Preferences, Quit, and `NoRole` on actions whose text could trigger the heuristic), every shortcut from D-01 to D-10 and MAC-02 to MAC-05, the View menu contents, Mac key names in the user-visible strings (no "Ctrl" or "Alt" left), no `std::system` in the batch-finished path, and no `DontUseNativeDialog` anywhere.
  - **Live Cocoa harness, run once and recorded:** what the offscreen platform cannot show. That covers the native menu bar and its application menu, the menu bar while a modal dialog is open (MAC-09), the native file panels including relinking (MAC-11), ⌫ and ⌦ reaching zone and content-box removal, and a right-click, a two-finger (secondary) click and a Control-click each opening the zone context menu (MAC-08). Events are fed at the Cocoa level inside the harness process, so they pass through Qt's Cocoa event translation. The harness keeps its settings under `build/` (like `retina_check` with `RETINA_CHECK_ROOT`), so nothing touches `~/.config` or `~/Library`. The real app binary is not launched against the user's settings. Verdicts go into `.planning/phases/05-mac-menus-and-shortcuts/records/`.

### Claude's Discretion
- **Key names in text (MAC-06):** Apple's style, symbols without `+` separators: "⌘-click", "⇧-click", "⌥⌘-click", "⇧⌘S", "⌥⌘1". Help text derived from a `QKeySequence` uses `toString(QKeySequence::NativeText)` where practical. A small helper for modifier names is acceptable (PITFALLS Pitfall 9). The inventory is under code_context. Physical Control (⌃) is named only where the physical Control key is meant.
- **Application menu (MAC-01, MAC-02):** explicit `menuRole` set before the window is shown: `AboutRole` on `actionAbout`, `PreferencesRole` on `actionSettings` (shortcut `QKeySequence::Preferences`, Cmd+,), `QuitRole` on `actionQuit` (stays connected to `close()` so the unsaved-changes prompt in `closeEvent` runs for Cmd+Q, the app menu and the Dock). Use `NoRole` on "Default parameters ..." and any other action the text heuristic could catch. The item appears as macOS names it ("Settings…" on current macOS). The Tools menu keeps its other entries (Fix DPI, Relinking, Reverse two-page order, Debug Mode, Default parameters, Units).
- **App menu title:** without a bundle, the bold application menu and "About/Quit <name>" may show the process name `scantailor-advanced` (PITFALLS Pitfall 10). Research checks what Qt 6.12 shows. If it shows the bare process name, Claude may embed a minimal Info.plist section (`-sectcreate __TEXT __info_plist`, `CFBundleName` "ScanTailor Advanced"), on the condition that settings and profile paths stay unchanged. Otherwise accept the name and record it.
- **Save As (MAC-03):** `QKeySequence::SaveAs` (⇧⌘S) on `actionSaveProjectAs`. The shared "Save project as" helper is CLEAN-05 in Phase 6. Do not restructure the flow here.
- **Menu bar while a dialog is open (MAC-09):** for example a parentless `QMenuBar` as the default menu bar, or a measured confirmation that modal dialogs keep the main window's bar. Choose by what the harness shows.
- **Batch-finished alarm (MAC-10):** always `QApplication::beep()` (the system alert sound) when "beep when finished" is checked. No shell command, no `std::system`, no `QProcess`. A stored `main_window/external_alarm_cmd` value is ignored and left in the settings file (D-00d). The existing `QApplication::alert(this)` stays.
- **Native dialogs (MAC-11):** remove `QFileDialog::DontUseNativeDialog` from both calls in `src/app/RelinkingDialog.cpp:67-74`. Research why upstream disabled it (for example, choosing a missing file or folder) and make sure relinking still works with the native panel.
- **Context-menu workaround:** the 200 ms `Q_OS_MAC` delay in `ZoneContextMenuInteraction.cpp:140-145` is tested against Qt 6.12 and current macOS. Keep, adjust or drop it by what the harness shows.
- **⌥⌘ combinations on the German layout:** Option changes the character a key produces (⌥1 gives "¡", ⌥+ gives "±"). Research and the harness confirm that Qt 6.12 matches ⌥⌘1–5 and ⌥⌘+/- on the German layout. If one does not fire, Claude picks the nearest working binding (for example ⌃⌘), records why, and tells the user.
- **Action wiring:** whether ⌘1–⌘6 are added to `actionSwitchFilter1..6` (with ⌘N as the primary shortcut so the menu shows it) or as new actions, where the View menu actions live, and the exact menu text.
- **Harness location:** extend `src/app/retina_check/` into a general live harness or add a sibling tool. Cocoa-level event injection may need a small test-only Objective-C++ helper. That is acceptable in the harness, never in the app. No macOS privacy permission (Accessibility, screen recording) may be required.
- **Plan and commit split**, and the order of the work.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase scope and requirements
- `.planning/ROADMAP.md` § "Phase 5: Mac Menus and Shortcuts" — goal and the five success criteria
- `.planning/REQUIREMENTS.md` § Mac Shortcuts & Menus (MAC-01 to MAC-11), § Out of Scope (`AA_MacDontSwapCtrlAndMeta`, trackpad gestures, system dark mode), § v2 Mac Polish (stays deferred)
- `.planning/STATE.md` § Blockers/Concerns — Phase 5 note on Backspace under Qt 6; the gate script for later phases

### Research
- `.planning/research/FEATURES.md` § "Area 3: Mac-conventional shortcuts and menus" — Qt facts (Ctrl is Cmd, Control-click as right-click, default menu bar, menu roles, standard keys), repo evidence, table stakes MAC-01 to MAC-14 (research numbering, not the requirement IDs), anti-features
- `.planning/research/PITFALLS.md` § "Pitfall 9" (Ctrl/Cmd confusion, string inventory, missing Mac keys, Backspace and text fields), § "Pitfall 10" (menu roles, app-menu name without Info.plist, native dialogs), § "Pitfall 17" (trackpad wheel semantics, do not make worse), § "Pitfall 18" (batch alarm and the shell string)
- `.planning/research/ARCHITECTURE.md` — Mac menus row of the change table (`qt_tests` can assert `shortcut()`/`menuRole()` if `MainWindow` is constructible offscreen)
- `.planning/research/STACK.md` — unbundled executable becomes a foreground app (window, focus, Dock icon, Cmd-Tab entry without a bundle)

### Prior decisions and tools
- `.planning/phases/03-qt-6-and-macos-only/gate.sh` — per-commit gate (Release and Debug `ctest`, goldens, build-command diff)
- `.planning/phases/03-qt-6-and-macos-only/03-CONTEXT.md` — D-04 (the batch alarm's Unix side kept until MAC-10), D-07 (stored settings left alone)
- `.planning/phases/04-retina-rendering/04-CONTEXT.md` — D-01 and D-02 (Claude-only verification with a live Cocoa harness), the home-folder rule for launching the app
- `src/app/retina_check/main.cpp` and `src/app/CMakeLists.txt:79-85` — the Phase 4 live Cocoa harness (settings root under `build/`), the starting point for D-12
- `.planning/phases/02-golden-baselines/records/access-approval.md` — standing approval for build-tool reads under `/opt/homebrew` and the Xcode/CLT installation in Phases 3 to 6
- `TESTING.md` — the seven suites, presets, formatting with `xcrun clang-format`

</canonical_refs>

<code_context>
## Existing Code Insights

### Menus and actions
- `src/app/MainWindow.ui` — native-capable `QMenuBar` with File (New ⌘N, Open ⌘O, Save ⌘S, Save As without a shortcut, Close ⌘W, Quit ⌘Q), Tools (object `menuDebug`: Fix DPI, Relinking, Reverse two-page order, Debug Mode, Settings, Default parameters, Units submenu) and Help (About). No `menuRole`, no `QKeySequence::StandardKey`, no View menu. 37 actions; window-level actions without a menu: page navigation (PgUp/PgDown/Q/W, Shift variants, Home, End), Go To Page ⌘G, stage letters O/E/N/V/K/P, thumbnail magnify and diminish (⌘+ / ⌘-), Reload F5.
- `src/app/MainWindow.cpp:315-317` — `actionDebug` hidden unless `DEVELOPER_VERSION`; `src/app/main.cpp:25-26` — application and organisation names (settings path depends on them; do not change).
- Stage names come from each `Filter::getName()` (`QCoreApplication::translate("<ns>::Filter", ...)`), e.g. "Fix Orientation", "Split Pages", "Deskew", "Output".

### Keys and interactions
- `src/core/filters/output/TabbedImageView.cpp:17-26` — Output tab shortcuts and status tip (D-02).
- `src/core/interaction/ZoomHandler.cpp:18-19` — plain `Key_Plus` / `Key_Minus` zoom on every `ImageViewBase`; page-view zoom actions for ⌘+/⌘- connect to the same `zoom()` steps (2^(±1/6)).
- `src/core/filters/select_content/ImageView.cpp:150-157` — Create/Remove Content Box actions (Ins, Delete) added to the view.
- `src/core/zones/ZoneDefaultInteraction.cpp:18-38` — zone status tips and `Key_Delete` / `Key_D` shortcuts; `src/core/ZoneEditorBase.cpp:66-70` — Z, X, C; `src/core/filters/output/DewarpingView.cpp:94` — D.
- `src/core/zones/ZoneContextMenuInteraction.cpp:140-145` — the only Mac-specific code (200 ms delay).
- `src/app/MainWindow.cpp:1327-1337` — batch-finished alarm: default command `play /usr/share/sounds/...`, `main_window/external_alarm_cmd` setting, `std::system`.
- `src/app/RelinkingDialog.cpp:67-74` — the two `DontUseNativeDialog` calls. Other dialogs (`MainWindow.cpp:1427, 1471, 1862`, `ProjectFilesDialog.cpp:244, 258`, `OutOfMemoryDialog.cpp:62`) already use the native panel.

### User-visible strings with Ctrl/Alt/Del/Ins (MAC-06 inventory)
- `src/app/MainWindow.ui:425, 482, 549` (toolbar tooltips), `:934` (navigation help, "Alt+Wheel")
- `src/core/filters/deskew/ImageView.cpp:33` ("Ctrl+Wheel", "Ctrl+Shift+Wheel")
- `src/core/filters/output/TabbedImageView.cpp:17`
- `src/core/zones/ZoneDefaultInteraction.cpp:20, 24-25, 30` ("Hold Ctrl", "Shift+Ctrl", "Press Del", "Ctrl+Alt+Click")
- `src/core/interaction/InteractiveXSpline.cpp:58, 206` ("Hold Ctrl or Shift")
- `src/core/filters/page_layout/OptionsWidget.ui:905-906` (rich-text help, "Ctrl+Alt+LMB", "Ctrl+LMB")
- "Use Ctrl+Click / Shift+Click to select multiple pages." in nine dialogs: `select_content/ApplyDialog.ui:141`, `fix_orientation/ApplyDialog.ui:97`, `output/ApplyColorsDialog.ui:86`, `output/ChangeDewarpingDialog.ui:166`, `page_layout/ApplyMarginsDialog.ui:94`, `page_split/SplitModeDialog.ui:199`, `output/ChangeDpiDialog.ui:149`, `page_layout/ApplyDialog.ui:97`, `deskew/ApplyDialog.ui:94`
- `Qt::ControlModifier` / `AltModifier` logic (unchanged, D-00b) in `select_content/ImageView.cpp`, `page_layout/ImageView.cpp`, `deskew/ImageView.cpp`, `zones/ZoneDefaultInteraction.cpp`, `zones/ZoneVertexDragInteraction.cpp`, `interaction/InteractiveXSpline.cpp`, `app/MainWindow.cpp`, `app/ThumbnailSequence.cpp`

### Reusable assets
- `src/qt_tests/` (offscreen, two screens since Phase 4) — home of the permanent D-12 checks.
- `src/app/retina_check/` — live Cocoa harness with settings under `build/`.
- `src/app/ui_snapshot/` — offscreen grab tool (note Phase 3 review finding WR-01: it compiles the GUI sources a second time).

</code_context>

<specifics>
## Specific Ideas

- German keyboard layout: ⌘+ is the zoom-in key the user presses; ⌘= is bound for completeness.
- The pattern the user chose twice: ⌘ for the main target (stages, page view), ⌥⌘ for the secondary one (Output tabs, thumbnails).
- Everything with a shortcut should be findable in the menu bar (the View menu for stages and zoom).
- No manual testing again: the user picked Claude-only verification, as in Phases 2 to 4.

</specifics>

<deferred>
## Deferred Ideas

- v2 Mac polish stays deferred: Dock app icon (POLISH-01), File > Open Recent (POLISH-02), proxy icon and unsaved-changes dot (POLISH-03), Dock bounce (POLISH-04), `.app` bundle (POLISH-05).
- Trackpad zoom feel (PITFALLS Pitfall 17) and gestures: out of scope; do not make the wheel behaviour worse.
- ⌘0 for zoom-to-fit and a shortcut cheat-sheet in Help (FEATURES MAC-26): not requested; possible later features.
- Not discussed, defaults recorded under Claude's Discretion: the Tools menu after Settings leaves it, the menu bar during dialogs, the batch sound, and the app-menu title.

</deferred>

---

*Phase: 05-mac-menus-and-shortcuts*
*Context gathered: 2026-11-14*
