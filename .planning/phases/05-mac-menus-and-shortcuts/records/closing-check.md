# Closing check of Phase 5 (plan 05-11)

Checked on 2026-11-15 on HEAD `42d28934` (the gate record of `m15-docs`; the documentation commit is `0465921d`). The last commit that changed anything under `src/` or a build file is `efe9cbdb` (`m14-mac-check-dialogs`), so the last gated source commit and the tree measured here are the same code. This plan fixed no code: the sanitizer run had no report in a file Phase 5 changed, so `m16-sanitizer-fix` does not exist. Phase 5 base (the planning base, whose `src/` equals the end of Phase 4): `f6cc4457`. Output is normalised: `{repo}` for the repository path, `~` for the home folder.

No key or click was physically pressed. Every key and mouse event of the live runs was synthesised inside the `mac_check` harness process and sent to its own windows through `[NSApp sendEvent:]`; nothing went to another application and no Accessibility, Input Monitoring or screen-recording permission was used. The pointer was moved by the harness (`CGWarpMouseCursorPosition`, accepted in 05-05 and 05-06) and dialogs and file panels opened for a few minutes, which the user had been told about. The application binary `scantailor-advanced` was not launched (it reads and writes `~/.config` and `~/Library`); the harness and `qt_tests` stand in for it, and this record says so wherever a statement rests on that stand-in.

Evidence is in this directory (`gate-log.md`, `menubar.md`, `layout.md`, `keys.md`, `zone.md`, `context.md`, `dialogs.md`, `panels.md`), in the permanent tests of `TestMacQt` (`src/qt_tests/tst_mac_qt.cpp`, 25 tests) and in the closing runs below.

## Closing runs at HEAD

Two live runs, made once each (no re-run was needed; the runs report `context_pointer_interference: 0`, `context_failed_attempts_pointer_confirmed: 0` and every section `app_active: yes`):

```
$ build/macos-release/mac_check build/scratch/mac/closing-input --section menubar,layout,keys,zone,context -ApplePersistenceIgnoreState YES
menubar_verdict: PASS
layout_verdict: PASS
zone_verdict: PASS
context_verdict: PASS
keys_verdict: PASS
verdict: PASS            (exit 0)

$ D=build/scratch/mac/closing-panels; env HOME={repo}/$D/home CFFIXED_USER_HOME={repo}/$D/home build/macos-release/mac_check $D --section dialogs,panels -ApplePersistenceIgnoreState YES
dialogs_verdict: PASS
panels_verdict: PASS
verdict: PASS            (exit 0)
```

Both runs: platform cocoa, Qt 6.12.0, macOS 26.7.2, keyboard layout `com.apple.keylayout.German`, window device pixel ratio 2. Summaries: `build/scratch/mac/closing-input/summary.txt` and `build/scratch/mac/closing-panels/summary.txt`, console output in `build/scratch/mac/closing-input-run.txt` and `closing-panels-run.txt`. The redirected home of the second run stayed empty. The console shows the existing `QObject::disconnect: wildcard call disconnects from destroyed signal` lines when a `MainWindow` is destroyed (also present in the 05-01, 05-05 and 05-10 runs; they come from older teardown code).

```
$ ctest --preset macos-release      -> 100% tests passed out of 7    (build/scratch/closing-ctest-release.txt)
$ ctest --preset macos-debug        -> 100% tests passed out of 7    (build/scratch/closing-ctest-debug.txt)
```

## Success criteria

ROADMAP Phase 5, criteria 1 to 5.

| # | Verdict | Evidence |
|---|---------|----------|
| 1 | PASS | Application menu: the live bar holds About, Settings with `⌘,` and Quit with `⌘q` once each, none of them again in File or Tools (`records/menubar.md`, `menubar_verdict: PASS`, repeated in the closing input run); `testSettingsInFileWithPreferencesRole`, `testSettingsShortcutOpensSettings` (`⌘,` opens `SettingsDialog`), `testMenuRoles`, `testToolsMenuWithoutSettings`, `testFileMenuTextsAndKeys`. Settings is no longer in the Tools (Debug) menu in Release and in Debug (`testToolsMenuWithoutSettings` runs in both ctest presets). The bar stays populated while a dialog is open: `records/dialogs.md` (`dialogs_verdict: PASS`, repeated in the closing panels run) shows File, View and Tools plus the application menu under Settings, About, Fix DPI, Relinking, Default Parameters, an application-modal message box and the native Open Project panel, so no fallback bar exists. Gate rows `m01-tracer`, `m02-menu-roles`, `m03h-help-menu`, `m03p-app-name`, `m14-mac-check-dialogs`. The native Settings item reads "Preferences..." (Qt's merged text), not "Settings…"; see "For the user". |
| 2 | PASS | `⌘N`, `⌘O`, `⌘S`, `⌘W`, `⌘Q`, `⇧⌘S`: `testFileMenuTextsAndKeys` (each key fires exactly once), `testQuitClosesWindow`, and the live `keys` section (`records/keys.md`, `keys_verdict: PASS`, repeated in the closing run: every File and Settings key fires its own action once, `⌘Q` reaches `actionQuit` once). `⌘=` and `⌘-` (the zoom pair, also `⌘+`) and `⌘1` to `⌘6` (stages): `testZoomActions`, `testZoomDisabledWithoutView`, `testStageActions`, `testViewMenuStages`, `testViewMenuContents`, `testOutputTabKeys`, `testThumbnailKeys`, and the live `layout` and `keys` sections (`layout_verdict: PASS`; zoom by 1.122462 and 0.890899 per press; `⌘=` also fires on the German layout through Shift-0 and is not part of the verdict, D-06). Mac key names: `testNoPcKeyNames` scans every `.ui` file and C++ string literal under the scan scope (at least 30 and 500 files) for `Ctrl`, `Alt` and the like and passes with zero hits, `testRewrittenStrings` pins the rewritten texts; the nine apply-dialog hints and the Margins help are edited in `10e50d98`, `31de6ead`, `c4594341`, `a4314bc3`. Gate rows `m04-stage-keys`, `m05-zoom-keys`, `m08-mac-check-keys`, `m10a-dialog-hints` to `m11-key-names-scan`. Not rendered: the shorter dialog hints and the 7 pt bold glyphs of the Margins help were checked as markup and string length, never drawn (see "For the user"). |
| 3 | PASS | `"Selected zone" is read under D-10: the zone editor keeps no zone selection, and ⌫ and ⌦ remove the zone under the mouse pointer, the one whose status tip names the delete keys, as Delete did before Phase 5.` (ROADMAP criterion 3 says "removes the selected zone"; CONTEXT D-10 locks the under-pointer meaning.) Removal: `testZoneRemoveKeys`, `testZoneKeysIgnoredInTextFields` (Backspace in a focused line edit or spin box edits the text and removes no zone), `testZoneStatusTips`; live `zone` section (`records/zone.md`, `zone_verdict: PASS`, repeated in the closing run: Backspace and forward delete each remove the zone under the pointer; the status tip `Press ⌫` proves the pointer was over it first). Content box: `B` creates it and Backspace or forward delete removes it: `testContentBoxKeys`, `testContentMenuShowsKeys`, live `content_b_created: yes`, `content_backspace_removed: yes`. The STATE.md question is answered in `records/zone.md`: Backspace reaches the shortcuts under Qt 6.12 on Cocoa. Context menus: a right-click and a Control-click each opened the Qt-drawn zone menu once, the first item removed the zone and Escape dismissed it (`records/context.md`, `context_verdict: PASS`, repeated in the closing run with 0 pointer interference); `testRightClickOpensZoneMenu`, `testZoneMenuDismissRestoresDefaultAtOnce`. The two-finger click is an unverified residual (see "For the user"): it arrives as the same `rightMouseDown` as a right-click and was not injected separately. Gate rows `m06-zone-keys`, `m07-content-box-keys`, `m09-mac-check-context`, `m09d-menu-delay`. |
| 4 | PASS (source check, not heard) | `testBatchAlarmUsesBeep`: `MainWindow.cpp` has none of `std::system`, `system(`, `QProcess`, `popen`, `/usr/share/sounds`, `external_alarm_cmd`; the batch-finished block holds `QApplication::alert(this);` once, the "Beep when finished" check once and `QApplication::beep();` once, the beep after the alert; no `.cpp`, `.h` or `.mm` file of the scan scope contains `std::system`, `popen(` or `QProcess`. A stopped batch run still returns before the block and stays silent. Gate row `m12-beep`. The beep was not heard (see "For the user"). |
| 5 | PASS | `records/panels.md` (`panels_verdict: PASS`, repeated in the closing panels run): the relink file, relink folder, Open Project, input folder and output folder panels are each an `NSOpenPanel`, start below the output directory and return an empty path when cancelled; both relinking rows leave the substitution unchanged after `commitChanges`. `testNoDontUseNativeDialog`: no file of the scan scope contains `DontUseNativeDialog`, `AA_DontUseNativeDialogs`, `AA_DontUseNativeMenuBar`, `setNativeMenuBar(false)` or `AA_MacDontSwapCtrlAndMeta`, and `RelinkingDialog.cpp` still calls both choosers. `Save Project As`, the save panel of `OutOfMemoryDialog` and `Files to insert` need a loaded project; they are plain `QFileDialog` static calls without options, native by the same mechanism, and were checked by source, not opened. Gate rows `m13-native-relink`, `m14-mac-check-dialogs`. |

## Requirements

| ID | Plans | Tests | Records |
|----|-------|-------|---------|
| MAC-01 | 05-01, 05-02, 05-10 | `testSettingsInFileWithPreferencesRole`, `testMenuRoles`, `testToolsMenuWithoutSettings` | `menubar.md`, `dialogs.md` |
| MAC-02 | 05-01, 05-02, 05-05 | `testSettingsShortcutOpensSettings`, `testSettingsInFileWithPreferencesRole` | `menubar.md`, `keys.md` |
| MAC-03 | 05-01, 05-05 | `testFileMenuTextsAndKeys` | `keys.md`, `menubar.md` (`standard_keys_match: yes` for Save As) |
| MAC-04 | 05-02, 05-03, 05-05 | `testZoomActions`, `testZoomDisabledWithoutView`, `testThumbnailKeys`, `testViewMenuContents` | `layout.md`, `keys.md` |
| MAC-05 | 05-01, 05-02, 05-03, 05-05 | `testFileMenuTextsAndKeys`, `testQuitClosesWindow`, `testProjectActionsDisabledWithoutProject`, `testStageActions`, `testViewMenuStages`, `testOutputTabKeys` | `keys.md`, `layout.md`, `menubar.md` |
| MAC-06 | 05-03, 05-04, 05-07, 05-08 | `testNoPcKeyNames`, `testRewrittenStrings`, `testZoneStatusTips`, `testContentMenuShowsKeys`, `testOutputTabKeys` | none (strings are checked by tests; rendering not observed) |
| MAC-07 | 05-04, 05-05 | `testZoneRemoveKeys`, `testZoneKeysIgnoredInTextFields`, `testContentBoxKeys`, `testContentMenuShowsKeys` | `zone.md` |
| MAC-08 | 05-04, 05-06 | `testRightClickOpensZoneMenu`, `testZoneMenuDismissRestoresDefaultAtOnce` | `context.md` (right-click and Control-click measured; two-finger click unverified) |
| MAC-09 | 05-10 | none offscreen (the native bar needs Cocoa) | `dialogs.md` |
| MAC-10 | 05-09 | `testBatchAlarmUsesBeep` | none (the beep was not heard) |
| MAC-11 | 05-09, 05-10 | `testNoDontUseNativeDialog` | `panels.md` |

## Gate log

`records/gate-log.md` has 21 rows (`m01-tracer` to `m15-docs`); every row reads `100% tests passed out of 7` for Release and for Debug and `unchanged` for the goldens (checked by script at the closing, 21 of 21 in each of the three columns). The conditional commits `m03p-app-name`, `m03h-help-menu` and `m09d-menu-delay` exist; `m14f-menubar-fallback` does not (no bar was measured reduced) and `m16-sanitizer-fix` does not (no report in a Phase 5 file). Two harness-only commits of 05-06 (`3effa807`, `39dd0632`) were not gated on their own; they change only `src/app/mac_check/main.cpp` and are covered by the later gate `m09d-menu-delay` (command diff 0 / 0). The generic GSD regression gate finds no test runner in this repository; the commands that count are the two ctest presets above.

## Static checks

Commands run from the repository root; outputs are in `build/scratch/closing/static-checks.txt`.

```
$ git diff f6cc4457 -- src/pipeline_tests/golden src/imageproc src/dewarping src/core/filters/output/OutputGenerator.cpp 'src/core/filters/*/Task.cpp' 'src/core/filters/*/Task.h'
(no output)
$ git diff --quiet a796c2ae -- src/pipeline_tests/golden
exit status: 0
$ git ls-files '*.mm'
src/app/mac_check/CocoaProbe.mm
$ grep -rn -e AA_MacDontSwapCtrlAndMeta -e AA_DontUseNativeMenuBar -F -e 'setNativeMenuBar(false)' src | grep -v '^src/qt_tests/'
(no output, 0 lines)
$ same patterns in src/qt_tests/tst_mac_qt.cpp        (positive control)
1421: const QStringList forbidden{"DontUseNativeDialog", "AA_DontUseNativeDialogs", "AA_DontUseNativeMenuBar",
1422:                             "setNativeMenuBar(false)", "AA_MacDontSwapCtrlAndMeta"};
1433: QCOMPARE(firstForbidden(QStringLiteral("setAttribute(Qt::AA_DontUseNativeMenuBar);")),
$ git diff --stat f6cc4457 -- src | tail -1            (positive control: the command does see the phase's changes)
 32 files changed, 4407 insertions(+), 123 deletions(-)
$ git log --format=%H --grep='^fix(05-11):' f6cc4457..HEAD | wc -l
0
```

D-00a holds: output generation and the goldens are untouched and `UPDATE_GOLDENS` was never set. D-00b holds: the only Objective-C++ file is `CocoaProbe.mm`, built into `mac_check` only, and the forbidden menu-bar and modifier attributes occur in `src` only inside the test scan lists of `tst_mac_qt.cpp`.

## Sanitizer run of qt_tests

The `macos-asan` preset (Debug, `SANITIZE=address,undefined`) was configured and built for the `qt_tests` target (`build/scratch/closing-asan-build.txt`, exit 0) and `ctest --preset macos-asan -R qt_tests --output-on-failure` reported `100% tests passed out of 1` (`build/scratch/closing-asan-ctest.txt`). ctest hides the reports of passing tests, so `qt_tests` was also run directly with `ASAN_OPTIONS=detect_leaks=0:halt_on_error=0 UBSAN_OPTIONS=print_stacktrace=1` (`build/scratch/closing-asan-qt-tests.txt`, exit 0): Totals 8 + 7 + 28 + 27 passed, 0 failed, 0 skipped (`TestCoreQt`, `TestUxQt`, `TestHiDpiQt`, `TestMacQt`).

- `ERROR: AddressSanitizer` lines: 0.
- `runtime error:` lines: 1, `{repo}/src/imageproc/BinaryImage.cpp:1044:37: member access within null pointer of type 'SharedData'`, raised during `TestHiDpiQt::testZoneMaskAtDeviceResolution` through the Phase 4 test helper `makeZoneMask` (`tst_hidpi_qt.cpp:256`) and `BinaryImage::fromRgb32`.

Attribution: the location is `src/imageproc/BinaryImage.cpp`, which Phase 5 did not change (`git diff --name-only f6cc4457 -- src` does not list it) and which the Phase 1 record, the Phase 2 baseline (`.planning/phases/02-golden-baselines/records/sanitizer-findings.md`) and the Phase 4 closing record list as known. The report predates Phase 5 and is recorded, not fixed (Phase 2 rule). No report is located in a file Phase 5 created or changed, and `TestMacQt` (27 tests including init and cleanup) ran clean, so there is nothing to fix under `m16-sanitizer-fix`.

## For the user

- Chord group: the Output tabs are bound to `⌥⌘1` to `⌥⌘5` and the thumbnail sizes to `⌥⌘+` and `⌥⌘-` (from `records/layout.md`: on the German layout every Option-Command chord fired exactly once, so no Control-Command fallback exists in the code). The stage keys are `⌘1` to `⌘6` (the old letter keys still work), zoom is `⌘+` or `⌘=` and `⌘-`.
- Accessibility Zoom caveat: if "Use keyboard shortcuts to zoom" is on in System Settings > Accessibility > Zoom, macOS takes `⌥⌘+` and `⌥⌘-` before the app sees them. The harness cannot see that setting; if the thumbnail keys seem dead, check it first.
- Unverified residual: two-finger click. A two-finger trackpad click reaches the app as the same `rightMouseDown` as a right-click, so the measured right-click row stands for it; no two-finger event was injected separately and nobody clicked; see `records/context.md`. D-11 allows no manual step in its place.
- Unverified residual: beep. The system beep is proven by the source check `testBatchAlarmUsesBeep`, not heard; D-11 allows no manual step in its place.
- Application-menu name decision: the application menu first read "About mac_check" and "Quit mac_check" (the process name), so `src/app/Info.plist` with `CFBundleName` only is embedded into `scantailor-advanced` and `mac_check`; the menu now reads "About ScanTailor Advanced", "Hide ScanTailor Advanced" and "Quit ScanTailor Advanced", and the settings and profile paths were measured identical with and without it (`records/menubar.md`). The Settings item itself reads "Preferences..." (three ASCII dots), Qt's merged text, not the UI-SPEC wording "Settings…"; the key and the function are right and the text was left alone (a `QTranslator` for the context `QMenuBar` would change it, if the user wants).
- Help-menu decision: macOS showed an empty Help menu, so About moved into File and `menuHelp` was removed; there is no Help menu.
- 200 ms decision: the 200 ms hide delay of the zone context menu was removed, because `triggered()` ran before the queued `aboutToHide` slot in 20 of 20 confirmed runs (`records/context.md`).
- Fallback-bar decision: no fallback menu bar is needed, because the bar stayed populated in all seven dialog rows; `m_fallbackMenuBar` does not exist.
- `⌘N` under a dialog (`records/dialogs.md`): `⌘N` fired 0 times in all seven rows, so no dialog lets `⌘N` reach New Project; Qt disables the item under a window-modal or application-modal dialog, and the native panel's modal session does not route the key to the main menu. Nothing is broken by that; it is how modal dialogs behave.
- `Files to insert` observation (`records/panels.md`): the dialog is native at HEAD, so its `ProxyModel`, which hid files already in the project in the old Qt dialog, is not consulted any more. This predates the phase and was not repaired.
- Not rendered, only argued from markup and string length: the shorter hints of the nine apply dialogs and the 7 pt bold glyphs of the Margins help (05-07, 05-08). Whether the relinking panel title is drawn and whether the folder panels grey out files was not observed either (no screenshot permission, D-12).
- Phase 3 review finding WR-01 (the GUI sources compiled twice) is closed by the static library `app_gui`; STATE.md's Phase 5 Backspace question is answered in `records/zone.md`.
- The codebase map `.planning/codebase/` predates Phase 3's removals and Phase 5's new targets (`app_gui`, `mac_check`, `TestMacQt`); the drift gate reports it as a non-blocking advisory. It was not refreshed in this plan.

Phase 5 verdict: PASS
