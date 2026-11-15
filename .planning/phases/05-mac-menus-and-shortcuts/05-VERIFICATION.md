---
phase: 05-mac-menus-and-shortcuts
verified: 2026-11-15T13:30:00Z
status: human_needed
score: 6/8 must-haves verified
covered_files:
  - ".planning/phases/05-mac-menus-and-shortcuts/05-01-PLAN.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-01-SUMMARY.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-02-PLAN.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-02-SUMMARY.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-03-PLAN.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-03-SUMMARY.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-04-PLAN.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-04-SUMMARY.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-05-PLAN.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-05-SUMMARY.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-06-PLAN.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-06-SUMMARY.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-07-PLAN.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-07-SUMMARY.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-08-PLAN.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-08-SUMMARY.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-09-PLAN.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-09-SUMMARY.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-10-PLAN.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-10-SUMMARY.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-11-PLAN.md"
  - ".planning/phases/05-mac-menus-and-shortcuts/05-11-SUMMARY.md"
  - "TESTING.md"
  - "src/app/CMakeLists.txt"
  - "src/app/Info.plist"
  - "src/app/MainWindow.cpp"
  - "src/app/MainWindow.h"
  - "src/app/MainWindow.ui"
  - "src/app/RelinkingDialog.cpp"
  - "src/app/mac_check/CocoaProbe.h"
  - "src/app/mac_check/CocoaProbe.mm"
  - "src/app/mac_check/main.cpp"
  - "src/core/filters/deskew/ApplyDialog.ui"
  - "src/core/filters/deskew/ImageView.cpp"
  - "src/core/filters/fix_orientation/ApplyDialog.ui"
  - "src/core/filters/output/ApplyColorsDialog.ui"
  - "src/core/filters/output/ChangeDewarpingDialog.ui"
  - "src/core/filters/output/ChangeDpiDialog.ui"
  - "src/core/filters/output/TabbedImageView.cpp"
  - "src/core/filters/page_layout/ApplyDialog.ui"
  - "src/core/filters/page_layout/ApplyMarginsDialog.ui"
  - "src/core/filters/page_layout/OptionsWidget.ui"
  - "src/core/filters/page_split/SplitModeDialog.ui"
  - "src/core/filters/select_content/ApplyDialog.ui"
  - "src/core/filters/select_content/ImageView.cpp"
  - "src/core/interaction/DragHandler.cpp"
  - "src/core/interaction/InteractiveXSpline.cpp"
  - "src/core/zones/ZoneContextMenuInteraction.cpp"
  - "src/core/zones/ZoneContextMenuInteraction.h"
  - "src/core/zones/ZoneDefaultInteraction.cpp"
  - "src/core/zones/ZoneDefaultInteraction.h"
  - "src/qt_tests/CMakeLists.txt"
  - "src/qt_tests/main.cpp"
  - "src/qt_tests/tst_mac_qt.cpp"
  - "src/qt_tests/tst_mac_qt.h"
covered_digest: "v4:sha256:37736891432c67a61d59a2b78d6fc6b440712bf132979a2215444c33a28facc4"
behavior_unverified: 2
overrides_applied: 0
behavior_unverified_items:
  - truth: "A right-click, a two-finger click and a Control-click each open the context menu"
    test: "On the MacBook trackpad, two-finger click (or Control-click is already measured) a zone in the zone editor of Picture zones or Fill zones"
    expected: "The zone context menu opens once, the first item removes the zone, Escape dismisses it"
    why_human: "Right-click and Control-click were injected and measured live; the two-finger click arrives as the same rightMouseDown but no two-finger event was injected or clicked (D-11). Presence plus the right-click measurement cannot prove the trackpad gesture setting maps to it."
  - truth: "When a batch run finishes the Mac plays the system beep and no shell command runs"
    test: "Tick 'Beep when finished' in the batch panel, run a short batch to the end"
    expected: "The macOS alert sound plays once and the Dock icon bounces"
    why_human: "Source check proves QApplication::beep() is called after QApplication::alert() and no shell/QProcess/system() call exists; nobody heard the sound (D-11)."
human_verification:
  - test: "Two-finger trackpad click on a zone (see behavior_unverified_items)"
    expected: "Zone context menu opens"
    why_human: "Not injected separately; same rightMouseDown as a right-click"
  - test: "Batch run with 'Beep when finished' ticked"
    expected: "System beep audible"
    why_human: "Audible output cannot be asserted from source"
  - test: "Look at the nine apply dialogs (multi-page hint) and the Margins help in the running app"
    expected: "The shortened hints with the Mac key glyphs and the 7 pt bold glyphs of the Margins help are readable and not clipped"
    why_human: "Checked only as markup and string length, never rendered (no screenshot permission, D-12)"
coincidental_reliance_items: []
advisory: []
warnings:
  - id: WR-01
    summary: "Zoom In/Out enable state is not refreshed on Output tab changes or when DespeckleView swaps its image view in"
    effect: "Cmd-plus and Cmd-minus can be greyed on the Output stage's Despeckling tab (and stay greyed after leaving it) until an unrelated layout change; plain +/- keys, the wheel and pinch still zoom"
    classification: "follow-up, not a goal failure (narrow case); a small fix is recommended before Phase 6 splits MainWindow.cpp"
---

# Phase 5: Mac Menus and Shortcuts Verification Report

**Phase Goal:** The app behaves like a Mac app. It has a standard application menu, Cmd shortcuts, Mac key names and native dialogs, and it works with MacBook keyboards and trackpads.
**Verified:** 2026-11-15T13:30:00Z
**Status:** human_needed
**Re-verification:** No, initial verification

Verified at HEAD `4f7a4268` (only `.planning/` changed after the last source commit `efe9cbdb`). The verifier did not launch the app or the live Cocoa harness (user constraint); live results are taken from the records in `records/` and the closing summaries in `build/scratch/mac/closing-*/summary.txt`, which were read (every section verdict PASS, `app_active: yes`, `context_pointer_interference: 0`). Source claims were checked directly in the code.

## Goal Achievement

### Observable Truths (ROADMAP success criteria, split where a criterion has separable parts)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Application menu holds About, Preferences, Quit; Cmd+, opens Preferences; Settings is gone from the Debug (Tools) menu; menu bar stays populated while a dialog is open (MAC-01, MAC-02, MAC-09) | VERIFIED | `MainWindow.ui`: `actionAbout` AboutRole, `actionSettings` PreferencesRole with `Ctrl+,`, `actionQuit` QuitRole with `Ctrl+Q`, all in `menuFile`; `menuDebug` (Tools) lists Fix DPI, Relinking, Reverse order, Defaults, Units, Debug Mode and no Settings. `app_gui` + `Info.plist` give the app name. Live: `menubar_verdict: PASS`, `dialogs_verdict: PASS` (bar populated in 7 dialog rows). `grep` finds no `setNativeMenuBar(false)`, `AA_DontUseNativeMenuBar`, `m_fallbackMenuBar` in `src` outside tests. |
| 2a | Cmd+N, O, S, W, Q, Shift+Cmd+S and Cmd+1..6 work (MAC-03, MAC-05) | VERIFIED | `MainWindow.ui` has `Ctrl+N/O/S/Shift+S/W/Q`; `setupMacKeys()` binds `CTRL|Key_1..6` first, old letters kept; `updateProjectActions()` enables them with a project. `TestMacQt` (27 cases) passes in a fresh offscreen `ctest -R qt_tests` run by the verifier; live `keys_verdict: PASS` (each key fires once). |
| 2b | Cmd+= and Cmd+- zoom the page view (MAC-04) | VERIFIED (with warning WR-01) | `setupMacKeys()`: zoom in `Cmd+Plus` and `Cmd+=`, zoom out `Cmd+-`; `zoomCurrentView()` sets zoom with cursor/centre focus; live `layout_verdict: PASS` (steps 1.122462 and 0.890899). Works on every stage view and on the Output stage's image-view tabs. Defect on the Output Despeckling tab, see WR-01 below. |
| 2c | Tooltips, status tips and help text show ⌘ and ⌥, not "Ctrl"/"Alt" (MAC-06) | VERIFIED (rendering not observed) | The verifier's own grep of non-test `.ui`/`.cpp`/`.h` files finds no PC key names outside key-sequence strings; `ZoneDefaultInteraction.cpp` status tip reads "Press ⌫ to delete this zone" with ⇧⌘; `testNoPcKeyNames` (scan of all `.ui` and string literals) and `testRewrittenStrings` pass. Rendering of the shortened hints is a human item. |
| 3a | In the zone editor Backspace or Delete removes a zone; a content box is created with a key a MacBook has (MAC-07) | VERIFIED | `ZoneDefaultInteraction.cpp:33-34` binds both `Key_Delete` and `Key_Backspace`; `select_content/ImageView.cpp:154-155` binds `B` / `Insert` to create and `Backspace` / `Delete` to remove. Tests `testZoneRemoveKeys`, `testZoneKeysIgnoredInTextFields`, `testContentBoxKeys`; live `zone_verdict: PASS` (answers the STATE.md Backspace question). Wording note: the roadmap says "the selected zone"; the zone editor has no selection, so the keys remove the zone under the pointer (CONTEXT D-10), the same meaning Delete had before Phase 5. |
| 3b | A right-click, a two-finger click and a Control-click each open the context menu (MAC-08) | PRESENT_BEHAVIOR_UNVERIFIED | Right-click and Control-click opened the zone menu in the live `context` section (`context_verdict: PASS`, 0 pointer interference) and in `testRightClickOpensZoneMenu`; 200 ms deferral removed (`ZoneContextMenuInteraction.cpp` changed; measured 20/20). The two-finger click was not injected separately (D-11 residual). |
| 4 | When a batch run finishes, the system beep plays and no shell command runs (MAC-10) | PRESENT_BEHAVIOR_UNVERIFIED | `MainWindow.cpp:1343-1347`: `QApplication::alert(this)` then `QApplication::beep()` behind the "Beep when finished" check; no `std::system`, `system(`, `QProcess`, `popen`, `external_alarm_cmd` remains in `MainWindow.cpp`; `testBatchAlarmUsesBeep` pins it. Not heard (D-11 residual). |
| 5 | Every open, save and folder dialog, including relinking, is the native macOS panel (MAC-11) | VERIFIED | Source: all `QFileDialog` uses are static calls without options (`RelinkingDialog.cpp:68,72`, `MainWindow.cpp:1437,1481`, `ProjectFilesDialog.cpp:244,258`, `OutOfMemoryDialog.cpp:62`) or the "Files to insert" instance with no `DontUseNativeDialog`; `grep DontUseNative` finds nothing outside tests. Live `panels_verdict: PASS`: relink file/folder, Open Project, input and output folder panels are `NSOpenPanel`. Save Project As, the out-of-memory save panel and Files to insert were checked by source only (need a loaded project). |

**Score:** 6/8 truths verified (2 present, behavior-unverified; none failed)

### Weighing review finding WR-01 (zoom enable state)

Verified in code. `updateZoomActions()` is connected only to `QStackedLayout::currentChanged` and `widgetRemoved` (`MainWindow.cpp:151-152`). `currentPageView()` resolves through `findPrimaryImageView()` to `TabbedImageView::currentWidget()` on the Output stage (`MainWindow.cpp:748-760`). `Task.cpp:579` calls `setCurrentTab(lastTab)`, so when the user last used the Despeckling tab the new Output page installs with a `DespeckleView` whose current widget is still the processing indicator (no `ImageViewBase`). The action is then computed disabled; `DespeckleView::imageViewCreated` (`DespeckleView.cpp:97,179`) and `QTabWidget::currentChanged` do not refresh it. Disabled actions swallow their shortcuts, so Cmd+plus and Cmd+minus do nothing and the View menu items are grey.

Assessment: **a real defect, but a follow-up and not a failure of the phase goal.** It is confined to the Output stage's Despeckling tab. Zoom works on the five other stages and on every other Output tab (a tab switch among image-view tabs keeps the action enabled), and plain +/-, the wheel and pinch still zoom on the Despeckling tab. Criterion 2 and MAC-04 are met for the working paths, which the live `layout` and `keys` runs and the tests exercise. The tests do not cover it (`testZoomActions` and `testZoomDisabledWithoutView` only add and remove widgets from the main layout). I did not reproduce it by running the app (not allowed); the finding rests on the code trace above. Recommended: connect `QTabWidget::currentChanged` and `imageViewCreated` to `updateZoomActions`, or leave both actions always enabled and let `zoomCurrentView()` ignore a missing view, and add a test. If the user prefers a strict reading of "all work", treat it as a gap and run `/gsd-plan-phase --gaps`; my recommendation is a quick fix.

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/app/MainWindow.ui` | Roles, Cmd keys, menus | VERIFIED | Roles and keys present, Settings in File, Help menu removed |
| `src/app/MainWindow.cpp` / `.h` | `setupMacKeys`, zoom actions, beep | VERIFIED | Wired (called after `retranslateUi`); beep block present |
| `src/app/CMakeLists.txt`, `Info.plist` | `app_gui` library, app name | VERIFIED | `app_gui` linked by app, tests, `mac_check`, `ui_snapshot` |
| `src/app/RelinkingDialog.cpp` | Native relink panels | VERIFIED | Static `QFileDialog` calls, no options |
| `src/core/zones/ZoneDefaultInteraction.*`, `ZoneContextMenuInteraction.*` | ⌫/⌦ and menu behaviour | VERIFIED | Shortcuts bound, tips use ⌫ |
| `src/core/filters/select_content/ImageView.cpp` | B / ⌫ content box | VERIFIED | Lines 154-155 |
| nine `ApplyDialog`-type `.ui` files, `page_layout/OptionsWidget.ui` | Mac key names | VERIFIED | Scan test passes; rendering not observed |
| `src/qt_tests/tst_mac_qt.*` | Permanent tests | VERIFIED | 27 cases pass (fresh offscreen run) |
| `src/app/mac_check/*` | Live harness (not shipped) | VERIFIED | Built into `mac_check` only; `.mm` limited to `CocoaProbe.mm` |

### Key Link Verification

| From | To | Via | Status |
|------|----|-----|--------|
| `actionSettings` | `SettingsDialog` | PreferencesRole + `Ctrl+,` | WIRED (`testSettingsShortcutOpensSettings`) |
| `actionZoomIn/Out` | `ImageViewBase::setZoomLevel` | `zoomCurrentView` | WIRED (stale enable state, WR-01) |
| `QShortcut(Key_Backspace/Delete)` | zone removal | `ZoneDefaultInteraction` | WIRED |
| batch finished | `QApplication::beep` | `MainWindow.cpp:1343-1347` | WIRED |
| relink buttons | native panels | `RelinkingDialog::pathButtonClicked` | WIRED |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| qt_tests (TestCore/Ux/HiDpi/Mac) offscreen | `ctest --preset macos-release -R qt_tests` | 100% passed out of 1 (Totals 8, 7, 28, 27 passed) | PASS |
| Full suites (orchestrator) | `ctest --preset macos-release` and `macos-debug` | 100% passed out of 7 each | PASS (not re-run by verifier) |
| Output and goldens untouched | `git diff f6cc4457 -- src/pipeline_tests/golden src/imageproc src/dewarping src/core/filters/output/OutputGenerator.cpp` | empty | PASS |
| Gate log | `records/gate-log.md` | 21 rows, release/debug 100% of 7, goldens unchanged | PASS |
| Live Cocoa runs | read `build/scratch/mac/closing-input/summary.txt`, `closing-panels/summary.txt` | every section verdict PASS | PASS (read, not re-run) |

### Probe Execution

No `probe-*.sh` declared; Step 7c not applicable.

### Requirements Coverage

All 11 IDs appear in plan frontmatter and in REQUIREMENTS.md (all marked complete, mapped to Phase 5). No orphaned requirement.

| Requirement | Source Plans | Status | Evidence |
|-------------|-------------|--------|----------|
| MAC-01 | 05-01, 05-02, 05-10, 05-11 | SATISFIED | Truth 1 |
| MAC-02 | 05-01, 05-02, 05-05, 05-11 | SATISFIED | Truth 1, `testSettingsShortcutOpensSettings` |
| MAC-03 | 05-01, 05-05, 05-11 | SATISFIED | Truth 2a, live `standard_keys_match` |
| MAC-04 | 05-02, 05-03, 05-05, 05-11 | SATISFIED with WR-01 warning | Truth 2b |
| MAC-05 | 05-01, 05-02, 05-03, 05-05, 05-11 | SATISFIED | Truth 2a |
| MAC-06 | 05-03, 05-04, 05-07, 05-08, 05-11 | SATISFIED (rendering unobserved) | Truth 2c |
| MAC-07 | 05-04, 05-05, 05-11 | SATISFIED | Truth 3a |
| MAC-08 | 05-04, 05-06, 05-11 | NEEDS HUMAN (two-finger click) | Truth 3b |
| MAC-09 | 05-10, 05-11 | SATISFIED | Truth 1, `dialogs.md` |
| MAC-10 | 05-09, 05-11 | NEEDS HUMAN (beep not heard) | Truth 4 |
| MAC-11 | 05-09, 05-10, 05-11 | SATISFIED | Truth 5 |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `src/app/MainWindow.cpp` | 1426, 1934 | `XXX` comments | Info | Both exist unchanged at the Phase 5 base (`f6cc4457`: lines 1416, 1866), from upstream; no marker was added by this phase (`git diff` adds none). Not a Phase 5 blocker; Phase 6 CLEAN scope. |
| `src/app/MainWindow.cpp` | 151-152, 1654 | Stale zoom enable state | Warning | WR-01 above |
| `src/app/mac_check/main.cpp` | 790-796 | keys section has no home-redirect guard (WR-02) | Warning | Test tooling only, not shipped; opens a real-home panel only if a slot disconnect fails |
| review IN-01 to IN-06 | | test brittleness, harness polish | Info | Non-blocking; all open in `05-REVIEW-DISPOSITION.md` |

Stub scan of the shipped changes found no placeholder implementations. Phase constraint "output unchanged" holds (goldens and output code diff empty). The `macos-asan` qt_tests run reported one UBSan finding in `BinaryImage.cpp`, a file Phase 5 did not change and which earlier phases already record.

### Human Verification Required

1. **Two-finger trackpad click.** Two-finger click a zone in the Picture or Fill zones view. Expected: the zone context menu opens. Why human: no separate two-finger event was injected (D-11 residual).
2. **Batch-finished beep.** Tick "Beep when finished" and run a short batch. Expected: the system alert sound and a Dock bounce. Why human: audio.
3. **Rendering of the Mac key hints.** Open the nine apply dialogs and the Margins help. Expected: the shortened hints and 7 pt glyphs read well and are not clipped. Why human: never rendered.

### Gaps Summary

No truth failed. The phase goal is achieved for the menu, key, key-name, zone-key, context-menu (right-click and Control-click), beep-source and native-panel work, backed by source, 27 permanent tests, 21 clean gate rows and live Cocoa runs. The status is `human_needed` only because three items were not observed (two-finger click, audible beep, drawn hints), which the phase design (D-11, D-12) accepted as residuals. WR-01 is a real, narrow defect (Cmd+plus and Cmd+minus can stay disabled on the Output Despeckling tab) that I rate a follow-up; the user can decide to close it before Phase 6.

---

_Verified: 2026-11-15T13:30:00Z_
_Verifier: Claude (gsd-verifier)_
