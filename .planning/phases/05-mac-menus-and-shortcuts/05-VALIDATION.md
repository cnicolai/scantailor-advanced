---
phase: "05"
slug: "mac-menus-and-shortcuts"
# status lifecycle: draft (seeded by plan-phase) → validated (set by validate-phase §6)
# audit-milestone §5.5 distinguishes NOT-VALIDATED (draft) from PARTIAL (validated + nyquist_compliant: false) (#2117)
status: draft
nyquist_compliant: false
wave_0_complete: false
created: "2026-11-14"
---

# Phase 05 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
> Seeded from `05-RESEARCH.md` § Validation Architecture.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Qt Test 6.12.0 inside the existing `qt_tests` executable (new class `TestMacQt`), plus a live harness `mac_check` (not a ctest suite) |
| **Config file** | `src/qt_tests/CMakeLists.txt`, `src/qt_tests/main.cpp` |
| **Quick run command** | `cmake --build --preset macos-release && ctest --preset macos-release -R qt_tests` |
| **Full suite command** | `ctest --preset macos-release` and `ctest --preset macos-debug` (7 of 7); per commit `bash .planning/phases/03-qt-6-and-macos-only/gate.sh check <label>` |
| **Live check** | `build/macos-release/mac_check build/scratch/mac/all --section all -ApplePersistenceIgnoreState YES` (exit 0 pass, 1 fail, 2 usage or path outside `build/`, 3 platform not Cocoa) |
| **Estimated runtime** | ~60 seconds for `qt_tests`; full gate several minutes |

---

## Sampling Rate

- **After every task commit:** Run the quick run command and `xcrun clang-format --dry-run -Werror` on touched C++ files
- **After every plan wave:** Run the full suite command (Release and Debug) through `gate.sh check <label>`
- **Before `/gsd-verify-work`:** Full suite must be green, goldens unchanged (`git diff a796c2ae -- src/pipeline_tests/golden` empty), no output-generation path changed since the phase start (`git diff f6cc4457 -- …` empty, see "Diff bases"), harness records complete
- **Max feedback latency:** about 120 seconds for the quick loop (`qt_tests` build and run after a task); the per-commit gate (`gate.sh check <label>`, Release and Debug ctest) takes 5 to 10 minutes and runs in the background after each commit

---

## Diff Bases

Two commits serve as diff bases in this phase. Both must show no change in their scope.

| Base | Commit | Used by | Scope |
|------|--------|---------|-------|
| Golden baseline | `a796c2ae` (`test(02-07): record mixed-page goldens from macos-release`) | `gate.sh` (`GOLDEN_BASE=a796c2ae`) on every gated commit; the closing static check of 05-11 (`git diff --quiet a796c2ae -- src/pipeline_tests/golden`) | `src/pipeline_tests/golden` |
| Phase-start base | `f6cc4457` (planning HEAD; its `src/` equals the end of Phase 4) | the output-path guard of every code task (convention 14 of 05-01: `git diff f6cc4457 -- src/pipeline_tests/golden src/imageproc src/dewarping src/core/filters/output/OutputGenerator.cpp 'src/core/filters/*/Task.cpp' 'src/core/filters/*/Task.h'` empty), the `[HEAD=… @f6cc4457]` baselines of the acceptance criteria, and `git diff --name-only f6cc4457 -- src` for the files Phase 5 changed (05-11 sanitizer triage) | goldens plus every output-generation path |

---

## Per-Task Verification Map

Filled from the final plans 05-01 to 05-11 (revision of 2026-11-14). `qt_tests` stands for `cmake --build --preset macos-release --target qt_tests && build/macos-release/qt_tests` with the named `TestMacQt` slots required to PASS; `gate <label>` stands for `bash .planning/phases/03-qt-6-and-macos-only/gate.sh check <label>` with `result: PASS` and `goldens: unchanged`. "Live harness" rows run `build/macos-release/mac_check` on the Cocoa platform; nothing is physically pressed (D-11).

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 05-01-T1 | 05-01 | 1 | MAC-01, MAC-02 | T-05-01, T-05-02 | MainWindow built in qt_tests keeps settings under build/<preset>/test-tmp | unit (qt_tests offscreen) + gate | `qt_tests` (testSettingsInFileWithPreferencesRole, testSettingsShortcutOpensSettings); `gate m01-tracer` | ❌ W0: `tst_mac_qt.{h,cpp}` created in this task | ⬜ pending |
| 05-01-T2 | 05-01 | 1 | MAC-01, MAC-03, MAC-05 | T-05-03 | NoRole on every other top-level action, so the heuristic hoists nothing | unit + gate | `qt_tests` (testMenuRoles, testFileMenuTextsAndKeys, testToolsMenuWithoutSettings, testQuitClosesWindow, testProjectActionsDisabledWithoutProject); `gate m02-menu-roles` | ✅ after 05-01-T1 | ⬜ pending |
| 05-02-T1 | 05-02 | 2 | MAC-01, MAC-02 | T-05-04, T-05-05 | output path below build/ (exit 2 otherwise); events only to the harness's own windows | build + guard runs + gate | `mac_check build/../outside-mac`, `--section nosuch`, `--section identity,menubar` and `--section identity` with the real home all exit 2 (the last with `home not redirected`, no summary written); `gate m03-mac-check` | ❌ W0: `src/app/mac_check/` created in this task | ⬜ pending |
| 05-02-T2 | 05-02 | 2 | MAC-01, MAC-02, MAC-04, MAC-05 | T-05-06, T-05-07 | plist carries CFBundleName only; plist kept only if the identity runs show it in the process and identical names, default settings file and profiles path; records without home path | live harness | live harness: `mac_check build/scratch/mac/m03 --section menubar,layout` (menubar_verdict, layout_verdict PASS); conditional on the plist: `env HOME=… CFFIXED_USER_HOME=… mac_check build/scratch/mac/m03i --section identity` and the same into `m03pi` (bundle_name none / ScanTailor Advanced, identity_verdict PASS, five identity lines identical), `gate m03p-app-name`; conditional `gate m03h-help-menu` | ✅ after 05-02-T1 | ⬜ pending |
| 05-03-T1 | 05-03 | 3 | MAC-05, MAC-06 | T-05-08 | no ambiguous shortcut between stage keys and Output tabs | unit + gate | `qt_tests` (testStageActions, testViewMenuStages, testOutputTabKeys); `gate m04-stage-keys` | ✅ | ⬜ pending |
| 05-03-T2 | 05-03 | 3 | MAC-04, MAC-06 | T-05-08, T-05-09 | zoom acts only on the page view, never on thumbnails | unit + gate | `qt_tests` (testZoomActions, testZoomDisabledWithoutView, testThumbnailKeys, testViewMenuContents); `gate m05-zoom-keys` | ✅ | ⬜ pending |
| 05-04-T1 | 05-04 | 4 | MAC-07, MAC-06 | T-05-10, T-05-11 | ⌫ typed into a field never deletes a zone | unit + gate | `qt_tests` (testZoneRemoveKeys, testZoneKeysIgnoredInTextFields, testZoneStatusTips); `gate m06-zone-keys` | ✅ | ⬜ pending |
| 05-04-T2 | 05-04 | 4 | MAC-07, MAC-08 | T-05-10, T-05-11 | content-box removal guarded during a drag | unit + gate | `qt_tests` (testContentBoxKeys, testContentMenuShowsKeys, testRightClickOpensZoneMenu); `gate m07-content-box-keys` | ✅ | ⬜ pending |
| 05-05-T1 | 05-05 | 5 | MAC-02, MAC-03, MAC-04, MAC-05, MAC-07 | T-05-12, T-05-13 | production slots disconnected before counting; close events swallowed | live harness + gate | live harness: `mac_check build/scratch/mac/m08 --section keys,zone` (keys_verdict, zone_verdict PASS); `gate m08-mac-check-keys` | ✅ (`mac_check` from 05-02) | ⬜ pending |
| 05-05-T2 | 05-05 | 5 | MAC-02, MAC-03, MAC-04, MAC-05, MAC-07 | T-05-07 | records without home path | record check | grep of verdicts, "physically pressed", the Backspace answer and a `$HOME` count of 0 in `records/keys.md` and `records/zone.md` | ❌ records created in this task | ⬜ pending |
| 05-06-T1 | 05-06 | 6 | MAC-08 | T-05-12 | events only to own windows; pointer moved with QCursor::setPos | live harness + gate | live harness: `mac_check build/scratch/mac/m09 --section context` (context_verdict PASS, context_order_runs 20, context_two_finger_click line); `gate m09-mac-check-context` | ✅ | ⬜ pending |
| 05-06-T2 | 05-06 | 6 | MAC-08 | T-05-14, T-05-07 | deferral removed only after 20 of 20 measured runs | unit (conditional) + record check | conditional `qt_tests` (testZoneMenuDismissRestoresDefaultAtOnce) and `gate m09d-menu-delay`; grep of `records/context.md` for the decision and "Unverified residual: two-finger click" | ❌ `records/context.md` created in this task | ⬜ pending |
| 05-07-T1 | 05-07 | 7 | MAC-06 | T-05-23 | text-only edit: one line per file | grep + numstat + gate | new hint in the four dialogs of stages 1 to 4; `git show --numstat` totals 4/4/4; `gate m10a-dialog-hints` | ✅ | ⬜ pending |
| 05-07-T2 | 05-07 | 7 | MAC-06 | T-05-23 | text-only edit: one line per file | grep + numstat + gate | old hint in 0 files, new hint in 9; `git show --numstat` totals 5/5/5; `gate m10b-dialog-hints` | ✅ | ⬜ pending |
| 05-08-T1 | 05-08 | 8 | MAC-06 | T-05-23 | text-only edit | grep + gate | whole-word Ctrl/Alt/LMB/Del/Ins in translatable `.ui` lines: 0; `gate m10c-key-names-ui` | ✅ | ⬜ pending |
| 05-08-T2 | 05-08 | 8 | MAC-06 | T-05-17, T-05-23 | scan reads at least 30 `.ui` and 500 C++ files | source scan (qt_tests) + gate | `qt_tests` (testNoPcKeyNames, testRewrittenStrings); `gate m11-key-names-scan` | ✅ | ⬜ pending |
| 05-09-T1 | 05-09 | 9 | MAC-10 | T-05-15 | no shell command from a settings value | source scan (qt_tests) + gate | `qt_tests` (testBatchAlarmUsesBeep); `gate m12-beep` | ✅ | ⬜ pending |
| 05-09-T2 | 05-09 | 9 | MAC-11 | T-05-16 | no dialog forces Qt's own panel | source scan (qt_tests) + gate | `qt_tests` (testNoDontUseNativeDialog); `gate m13-native-relink` | ✅ | ⬜ pending |
| 05-10-T1 | 05-10 | 10 | MAC-09, MAC-11 | T-05-18 | dialogs and panels refuse to run without a home redirected below build/ | build + guard run + gate | `mac_check build/scratch/mac/m14-guard --section panels` exits 1 with `panels_reason: home not redirected`; `gate m14-mac-check-dialogs` | ✅ | ⬜ pending |
| 05-10-T2 | 05-10 | 10 | MAC-09, MAC-11, MAC-01 | T-05-18, T-05-19, T-05-20 | every panel opens below the output directory and is cancelled by the harness | live harness | live harness: `env HOME=… CFFIXED_USER_HOME=… mac_check build/scratch/mac/m14 --section dialogs,panels` (dialogs_verdict, panels_verdict PASS); conditional `qt_tests` (testFallbackMenuBarHoldsSameActions) and `gate m14f-menubar-fallback` | ❌ `records/dialogs.md`, `records/panels.md` created in this task | ⬜ pending |
| 05-11-T1 | 05-11 | 11 | MAC-01 to MAC-11 (documentation) | T-05-21 | no home path in TESTING.md | docs grep + gate | grep of `## Live Mac check`, `TestMacQt`, `app_gui`, `CFFIXED_USER_HOME` in TESTING.md; `gate m15-docs` | ✅ TESTING.md | ⬜ pending |
| 05-11-T2 | 05-11 | 11 | MAC-01 to MAC-11 | T-05-21, T-05-22 | closing panel run only with the redirected home | live harness + ctest + sanitizer + static checks | live harness: both closing `mac_check` runs (`verdict: PASS`); `ctest --preset macos-release` and `macos-debug` (7 of 7); `ctest --preset macos-asan -R qt_tests`; static greps; conditional `gate m16-sanitizer-fix` | ❌ `records/closing-check.md` created in this task | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/app/CMakeLists.txt` — `app_gui` static library; `scantailor-advanced` and `ui_snapshot` link it (closes review finding WR-01)
- [ ] `src/qt_tests/tst_mac_qt.{h,cpp}` — `TestMacQt` registered in `src/qt_tests/main.cpp` and `CMakeLists.txt`, with the string-scan helpers
- [ ] `src/app/mac_check/` — live harness (Objective-C++ probe, path guard like `retina_check`)
- [ ] `.planning/phases/05-mac-menus-and-shortcuts/records/` — verdict records (gate-log 05-01; menubar, layout 05-02; keys, zone 05-05; context 05-06; dialogs, panels 05-10; closing-check 05-11)
- [ ] `TESTING.md` — "Live Mac check" section; `qt_tests` row lists the new tests

---

## Manual-Only Verifications

None. D-11: Claude verifies everything; there is no GUI walkthrough and no human-verify step. Behaviours that no in-process check can observe are listed below as unverified residuals, stated in the records and in the end-of-phase report, not handed to the user as manual steps.

## Unverified Residuals

| Behavior | Requirement | Why it cannot be measured in-process | What stands in for it | Where it is stated |
|----------|-------------|--------------------------------------|-----------------------|--------------------|
| A two-finger trackpad click opens the context menu | MAC-08 | At the NSEvent level a two-finger click and a right-click are the same `rightMouseDown`; in-process injection cannot produce a distinct two-finger event, so none is injected | The live right-click row (05-06: one context-menu event, menu shown, first click deletes) and the Control-click row | `records/context.md` section "Unverified residual: two-finger click"; "For the user" in `records/closing-check.md` (05-11) |
| The system beep is audible when a batch run finishes | MAC-10 | Sound output cannot be observed in-process | Source check `testBatchAlarmUsesBeep` (05-09): `QApplication::beep()` exactly once after `QApplication::alert(this)`, no shell path | "For the user" in `records/closing-check.md` (05-11), line "Unverified residual: beep" |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 120s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
