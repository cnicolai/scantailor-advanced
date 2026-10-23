# Smoke run findings (plan 01-06, BUILD-03, D-05)

BUILD-03 verdict: FAIL

Reasons (the plan sets PASS only when every project-file expectation is met; two are not):

1. The saved project file holds no dewarping mode `auto`. The only dewarping `mode` value in it is `off` (expected `auto` among them). See `## Project file`.
2. The saved project file holds 0 `zone` elements under the output filter's `zones` (expected at least 1). See `## Project file`.

Everything else the plan asks for passed: steps 1 to 11 were reported as successful, the app quit on Command+Q, the four output TIFFs open in Preview, and the depth lines are `ok`. The two failed expectations are unresolved rather than understood. The user's reports do not say that a dewarping `Auto` setting or a picture zone was actually applied (see `## What is open`), so this may be a gap in the walkthrough and not in the app. Deciding which is not a step Claude takes on its own; the decision is handed to the orchestrator and the user.

## Environment

- Date: 2026-10-23
- Commit at build time: `015b7a4`
- Preset: `macos-release` (`cmake --preset macos-release --fresh`, then `cmake --build --preset macos-release`; both exit 0)
- Binary: `{repo}/build/macos-release/scantailor-advanced`
- Pages directory: `{scratch}/smoke/pages/` (regenerated in this session with the commands in `records/smoke-pages.md` `## Regenerate`; the earlier session's copy was gone)
- Output directory: `{scratch}/smoke/out/` (created empty)
- SHA-256 of the regenerated pages against the table in `records/smoke-pages.md`:

| Page | SHA-256 | Match |
|------|---------|-------|
| `01-flat-text.tif` | `2d47f7364b7c087e202076096848a7f4bf309aae002b8052fd8f46cabfea2c8f` | match |
| `02-book-spread.tif` | `de3b20d497c2802f716fd1c1cd5ee430ab2855ea7aeee2ed944f170f05f2a416` | match |
| `03-mixed.tif` | `d7876136fd3c39e212bd932702765a97f64a649b9bd737013f2bba07dc8c5d7b` | match |

- Prior settings state (D-02 outcome, `records/prior-state.md`): `list-corrected`. The listed locations (`~/.config/scantailor-advanced/`, `~/Library/Application Support/scantailor-advanced/`, `~/Library/Preferences` entries containing "scantailor") did not exist: no ScanTailor entries found. The walkthrough therefore starts from a clean settings state.
- Walkthrough presented by the orchestrator (Task 2): steps 1 to 11, one step per message. Step 12 was not presented because step 11 quit the app.

### Deviation: project file saved to the Desktop

At step 8 the user saved the project to their Desktop instead of the planned `{scratch}/smoke/smoke.ScanTailor`, and reopened it from there. The planned scratchpad file does not exist. Asked how the project-file check should proceed, the user chose "Read the Desktop file" and named the file `~/Desktop/test.ScanTailor`. That one file was read, and only by `python3 -I` with the standard-library XML parser (`xml.etree.ElementTree`); nothing else on the Desktop or in the home folder was listed, searched or opened. This record holds only what the user allowed: whether it parses, the distinct `colorMode` values, the dewarping `mode` values, the zone count, and the per-page mode mapping by page and sub-page id. The file was not copied, hashed, moved or committed.

## Walkthrough

| Step | What the user did | What to look for | User report | Result |
|------|-------------------|------------------|-------------|--------|
| 1 | Opened Terminal, started `./build/macos-release/scantailor-advanced` with `QT_LOGGING_RULES="qt.highdpi.*=true"` and stderr redirected to the scratchpad stderr file | Main window opens; sharpness (Retina); menu language | it opened, looks sharp, and is in English | ok (no `qt.highdpi` line reached the stderr file, see `## Device pixel ratio`) |
| 2 | File, New Project: pages directory, three files, output directory in the scratchpad | Three thumbnails 01, 02, 03 in order; any Fix DPI dialog | 01, 02, 03 in order, no dialog | ok |
| 3 | Batch run of all six stages on defaults, "Beep when finished" ticked | No red error panel; sound (silence expected) | finished, no error panels, no sound (sound not important!) | ok |
| 4 | Selected a half of page 02, mode Black and White, Dewarping Change..., Auto, this page only | Output re-renders; rows straighter | split into two halves, rows look straighter | reported ok; not confirmed by the saved file, which holds dewarping mode `off` for both halves (see `## Project file`) |
| 5 | Page 01, mode Color / Grayscale | Greyscale; rows horizontal | greyscale, rows horizontal | ok (saved file: `colorOrGray`; output depth 8) |
| 6 | Page 03, mode Mixed, Picture Zones tab, zone around the colour block | Colour block in colour; text black and white | colour block in colour, text black and white | reported ok; the saved file holds 0 zone elements (see `## Project file`) |
| 7 | `open -a Preview` on the output `.tif` files | Every file opens; count equals output pages in the app | 4 files opened, all fine | ok (4 files = 4 output pages) |
| 8 | Save Project As, Close Project, Open Project | Same pages, order, modes, dewarping, zone | I saved to the desktop, and loaded from there. It seems like it came back as I have left it | ok as reported; deviation: saved to `~/Desktop/test.ScanTailor`, not the scratchpad path |
| 9 | Opened the File menu | Shortcut spelling; whether Quit is listed | ⌘ symbols, Quit not in File menu | logged, not fixed (D-06) |
| 10 | Looked for "Settings ..." | Which menu holds it, its label | app menu, abeled preferences | logged, not fixed (D-06) |
| 11 | Command+Q with the main window in front | App quits, Terminal prompt returns | the app quit | ok |
| 12 | not presented | - | - | not applicable: step 11 quit the app |

## Output files

`magick identify -format '%f %wx%h %x %y %U %[colorspace] depth=%z %[compression]\n'` on every `.tif` in `{scratch}/smoke/out/`:

```
01-flat-text.tif 4566x7095 600 600 PixelsPerInch Gray depth=8 LZW
02-book-spread_1L.tif 4566x7095 600 600 PixelsPerInch Gray depth=1 Group4
02-book-spread_2R.tif 4566x7095 600 600 PixelsPerInch Gray depth=1 Group4
03-mixed.tif 4566x7095 600 600 PixelsPerInch sRGB depth=8 LZW
```

`identify` also printed one warning, for `03-mixed.tif`: `Sum of Photometric type-related color channels and ExtraSamples doesn't match SamplesPerPixel. Defining non-color channels as ExtraSamples.` The same file reports four channels (`srgb`, three colour plus one extra). The three input pages carry an alpha channel (`graya`, `graya`, `srgba`), and the app's stderr holds the same libtiff warning four times (see `## stderr summary`). Whether the extra sample in the Mixed output comes from the input alpha channel was not investigated; it is logged for plan 01-07 and later phases, not fixed.

Expected depth per output page, from the modes in the saved project file (page mapping by page and sub-page id is under `## Project file`):

| Output file | Mode | Expected depth | Measured depth | Check |
|-------------|------|----------------|----------------|-------|
| `01-flat-text.tif` | Color / Grayscale | 8 | 8 | ok |
| `02-book-spread_1L.tif` | Black and White | 1 | 1 | ok |
| `02-book-spread_2R.tif` | Black and White | 1 | 1 | ok |
| `03-mixed.tif` | Mixed | 8 | 8 | ok |

All four files have the same size (4566x7095 at 600 DPI), including the two halves of the spread. The output folder also holds a `cache` folder the app created; it was ignored.

## Project file

Parsed with `python3 -I` and the standard XML parser. File: `~/Desktop/test.ScanTailor` (the user's named file, see the deviation above).

- Parses: yes (root element `project`).
- Pages in the project: 4.
- Distinct `colorMode` values: `bw`, `colorOrGray`, `mixed` (expected: all three; met).
- Distinct dewarping `mode` values: `off` (expected: `auto` among them; NOT met).
- `zone` elements under the output filter's `zones`: 0 (expected: at least 1; NOT met).

Per-page mapping (page id and sub-page; the image ids 3, 6, 10 follow the creation order of the three input pages, so image 3 is page 01, image 6 is page 02 and image 10 is page 03):

| Page id | Image id | Sub-page | colorMode | Dewarping mode | Picture zones |
|---------|----------|----------|-----------|----------------|---------------|
| 4 | 3 | single | `colorOrGray` | `off` | 0 |
| 7 | 6 | left | `bw` | `off` | 0 |
| 8 | 6 | right | `bw` | `off` | 0 |
| 11 | 10 | single | `mixed` | `off` | 0 |

The three colour modes match the output depths above and the user's steps 5 and 6. Edge probe BUILD-03 adjacency (the saved file carries the settings of the walkthrough): met for the colour modes, not met for dewarping and the picture zone.

## What is open

The saved file does not carry the dewarping `Auto` setting or a picture zone, although the user reported the reopened project as "as I have left it". Three explanations fit the evidence, and none is excluded:

1. The Auto dewarping and the picture zone were not applied, or not on the page the later steps checked. The user's verbatim reports for steps 4 and 6 do not say they were: step 4 says "split into two halves, rows look straighter" (the split alone, or deskew, would explain it) and step 6 says "colour block in colour, text black and white" (Mixed mode can detect a picture block on its own, so a zone is not needed for that result).2. `~/Desktop/test.ScanTailor` is not the file saved in step 8 of this run (for example an earlier file with that name that was not overwritten). Its three colour modes and its page structure match this run's pages, but that would also hold for an earlier attempt.
3. The app saved the project without these two settings. Reading the code finds no reason: `Settings::setDewarpingOptions` stores the mode in the page's `Params`, `Filter::writePageSettings` writes `params` (with `dewarping-options`) and `zones` for every page, and `Filter::loadSettings` reads them back. This was not tested in the running app.

How to settle it is the user's choice; the least effort is to repeat steps 4, 6 and 8 on the scratchpad path, checking after step 4 that the Dewarping box reads `Auto` and after step 6 that the Picture Zones tab shows the zone, then re-run the project-file check. Until then the verdict stays FAIL.

## Device pixel ratio

- Probe (`records/probe-output.txt`, plain executable, no bundle, Homebrew Qt 6.12.0, Cocoa): `screen[0] name: Studio Display`, `screen[0] devicePixelRatio: 2`, `screen[0] logicalDotsPerInch: 72`, `screen[0] physicalDotsPerInch: 109.75`, `window devicePixelRatio: 2`.
- `qt.highdpi` lines in `records/smoke-stderr.txt`: 0. The user was asked to start the app with `QT_LOGGING_RULES="qt.highdpi.*=true"`; no line of that category reached the stderr file. Whether the variable was not set in that Terminal or Qt 6.12 prints no such line for this configuration was not determined. The cross-check of the probe's ratio against the app's own `qt.highdpi` output therefore could not be made.
- User report, step 1: "it opened, looks sharp, and is in English".
- Phase 4 consequence: the measured ratio on the Studio Display is 2.0, and the app (a plain executable, as the probe was) looks sharp. By the plan's rule (ratio 1.0 would mean HIDPI-01 needs the embedded Info.plist; 2.0 means it does not), HIDPI-01 does not need the embedded Info.plist on this evidence. This is provisional: it rests on the probe and the user's judgement of sharpness, without the app's own `qt.highdpi` confirmation.

## Settings paths

- Derived INI path (probe, from `QSettings::IniFormat` at `src/app/main.cpp:47`): `~/.config/scantailor-advanced/scantailor-advanced.ini` (`XDG_CONFIG_HOME` unset).
- Derived AppData path (probe): `~/Library/Application Support/scantailor-advanced/scantailor-advanced`; the app adds `/profiles` (`src/core/DefaultParamsProfileManager.cpp:24`).
- D-02 outcome: `list-corrected`, no prior ScanTailor entries existed before the first launch (`records/prior-state.md`).
- Not checked after the run: whether the app created these locations. That needs a listing outside the project, which was not approved for this plan.

## Logged for later phases (not fixed)

Nothing below was fixed (D-06).

- **Retina sharpness (Phase 4):** reported sharp; ratio 2.0 (see `## Device pixel ratio`).
- **Shortcut spelling (Phase 5):** shown with Command symbols ("⌘ symbols"), so Qt maps the Ctrl-based shortcuts to Command on its own.
- **Menu layout (Phase 5):** "Quit" is not in the File menu (user report: "Quit not in File menu"); Qt moved it to the app menu. "Settings" sits in the app menu, labelled "preferences" (user's wording: "app menu, abeled preferences"), not in the Tools menu where `src/app/MainWindow.ui` (the `actionSettings` entry, lines 82 and 1208) defines it.
- **Command+Q (Phase 5):** the app quit ("the app quit"), so step 12 was not needed.
- **Batch-finished alarm (Phase 5, MAC-10):** `src/app/MainWindow.cpp:1337` runs the value of `main_window/external_alarm_cmd` through `std::system`; on Unix (including macOS) the default is a Linux `play ... bell.oga` command. The user ticked "Beep when finished" and heard nothing ("no sound (sound not important!)"). `records/smoke-stderr.txt` holds `sh: play: command not found` twice, so the command ran twice during the walkthrough (which steps triggered them was not recorded). Phase 5 replaces it with the system beep.
- **UI language (Phase 3, STRIP-06):** English ("is in English").
- **Mixed output has an extra sample (for plan 01-07 and later):** `03-mixed.tif` reports four channels and triggers a libtiff `ExtraSamples` warning in `magick identify` (see `## Output files`). Not fixed; no baseline exists yet to say whether this is a change from earlier behaviour.
- **Qt warnings in the log (later phases):** 30 `QObject::disconnect: wildcard call disconnects from destroyed signal` lines, grouped in the six filter option widgets (`output`, `page_layout`, `select_content`, `deskew`, `page_split`, `fix_orientation`), in five blocks of six lines, which is consistent with project close and re-open. Also `qt.qpa.fonts: ... missing font family "MS Shell Dlg 2"` once. Not fixed.
- No other issue was reported by the user.

## Launch fixes

None. The walkthrough passed every step as reported, no launch failure occurred, and no `src/` file was edited in this plan. `records/fix-log.md` has no row from this plan.

## stderr summary

Source: `records/smoke-stderr.txt` (37 lines; it holds no home-folder path and no scratchpad path, checked by grep).

| Pattern | Count |
|---------|-------|
| Lines starting with `qt.` (all `qt.qpa.fonts`) | 1 |
| `qt.highdpi` lines | 0 |
| `QObject::` warnings (`disconnect: wildcard call ...`) | 30 |
| `sh: play: command not found` | 2 |
| `TIFFReadDirectory: Warning, Sum of Photometric type-related color channels and ExtraSamples ...` | 4 |
| Crash, `ASSERT` or `Segmentation` lines | 0 |

Crash reports under `~/Library/Logs/DiagnosticReports` were not read (no yes from the user, and the app did not crash).

## Checks that no smoke file is in git

No image file and no `.ScanTailor` file was staged or committed in this plan; the smoke outputs stay in `{scratch}/smoke/out/` and the project file stays on the Desktop.
