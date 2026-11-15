---
phase: "5"
slug: "mac-menus-and-shortcuts"
status: verified
# threats_open = count of OPEN threats at or above workflow.security_block_on severity (the blocking gate)
threats_open: 0
asvs_level: 1
created: "2026-11-15"
---

# Phase 5 — Security

> Per-phase security contract: threat register, accepted risks, and audit trail.

---

## Trust Boundaries

| Boundary | Description | Data Crossing |
|----------|-------------|---------------|
| Settings file ↔ batch-finished alarm | A stored setting used to name a shell command run by `std::system` (removed in 05-09) | `main_window/external_alarm_cmd` string from the user's INI file |
| Keyboard focus ↔ zone and content-box editing | Delete keys remove zones and the content box; there is no undo | Key events from the focused widget |
| Harness process → window server | `mac_check` synthesises key and mouse events for its own windows and moves the pointer | NSEvents via `[NSApp sendEvent:]`; pointer warp via `CGWarpMouseCursorPosition` (no event posted) |
| Test and harness processes ↔ file system | `qt_tests` and `mac_check` write settings, temporary files and summaries | Files below `build/` only; records under `.planning/` hold no home path |
| Native file panels ↔ home folder | Panels can list folders and write panel state to preferences | Folder listings and panel state; `dialogs` and `panels` run only with HOME and CFFIXED_USER_HOME redirected below `build/` |
| App bundle identity ↔ macOS | The embedded Info.plist names the application for the menu bar | `CFBundleName` only |

---

## Threat Register

Built from the `<threat_model>` blocks of plans 05-01 to 05-11; evidence checked at ASVS level 1 (grep depth) on 2026-11-15 at HEAD `2ee9cfc0`.

| Threat ID | Category | Component | Severity | Disposition | Mitigation | Status |
|-----------|----------|-----------|----------|-------------|------------|--------|
| T-05-01 | Tampering | `app_gui` library and the scantailor-advanced link | medium | mitigate | Gate `m01-tracer` PASS with the command diff classified (`records/gate-log.md`) | closed |
| T-05-02 | Information disclosure | MainWindow settings and geometry written by qt_tests | medium | mitigate | `src/qt_tests/main.cpp:41-46` sets `TMPDIR` and `QSettings::setPath` below `TEST_TMP_ROOT` = `build/<preset>/test-tmp` (`src/qt_tests/CMakeLists.txt:25`) | closed |
| T-05-03 | Elevation of privilege | Role heuristic hoisting an unrelated action into the application menu | low | mitigate | `NoRole` on the other top-level actions in `src/app/MainWindow.ui` (20 entries); `TestMacQt::testMenuRoles` | closed |
| T-05-04 | Tampering | mac_check outputs and settings | medium | mitigate | Output path must lie below `build/` (exit 2 otherwise); `QSettings::setPath` to `<out>/settings` in `src/app/mac_check/main.cpp` | closed |
| T-05-05 | Spoofing | Synthesised NSEvents | medium | mitigate | Events only through `[NSApp sendEvent:]` in `src/app/mac_check/CocoaProbe.mm`; `CGEventPost`, `CGEventTap`, `CGEventCreate` count 0 in `src/app/mac_check/` | closed |
| T-05-06 | Tampering | Info.plist embed | medium | mitigate | `src/app/Info.plist` holds `CFBundleName` only; identity runs identical before and after (05-02) | closed |
| T-05-07 | Information disclosure | Records | low | mitigate | No `/Users/` path in `records/*.md` or TESTING.md (count 0); repository written as `{repo}` | closed |
| T-05-08 | Denial of service | Ambiguous shortcuts (stage ⌘ digits versus Output tabs, ⌘+ versus thumbnails) | medium | mitigate | `TestMacQt::testOutputTabKeys`, `testThumbnailKeys`; live `keys_verdict: PASS` with no cross fires (`records/keys.md`) | closed |
| T-05-09 | Tampering | Zoom acting on the wrong widget | low | mitigate | `zoomCurrentView` uses `findPrimaryImageView` only (`src/app/MainWindow.cpp`); `testZoomActions` | closed |
| T-05-10 | Tampering | Zone and content-box deletion from ⌫ typed into a field | medium | mitigate | Window-context shortcuts that the focused field takes first; `testZoneKeysIgnoredInTextFields`, `testContentBoxKeys`; live zone row (`records/zone.md`) | closed |
| T-05-11 | Tampering | ⌫ during a drag | low | mitigate | `captured()` early returns in `src/core/filters/select_content/ImageView.cpp` (3); zones unchanged from the ⌦ behaviour | closed |
| T-05-12 | Spoofing | Synthesised NSEvents and pointer warp | medium | mitigate | As T-05-05; pointer placed with `cocoaprobe::warpPointer` (`CGWarpMouseCursorPosition`, posts no event, no privacy permission; accepted deviation from `QCursor::setPos`, 05-05); records state the pointer moved | closed |
| T-05-13 | Information disclosure | Panels or dialogs opened by counted keys | medium | mitigate | Production slots disconnected and close events swallowed in `src/app/mac_check/main.cpp`; settings below `build/scratch/mac/` | closed |
| T-05-14 | Tampering | Removing the 200 ms deferral | low | mitigate | Removed only after `triggered()` preceded `aboutToHide` in 20 of 20 confirmed runs (`records/context.md`); `testZoneMenuDismissRestoresDefaultAtOnce`; gate `m09d-menu-delay` | closed |
| T-05-15 | Elevation of privilege | Batch-finished alarm running `main_window/external_alarm_cmd` through `std::system` (CWE-78) | high | mitigate | Code path removed in 05-09: `std::system`, `popen`, `QProcess`, `external_alarm_cmd` count 0 in `src/app/MainWindow.cpp`; `QApplication::beep();` once; pinned by `testBatchAlarmUsesBeep` | closed |
| T-05-16 | Tampering | Replacement paths from the native panel | low | accept | See Accepted Risks Log AR-05-01 | closed |
| T-05-17 | Repudiation | A scan that reads nothing and passes | low | mitigate | `testNoPcKeyNames` fails below 30 `.ui` or 500 C++ files read; mutation control in 05-08 and 05-09 made the scans fail | closed |
| T-05-18 | Information disclosure | Native panels listing user folders, panel state written to the user's preferences | high | mitigate | `homeRedirected(out)` guard in `src/app/mac_check/main.cpp`; guard run without redirection opened nothing (`panels_reason: home not redirected`, 05-10); every panel started below the output directory and was cancelled | closed |
| T-05-19 | Elevation of privilege | ⌘N reaching New Project while a modal dialog is open | low | accept | See Accepted Risks Log AR-05-02 | closed |
| T-05-20 | Denial of service | A fallback bar that shrinks the menu bar during dialogs | low | mitigate | Not needed: every dialog row kept the full bar (`dialogs_verdict: PASS`); no fallback bar added (`m_fallbackMenuBar` count 0) | closed |
| T-05-21 | Information disclosure | closing-check.md and TESTING.md | low | mitigate | No `/Users/` path (count 0); repository paths written as `{repo}` | closed |
| T-05-22 | Information disclosure | Closing panel run | high | mitigate | Closing dialogs and panels run used the redirected home below `build/scratch/mac/closing-panels/home`, which stayed empty (0 files); the input run opens no panel; the app binary was not launched | closed |
| T-05-23 | Tampering | Key-name text edits in `.ui` files (05-07) | low | mitigate | One line per file (`git show --numstat`); gates `m10a-dialog-hints`, `m10b-dialog-hints` PASS | closed |
| T-05-24 | Information disclosure | Records keys.md and zone.md | low | mitigate | As T-05-07 | closed |
| T-05-25 | Information disclosure | Record context.md | low | mitigate | As T-05-07 | closed |
| T-05-26 | Spoofing | Synthesised NSEvents and pointer warp in the context section | medium | mitigate | As T-05-12 | closed |
| T-05-27 | Tampering | Key-name text edits in `.ui` files and C++ string literals (05-08) | low | mitigate | Literals only, modifier logic untouched; `testNoPcKeyNames`, `testRewrittenStrings`; gates `m10c-key-names-ui`, `m11-key-names-scan` PASS | closed |
| T-05-SC | Tampering | npm/pip/cargo installs | low | accept | See Accepted Risks Log AR-05-03 | closed |

*Status: open · closed · open — below high threshold (non-blocking)*
*Severity: critical > high > medium > low — only open threats at or above workflow.security_block_on count toward threats_open*
*Disposition: mitigate (implementation required) · accept (documented risk) · transfer (third-party)*

---

## Accepted Risks Log

| Risk ID | Threat Ref | Rationale | Accepted By | Date |
|---------|------------|-----------|-------------|------|
| AR-05-01 | T-05-16 | Relinking takes whatever path the user picks in the native panel; the path handling after the panel (normalize call, both early returns) is unchanged from before the phase, and the user choosing their own files is the intended behaviour | Plan 05-09 threat model (disposition accept) | 2026-11-15 |
| AR-05-02 | T-05-19 | Modal dialogs block the main window; live measurement found ⌘N fired 0 times under all 7 dialog rows (`records/dialogs.md`), so the accepted risk did not arise | Plan 05-10 threat model (disposition accept) | 2026-11-15 |
| AR-05-03 | T-05-SC | The phase installs no packages; no package manifest changed since `f6cc4457`; dependencies remain the Homebrew set in the project constraints | Plan threat models (GSD convention row, disposition accept) | 2026-11-15 |

*Accepted risks do not resurface in future audit runs.*

---

## Security Audit Trail

| Audit Date | Threats Total | Closed | Open | Run By |
|------------|---------------|--------|------|--------|
| 2026-11-15 | 28 | 28 | 0 | secure-phase orchestrator (ASVS 1 short-circuit: register authored at plan time, 0 open; no auditor spawned) |

---

## Sign-Off

- [x] All threats have a disposition (mitigate / accept / transfer)
- [x] Accepted risks documented in Accepted Risks Log
- [x] `threats_open: 0` confirmed
- [x] `status: verified` set in frontmatter

**Approval:** verified 2026-11-15
