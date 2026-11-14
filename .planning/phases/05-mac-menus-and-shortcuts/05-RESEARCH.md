# Phase 5: Mac Menus and Shortcuts - Research

**Researched:** 2026-11-14
**Domain:** Qt 6.12.0 Widgets on macOS 26 (Cocoa platform plugin): native menu bar and menu roles, window shortcuts, key names in text, context-menu triggers, native file panels, Cocoa-level test harness
**Confidence:** HIGH for what is in the repo and what Qt's source and docs state; MEDIUM for how the Cocoa layer behaves live (several points can only be settled by the harness, listed in Open Questions)

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

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

### Deferred Ideas (OUT OF SCOPE)
- v2 Mac polish stays deferred: Dock app icon (POLISH-01), File > Open Recent (POLISH-02), proxy icon and unsaved-changes dot (POLISH-03), Dock bounce (POLISH-04), `.app` bundle (POLISH-05).
- Trackpad zoom feel (PITFALLS Pitfall 17) and gestures: out of scope; do not make the wheel behaviour worse.
- ⌘0 for zoom-to-fit and a shortcut cheat-sheet in Help (FEATURES MAC-26): not requested; possible later features.
- Not discussed, defaults recorded under Claude's Discretion: the Tools menu after Settings leaves it, the menu bar during dialogs, the batch sound, and the app-menu title.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| MAC-01 | About, Preferences and Quit appear in the application menu (menu roles); Settings moves out of the Debug menu | Roles apply only to actions that are direct children of a top-level menu on the bar (Qt docs); `actionSettings`, `actionAbout`, `actionQuit` must stay declared in File/Help, not in a submenu. Role property timing, empty Help menu, `qt_mac_applicationName()` (CFBundleName) and the spike-verified Info.plist embed are under Pitfalls 1-3. |
| MAC-02 | Cmd+, opens Preferences | Explicit `Ctrl+,` sequence. `QKeySequence::Preferences` is EMPTY on the offscreen platform (spike), so the offscreen test must compare with `QKeySequence(Qt::CTRL \| Qt::Key_Comma)`; the live harness compares with the Cocoa `StandardKey`. |
| MAC-03 | Cmd+Shift+S saves the project under a new name | `actionSaveProjectAs` gets `Ctrl+Shift+S` (`QKeySequence::SaveAs` resolves to it offscreen too, spike). Flow untouched (CLEAN-05 is Phase 6). |
| MAC-04 | Cmd+= and Cmd+- zoom the page view | `⌘+` and `⌘=` on one action, `⌘-` on another, owned by `MainWindow` View-menu actions that call the public `ImageViewBase::setZoomLevel`; `MainWindow::findPrimaryImageView()` already finds the current view. Spike: both keys fire one action once. |
| MAC-05 | Existing shortcuts work with Cmd: New, Open, Save, Close, Quit, and Cmd+1… to switch stages | File shortcuts exist as `Ctrl+N/O/S/W/Q`; add `⌘1..⌘6` as primary shortcut of `actionSwitchFilter1..6` with the letter as second sequence. Move the Output tab shortcuts to `⌥⌘1..5` in the same commit (ambiguity, Pitfall 4). |
| MAC-06 | Tooltips, status tips and help text show Mac key names (⌘, ⌥) instead of "Ctrl"/"Alt" | Full inventory with verified baseline (zero hits expected after the change) and two Shift strings the CONTEXT inventory misses. XML-aware scan test design under Validation Architecture. |
| MAC-07 | In the zone editor, Backspace or Delete removes a zone, and creating a content box uses a key a MacBook has | Qt 6.11 key mapper: Mac delete key (`NSDeleteCharacter`) maps to `Key_Backspace`, fn+delete (`NSDeleteFunctionKey`) to `Key_Delete`. `QShortcut` takes one sequence, so two `QShortcut` objects; content box uses `QAction::setShortcuts`. Spike proves ShortcutOverride keeps ⌫, ⌦ and B out of spin boxes and line edits. |
| MAC-08 | Context menus open with a right-click, a two-finger click and a Control-click | Qt maps Control+left mouse down to the right button on Cocoa (`qnsview_mouse.mm`); a two-finger click is a `rightMouseDown` at NSEvent level. All reach `contextMenuEvent`. The zone menu reads `QCursor::pos()`, so tests and harness must set the cursor position. |
| MAC-09 | The menu bar stays populated while dialogs are open | Qt keeps the previous bar when the focus window has no bar and no parentless bar exists; with a parentless bar it replaces the main bar by that smaller bar. So adding the fallback bar can make MAC-09 worse. Measure first (Pitfall 6). |
| MAC-10 | The batch-finished alarm uses the system beep instead of a shell command | `QCocoaIntegration::beep()` is `NSBeep()`. Code at `MainWindow.cpp:1328-1337`. Beep is not observable offscreen; prove by source scan plus code review. |
| MAC-11 | Native macOS file dialogs are used everywhere, including the relinking dialog | Two `DontUseNativeDialog` uses in `RelinkingDialog.cpp:69,74`. `MainWindow.cpp:1862` ("Files to insert") is already native but its `ProxyModel` filter is dead code on a native panel (Pitfall 9). |
</phase_requirements>

## Summary

Phase 5 is mostly small, local edits in `MainWindow.ui`, `MainWindow.cpp`, `TabbedImageView.cpp`, `ZoneDefaultInteraction.cpp`, `select_content/ImageView.cpp`, `RelinkingDialog.cpp` and about 20 user-visible strings. The hard part is proof. Three structural facts shape the plan:

1. **`MainWindow` cannot be built by `qt_tests` today.** It is compiled only into the `scantailor-advanced` target and, a second time, into `ui_snapshot` (`src/app/CMakeLists.txt`). D-12's offscreen checks of roles and shortcuts need a construct-able `MainWindow`. The clean fix, which also closes open review finding WR-01, is a static library `app_gui` holding the GUI sources (all of `gui_only_sources` except `main.cpp`), linked by `scantailor-advanced`, `ui_snapshot`, `qt_tests` and the new harness. This is a Wave 0 task.
2. **Offscreen has no Cocoa theme.** Spike (Qt 6.12.0, offscreen): `QKeySequence(QKeySequence::Preferences)` and `QKeySequence(QKeySequence::Quit)` are empty there, while `SaveAs` works. Production code should therefore set explicit sequences (`Ctrl+,`, `Ctrl+Q`, `Ctrl+Shift+S`), and the offscreen tests compare against explicit sequences; the live harness compares with the real `StandardKey`.
3. **The live harness decides several bindings.** The machine's only enabled keyboard layout is German (TIS spike), so the harness measures the German-layout question for ⌥⌘1-5, ⌥⌘+/- and ⌘+ directly. Build the harness skeleton early (before the tests and strings that depend on the answer), because a ⌃⌘ fallback would change menu text, tooltips, status tips and tests together.

Everything else is conventional Qt: explicit `QAction::menuRole`, `setShortcuts` lists (first entry is the primary, drawn in the menu), `NativeText` glyphs (spike output: `⌘1`, `⌥⌘1`, `⌘+`, `⇧⌘S`, `⌫`, `⌦`), and `QApplication::beep()`.

**Primary recommendation:** Wave 0: build `app_gui` and an empty `TestMacQt` in `qt_tests`, plus a harness skeleton that records the German-layout, Backspace and menu-bar-during-dialog facts. Then the menu and shortcut work, the zone keys, the strings, the alarm and the native dialogs, each as a gated commit; set explicit key sequences in code, and set `menuRole` before the first show.

## Project Constraints (from CLAUDE.md)

Source: `/Users/christoph/Developer/scantailor-advanced/.claude/CLAUDE.md` and the user's global `~/.claude/CLAUDE.md`.

- C++17 only; no C++20. Keep the Qt/CMake architecture and the six-stage pipeline. Qt 6.5+ API only.
- Output for existing settings must not change; Phase 5 is GUI-only (D-00a).
- Synthetic test data only; nothing personal in the repo.
- Naming: PascalCase files, `m_` members, `lowerCamelCase` functions, Boost/Qt Test conventions (`Test` prefix for Boost, `tst_` prefix for Qt Test files in `src/qt_tests/`). Include guards `FILENAME_H`-style named after the file (the repo uses `SCANTAILOR_<MODULE>_<NAME>_H_` in newer headers; copy the neighbour's style). No `#pragma once`.
- New source files start with the project copyright header (copy from a neighbour).
- Parse functions use an `ok` out-parameter; do not swallow exceptions; keep `qDebug` output low.
- User-visible strings go through `tr()` / `QCoreApplication::translate()`.
- Format with `xcrun clang-format -i <file>` and check with `xcrun clang-format --dry-run -Werror <file>`.
- Start work through a GSD command; no direct repo edits outside a GSD workflow.
- Global user rules: walk the user through manual steps one at a time (not relevant here: D-11 means no manual steps); never access OmniFocus; never access files outside the project without asking. Do not write scratch files to `/tmp`; use `build/scratch/`.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Application menu (About, Settings…, Quit) | Qt Cocoa platform plugin (builds the native NSMenu from `menuRole`) | `src/app/MainWindow.ui` (declares the actions) | Qt moves role actions into the app menu at sync time; the app only declares actions and roles. |
| Menu bar structure (File, View, Tools) and shortcuts of menu items | `src/app` (`MainWindow.ui`, `MainWindow.cpp`) | Qt `QAction` + native key equivalents | Actions are window-level and own the shortcut; Cocoa draws the glyphs. |
| Stage switching keys ⌘1-⌘6 | `src/app` (`actionSwitchFilter1..6`) | none | Window-wide. |
| Output tab keys ⌥⌘1-⌥⌘5 | `src/core/filters/output/TabbedImageView` | none | Local to the Output stage view; must not collide with the window actions. |
| Page-view zoom keys (⌘+, ⌘=, ⌘-) | `src/app` View-menu actions | `ImageViewBase` public zoom API | One owner of the key (avoids an ambiguous shortcut); plain +/- stay in `ZoomHandler`. |
| Thumbnail zoom keys (⌥⌘+, ⌥⌘=, ⌥⌘-) | `src/app` (`actionMagnifyThumbnails`, `actionDiminishThumbnails`) | none | Existing actions, new keys. |
| Zone and content-box key handling | `src/core/zones/ZoneDefaultInteraction`, `src/core/filters/select_content/ImageView` | `QShortcutMap` (ShortcutOverride) | Shortcuts live on the view; Qt's override logic protects text fields. |
| Context-menu triggers (right, two-finger, Control-click) | Cocoa event translation inside Qt (`qnsview_mouse.mm`) | `ImageViewBase::contextMenuEvent` and the interaction handlers | The mapping to a right button happens below the app; the app only reacts to `QContextMenuEvent`. |
| Key names in text | `src/core` view classes and `.ui` files | `src/app` | Pure copy change, no logic. |
| Batch-finished sound | `src/app/MainWindow.cpp` | `QCocoaIntegration::beep()` = `NSBeep()` | App calls `QApplication::beep()`; the platform plays the alert sound. |
| Native file panels | Qt Cocoa (`QCocoaFileDialogHelper`, NSOpenPanel/NSSavePanel) | `src/app` call sites | The app only stops forcing the widget dialog. |
| Proof of Cocoa-level behaviour | New live harness (test-only Objective-C++) | `qt_tests` for everything offscreen can show | The offscreen platform has no native menu bar, panels or key mapper. |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt 6 Widgets/Gui/Core | 6.12.0 (`qt_runtime = 6.12.0` in `src/pipeline_tests/golden/flat/MANIFEST.txt`; spikes linked against the Homebrew install) | `QAction`, `QMenuBar`, `QShortcut`, `QKeySequence`, `QFileDialog`, `QApplication::beep()` | Already the project's framework. Everything needed is in it. |
| Qt Test (`Qt6::Test`) | 6.12.0 | `TestMacQt` in `qt_tests`: `QTest::keyClick`, `qWaitForWindowActive`, `QSignalSpy` | The project's existing UI-test framework. |
| Apple frameworks AppKit, Carbon (HIToolbox `TIS*`) | macOS 26 SDK (Xcode CLT) | Live harness only, in a test-only `.mm` file: `NSEvent` synthesis, `NSMenu` introspection, `NSRunningApplication`, current keyboard layout | Required for Cocoa-level proof (D-12). Never linked into the app. |

### Supporting
| Tool | Version | Purpose | When to Use |
|------|---------|---------|-------------|
| `xcrun clang-format` | Xcode CLT | Formatting gate | Every touched C++ file |
| `otool` | Xcode CLT | Check the embedded `__info_plist` section of a binary | Only if the Info.plist embed is adopted |
| CMake `enable_language(OBJCXX)` | CMake 3.25+ | Compile the harness `.mm` helper | Harness only, inside `if (BUILD_TESTS)` |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Static library `app_gui` for `qt_tests`, `ui_snapshot`, harness | Add `MainWindow.cpp` and friends to `qt_tests` sources | A third compile of the GUI sources, a third copy of the CMake list; worse than the library. |
| Explicit sequences in code | `QKeySequence::Preferences` / `Quit` | The standard keys are empty offscreen (spike), so the permanent test could not assert them. |
| Embedded `__info_plist` for the app-menu name | Accept the process name | See Pitfall 2; try the embed, measure, keep only if the system-drawn title changes. |

**Installation:** none. No new package is installed; no Homebrew formula or vcpkg port is added.

**Version verification:** `build/macos-release/CMakeCache.txt` has `Qt6_DIR:PATH=/opt/homebrew/lib/cmake/Qt6`; `MANIFEST.txt` has `qt_runtime = 6.12.0`. The spikes under `build/scratch/qt-spike` configured against Qt 6.5+ and ran on the same install. [VERIFIED: build/scratch/qt-spike run, 2026-11-14]

## Package Legitimacy Audit

No external package is installed or recommended in this phase (Qt, AppKit and Carbon are the existing toolchain and the macOS SDK). The audit is therefore empty.

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| none | - | - | - | - | - | - |

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

## Architecture Patterns

### System Architecture Diagram

```
 user key / click                         (live harness feeds NSEvent here)
        |
        v
  Cocoa: NSApp sendEvent
        |
        +--> keyDown with Command --> NSApp mainMenu performKeyEquivalent --> native menu item
        |                                   |                                      |
        |                                   | (no match / not a menu item)         | QCocoaMenuItem::activated
        |                                   v                                      v
        +--> QNSView keyDown --> sendAsShortcut --> QShortcutMap ---------> QAction::activate (once)
        |                           |  (ShortcutOverride goes to focus widget first:
        |                           |   spin box / line edit accept Backspace, Delete, text keys)
        |                           v
        |                     QShortcut::activated  (zone remove, zone mode, plain +/-, Output tabs)
        |
        +--> mouseDown: Control+left  --> Qt sets RightButton --> QWidgetWindow --> QContextMenuEvent
             rightMouseDown (right-click, two-finger click) --------------------^
                                                                        |
                                                                        v
                                   ImageViewBase::contextMenuEvent --> InteractionHandler chain
                                      ZoneDefaultInteraction::onContextMenuEvent
                                      select_content::ImageView::onContextMenuEvent (popup)
                                      page_layout::ImageView::onContextMenuEvent (popup)
                                      output::ImageView::contextMenuEvent (QMenu::exec)

 App start: MainWindow ctor -> setupUi -> roles on 3 actions -> menus built (File, View, Tools, Help)
            -> window shown -> QCocoaMenuBar merges About/Settings/Quit into the app menu
            -> NSApp.mainMenu = this window's bar (no parentless bar exists)

 Dialog opens (QDialog::exec or native NSOpenPanel): focus window has no bar
            -> QCocoaMenuBar::updateMenuBarImmediately returns early, NSApp.mainMenu unchanged
```

### Recommended Project Structure
```
src/app/
├── CMakeLists.txt            # app_gui static library; scantailor-advanced = main.cpp + resources + app_gui
├── MainWindow.{h,cpp,ui}     # roles, View menu, shortcuts, zoom actions, beep
├── mac_check/                # new live harness (sibling of retina_check)
│   ├── main.cpp              # sections: menubar | dialogs | keys | zone | context | panels | all
│   └── CocoaProbe.{h,mm}     # test-only Objective-C++: NSEvent synthesis, NSMenu dump, TIS layout
src/qt_tests/
├── tst_mac_qt.{h,cpp}        # TestMacQt, registered in main.cpp and CMakeLists.txt
.planning/phases/05-mac-menus-and-shortcuts/records/   # harness verdicts
```

### Pattern 1: Explicit role, explicit key sequences, first sequence is the primary
**What:** Declare About, Settings and Quit as direct children of a top-level menu (File and Help), give them roles, and give every action its sequences in code or `.ui`. The first sequence of `setShortcuts` is the one drawn in the menu.
**When to use:** All menu items in this phase.
**Example:**
```cpp
// Source: https://doc.qt.io/qt-6/qaction.html (MenuRole, setShortcuts: "The first element of the list is the primary shortcut.")
actionAbout->setMenuRole(QAction::AboutRole);
actionSettings->setMenuRole(QAction::PreferencesRole);
actionSettings->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma));  // explicit: StandardKey is empty offscreen
actionQuit->setMenuRole(QAction::QuitRole);
actionSaveProjectAs->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
// Stage actions: primary ⌘n (drawn in the View menu), letter second (D-03).
const Qt::Key digits[6] = {Qt::Key_1, Qt::Key_2, Qt::Key_3, Qt::Key_4, Qt::Key_5, Qt::Key_6};
const Qt::Key letters[6] = {Qt::Key_O, Qt::Key_E, Qt::Key_N, Qt::Key_V, Qt::Key_K, Qt::Key_P};
for (int i = 0; i < 6; ++i) {
  stageActions[i]->setShortcuts({QKeySequence(Qt::CTRL | digits[i]), QKeySequence(letters[i])});
}
```
Roles are set before the window's first `show()`; Qt docs: "The menuRole property can only be changed before the actions are placed in the menubar on macOS, usually just before the first application window is shown." [CITED: doc.qt.io/qt-6/qaction.html]. Every other action that is a direct child of a top-level menu gets `NoRole` (UI-SPEC).

### Pattern 2: Two QShortcut objects for two keys
**What:** `QShortcut` holds one `QKeySequence` (up to four chords, not alternatives). For ⌫ and ⌦ on the zone editor create two shortcuts; for the content-box action use `QAction::setShortcuts`.
**Example:**
```cpp
// ZoneDefaultInteraction.cpp (constructor); header gets one more unique_ptr<QShortcut> member.
m_removeZoneShortcut = std::make_unique<QShortcut>(Qt::Key_Delete, &m_context.imageView());          // ⌦
m_removeZoneBackspaceShortcut = std::make_unique<QShortcut>(Qt::Key_Backspace, &m_context.imageView()); // ⌫
for (QShortcut* s : {m_removeZoneShortcut.get(), m_removeZoneBackspaceShortcut.get()}) {
  QObject::connect(s, &QShortcut::activated, &m_context.imageView(),
                   std::bind(&ZoneDefaultInteraction::removeZoneUnderMouse, this));
}
// select_content/ImageView.cpp
create->setShortcuts({QKeySequence(Qt::Key_B), QKeySequence(Qt::Key_Insert)});
remove->setShortcuts({QKeySequence(Qt::Key_Backspace), QKeySequence(Qt::Key_Delete)});
```
`QKeySequence(Qt::Key_Insert)` has the same sequence as `QKeySequence("Ins")`. Spike: `B`, `Insert` each fire the action once; `Backspace` and `Delete` each fire their shortcut once when sent to a window with no editable focus widget. [VERIFIED: build/scratch/qt-spike run]

### Pattern 3: One owner per key (no ambiguous shortcuts)
**What:** Qt activates nothing when two live shortcuts share a sequence in one context ("ambiguous shortcut overload"). Each key below has exactly one owner after the phase: ⌘1-⌘6 = stage actions; ⌥⌘1-⌥⌘5 = `TabbedImageView` shortcuts; ⌘+/⌘=/⌘- = View zoom actions; ⌥⌘+/⌥⌘=/⌥⌘- = thumbnail actions; plain +/- = `ZoomHandler`.
**Consequence:** `TabbedImageView` moves to ⌥⌘1-5 in the same commit that gives the stage actions ⌘1-⌘5, or in an earlier one; otherwise ⌘1-⌘5 is ambiguous in the Output stage for the length of the gap.

### Pattern 4: Zoom from the View menu through the public view API
`ImageViewBase::setZoomLevel` and `setWidgetFocalPointWithoutMoving` are public (`src/core/ImageViewBase.h`, `public:` starts at line 49, `protected:` at line 246, both methods lie between). `MainWindow::findPrimaryImageView(QWidget*)` (`MainWindow.cpp:733-745`) already resolves the Output tab widget to the current `ImageViewBase`. The step is `2^(±1/6)` as in `ZoomHandler`. Disable the two actions when `findPrimaryImageView(m_imageFrameLayout->widget(0))` is null; update that at the end of `setImageWidget` and `removeImageWidget`.
```cpp
void MainWindow::zoomCurrentView(const double factor) {
  ImageViewBase* const view = m_imageFrameLayout->count() ? findPrimaryImageView(m_imageFrameLayout->widget(0)) : nullptr;
  if (!view) { return; }
  const double zoom = std::max(1.0, view->zoomLevel() * factor);
  view->setWidgetFocalPointWithoutMoving(QRectF(view->rect()).center());
  view->setZoomLevel(zoom);
}
```
(`ZoomHandler::zoom` additionally calls `moveTowardsIdealPosition(20)` at zoom 1.0 when zooming out and honours the interaction permitter; the menu path does not need either. Its `CURSOR` focus uses `m_virtualMousePos`, which stays `(0,0)` until the first mouse move, so "cursor if over the view, else the view centre" in the UI-SPEC is not what the plain keys do today; the menu path uses the view centre.)

### Anti-Patterns to Avoid
- **Roles on actions in the Units submenu or in a menu that is not on the bar:** ignored by Qt ("Actions in submenus of those menus are never moved"). `actionSettings` and `actionAbout` must stay direct children of `menuFile`/`menuHelp` (or any top-level menu).
- **A parentless `QMenuBar` as a blind fix for MAC-09:** it replaces the main window's bar while a dialog is focused (Pitfall 6).
- **`setShortcut` called twice, or one `QShortcut` for two keys:** the second call replaces the first.
- **Asserting `QKeySequence::Preferences` or `Quit` in an offscreen test:** both are empty there.
- **Typing `+` separators into menu text:** shortcuts come from `QAction`, never from the text.
- **Placing `Info.plist` next to an executable:** CFBundle treats the directory as a flat bundle (spike); keep the file under `cmake/` or `src/app/`, never in the build output directory.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Moving About/Settings/Quit into the app menu | A custom NSMenu in Objective-C++ | `QAction::menuRole` | Out of Scope (D-00b); Qt does the merge. |
| Mac key glyphs | A string table of ⌘/⌥/⇧ | `QKeySequence::toString(QKeySequence::NativeText)` for derived text; literal symbols in fixed `tr()` strings (UI-SPEC choice) | Spike output shows Qt already yields `⌘1`, `⌥⌘1`, `⌥⌘+`, `⇧⌘S`, `⌫`, `⌦`. Use it in tests to compare menu glyphs. |
| Alert sound | `afplay`, `QProcess`, `std::system`, `QSound` | `QApplication::beep()` | `QCocoaIntegration::beep()` is `NSBeep()` [CITED: qcocoaintegration.mm 6.11]. |
| File panel | `QFileDialog` forced to widgets | Static `QFileDialog` functions without options | Native by default on macOS [CITED: doc.qt.io/qt-6/qfiledialog.html]. |
| Alternative-key matching under Option on German | Own key translation | Qt's `QAppleKeyMapper::possibleKeyCombinations` | It builds the Command-layer and other layer variants of an event for shortcut matching [CITED: qapplekeymapper.mm 6.11]; the harness verifies the result. |
| Context menu for Control-click | A special case for `Qt::RightButton` or `Qt::MetaModifier` | Nothing | Qt turns Control+left into a right button before widgets see it [CITED: qnsview_mouse.mm 6.11]. |
| Ignoring text fields for single-key shortcuts | Focus checks in slots | Qt's `ShortcutOverride` | `QLineEdit` and `QAbstractSpinBox` accept Backspace, Delete and text keys [CITED: qwidgetlinecontrol.cpp, qabstractspinbox.cpp 6.11; VERIFIED: spike]. |

**Key insight:** Nearly all Mac behaviour here is already implemented by Qt's Cocoa plugin. The risk is not missing machinery but wrong declarations (roles on the wrong action, an ambiguous key, an empty StandardKey offscreen) and unproven Cocoa-level behaviour.

## Requirement to Source Map

Line numbers are from files opened this session.

| Req | File and line | Current value (verbatim) | Change |
|-----|---------------|--------------------------|--------|
| MAC-01 | `src/app/MainWindow.ui:63-86` Tools menu | `<addaction name="actionDebug"/>` ... `<addaction name="actionSettings"/>` ... `<addaction name="menuUnits"/>` | Remove `actionSettings` from `menuDebug`; declare it in `menuFile` (a direct child of a top-level menu is required for the role). Move `actionDebug` (plus its separator) to the end. |
| MAC-01 | `MainWindow.ui:87-109` | menus `menuFile`, `menuDebug`, `menuHelp`; bar order `menuFile`, `menuDebug`, `menuHelp` | Insert `menuView` between File and Tools; `actionAbout` stays in `menuHelp`. |
| MAC-01 | `MainWindow.cpp:314-318` | `#ifdef ENABLE_DEBUG_FEATURES` / `connect(actionDebug, SIGNAL(toggled(bool)), SLOT(debugToggled(bool)));` / `#else` / `actionDebug->setVisible(false);` | In the `#else` branch also hide the separator before Debug Mode (no object name in `.ui`; find it as the action before `actionDebug` in `menuDebug->actions()`). `ENABLE_DEBUG_FEATURES` follows `DEVELOPER_VERSION`, default ON in Debug builds only (`CMakeLists.txt:127-132`), so the Tools menu differs between the Release and Debug test runs. |
| MAC-01/02 | `MainWindow.cpp:320, 328` | `connect(actionSettings, SIGNAL(triggered(bool)), this, SLOT(openSettingsDialog()));` and `connect(actionQuit, SIGNAL(triggered(bool)), this, SLOT(close()));` | Unchanged; add roles. `closeEvent` (`MainWindow.cpp:544-551`) ignores the first close and starts a 0 ms timer, so Quit is asynchronous: tests must not expect `close()` to finish synchronously. |
| MAC-02 | `MainWindow.ui` `actionSettings` | `<string notr="true"/>` (empty shortcut) | `Ctrl+,` |
| MAC-03 | `MainWindow.ui` `actionSaveProjectAs` | `<string>Save Project As ...</string>`, empty shortcut | `Ctrl+Shift+S`; text `Save Project As…` |
| MAC-04 | `MainWindow.cpp:216-217, 262-263` | `addAction(actionMagnifyThumbnails);` `connect(actionMagnifyThumbnails, &QAction::triggered, magnifyThumbnails);` | New `actionZoomIn`, `actionZoomOut` (View menu), slot `zoomCurrentView`; thumbnail keys to ⌥⌘. |
| MAC-05 | `MainWindow.ui:1103, 1147, 1158, 1191, 1202` | `Ctrl+S`, `Ctrl+N`, `Ctrl+O`, `Ctrl+W`, `Ctrl+Q` | Unchanged (all already ⌘ on Cocoa). Add ⌘1-⌘6 to `actionSwitchFilter1..6` (current strings `O`, `E`, `N`, `V`, `K`, `P`, shortcut properties at lines 1292-1363). |
| MAC-05/D-02 | `src/core/filters/output/TabbedImageView.cpp:17-23` | `setStatusTip(tr("Use Ctrl+1..5 to switch the tabs."));` and five `new QShortcut(Qt::CTRL \| Qt::Key_1..5, this)` | `Qt::CTRL \| Qt::ALT \| Qt::Key_1..5`; new status tip. |
| D-07 | `MainWindow.ui:1511, 1519` | `<string notr="true">Ctrl++</string>` and `<string notr="true">Ctrl+-</string>` | `⌥⌘+` list (`Ctrl+Alt++`, `Ctrl+Alt+=`), `Ctrl+Alt+-`. |
| MAC-06 | see String inventory below | | |
| MAC-07 | `src/core/zones/ZoneDefaultInteraction.cpp:33` | `m_removeZoneShortcut = std::make_unique<QShortcut>(Qt::Key_Delete, &m_context.imageView());` | Second shortcut for `Key_Backspace` (Pattern 2). Header `ZoneDefaultInteraction.h` gains one member. |
| MAC-07 | `src/core/filters/select_content/ImageView.cpp:152-153` | `create->setShortcut(QKeySequence("Ins"));` and `remove->setShortcut(QKeySequence("Delete"));` | `setShortcuts` lists (Pattern 2). Note `removeContentBox()` (lines 181-195) is not "under the pointer": it removes the one content box whenever one exists. |
| MAC-08 | `src/core/ImageViewBase.cpp:558-563` | `void ImageViewBase::contextMenuEvent(QContextMenuEvent* event) {` ... `m_rootInteractionHandler.contextMenuEvent(event, m_interactionState);` | No change. |
| MAC-08 | `src/core/zones/ZoneContextMenuInteraction.cpp:48-52, 135-148` | `context.imageView().mapFromGlobal(QCursor::pos())` and `if (m_extraDelaysDone++ < 1) {` `QTimer::singleShot(200, this, SLOT(menuAboutToHide()));` | The 200 ms delay is **unconditional**: there is no `Q_OS_MAC` guard anywhere in `src/` (grep for `Q_OS_` returns nothing). Keep (default). `zonesUnderMouse` reads `QCursor::pos()`, not the event position. |
| MAC-10 | `src/app/MainWindow.cpp:1328-1337` | `QApplication::alert(this);  // Flash the taskbar entry.` / `QString extPlayCmd("play /usr/share/sounds/freedesktop/stereo/bell.oga");` / `settings.value("main_window/external_alarm_cmd", extPlayCmd)` / `Q_UNUSED(std::system(cmd.toStdString().c_str()));` | Replace the block by `QApplication::beep();` inside `if (m_checkBeepWhenFinished())`; keep `alert`. Only this file references `external_alarm_cmd` in code. |
| MAC-11 | `src/app/RelinkingDialog.cpp:67-74` | `QString(), nullptr, QFileDialog::DontUseNativeDialog);` and `dir.exists() ? prefixPath : m_projectFileDir, QFileDialog::DontUseNativeDialog);` | Drop the options; the `getOpenFileName` call needs `QString(), nullptr` replaced by the shorter overload; remove the comment block at lines 76-79 (`// So what's wrong with native dialogs? ...`). |
| MAC-11 | `src/app/MainWindow.cpp:1862-1866` | `auto dialog = std::make_unique<QFileDialog>(this, tr("Files to insert"), dialogDir);` / `dialog->setProxyModel(new ProxyModel(*m_pages));` | Already native; see Pitfall 9. |
| MAC-11 | `MainWindow.cpp:1427, 1471`, `ProjectFilesDialog.cpp:244, 258`, `OutOfMemoryDialog.cpp:62` | static `getSaveFileName`, `getOpenFileName`, `getExistingDirectory` | Already native. |

### String inventory (MAC-06) with baseline

Baseline measured by grep this session (string literals in `src/` outside tests, and `.ui` text outside `notr`):

- C++ literals with `Ctrl|Alt|Control|Cmd|Command|Option|Del|Ins|LMB|Meta` as a word: `TabbedImageView.cpp:17`, `ZoneDefaultInteraction.cpp:20, 24, 30`, `deskew/ImageView.cpp:33`, `InteractiveXSpline.cpp:58`, and the key sequence `QKeySequence("Ins")` at `select_content/ImageView.cpp:152` (not a user-visible string).
- `.ui` strings: nine `Use Ctrl+Click / Shift+Click to select multiple pages.` (files named in CONTEXT), `MainWindow.ui:425, 482, 549, 934`, and the multi-line rich text at `page_layout/OptionsWidget.ui:905-907`.
- `InteractiveXSpline.cpp:206` named in CONTEXT is a code comment (`} else {  // Ctrl or Shift is pressed`), not a user-visible string. Only line 58 needs a change; the comment is left alone (the C++ scan must skip comments). [VERIFIED: src/core/interaction/InteractiveXSpline.cpp:206]
- **Not in the CONTEXT inventory, found here** (they say `Shift`, which UI-SPEC rewrites as ⇧ in other strings): `select_content/ImageView.cpp:45` ("Hold Shift to drag a box.") and `interaction/DragHandler.cpp:22` ("holding down the Shift key"). Convert both for consistency: `Hold ⇧ to drag a box.` and `holding down the ⇧ key.` The `Shift with corners while they are in black.` strings (`select_content/OptionsWidget.ui:116`, `DefaultParamsDialog.ui:900`) use "shift" as a verb; leave them.

## Common Pitfalls

### Pitfall 1: Roles only work on direct children of top-level menus, and only before first show
**What goes wrong:** Settings or About vanish, show twice, or Settings is disabled.
**Why:** Qt moves a role action only if it lives directly in a menu of the bar. Doc: "The role only affects items in the menubar's immediate menus. Actions in submenus ... are never moved." Without a match the Settings item is disabled. [CITED: doc.qt.io/qt-6/qaction.html, qmenubar.html]
**How to avoid:** Keep `actionSettings` in `menuFile` (declare it there when removing it from `menuDebug`), `actionAbout` in `menuHelp`, `actionQuit` in `menuFile`. Set roles in `.ui` (property `menuRole`, `[ASSUMED]` that uic emits it before `addAction`) or in code right after `setupUi(this)` and before `show()`. A permanent test asserts that each role action is contained in one of `menuBar()->actions()`'s menus.
**Warning signs:** harness NSMenu dump lacks the three items in the app menu or lists them twice.

### Pitfall 2: Application name comes from `CFBundleName`, not from the application name
**What goes wrong:** "About scantailor-advanced", "Quit scantailor-advanced" and the app-menu title use the executable name.
**Why:** `qt_mac_applicationName()` reads `CFBundleName` from the main bundle's info dictionary and falls back to the process name. It does not use `QCoreApplication::applicationName` [CITED: qcocoahelpers.mm 6.11, qcocoamenuloader.mm 6.11 uses it for the About/Hide/Quit titles]. The settings paths depend on `Application::setApplicationName` and `setOrganizationName` (`src/app/main.cpp:25-26`), which are not involved.
**Verified spike (`build/scratch/plist-spike`):** an executable linked with `-Wl,-sectcreate,__TEXT,__info_plist,<file>` makes `CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), CFSTR("CFBundleName"))` return the plist value; without it the call returns null; a plain `Info.plist` lying next to the executable fakes a bundle and gives a false positive. `bundleIdentifier` stays null with a plist that has only `CFBundleName`. [VERIFIED: build/scratch/plist-spike run, 2026-11-14]
**Not verified:** whether the system-drawn bold title in the menu bar and `NSRunningApplication.localizedName` change (the spike did not start NSApp). The harness records `NSRunningApplication.currentApplication.localizedName` and the Qt-set item titles. Adopt the embed only if Qt's item titles ("About ScanTailor Advanced") improve, keep it minimal (`CFBundleName` only, no `CFBundleIdentifier`: an identifier may change Qt's `QStandardPaths` result for app-data and cache locations), and prove `QSettings().fileName()` and `QStandardPaths::writableLocation(AppDataLocation)` are byte-identical before and after, in the harness. CMake: `target_link_options(scantailor-advanced PRIVATE "LINKER:-sectcreate,__TEXT,__info_plist,${CMAKE_SOURCE_DIR}/src/app/Info.plist")` plus `set_property(TARGET scantailor-advanced APPEND PROPERTY LINK_DEPENDS ...)`. `[ASSUMED]` that CMake's `LINKER:` prefix yields `-Wl,-sectcreate,...` unchanged (the spike used `-Wl,` directly).

### Pitfall 3: Empty Help menu and macOS-injected items
**What goes wrong:** An empty "Help" menu appears, or "Enter Full Screen" shows up in View.
**Why:** After About is merged, `menuHelp` has no visible item. Qt's Cocoa menu bar code hides menus without visible items (`[ASSUMED]` from memory of `syncMenu_helper`; the harness records it). macOS may add "Enter Full Screen" to a menu named "View" on fullscreen-capable windows (`[ASSUMED]`).
**How to avoid:** Keep the UI-SPEC choice (leave Help in the `.ui`); the harness dump decides. If an empty Help shows, remove `menuHelp` from the bar and declare `actionAbout` in `menuFile` instead (role still applies). Accept any macOS-injected item and record it; the permanent test inspects the Qt menu, not the NSMenu.

### Pitfall 4: Ambiguous shortcuts silently disable both owners
**What goes wrong:** ⌘1 does nothing in the Output stage; Qt warns "Ambiguous shortcut overload".
**Why:** `TabbedImageView` registers `Qt::CTRL | Qt::Key_1..5` today (`TabbedImageView.cpp:19-23`).
**How to avoid:** Change the tabs first (or in the same commit). Permanent test: with the Output stage's `TabbedImageView` shown inside a window that holds the stage actions, `QTest::keyClick` ⌘1 triggers `actionSwitchFilter1` exactly once and changes no tab; ⌥⌘1 changes the tab and triggers no action. Spike: an action present in both a menu and the window fires once.

### Pitfall 5: `QKeySequence` StandardKeys are platform-theme data
**What goes wrong:** A test written as `QCOMPARE(actionSettings->shortcut(), QKeySequence(QKeySequence::Preferences))` compares two empty sequences offscreen, passing for the wrong reason, or fails when the action has the real `Ctrl+,`.
**Why:** Spike on offscreen: `prefs native:  portable: ` and `quit native:  portable: ` (empty) while `SaveAs` prints `⇧⌘S`. [VERIFIED: build/scratch/qt-spike run]
**How to avoid:** Explicit sequences in code and tests; the harness (Cocoa theme) asserts `QKeySequence::keyBindings(QKeySequence::Preferences)` contains the action's shortcut.

### Pitfall 6: A parentless menu bar can make MAC-09 worse
**What goes wrong:** While Settings is open the bar shrinks to File and Help (the two menus of the "fallback" bar), or the whole bar greys out.
**Why (Qt 6.11 source summary):** `QCocoaMenuBar::updateMenuBarImmediately` starts from the global (parentless) bar, replaces it by the focus window's own bar if it has one, and returns early if neither exists, leaving `NSApp.mainMenu` as it was. `shouldDisable` greys the bar only when a visible application-modal window exists next to another visible top-level window. [CITED: qcocoamenubar.mm 6.11] Consequences: (a) today, with no parentless bar, a dialog keeps the main window's bar populated and, because of the early return, probably not greyed (`[ASSUMED]`; MAC-09 as worded then already holds); (b) with a parentless bar, dialogs get that bar instead, greyed, and populated only with what it holds.
**How to avoid:** Measure first (UI-SPEC step 1). If a fallback is needed, the parentless bar must contain the same menus as the main bar (File, View, Tools), built from the same `QAction` objects; File+Help only would shrink the bar.
**Second risk to measure:** with the early return the enabled state is not updated, so menu items may stay enabled under a modal dialog, and ⌘N/⌘O might fire through the native key equivalent (Qt's own shortcut map blocks them). The harness posts ⌘N while Settings is open and records whether `newProject` runs.

### Pitfall 7: Zone and content-box key handling
**What goes wrong:** ⌫ edits the field and also deletes a zone, or ⌫ never arrives.
**Facts:** Mac delete key (`NSDeleteCharacter`) maps to `Qt::Key_Backspace`; fn+delete (`NSDeleteFunctionKey`) to `Qt::Key_Delete` [CITED: qapplekeymapper.mm 6.11]. QTBUG-33015 ("Delete and Backspace menu item shortcuts don't work on Mac") is a Qt 5.1-era report about menu-item key equivalents; it is not evidence about window-level `QShortcut` under Qt 6 [CITED: bugreports.qt-project.org/browse/QTBUG-33015]. `QLineEdit` accepts `ShortcutOverride` for Backspace and Delete unless read-only, and for text keys without or with Shift; `QAbstractSpinBox` forwards the event to its line edit [CITED: qwidgetlinecontrol.cpp, qabstractspinbox.cpp 6.11; VERIFIED: spike, spin box accepts Backspace and B; line edit accepts Delete; plain widget does not].
**Limits to respect in tests:** a non-editable `QComboBox` and a `QCheckBox` do not accept the override, so ⌫ and B fire there (as the existing letter shortcuts Z, X, C, D, O, E, ... already do). UI-SPEC's wording "combo-box field" is true only for editable combos. Assert the spin box and the line edit (as the UI-SPEC test row does) and nothing about non-editable combos.
**Test mechanics:** `QTest::keyClick(w, key)` sends the `ShortcutOverride` to `w`, not to the focus widget (spike: ⌘1 and `O` fired although a spin box had focus). Send the key to `QApplication::focusWidget()`, or directly to the spin box.
**Two bindings, one action:** `QShortcut` needs two objects; `ZoneDefaultInteraction` is rebuilt after every interaction, so both live as `unique_ptr` members like the existing one.

### Pitfall 8: Context-menu tests depend on the cursor position
`ZoneContextMenuInteraction::zonesUnderMouse` uses `QCursor::pos()` (`ZoneContextMenuInteraction.cpp:51`), and `ZoneDefaultInteraction::removeZoneUnderMouse` uses the spline found by the last mouse-move event. Offscreen tests need `QCursor::setPos()` (the offscreen platform has a settable cursor) and a `QTest::mouseMove` before the key. The live harness must `QCursor::setPos` too, which moves the real pointer for an instant (`[ASSUMED]` no privacy permission needed, `CGWarpMouseCursorPosition` underneath).
`QTest::mouseClick(viewport, Qt::RightButton)` offscreen goes through `QWidgetWindow` and produces the `QContextMenuEvent` (Qt synthesises it for right-button press/release in `QWidgetWindow::handleMouseEvent`, [CITED: qwidgetwindow.cpp 6.11]), so offscreen proves "right button reaches `contextMenuEvent`"; Control-click and the two-finger click are Cocoa-only.
`QMenu::popup` on Cocoa is a native NSMenu with a blocking tracking loop; the harness cannot click an item with a synthetic NSEvent reliably. Measure "the menu opened" (`aboutToShow` counter, handler invocation) and dismiss it from a main-queue block; the "first click works" claim and the 200 ms measurement may be unprovable in-process, in which case keep the delay and say so. (Medium to low confidence; first harness spike.)
A two-finger click and a right-click are the same `rightMouseDown` at NSEvent level, so in-process injection cannot tell them apart; say that in the record.

### Pitfall 9: The "Files to insert" dialog and relinking
- `MainWindow.cpp:1862-1866` builds a plain `QFileDialog` (exact class, so `canBeNativeDialog()` is true; no `DontUseNativeDialog`), sets a `ProxyModel` that hides files already in the project, and a name filter. On a native panel the proxy is never consulted (the panel lists the folder itself), so the "files already in the project are hidden" behaviour is lost today, not by this phase [CITED: qfiledialog.cpp 6.11 summary: no proxy condition in `canBeNativeDialog`; native name filters are honoured]. Out of scope to repair; record it as an observation in the MAC-11 record. The harness opens this dialog (with a real `ProjectPages`, or a stub) and records the panel class.
- Native folder panel: `QCocoaFileDialogHelper` sets `canChooseFiles` NO and `canChooseDirectories` YES for directory mode, so files appear greyed [CITED: qcocoafiledialoghelper.mm 6.11]. That is why upstream forced the widget dialog (comment in `RelinkingDialog.cpp:76-79`). Accept (UI-SPEC E12).
- The dialog title goes to `m_panel.title` and no message is set; the "Substitution File for %1" text may not be visible on macOS. Accept and record.
- The native panel returns existing paths only, so the "missing file" worry of the old comment does not arise; `RelinkablePath::normalize` and the empty-path early return stay.

### Pitfall 10: ⌥⌘+ and ⌥⌘- are macOS Accessibility Zoom shortcuts
Apple: with "Use keyboard shortcuts to zoom" turned on (System Settings > Accessibility > Zoom), ⌥⌘+ and ⌥⌘- zoom the screen before the app sees them [CITED: support.apple.com/guide/mac-help/mchl779716b8/mac]. The setting is off by default. D-07 keeps ⌥⌘+/-; tell the user in the report that a dead ⌥⌘+ means that setting is on. Reading the setting would mean reading `~/Library` preferences, which the project rule forbids; do not.

### Pitfall 11: Gate and CMake side effects
`gate.sh` fails only on configure/build/ctest/goldens or a missing parent reference, not on a non-empty command diff; Phase 4 recorded expected diffs per commit (`records/gate-log.md`). The `app_gui` library and the new `qt_tests` link line produce a larger diff once; record it as expected. `enable_language(OBJCXX)` inside `if (BUILD_TESTS)` adds a language at configure time; compile options in the top-level `CMakeLists.txt` are guarded by `$<COMPILE_LANGUAGE:C,CXX>`, so the `.mm` file does not get `-ffp-contract=off`, which is irrelevant for a harness. Do not add `retina_check`'s sibling to the gate's TARGETS list (retina_check is not in it either).

### Pitfall 12: Debug versus Release differ in the Tools menu
`actionDebug` is visible only when `ENABLE_DEBUG_FEATURES` is defined (`DEVELOPER_VERSION`, ON by default for Debug builds only). Permanent tests must branch on `#ifdef ENABLE_DEBUG_FEATURES` (include `config.h`, as `main.cpp` does) when asserting Tools contents, or the Debug run fails while Release passes.

### Pitfall 13: `closeEvent` is asynchronous
`actionQuit` -> `close()` -> `closeEvent` ignores the event and starts `startTimer(0)`; `timerEvent` calls `closeProjectInteractive()` and only then closes for real (`MainWindow.cpp:544-562`). A test that triggers `actionQuit` must not leave a pending close with a modal prompt: in a fresh offscreen window with no project loaded, `closeProjectInteractive()` has nothing to ask. Prefer asserting `actionQuit` is connected (via `QSignalSpy` on a window-level `close` hook or the meta-object connection list) over triggering it.

## Code Examples

### CMake: GUI sources as a library (Wave 0)
```cmake
# src/app/CMakeLists.txt (sketch; AUTOUIC/AUTOMOC already on)
set(gui_only_sources_no_main ${gui_only_sources})
list(REMOVE_ITEM gui_only_sources_no_main main.cpp)
add_library(app_gui STATIC ${gui_only_sources_no_main} ${gui_only_ui_files})
target_link_libraries(app_gui PUBLIC core)
target_include_directories(app_gui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    "$<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_INCLUDE_DIRECTORIES>")
add_executable(scantailor-advanced main.cpp ${resource_files})
target_link_libraries(scantailor-advanced PRIVATE app_gui)
# Resources stay on the executables: .qrc objects inside a static library are dropped by the linker
# unless referenced. qt_tests and the harness need ${resource_files} too (set AUTORCC ON there).
```
`src/qt_tests/CMakeLists.txt` adds `app_gui`, `${resource_files}`, `CMAKE_AUTORCC ON`, and `SOURCE_ROOT="${CMAKE_SOURCE_DIR}"` for the scan tests. `main.cpp` of `qt_tests` constructs a plain `QApplication`; `MainWindow` does not need the `Application` subclass (`MainWindow.cpp` only includes the header).

### Qt Test: shortcut fires exactly once (offscreen)
```cpp
// tst_mac_qt.cpp (sketch); mirrors build/scratch/qt-spike/src/main2.cpp which ran green.
void TestMacQt::testStageKeys() {
  MainWindow window;
  window.show();
  QVERIFY(QTest::qWaitForWindowActive(&window));
  QAction* a = window.findChild<QAction*>("actionSwitchFilter3");
  QVERIFY(a);
  QCOMPARE(a->shortcuts().value(0), QKeySequence(Qt::CTRL | Qt::Key_3));
  QCOMPARE(a->shortcuts().value(0).toString(QKeySequence::NativeText), QString::fromUtf8("⌘" "3"));
  QSignalSpy spy(a, &QAction::triggered);
  QTest::keyClick(&window, Qt::Key_3, Qt::ControlModifier);
  QCOMPARE(spy.count(), 1);
}
```
Spike results, offscreen, Qt 6.12.0: ⌘1 and `O` each triggered the stage action once; ⌘+ and ⌘= each triggered zoom-in once; ⌥⌘+ triggered the thumbnail action once; ⌫ and ⌦ each triggered one removal shortcut once; `B` and `Insert` each created the box once; ⌫, ⌦ and `B` sent to a focused `QSpinBox` triggered nothing. [VERIFIED: build/scratch/qt-spike run]

### Qt Test: user-visible strings without Ctrl or Alt (static scan)
```cpp
// Walk SOURCE_ROOT/src (skip qt_tests, pipeline_tests, test_support, imageproc, math, dewarping tests).
// .ui: QXmlStreamReader; collect <string> elements that lack notr="true" (rich text is XML-escaped HTML,
//      one <string> can span several lines, so a line grep misses it).
// .cpp/.h: take string literals outside comments.
// Forbid whole-word (case-sensitive) Ctrl|Alt|Control|Cmd|Command|Option|Del|Ins|LMB; allow the literal
// "Ins" only as the argument of QKeySequence(. Baseline today = the inventory above; target = 0 hits.
```
Baseline for the C++ scan today: 6 string literals (`TabbedImageView.cpp:17`, `ZoneDefaultInteraction.cpp:20, 24, 30`, `deskew/ImageView.cpp:33`, `InteractiveXSpline.cpp:58`) plus the allowed `QKeySequence("Ins")` (which becomes `Qt::Key_Insert` and drops out). [VERIFIED: grep this session]

### Live harness: Cocoa-level probes (test-only `.mm`)
```objc
// CocoaProbe.mm (sketch). All in-process: no Accessibility or screen-recording permission.
// 1. Layout:   TISCopyCurrentKeyboardLayoutInputSource() -> kTISPropertyInputSourceID  (spike printed com.apple.keylayout.German)
// 2. Menu bar: for (NSMenuItem* i in NSApp.mainMenu.itemArray) record i.title, i.submenu.itemArray with
//              title / keyEquivalent / keyEquivalentModifierMask / isEnabled / isHidden.
// 3. Name:     NSRunningApplication.currentApplication.localizedName; NSBundle.mainBundle.infoDictionary[@"CFBundleName"].
// 4. Key:      NSEvent keyEventWithType:NSEventTypeKeyDown location:... modifierFlags:NSEventModifierFlagCommand|NSEventModifierFlagOption
//              timestamp:... windowNumber:win.windowNumber context:nil characters:... charactersIgnoringModifiers:...
//              isARepeat:NO keyCode:kVK_ANSI_1   (use the kVK_* names from Carbon, never literals), then [NSApp sendEvent:ev].
//              Delete key: characters @"\x7f"; fn+delete: [NSString stringWithFormat:@"%C", (unichar)NSDeleteFunctionKey].
// 5. Mouse:    NSEventTypeRightMouseDown / Up (secondary click, two-finger click), NSEventTypeLeftMouseDown with
//              NSEventModifierFlagControl (Control-click); location in window coordinates (y flipped).
// 6. Panels:   open QFileDialog via exec(); from dispatch_after(main queue) read [NSApp modalWindow] class
//              (NSOpenPanel / NSSavePanel), dump NSApp.mainMenu, then cancel the panel.
```
Qt-level injection (`QTest::keyClick` on Cocoa) is not enough: it bypasses `QAppleKeyMapper`, which supplies the native virtual key that `possibleKeyCombinations` uses for the Option layers. NSEvent-level injection is required to answer the German-layout question.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Heuristic role from action text ("Settings ...", "About", "Quit") | Explicit `menuRole` | Always available; docs mark heuristic as default only | Today's three actions are caught by the heuristic by accident; explicit roles are deterministic. |
| `setShortcut(QKeySequence("Ins"))` | `setShortcuts({...})` | Qt 4.2+ | Needed for Mac keys. |
| Shell command for the alarm | `QApplication::beep()` -> `NSBeep()` | Qt 6 Cocoa integration | Removes `std::system` on a string read from the settings file. |
| Forced widget file dialog for folder choice | Native NSOpenPanel with greyed files | Qt 6 Cocoa file dialog helper | Matches the OS; files cannot be picked in folder mode. |
| "Preferences…" wording | macOS 13+ names the item "Settings…" | macOS 13 | Qt builds the title; the harness records it. |
| macOS 26 menus can show icons | Qt 6.12 `ShowIconsInMenus` true on Tahoe [CITED: qcocoatheme.mm 6.11] | macOS 26 | The menu actions here carry no icons (`MainWindow.cpp:2179-2192` sets icons on buttons only), so no change. |

**Deprecated/outdated:** `Qt::Key_Menu` is not a context-menu key on Mac; no change. The `Q_OS_MAC` comment in CONTEXT about the 200 ms delay is outdated: the code has no guard.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | uic emits a `menuRole` property of `QAction` before `addAction` into menus | Pitfall 1 | Set the roles in code right after `setupUi(this)` instead; docs allow "just before the first window is shown". |
| A2 | Qt's Cocoa menu bar hides a top-level menu with no visible items (empty Help) | Pitfall 3 | An empty "Help" shows; remove `menuHelp` and declare `actionAbout` in File. |
| A3 | macOS may add "Enter Full Screen" to a menu titled View | Pitfall 3 | Harmless; record. |
| A4 | The embedded `CFBundleName` changes the system-drawn app-menu title and `localizedName`, not only Qt's item titles | Pitfall 2 | Keep or drop the embed by the harness; if only Qt's item titles change, still worth keeping ("About ScanTailor Advanced"). |
| A5 | CMake `LINKER:-sectcreate,...` expands to `-Wl,-sectcreate,...` | Pitfall 2 | Use `target_link_options(... "-Wl,-sectcreate,__TEXT,__info_plist,path")`. |
| A6 | `[NSApp sendEvent:]` with synthesised `NSEvent`s reaches Qt and the main menu with no privacy permission | Harness | Fall back to `[NSApp.mainMenu performKeyEquivalent:]` and `[window sendEvent:]`; if neither works the Cocoa-level rows cannot be proven and are recorded as "not measured" (a human check would then be needed, against D-11). |
| A7 | `dispatch_after` on the main queue runs while a native panel or menu is tracking (common run-loop modes) | Harness | Use `performSelector:afterDelay:inModes:` with the modal-panel and event-tracking modes. |
| A8 | `QCursor::setPos` (CGWarpMouseCursorPosition) needs no permission | Pitfall 8 | The zone-menu rows would need another way to place the cursor; unlikely. |
| A9 | With no parentless bar, the main bar stays enabled while a Qt modal dialog is open (early return), so native key equivalents might fire under a modal dialog | Pitfall 6 | If true, add the harness finding to the record; a mitigation would be a guard in the affected slots, to be decided then. |
| A10 | Controlling a native `QMenu::popup` from the harness (observing it, dismissing it) is feasible without private Qt API | Pitfall 8 | MAC-08 live rows reduce to "contextMenuEvent delivered and menu shown"; the 200 ms delay stays. |
| A11 | German-layout ⌥⌘1-5, ⌥⌘+/- match via `possibleKeyCombinations` | Don't Hand-Roll | Fall back to ⌃⌘ per UI-SPEC, change menu text, tooltips, status tips and tests together. |
| A12 | The `Delete` and `Backspace` shortcuts reach `QShortcut` through `QNSView` as in the offscreen spike (QTBUG-33015 is Qt 5, menu items) | Pitfall 7 | The harness row decides; if ⌫ fails live, bind through a `QAction` in the window or `keyPressEvent`. |

## Open Questions

1. **What does the macOS bar show while each dialog is open, and are its items enabled?**
   - Known: Qt source says the main bar stays; enabled state is not updated on the early return.
   - Unclear: live behaviour on macOS 26, including native panels (focus window null) and `QMessageBox`.
   - Recommendation: harness section `dialogs` dumps the NSMenu for Settings, Fix DPI, Default Parameters, About, Relinking, `QMessageBox` and a native panel, and tries ⌘N during Settings. Add a parentless bar only if the dump shows an empty or system-only bar.
2. **Does the Info.plist embed change what the user sees?**
   - Recommendation: harness `menubar` section prints Qt-set titles and `localizedName` for a build with and without the plist (two small variants of the harness, or one run per link flag). Decide from that.
3. **Can the live harness drive a native `QMenu` popup?**
   - Recommendation: first spike inside Wave 0; if not, accept the reduced MAC-08 record and keep the delay.
4. **Where should `actionSettings` live in the `.ui`?** It must stay a direct child of a top-level menu. Recommendation: `menuFile`, after `actionCloseProject`, next to `actionQuit` (both are role actions; File shows neither once merged).
5. **Disabled stage actions without a project (UI-SPEC empty state)** also disable the letter keys O, E, N, V, K, P, which are on the same actions and are enabled today (`switchFilterN` does not check `isProjectLoaded()`). Recommendation: follow the UI-SPEC (harmless; the stage list is cosmetic without a project), mention it in the plan.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Xcode command line tools (`xcrun clang++`, `clang-format`, `otool`) | build, format, binary checks | yes | `ld PROJECT:ld-27106.1` | none needed |
| CMake and Ninja | build | yes (presets configured, `build/macos-release` exists) | CMake 3.25+ per `CMakeLists.txt` | none |
| Qt 6 with Test module | build and `qt_tests` | yes | 6.12.0 (spike linked `Qt6::Test`) | none |
| macOS GUI session (display awake, unlocked) | live harness | assumed, as in Phase 4 | macOS 26 (Darwin 25.6.0) | none; the harness exits 3 on a non-Cocoa platform |
| Keyboard layout German | German-layout proof | yes: TIS spike printed `current layout: com.apple.keylayout.German` and listed it as the only enabled layout | - | none needed; a changed layout would be recorded by the harness |
| Accessibility or screen-recording permission | not needed by design | not required | - | - |

**Missing dependencies with no fallback:** none.
**Missing dependencies with fallback:** none.

Scratch work this session lives in `build/scratch/plist-spike` and `build/scratch/qt-spike` (git-ignored). The spikes ran offscreen or without NSApp, did not show windows, and touched no settings file.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Qt Test 6.12.0 inside the existing `qt_tests` executable (new class `TestMacQt`, registered like `TestHiDpiQt`) |
| Config file | `src/qt_tests/CMakeLists.txt` (sources, link, `TEST_TMP_ROOT`), `src/qt_tests/main.cpp` (offscreen with two screens, settings under `build/<preset>/test-tmp`) |
| Quick run command | `ctest --preset macos-release -R qt_tests` (after `cmake --build --preset macos-release`) |
| Full suite command | `ctest --preset macos-release` and `ctest --preset macos-debug` (expect `100% tests passed out of 7`); per commit `bash .planning/phases/03-qt-6-and-macos-only/gate.sh check <label>` |
| Live check | `build/macos-release/mac_check build/scratch/mac/all --section all -ApplePersistenceIgnoreState YES` (not a ctest suite; exit 0 pass, 1 fail, 2 usage or path outside `build/`, 3 platform not Cocoa) |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| MAC-01 | `actionAbout`/`actionSettings`/`actionQuit` have `AboutRole`/`PreferencesRole`/`QuitRole`; every other direct child of File, View, Tools, Help has `NoRole`; the three are direct children of a top-level menu; `actionSettings` not in `menuDebug` | Qt Test offscreen `testMenuRoles`, `testSettingsNotInTools` | `ctest --preset macos-release -R qt_tests` | Wave 0 (`tst_mac_qt.cpp`) |
| MAC-01 | Native bar order, app-menu contents once each, title and `localizedName` | live harness `--section menubar` | harness run, record `records/menubar.md` | Wave 0 |
| MAC-02 | `actionSettings.shortcut() == Ctrl+,` (explicit) and fires once; native text `⌘,` | Qt Test `testPreferencesShortcut` | as above | Wave 0 |
| MAC-02 | Cocoa `StandardKey::Preferences` equals the action's shortcut; the app-menu item shows `,` with Command | live harness `menubar` | harness | Wave 0 |
| MAC-03 | `actionSaveProjectAs` shortcut `Ctrl+Shift+S`, native text `⇧⌘S`, text `Save Project As…` | Qt Test `testFileMenuShortcuts` | as above | Wave 0 |
| MAC-04 | `actionZoomIn` has `⌘+` then `⌘=`, `actionZoomOut` `⌘-`; both change `zoomLevel()` of the current page view (use a real `BasicImageView` or `select_content::ImageView` placed with `setImageWidget`) and are disabled with no view | Qt Test `testZoomActions`, `testZoomDisabledWithoutView` | as above | Wave 0 |
| MAC-04, D-06 | ⌘+ and ⌘= on the German layout via NSEvent zoom the page view | live harness `--section keys` | harness | Wave 0 |
| MAC-05 | File shortcuts `⌘N ⌘O ⌘S ⌘W ⌘Q`; stage actions carry `⌘1..⌘6` then `O E N V K P`, text equals `Filter::getName()` of the six filters; each key fires its action once | Qt Test `testStageActions`, `testFileMenuShortcuts` | as above | Wave 0 |
| D-02 | `TabbedImageView` tabs on `⌥⌘1..5`, status tip names them; ⌘1 in the Output stage triggers the stage action, not a tab (no ambiguity) | Qt Test `testOutputTabKeys` | as above | Wave 0 |
| D-07, D-08 | Thumbnail actions `⌥⌘+`, `⌥⌘=`, `⌥⌘-`; View menu lists six stages, then Zoom In, Zoom Out, Larger Thumbnails, Smaller Thumbnails, with separators as in UI-SPEC | Qt Test `testViewMenuContents`, `testThumbnailKeys` | as above | Wave 0 |
| D-12 | ⌥⌘1-5 and ⌥⌘+/- fire on the German layout (NSEvent), else switch group to ⌃⌘ | live harness `keys` | harness | Wave 0 |
| MAC-06 | No user-visible string contains `Ctrl`, `Alt`, `Control`, `Cmd`, `Command`, `Option`, `Del`, `Ins`, `LMB` as a word (`.ui` via XML, C++ via literal scan); the nine apply dialogs read `Use ⌘-click / ⇧-click to select multiple pages.`; the two Shift strings read with ⇧; toolbar tooltips read `(⌥⌘+)`, `(⌥⌘-)`, `(⌘G)` | Qt Test `testNoPcKeyNames`, `testRewrittenStrings` | as above | Wave 0 |
| MAC-07 | `ZoneDefaultInteraction` removes the zone under the pointer on ⌫ and on ⌦ (build a `PictureZoneEditor` as `tst_hidpi_qt.cpp:268` does, add a zone, `QCursor::setPos` + `QTest::mouseMove`, key); ⌫, ⌦ sent to a focused `QSpinBox` and `QLineEdit` remove nothing | Qt Test `testZoneRemoveKeys`, `testZoneKeysIgnoredInTextFields` | as above | Wave 0 |
| MAC-07 | Content box: `B` and `Insert` create, ⌫ and ⌦ remove, none while `interactionState().captured()` | Qt Test `testContentBoxKeys` (select_content `ImageView` with `ContentMask`, `ImageTransformation`) | as above | Wave 0 |
| MAC-07 | ⌫ and ⌦ as NSEvents reach zone and content-box removal on Qt 6.12 (answers STATE's open question) | live harness `--section zone` | harness | Wave 0 |
| MAC-08 | Offscreen: right-button click on a zone, on the content box, on the page-layout view reaches the menu; no `Qt::RightButton` special case added (source scan of the three `onContextMenuEvent` bodies not required) | Qt Test `testRightClickOpensZoneMenu` | as above | Wave 0 |
| MAC-08 | Live: `rightMouseDown`, Control+`leftMouseDown` each deliver `contextMenuEvent` and show the menu; two-finger click recorded as indistinguishable from right-click; 200 ms delay kept unless proven removable | live harness `--section context` | harness | Wave 0 |
| MAC-09 | Live: bar contents, enabled state, and ⌘N attempt for Settings, Fix DPI, Default Parameters, About, Relinking, `QMessageBox`, native panel | live harness `--section dialogs` | harness | Wave 0 |
| MAC-10 | Source scan: `MainWindow.cpp` batch-finished path has no `std::system`, `QProcess`, `external_alarm_cmd`, `play ` command; has `QApplication::beep()` | Qt Test `testBatchAlarmUsesBeep` | as above | Wave 0 |
| MAC-10 | Audible beep | not provable in-process; code review and the scan | none | - |
| MAC-11 | Source scan: no `DontUseNativeDialog` and no `AA_DontUseNativeDialogs` in `src/` outside `src/qt_tests` | Qt Test `testNoDontUseNativeDialog` | as above | Wave 0 |
| MAC-11 | Live: Relinking file and folder panels, Open, Save, Input and Output folder, Files to insert are `NSOpenPanel`/`NSSavePanel`; cancel returns empty and changes nothing | live harness `--section panels` | harness | Wave 0 |

### Sampling Rate
- **Per task commit:** `cmake --build --preset macos-release && ctest --preset macos-release -R qt_tests` and `xcrun clang-format --dry-run -Werror` on touched C++ files.
- **Per wave merge:** `bash .planning/phases/03-qt-6-and-macos-only/gate.sh check <label>` on a committed tree (Release and Debug ctest, goldens vs `a796c2ae`, command diff).
- **Phase gate:** both presets green (7 of 7), goldens untouched (`git diff a796c2ae -- src/pipeline_tests/golden` empty), no diff under `src/imageproc/`, `src/dewarping/`, `OutputGenerator.cpp` or any `Task::process`, harness records complete, then `/gsd-verify-work`.

### Wave 0 Gaps
- [ ] `src/app/CMakeLists.txt`: `app_gui` static library; `scantailor-advanced` and `ui_snapshot` switched to it (closes review finding WR-01); expected command diff recorded in `records/gate-log.md`.
- [ ] `src/qt_tests/tst_mac_qt.{h,cpp}`, registration in `src/qt_tests/main.cpp` and `CMakeLists.txt` (link `app_gui`, `${resource_files}`, `CMAKE_AUTORCC`, `SOURCE_ROOT` define); scan helpers (XML string collector, C++ literal collector).
- [ ] `src/app/mac_check/{main.cpp,CocoaProbe.{h,mm}}`, `enable_language(OBJCXX)` under `if (BUILD_TESTS)`, `-framework AppKit -framework Carbon`, path guard identical to `retina_check` (exit 2 outside `build/`, exit 3 off Cocoa), settings under `build/`.
- [ ] `.planning/phases/05-mac-menus-and-shortcuts/records/` for verdicts (menubar, dialogs, keys, zone, context, panels, gate-log, closing check stating that no key or click was physically pressed).
- [ ] `TESTING.md`: a "Live Mac check" section next to "Live Retina check"; the `qt_tests` row gains the new tests.
- [ ] No new framework install needed.

## Security Domain

`security_enforcement` is on (ASVS level 1, block on high).

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | - |
| V3 Session Management | no | - |
| V4 Access Control | no | - |
| V5 Input Validation | yes | The batch-alarm string read from the settings file and passed to a shell is removed (CWE-78); file panels return existing paths and existing `RelinkablePath::normalize` handling stays |
| V6 Cryptography | no | - |
| V14 Configuration | yes | The harness keeps settings under `build/` and refuses output paths outside it; no privacy permission requested; no network access |

### Known Threat Patterns for this stack

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| OS command execution from a persisted setting (`main_window/external_alarm_cmd` -> `std::system`) | Tampering, Elevation of privilege | Remove the code path (MAC-10); leave the stored value in place, unread (D-00d) |
| Test harness writes outside the repo | Tampering | Path guard (exit 2), settings and temp under `build/`, never launch the real app binary |
| Embedded `__info_plist` changes app identity or storage paths | Tampering | `CFBundleName` only, no identifier; prove settings and app-data paths unchanged in the harness |
| Synthetic events injected into other apps | Spoofing | Events go to this process's own `NSApp`/window only (`sendEvent`), never `CGEventPost` |

No new dependency, no network, no secret handling.

## Sources

### Primary (HIGH confidence)
- Repo files read this session: `.planning/phases/05-mac-menus-and-shortcuts/05-CONTEXT.md`, `05-UI-SPEC.md`, `.planning/REQUIREMENTS.md`, `.planning/STATE.md`, `.planning/research/PITFALLS.md` (Pitfalls 9, 10), `TESTING.md`, `gate.sh`, `src/app/MainWindow.{ui,cpp}`, `src/app/CMakeLists.txt`, `src/app/RelinkingDialog.cpp`, `src/app/ui_snapshot/main.cpp`, `src/app/retina_check/main.cpp` (head), `src/qt_tests/*`, `src/core/zones/ZoneDefaultInteraction.cpp`, `ZoneContextMenuInteraction.cpp`, `src/core/filters/select_content/ImageView.cpp`, `src/core/filters/output/TabbedImageView.cpp`, `src/core/interaction/ZoomHandler.{h,cpp}`, `src/core/ImageViewBase.{h,cpp}`, `CMakeLists.txt`, `CMakePresets.json`.
- Spikes run this session: `build/scratch/qt-spike` (Qt 6.12.0 offscreen: native-text glyphs, empty Preferences/Quit, ShortcutOverride, shortcut firing), `build/scratch/plist-spike` (CFBundleName via `-sectcreate`, TIS layout query).
- Qt 6 source at tag/branch 6.11 (read through WebFetch of raw.githubusercontent.com/qt/qtbase/6.11): `qcocoamenubar.mm`, `qcocoamenuloader.mm`, `qcocoahelpers.mm`, `qapplekeymapper.mm`, `qnsview_keys.mm`, `qnsview_mouse.mm`, `qcocoafiledialoghelper.mm`, `qcocoaintegration.mm`, `qcocoatheme.mm`, `qplatformtheme.cpp`, `qwidgetlinecontrol.cpp`, `qabstractspinbox.cpp`, `qwidgetwindow.cpp`, `qfiledialog.cpp`. The installed version is 6.12.0; behaviour is assumed unchanged between 6.11 and 6.12 for these files.

### Secondary (MEDIUM confidence)
- https://doc.qt.io/qt-6/qaction.html (MenuRole, shortcuts), https://doc.qt.io/qt-6/qmenubar.html (macOS menu bar, default bar), https://doc.qt.io/qt-6/qkeysequence.html (StandardKey table), https://doc.qt.io/qt-6/qfiledialog.html (native dialogs), https://doc.qt.io/qt-6/qt.html (application attributes).
- https://support.apple.com/guide/mac-help/mchl779716b8/mac (Accessibility Zoom keyboard shortcuts).
- https://bugreports.qt-project.org/browse/QTBUG-33015 (title and status only, via search results).

### Tertiary (LOW confidence)
- Forum threads on Backspace/Delete mapping in Qt 5 (search results only), used only to frame why the live check exists.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH. Nothing new; Qt 6.12.0 verified by build.
- Architecture and source map: HIGH. Every file and line read this session.
- Offscreen test design: HIGH. The key facts (glyph strings, empty StandardKeys, shortcut firing, ShortcutOverride) were reproduced in spikes.
- Cocoa-level behaviour (menu bar during dialogs, app-menu title, German ⌥⌘, native popup driving): MEDIUM to LOW; source reading plus assumptions A2-A12; the harness is the instrument.
- Pitfalls: HIGH for items backed by source or spike, MEDIUM otherwise (tagged).

**Research date:** 2026-11-14
**Valid until:** 2026-12-14 (Qt 6.12 and macOS 26 are current; re-check if either updates)
