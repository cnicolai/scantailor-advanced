# Phase 3 UI checks after removing translations (D-12)

Measured on the c10 tree (`56b5a540`, `refactor(03-06): remove translations`), Release build `build/macos-release`, rebuilt by the c10 gate (`ninja: no work to do` before the snapshot run). The repository path is written `{repo}`. The before values come from `records/before-state.md` and `records/screenshots/before/`.

## Commands

| Command | Exit | Relevant output |
|---------|------|-----------------|
| `build/macos-release/ui_snapshot build/scratch/ui-after --assert-no-language > build/scratch/ui-after.txt 2>&1` | 0 | `settings file: {repo}/build/scratch/ui-after/settings/scantailor-tests/ui_snapshot.ini`, `wrote .../main-window.png`, `wrote .../settings-dialog.png`, `wrote .../texts.txt`, `language control present: no` |
| `grep -c languageBox build/macos-release/src/app/scantailor-advanced_autogen/include/ui_SettingsDialog.h` | 1 (no match) | `0` |
| `nm -u build/macos-release/scantailor-advanced \| grep -ci translator` | 1 (no match) | `0` |
| `strings build/macos-release/scantailor-advanced \| grep -c 'scantailor_\*\.qm'` | 1 (no match) | `0` |
| `grep -ci -e '\.qm' -e lrelease -e lupdate -e translations build/macos-release/build.ninja` | 1 (no match) | `0` |
| `diff records/screenshots/before/texts.txt records/screenshots/after/texts.txt` | 1 (files differ) | two deleted lines, see "Texts diff" |

The snapshot run prints the same harmless Qt messages on stderr as the before run (no OpenGL context offscreen, `QObject::disconnect` wildcard notes at teardown). The harness kept its settings file below `build/scratch/ui-after/`; no user settings were read or written (D-07).

Gate for c10: `result: PASS`, `100% tests passed out of 7` in Release and Debug, goldens unchanged, build-command diff 0 lines in both presets (row `c10-translations` in `records/gate-log.md`).

## Static checks

| Check | Before | After |
|-------|--------|-------|
| `languageBox` in the generated dialog header | 4 | 0 |
| Qt translator symbols in the binary (`nm -u`) | 5 | 0 |
| `scantailor_*.qm` pattern string in the binary | 1 | 0 |
| translation references in `build.ninja` | 40 | 0 |
| `tr()` and `QCoreApplication::translate()` calls in `src/*.cpp`, `src/*.h` (D-06) | 249 | 249 |
| `<string>` entries in `src/*.ui` (D-06) | 574 | 573 |
| `language control present` in the snapshot run | yes | no |

Note: `find build/macos-release -name '*.qm'` still lists 7 stale files left by earlier builds in this existing build directory; the c10 build produces none (no `.qm` rule is left in `build.ninja`, count above). The check on a fresh directory is part of plan 03-07 (fresh clone).

## Texts diff

```
5d4
< label: Language: 
31d29
< combo languageBox: English
```

No line starts with `>`; both removed lines belong to the Language row.

## Visual check

I looked at all four PNGs with the Read tool.

Main window, before and after: 1280x800, dark scheme, menu bar File / Tools / Help, an empty dark central area with the New/Open panel ("New Project ..." and "Open Project ...") in its middle, and a status bar with a size grip. The two images are visually identical; the main window is untouched by c10.

Settings dialog, before: tabs General and Processing; group "User Interface" with the greyed-out OpenGL checkbox and its note, "Auto-save the existing project", "Color Scheme:" with Dark, and below it "Language:" with a greyed-out combo showing English; groups "Thumbnails" and "Saving" below, buttons OK and Cancel.

Settings dialog, after: the same dialog without the Language row. The "Color Scheme:" row is followed directly by the bottom edge of the "User Interface" group box; the Thumbnails group and everything below it moved up by about 27 px.

- (a) The Color Scheme row is followed directly by the end of its group box, with no empty row, gap or broken layout where the Language row was: yes.
- (b) Every visible text in both windows is English: yes (File, Tools, Help, New Project ..., Open Project ..., the dialog labels, checkboxes and combo entries).
- (c) Apart from the removed row, the before and after images show the same widgets in the same places (only shifted up by the height of the removed row): yes. Group titles, the Quality and Size spin boxes, both checkboxes, the two compression combos and the OK and Cancel buttons are all present with unchanged values.

Verdict: PASS
