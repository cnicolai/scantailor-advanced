---
phase: 05-mac-menus-and-shortcuts
verified: 2026-11-15T19:10:00Z
status: passed
score: 8/8 must-haves verified
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
  - "src/core/filters/output/TabbedImageView.h"
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
covered_digest: "v4:sha256:44bdd12538a50b0baf3479cd05ae21d96953c13dbe6f08a603a8b6d851e6471d"
behavior_unverified: 0
overrides_applied: 1
overrides:
  - must_have: "When a batch run finishes, the Mac plays the system beep and no shell command runs (the sound itself)"
    reason: "Source-verified only: QApplication::beep() follows QApplication::alert() behind the 'Beep when finished' check, no shell, QProcess or system() call remains, and testBatchAlarmUsesBeep pins both. The audible beep was never heard. The user skipped the listening check in UAT ('I could not care less whether it beeps or not'), so the residual is accepted."
    accepted_by: "user (05-UAT.md test 2, skipped, 2026-11-15); recorded here by the verifier from that entry"
    accepted_at: "2026-11-15T18:40:26Z"
re_verification:
  previous_status: human_needed
  previous_score: 6/8
  gaps_closed:
    - "WR-01 zoom actions stale on the Output Despeckling tab (fixed in 7a9c22dc, covered by testZoomFollowsOutputTabs)"
    - "Two-finger trackpad click (UAT test 1, user, MacBook trackpad: PASS)"
    - "Rendering of the Mac key hints and Margins help (UAT test 3, offscreen render: PASS)"
  gaps_remaining: []
  regressions: []
behavior_unverified_items: []
coincidental_reliance_items: []
advisory: []
human_verification: []
warnings: []
---

# Phase 5: Mac Menus and Shortcuts Verification Report

**Phase Goal:** The app behaves like a Mac app. It has a standard application menu, Cmd shortcuts, Mac key names and native dialogs, and it works with MacBook keyboards and trackpads.
**Verified:** 2026-11-15T19:10:00Z
**Status:** passed
**Re-verification:** Yes. The earlier report (`human_needed`, 6/8, at `4f7a4268`) went stale after code-review fixes `7a9c22dc` (WR-01) and `13519fcf` (WR-02) and after the human UAT. This run re-checks every truth against HEAD `92d0806d`.

The verifier did not launch the app or any live Cocoa harness section (user constraint: they move the pointer and take focus). Live results come from the committed records in `records/` and `closing-check.md`; the only thing run was the offscreen `qt_tests` binary. Source claims were checked directly in the code.

## Goal Achievement

### Observable Truths (ROADMAP success criteria, split where a criterion has separable parts)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Application menu holds About, Preferences, Quit; Cmd+, opens Preferences; Settings is gone from the Debug (Tools) menu; menu bar stays populated while a dialog is open (MAC-01, MAC-02, MAC-09) | VERIFIED | `MainWindow.ui`: `actionAbout` AboutRole (l.1276), `actionSettings` PreferencesRole (l.1246) with `Ctrl+,` (l.1243), `actionQuit` QuitRole (l.1226) with `Ctrl+Q`; all three sit in `menuFile` (l.92-97, no `actionSettings` in `menuDebug` at l.80-82). `grep` finds no `DontUseNative`, `AA_DontUseNativeMenuBar` or `setNativeMenuBar` in `src` outside tests (positive control: 6 hits in `tst_mac_qt.cpp`; the single `mac_check/main.cpp:1875` hit is a message string). Records: `menubar_verdict: PASS` (the earlier FAIL row in `menubar.md` is superseded by the later PASS rows), `dialogs_verdict: PASS`; `testSettingsShortcutOpensSettings` passes. |
| 2a | Cmd+N, O, S, W, Q, Shift+Cmd+S and Cmd+1..6 work (MAC-03, MAC-05) | VERIFIED | `MainWindow.ui` has `Ctrl+N/O/W/Q/Shift+S` (l.1132-1232); `MainWindow.cpp:1630-1635` binds `CTRL|Key_1..6` first and keeps the old letters. `TestMacQt` 28/28 pass in a fresh offscreen run. Live `keys_verdict: PASS` in `records/keys.md` and `closing-check.md`; the WR-02 fix run (`keys3`, hands-off) also recorded PASS. |
| 2b | Cmd+= and Cmd+- zoom the page view (MAC-04) | VERIFIED (WR-01 closed) | `MainWindow.cpp:1638-1639`: zoom in `Cmd+Plus` and `Cmd+=`, zoom out `Cmd+-`. WR-01 fix re-checked in the diff of `7a9c22dc`: `TabbedImageView::primaryViewChanged()` is emitted in `tabChangedSlot` and, queued, from `DespeckleView`'s `QStackedWidget::currentChanged`; `updateZoomActions()` connects it with `Qt::UniqueConnection` to the `TabbedImageView` it finds in the main area (`Utils::castOrFindChild`), so the connection exists before any view is created and dies with the tab widget. The "disabled with no view" rule is kept (`testZoomDisabledWithoutView` passes unchanged). `testZoomFollowsOutputTabs` passes (tab without view disables, back enables, removing the despeckle image view disables, adding it enables, removing the tab widget disables). Live `layout_verdict: PASS` (zoom steps 1.122462 and 0.890899). |
| 2c | Tooltips, status tips and help text show ⌘ and ⌥, not "Ctrl"/"Alt" (MAC-06) | VERIFIED | `testNoPcKeyNames` and `testRewrittenStrings` pass (scan of all `.ui` and string literals). `ZoneDefaultInteraction.cpp:30-32` status tip reads "⌥⌘-click ... Use Z, X and C keys". Rendering, previously the open item: UAT test 3, offscreen render with the macOS system font, both colour schemes: all nine hints and the Margins help render with 0 missing glyphs and no clipping. |
| 3a | In the zone editor Backspace or Delete removes a zone; a content box is created with a key a MacBook has (MAC-07) | VERIFIED | `ZoneDefaultInteraction.cpp:33-34` binds `Key_Delete` and `Key_Backspace`; `select_content/ImageView.cpp:154-155` binds `B`/`Insert` to create and `Backspace`/`Delete` to remove. `testZoneRemoveKeys`, `testZoneKeysIgnoredInTextFields`, `testContentBoxKeys` pass; live `zone_verdict: PASS`. Wording note: the zone editor has no selection, so the keys remove the zone under the pointer (CONTEXT D-10), the meaning Delete had before Phase 5. |
| 3b | A right-click, a two-finger click and a Control-click each open the context menu (MAC-08) | VERIFIED | Right-click and Control-click: live `context_verdict: PASS` (0 pointer interference) and `testRightClickOpensZoneMenu`. Two-finger click, previously unverified: UAT test 1, the user on the MacBook trackpad with the release build at `59940b4f` (after both review fixes): one menu opened in the Picture Zones tab, Escape dismissed it, the first item removed the zone. |
| 4 | When a batch run finishes, the system beep plays and no shell command runs (MAC-10) | PASSED (override), source-verified only | `MainWindow.cpp:1343` `QApplication::alert(this)`, `:1347` `QApplication::beep()`; no `std::system`, `QProcess`, `popen` or `external_alarm` in `MainWindow.cpp` (positive control: the beep greps hit); `testBatchAlarmUsesBeep` passes. **The sound was never heard.** UAT test 2 was skipped by the user ("I could not care less whether it beeps or not"). Honest reading: the code path is in place and pinned by a test; audible output is unobserved and the user accepted that. |
| 5 | Every open, save and folder dialog, including relinking, is the native macOS panel (MAC-11) | VERIFIED | `RelinkingDialog.cpp:68,72` static `QFileDialog` calls without options; no `DontUseNative` anywhere in `src` outside tests. Live `panels_verdict: PASS` (relink file/folder, Open Project, input and output folder are `NSOpenPanel`). Save Project As, out-of-memory save panel and Files to insert were checked by source only (need a loaded project). |

**Score:** 8/8 truths verified (one of them, truth 4, as an accepted override: source-verified, not heard)

### Review fixes re-checked

| Fix | Commit | Check | Result |
|-----|--------|-------|--------|
| WR-01 zoom enable state | `7a9c22dc` | Diff read; `testZoomFollowsOutputTabs` run in a fresh offscreen `qt_tests` (`PASS`, TestMacQt totals 28 passed, 0 failed); `ctest --preset macos-release -R qt_tests` passed. The added `emit primaryViewChanged()` only re-evaluates `updateZoomActions()`; it changes no zoom value, output or golden. | Holds, no regression |
| WR-02 `mac_check` fails closed | `13519fcf` | Diff read: menus are disconnected first, then a wildcard `QObject::disconnect(action, &QAction::triggered, nullptr, nullptr)` returns true only if a receiver is left; a hit records `keys_slot_still_connected_<name>`, `keys_verdict=FAIL` and returns before the counter connects and before any key is sent. Live hands-off run `keys3` recorded in `05-REVIEW-FIX.md`: PASS, no leftover receiver. Test tooling only, not shipped. | Holds (live run not repeated by the verifier) |

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/app/MainWindow.ui` | Roles, Cmd keys, menus | VERIFIED | Roles and keys present; Settings in File |
| `src/app/MainWindow.cpp` / `.h` | `setupMacKeys`, zoom actions, beep | VERIFIED | Wired; `updateZoomActions` now follows the Output tabs |
| `src/core/filters/output/TabbedImageView.{h,cpp}` | `primaryViewChanged` | VERIFIED | Declared, emitted on tab change and queued from `DespeckleView`, consumed by `MainWindow` |
| `src/app/CMakeLists.txt`, `Info.plist` | `app_gui` library, app name | VERIFIED | Unchanged since the previous report |
| `src/app/RelinkingDialog.cpp` | Native relink panels | VERIFIED | Static calls without options |
| `src/core/zones/ZoneDefaultInteraction.*`, `ZoneContextMenuInteraction.*` | Delete keys and menu behaviour | VERIFIED | Shortcuts bound |
| `src/core/filters/select_content/ImageView.cpp` | B / Backspace content box | VERIFIED | l.154-155 |
| nine apply dialogs `.ui`, `page_layout/OptionsWidget.ui` | Mac key names | VERIFIED | Scan test passes; rendered offscreen (UAT 3) |
| `src/qt_tests/tst_mac_qt.*` | Permanent tests | VERIFIED | 28 cases pass |
| `src/app/mac_check/*` | Live harness (not shipped) | VERIFIED | Built into `mac_check` only; `.mm` limited to `CocoaProbe.mm` |

### Key Link Verification

| From | To | Via | Status |
|------|----|-----|--------|
| `actionSettings` | `SettingsDialog` | PreferencesRole + `Ctrl+,` | WIRED (`testSettingsShortcutOpensSettings`) |
| `actionZoomIn/Out` | `ImageViewBase::setZoomLevel` | `zoomCurrentView` | WIRED |
| `TabbedImageView::primaryViewChanged` | `MainWindow::updateZoomActions` | unique connection made in `updateZoomActions` | WIRED (`testZoomFollowsOutputTabs`) |
| `QShortcut(Key_Backspace/Delete)` | zone removal | `ZoneDefaultInteraction` | WIRED |
| batch finished | `QApplication::beep` | `MainWindow.cpp:1343-1347` | WIRED |
| relink buttons | native panels | `RelinkingDialog::pathButtonClicked` | WIRED |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| qt_tests offscreen | `QT_QPA_PLATFORM=offscreen build/macos-release/qt_tests` | Totals 8, 7, 28, 28 passed, 0 failed; `testZoomFollowsOutputTabs`, `testZoomActions`, `testZoomDisabledWithoutView`, `testRightClickOpensZoneMenu`, `testNoPcKeyNames`, `testBatchAlarmUsesBeep`, `testSettingsShortcutOpensSettings` all PASS | PASS |
| qt_tests via ctest | `ctest --preset macos-release -R qt_tests` | 100% passed out of 1 | PASS |
| Full suites (orchestrator, HEAD `92d0806d`) | `ctest --preset macos-release` and `macos-debug` | 7/7 each, builds up to date | PASS (not re-run by verifier) |
| Output and goldens untouched | `git diff f6cc4457 HEAD --stat -- src/pipeline_tests/golden src/imageproc src/dewarping src/core/filters/output/OutputGenerator.cpp` | empty | PASS |
| Live Cocoa runs | read `records/*.md`, `closing-check.md` | every section verdict PASS | PASS (read, not re-run) |

### Probe Execution

No `probe-*.sh` declared; Step 7c not applicable.

### Requirements Coverage

All 11 IDs appear in plan frontmatter (05-01 to 05-11; 05-11 lists all eleven) and in REQUIREMENTS.md (checked, mapped to Phase 5, "Complete"). No orphaned requirement.

| Requirement | Source Plans | Status | Evidence |
|-------------|-------------|--------|----------|
| MAC-01 | 05-01, 05-02, 05-10, 05-11 | SATISFIED | Truth 1 |
| MAC-02 | 05-01, 05-02, 05-05, 05-11 | SATISFIED | Truth 1 |
| MAC-03 | 05-01, 05-05, 05-11 | SATISFIED | Truth 2a |
| MAC-04 | 05-02, 05-03, 05-05, 05-11 | SATISFIED | Truth 2b (WR-01 closed) |
| MAC-05 | 05-01, 05-02, 05-03, 05-05, 05-11 | SATISFIED | Truth 2a |
| MAC-06 | 05-03, 05-04, 05-07, 05-08, 05-11 | SATISFIED | Truth 2c |
| MAC-07 | 05-04, 05-05, 05-11 | SATISFIED | Truth 3a |
| MAC-08 | 05-04, 05-06, 05-11 | SATISFIED | Truth 3b (UAT 1) |
| MAC-09 | 05-10, 05-11 | SATISFIED | Truth 1 |
| MAC-10 | 05-09, 05-11 | SATISFIED in source; sound not heard (accepted override) | Truth 4 |
| MAC-11 | 05-09, 05-10, 05-11 | SATISFIED | Truth 5 |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `src/app/MainWindow.cpp` | ~1426, ~1934 | `XXX` comments | Info | Present at the Phase 5 base `f6cc4457` (from upstream). `git diff f6cc4457 HEAD -U0 -- src` adds no `TBD`, `FIXME` or `XXX` line. Phase 6 CLEAN scope. |
| review IN-01 to IN-06 | | test brittleness, harness polish | Info | All six `open` in `05-REVIEW-DISPOSITION.md`; non-blocking, none touches a must-have |

Stub scan of the shipped changes found no placeholder implementations. Phase constraint "output unchanged" holds. Not a Phase 5 defect (from UAT 3): at the Filters dock's minimum width (274 pt) the Margins options panel (minimum 316 pt) is cut on the right; widening the dock shows everything. Phase 5 changed only help strings there.

### Human Verification Required

None open. UAT is complete (`05-UAT.md`: 2 passed, 1 skipped by the user, 0 issues).

### Gaps Summary

No truth failed. The phase goal is achieved: standard application menu with About, Preferences and Quit, Cmd shortcuts and Cmd+1..6 stage switching, Cmd+= and Cmd+- zoom (the Despeckling-tab defect WR-01 is fixed and tested), Mac key names in the UI text, Delete/Backspace and B zone keys, right-click, Control-click and two-finger-click menus, native file panels including relinking, and a menu bar that stays populated during dialogs. The one residual is the batch-finished beep: it is source-verified and test-pinned but was never heard. The user chose to skip that check and does not need it, so it is carried as an override, not as a pass on observed behaviour. The override entry is a judgement by the verifier based on the user's UAT note; delete it and the status becomes `human_needed` if the user wants the sound checked.

---

_Verified: 2026-11-15T19:10:00Z_
_Verifier: Claude (gsd-verifier)_
