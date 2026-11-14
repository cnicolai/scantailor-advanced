---
phase: 05-mac-menus-and-shortcuts
plan: 02
subsystem: ui
tags: [cocoa, appkit, objective-c++, nsmenu, uckeytranslate, info-plist, qt6, live-harness]

requires:
  - phase: 05-mac-menus-and-shortcuts
    provides: app_gui static library, explicit menu roles, TestMacQt, gate log (plan 05-01)
provides:
  - mac_check, the live Cocoa harness (sections menubar, layout, identity) with the Objective-C++ probe confined to src/app/mac_check
  - Measured native application menu (About, Settings with Cmd-comma, Quit with Cmd-Q, each once) and no Help menu
  - Embedded Info.plist section (CFBundleName only) so the application menu names ScanTailor Advanced
  - German-layout chord measurement; Option-Command is the group plan 05-03 binds for the Output tabs and the thumbnail keys
affects: [05-03, 05-04, 05-05, 05-06, 05-10, 05-11]

actuals:
  tokens: 17894
  tasks: 2
  commits: 9

plan_head_before: 37dde478c88b28305ce3d0aada3272a02c73355c
plan_head_after: 1a403f40b355434b24209fd4bf5a787f2ac65336
commits: 9

tech-stack:
  added: []
  patterns:
    - "Objective-C++ only in the test harness (CocoaProbe.mm), never in app_gui, core or scantailor-advanced"
    - "Synthesised NSEvents go to this process's own windows through [NSApp sendEvent:]; no CGEventPost, no event tap"
    - "Native menus are validated with -[NSMenu update] before isEnabled is read"
    - "Info.plist embedded with -sectcreate,__TEXT,__info_plist; the file stays in the source tree"

key-files:
  created:
    - src/app/mac_check/main.cpp
    - src/app/mac_check/CocoaProbe.h
    - src/app/mac_check/CocoaProbe.mm
    - src/app/Info.plist
    - .planning/phases/05-mac-menus-and-shortcuts/records/menubar.md
    - .planning/phases/05-mac-menus-and-shortcuts/records/layout.md
  modified:
    - src/app/CMakeLists.txt
    - src/app/MainWindow.ui
    - src/qt_tests/tst_mac_qt.cpp
    - .planning/phases/05-mac-menus-and-shortcuts/records/gate-log.md

key-decisions:
  - "About moved from the empty Help menu into File (role unchanged); there is no Help menu any more"
  - "Info.plist with CFBundleName 'ScanTailor Advanced' is embedded in scantailor-advanced and mac_check; settings and profiles paths measured identical with and without it"
  - "Plan 05-03 binds the Output tabs with Option-Command 1 to 5 and the thumbnail keys with Option-Command plus and minus (output_tabs_group and thumbnails_group both option-command)"
  - "Native Settings item text is Qt's 'Preferences...' not UI-SPEC's 'Settings…'; recorded as an open point, not changed"

patterns-established:
  - "Live harness section function per question, summary.txt with key: value lines and a verdict per section"
  - "Records quote the raw summary lines, replace the repository by {repo} and the home by ~, and state that nothing was physically pressed"

requirements-completed: [MAC-01, MAC-02, MAC-04, MAC-05]

coverage:
  - id: D1
    description: "mac_check builds with its Objective-C++ probe confined to the harness, refuses paths outside build/, unknown sections and an identity run without the redirected home"
    requirement: MAC-01
    verification:
      - kind: other
        ref: "build/macos-release/mac_check build/../outside-mac --section menubar (exit 2, 'is not under'); --section nosuch and identity,menubar (exit 2, usage); --section identity with the real home (exit 2, 'home not redirected', no summary)"
        status: pass
      - kind: integration
        ref: "gate m03-mac-check: 100% tests passed out of 7 in Release and Debug, goldens unchanged, command diff 0 / 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "The live native application menu holds About, Settings (Cmd-comma) and Quit (Cmd-Q) exactly once, none of them again in File or Tools, no empty Help menu, standard keys equal the explicit sequences"
    requirement: MAC-02
    verification:
      - kind: other
        ref: "build/scratch/mac/m03p/summary.txt menubar_verdict: PASS (records/menubar.md)"
        status: pass
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#TestMacQt::testFileMenuTextsAndKeys"
        status: pass
    human_judgment: true
    rationale: "The native Settings item reads 'Preferences...' where UI-SPEC expects 'Settings…'; whether that text matters is the user's call"
  - id: D3
    description: "The application menu names ScanTailor Advanced (About, Hide, Quit) through an embedded CFBundleName section without changing names, settings path or profiles path"
    requirement: MAC-05
    verification:
      - kind: other
        ref: "build/scratch/mac/m03pi/summary.txt against m03i: bundle_name differs, five identity lines byte-identical, identity_verdict PASS in both (records/menubar.md)"
        status: pass
      - kind: integration
        ref: "gate m03p-app-name: 100% tests passed out of 7 in Release and Debug, command diff only the scantailor-advanced link line"
        status: pass
    human_judgment: false
  - id: D4
    description: "On the German layout the chords Cmd-1 to Cmd-6, Cmd-plus, Cmd-minus, Backspace, Forward Delete, B and the Option-Command and Control-Command digits and plus/minus fire exactly once each"
    requirement: MAC-04
    verification:
      - kind: other
        ref: "build/scratch/mac/m03/summary.txt layout_verdict: PASS, output_tabs_group option-command, thumbnails_group option-command (records/layout.md)"
        status: pass
    human_judgment: false

duration: 35min
completed: 2026-11-14
status: complete
---

# Phase 5 Plan 2: Live Mac check harness Summary

**mac_check, a live Cocoa harness with an Objective-C++ probe (UCKeyTranslate key codes, NSEvent synthesis, NSMenu dump), proves the native application menu (About, Settings with Cmd-comma, Quit once each, no Help menu, names embedded from an Info.plist section) and shows that Option-Command digits and plus/minus fire on the German layout**

## Performance

- **Duration:** 35 min (measured, from the start-time file)
- **Started:** 2026-11-14T22:32:42Z
- **Completed:** 2026-11-14T23:10Z
- **Tasks:** 2
- **Files modified:** 10 (9 under `src/` and the records, plus the gate log; counts the new files)

## Accomplishments

- `mac_check` (`src/app/mac_check/`) links `app_gui`, `dewarping`, AppKit and Carbon, runs on Cocoa only (exit 3 otherwise), refuses an output directory outside `build/` (exit 2), keeps QSettings and TMPDIR below its output directory, and has three sections: `menubar`, `layout` and `identity`. `identity` runs alone and refuses (exit 2, before the application object exists) unless `HOME` and `CFFIXED_USER_HOME` name `<output-dir>/home`, so it never reads the user's settings file. `CocoaProbe.mm` is the only Objective-C++ file in the repository and is compiled into `mac_check` only.
- Live menu bar: the application menu holds About, Settings with `⌘,` and Quit with `⌘q` once each; none repeats in File or Tools; `QKeySequence::keyBindings` of Preferences, Quit and SaveAs contain the explicit sequences of plan 05-01. The first run showed an empty Help menu, so About moved into File and Help is gone (`menubar_verdict: PASS`).
- Application name: without a plist the items read "About mac_check" and "Quit mac_check". `src/app/Info.plist` (`CFBundleName` only) is embedded into `scantailor-advanced` and `mac_check`; the menu then reads "About ScanTailor Advanced", "Hide ScanTailor Advanced", "Quit ScanTailor Advanced". Two identity runs with a redirected home show `bundle_name` none and `ScanTailor Advanced`, and byte-identical application name, organization name, organization domain, settings path and profiles path. Window device pixel ratio stays 2.
- German layout: Cmd-1 to Cmd-6, Cmd-plus, Cmd-minus, Backspace, Forward Delete and B fire exactly once with no cross fires; Option-Command 1 to 5 and plus/minus fire too, as do the Control-Command chords. `output_tabs_group` and `thumbnails_group` are `option-command`: plan 05-03 binds Option-Command.
- Four gated commits (harness, keypad fix, Help fix, plist), each Release and Debug 100% of 7, goldens unchanged; the gate log has four new rows.

## Task Commits

1. **Task 1: the mac_check harness with its Objective-C++ probe** - `d5844830` (test); gate record `0e28257c`
2. **Task 2: run, record and apply the follow-ups**
   - keypad and activation fix found by the first live run - `239d3152` (fix); gate record `995aece7`
   - empty Help menu: About moved into File - `7db348da` (fix); gate record `72d57619`
   - application name embedded - `6c620b2b` (build); gate record `8d513d3d`
   - records `menubar.md` and `layout.md` - `1a403f40` (docs)

**Plan metadata:** the SUMMARY commit follows this file (docs: complete plan).

## Files Created/Modified

- `src/app/mac_check/main.cpp` - harness: path guard, settings isolation, sections menubar, layout, identity, summary.txt
- `src/app/mac_check/CocoaProbe.h`, `CocoaProbe.mm` - Objective-C++ helper API in namespace `cocoaprobe`
- `src/app/CMakeLists.txt` - `mac_check` target (OBJCXX), plist link option on `scantailor-advanced` and `mac_check`
- `src/app/Info.plist` - `CFBundleName` only
- `src/app/MainWindow.ui` - `menuHelp` removed, About in File before Settings
- `src/qt_tests/tst_mac_qt.cpp` - File menu expectation includes About
- `records/menubar.md`, `records/layout.md`, `records/gate-log.md`

## Decisions Made

- About lives in File and Help is gone: the live bar showed an empty Help menu (UI-SPEC E5, Pitfall 3).
- Embed `Info.plist`: About and Quit named the bare process; the identity runs showed no change to names or paths (UI-SPEC E15).
- Option-Command for the Output tabs and the thumbnail keys: every chord fired once on the German layout, so no Control-Command fallback is needed (D-02, D-07). The Accessibility Zoom caveat for Option-Command plus and minus is in `layout.md`.
- The Settings item text stays Qt's "Preferences..." (see Issues Encountered).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] keyCodeFor answered for `=` with the keypad key**
- **Found during:** Task 2 (first live run)
- **Issue:** the scan over key codes 0 to 127 reached the keypad `=` (code 81) before the real key (Shift-0 on German, code 29).
- **Fix:** the scan skips the keypad block. In the same commit `activate` also calls the cooperative `[NSApp activate]` on macOS 14 and later, and the summary records `frontmost_app` (it named `loginwindow` while the screen was locked).
- **Files modified:** `src/app/mac_check/CocoaProbe.mm`, `main.cpp`, `CocoaProbe.h`
- **Verification:** `keycode_equal: 29`, `keycode_equal_shift: yes`; gate `m03k-keypad` PASS
- **Committed in:** `239d3152`

**2. [Rule 1 - Bug] the harness read a stale disabled flag of Qt's merged About and Settings items**
- **Found during:** Task 2 (first live run, `settings_item_enabled: no`)
- **Issue:** Qt's merged application-menu items keep their initial disabled flag until AppKit validates the menu. An experiment showed `performKeyEquivalent:` with Cmd-comma and `performActionForItemAtIndex:` both triggered the Settings action, so the app was fine and only the reading was wrong.
- **Fix:** `mainMenu()` calls `-[NSMenu update]` on every top-level submenu before reading; the summary also records the enabled state of the three role items and of the three QActions.
- **Files modified:** `src/app/mac_check/CocoaProbe.mm`, `main.cpp`
- **Verification:** `settings_item_enabled: yes` in the Help re-run; gate `m03h-help-menu` PASS
- **Committed in:** `7db348da` (together with the Help fix)

**3. [Rule 3 - Blocking] CocoaProbe API gained two functions**
- **Found during:** Task 1
- **Issue:** `main.cpp` is plain C++ and needs the key codes of Backspace and Forward Delete without including a Carbon header; the plan's key `frontmost_app` needs the frontmost application's name.
- **Fix:** added `SpecialKey`, `specialKeyCode` and `frontmostAppName` to `CocoaProbe.h`. The planned functions are unchanged.
- **Committed in:** `d5844830`, `239d3152`

---

**Total deviations:** 3 auto-fixed (2 bugs in the harness, 1 blocking API gap)
**Impact on plan:** none on scope; the two bug fixes needed their own gated commit (`m03k-keypad`) or were folded into the plan's conditional Help commit.

## Issues Encountered

- The first harness start found the screen locked and the display asleep (`frontmost_app: loginwindow`); it measured nothing and was repeated once the session was unlocked, with no checkpoint needed. The first complete run produced `menubar_verdict: FAIL` because of the empty Help menu and the stale flag; both are handled above.
- **Open point for the orchestrator or user:** the native Settings item reads "Preferences..." (Qt's own merged text with three ASCII dots), while UI-SPEC section 1 says "Settings…". The key `⌘,`, the single item and its function are right. A later plan can override the text with a `QTranslator` for the context `QMenuBar`.
- `enable_language(OBJCXX)` worked inside `src/app/CMakeLists.txt` (`if (BUILD_TESTS)`); the top-level fallback was not needed.
- The plan's 05-01 note about `QObject::disconnect` warnings at MainWindow teardown applies to `mac_check` too; nothing fails on them.

## User Setup Required

None - no external service configuration required.

## Known Stubs

None.

## Threat Flags

None. T-05-04 (path guard, settings isolation, identity home guard; verify commands ran), T-05-05 (no `CGEventPost` or event tap in `CocoaProbe.mm`; events only through `[NSApp sendEvent:]`), T-05-06 (plist holds `CFBundleName` only; identity runs identical) and T-05-07 (records hold no home path; `grep -c "$HOME"` prints 0) are in place.

## Next Phase Readiness

- Plan 05-03 binds Option-Command 1 to 5 (Output tabs) and Option-Command plus and minus (thumbnail keys); Cmd-1 to Cmd-6, Cmd-plus and Cmd-minus are confirmed on the German layout. Cmd-equal is a US-layout binding (D-06).
- Plans 05-05, 05-06 and 05-10 add sections to `mac_check` and functions to the `cocoaprobe` namespace; the harness needs an unlocked session and an awake display.
- Option-Command plus and minus are taken by macOS before the app if "Use keyboard shortcuts to zoom" is on in Accessibility Zoom; the harness cannot see that setting (`layout.md`).

## Self-Check: PASSED

- Files present: `src/app/mac_check/main.cpp`, `CocoaProbe.h`, `CocoaProbe.mm`, `src/app/Info.plist`, `records/menubar.md`, `records/layout.md`, `records/gate-log.md`.
- Commits `d5844830`, `239d3152`, `7db348da`, `6c620b2b`, `0e28257c`, `995aece7`, `72d57619`, `8d513d3d`, `1a403f40` are ancestors of HEAD.
- Gates `m03-mac-check`, `m03k-keypad`, `m03h-help-menu`, `m03p-app-name` all record `result: PASS` and `goldens: unchanged`; the output-path guard of convention 14 prints nothing; `git ls-files '*.mm'` lists only `src/app/mac_check/CocoaProbe.mm`.

---
*Phase: 05-mac-menus-and-shortcuts*
*Completed: 2026-11-14*
