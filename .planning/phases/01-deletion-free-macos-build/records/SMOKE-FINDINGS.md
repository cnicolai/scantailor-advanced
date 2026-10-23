# Smoke run findings (plan 01-06, BUILD-03, D-05)

BUILD-03 verdict: PASS (after re-check of steps 4, 6, 8; first run FAIL, see below)

History of the verdict. The first walkthrough ended with `BUILD-03 verdict: FAIL`; the user then re-checked steps 4, 6 and 8 with the GUI state confirmed at each step (section "Re-check of steps 4, 6 and 8" at the end of this file), and the PASS rule is met on the evidence after the re-check. The first run's FAIL, its reasons and its analysis stay in this record unchanged.

First run, verdict FAIL. Reasons (the plan sets PASS only when every project-file expectation is met; two were not):

1. The saved project file held no dewarping mode `auto`. The only dewarping `mode` value in it was `off` (expected `auto` among them). See `## Project file`.
2. The saved project file held 0 `zone` elements under the output filter's `zones` (expected at least 1). See `## Project file`.

Everything else the plan asks for passed in the first run: steps 1 to 11 were reported as successful, the app quit on Command+Q, the four output TIFFs open in Preview, and the depth lines are `ok`. The two failed expectations were unresolved rather than understood: the user's reports did not say that a dewarping `Auto` setting or a picture zone was actually applied (see `## What is open`). Deciding which was not a step Claude took on its own; the decision went to the user, who chose to re-check steps 4, 6 and 8.

Re-check, in short: steps 4, 6 and 8 repeated with confirmation (R2 to R5); the re-saved project file now parses, holds dewarping `auto` on the left half of page 02 and one picture zone on page 03; the outputs re-rendered during the re-check pass the depth checks; the app quit on Command+Q. No app defect was found in this path.

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

First-run result (the re-run is under "Re-check of steps 4, 6 and 8").

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

First-run result (the re-run after the re-check is under "Re-check of steps 4, 6 and 8").

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

Status after the re-check (section "Re-check of steps 4, 6 and 8" at the end of this file). The three explanations from the first run are kept, each with its outcome:

1. **Resolved, this is what happened.** The Auto dewarping and the picture zone were not applied in the first walkthrough. Evidence: the first-run file also held `off` in `output-params` (the parameters the last render used) and no zone existed; in the re-check, with the GUI state confirmed (R2: the dewarping status word reads Auto; R3: the zone outline is still there after leaving and returning to the Picture Zones tab), the app applied, saved and reloaded both. The user's first-run reports for steps 4 and 6 ("split into two halves, rows look straighter", "colour block in colour, text black and white") are explained by the page split and by Mixed mode's own picture detection, as suspected.
2. **Not needed to explain the result.** Whether `~/Desktop/test.ScanTailor` was the file saved in the first run was not determined; the re-saved file's modification time (15:13:17 local) shows it was written during the re-check, so the re-check result does not depend on the answer.
3. **Excluded on this evidence.** The save and load path wrote and read back `auto` and the zone correctly (R5 reopened the project and the user confirmed both; the file holds both). No app defect was found.

Remaining open items, none of which blocks the verdict:

- The app's own `qt.highdpi` confirmation of the pixel ratio is still missing (first run: no line reached stderr; re-check: no stderr capture at all), so the HIDPI-01 conclusion stays provisional (see `## Device pixel ratio`).
- The two TIFFs re-rendered during the re-check (`02-book-spread_1L.tif`, `03-mixed.tif`) were checked with `magick identify` only; the user opened the first run's four files in Preview (step 7) and did not repeat that for the re-rendered ones.

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

## Re-check of steps 4, 6 and 8

### Reason

The first run ended in FAIL because the saved project held no dewarping `auto` and no picture zone, while the user had reported steps 4, 6 and 8 as successful. The cause was open (settings not applied, wrong file, or a save defect). The user chose "Re-check steps 4, 6, 8". The orchestrator presented five re-check steps, one per message, each asking the user to confirm the GUI state before moving on, so that the project-file check could be repeated on a file whose content was known to match the GUI.

### Steps and user reports

| Step | What the user did | What to look for | User report | Result |
|------|-------------------|------------------|-------------|--------|
| R1 | Relaunched the app; File, Open Project, `~/Desktop/test.ScanTailor` | Four pages | open | ok |
| R2 | Output stage, page 02 left half: read the dewarping status word above "Change ...", then Change ..., "Auto (experimental)", "This page only", OK | The word reads Auto; the page re-renders | first: "what do you mean with sttus line"; after the orchestrator explained it is the word above the "Change ..." button: "pass" | ok (the saved file now holds `auto` for this page, see below) |
| R3 | Page 03 (Mixed), Picture Zones tab: drew the zone, switched to Output and back | The zone outline is still there | pass | ok (the saved file holds one zone element on this page) |
| R4 | Save Project As to the scratchpad project path | Project saved | "No I can't paste that. You know, when I saved on the Desktop earlier, then there was a reason for that" | not possible, see the dialog constraint below |
| R4 revised | File, Save Project, back to `~/Desktop/test.ScanTailor` | Project saved | done | ok (file modification time 2026-10-23 15:13:17 local, written during the re-check) |
| R5 | File, Close Project; Open Project, `~/Desktop/test.ScanTailor`: 02-left status word Auto? 03 zone outline still there? Then Command+Q | Both settings survive the reopen; the app quits | pass | ok |

### Save dialog constraint (R4)

The macOS save dialog cannot take the scratchpad path: the user could not paste it, and says this is why the project was saved on the Desktop at step 8 of the first run. The Desktop save is therefore a practical constraint of the file dialog and the scratchpad location (a long session path under `/private/tmp`), not a user error. The plan's wording ("save to the scratchpad project path") cannot be followed through the GUI as written; a later walkthrough should name a project path the dialog can reach, or accept a user-chosen path and read it under the same restrictions. The project file stayed at the user's named Desktop path for the check.

### Project file, re-run

`python3 -I` with `xml.etree.ElementTree` on `~/Desktop/test.ScanTailor` (the one file the user allowed; only the parse result, values, counts and per-page mapping are recorded; the file was not copied, hashed, moved or committed):

- File modification time: 2026-10-23 15:13:17 local (the re-check save).
- Parses: yes (root element `project`).
- Pages in the project: 4.
- Distinct `colorMode` values: `bw`, `colorOrGray`, `mixed` (expected: all three; met).
- Distinct dewarping `mode` values in the page parameters: `auto`, `off` (expected: `auto` among them; met).
- Distinct dewarping `mode` values in `output-params`: `auto`, `off` (the parameters the last render used; the first-run file also held `off` there, per the orchestrator's check at the time).
- `zone` elements under the output filter's `zones`: 1 (expected: at least 1; met).

| Page id | Image id | Sub-page | colorMode | Dewarping mode (params) | Dewarping mode (output-params) | Picture zones |
|---------|----------|----------|-----------|-------------------------|--------------------------------|---------------|
| 4 | 3 | single (page 01) | `colorOrGray` | `off` | `off` | 0 |
| 7 | 6 | left (page 02) | `bw` | `auto` | `auto` | 0 |
| 8 | 6 | right (page 02) | `bw` | `off` | `off` | 0 |
| 11 | 10 | single (page 03) | `mixed` | `off` | `off` | 1 |

Compared with the first run: the left half of page 02 changed from `off` to `auto` (params and output-params), and page 03 gained its zone. Every other value is unchanged. Edge probe BUILD-03 adjacency (the saved file carries the settings of the walkthrough): met.

### Output files, re-run

`magick identify -format '%f %wx%h %x %y %U %[colorspace] depth=%z %[compression]\n'` on the four `.tif` files in `{scratch}/smoke/out/`:

```
01-flat-text.tif 4566x7095 600 600 PixelsPerInch Gray depth=8 LZW
02-book-spread_1L.tif 4566x7095 600 600 PixelsPerInch Gray depth=1 Group4
02-book-spread_2R.tif 4566x7095 600 600 PixelsPerInch Gray depth=1 Group4
03-mixed.tif 4566x7095 600 600 PixelsPerInch sRGB depth=8 LZW
```

`identify` printed the same libtiff `ExtraSamples` warning as before (for `03-mixed.tif`). File times: `02-book-spread_1L.tif` rewritten 15:10 and `03-mixed.tif` rewritten 15:11 (the re-renders during R2 and R3); `01-flat-text.tif` and `02-book-spread_2R.tif` unchanged from 14:53.

| Output file | Mode | Expected depth | Measured depth | Check |
|-------------|------|----------------|----------------|-------|
| `01-flat-text.tif` | Color / Grayscale | 8 | 8 | ok |
| `02-book-spread_1L.tif` | Black and White | 1 | 1 | ok |
| `02-book-spread_2R.tif` | Black and White | 1 | 1 | ok |
| `03-mixed.tif` | Mixed | 8 | 8 | ok |

### No stderr capture for the re-check

The re-check has no stderr file: the user apparently launched the app without the redirect, and `{scratch}/smoke/smoke-stderr-recheck.txt` does not exist. `records/smoke-stderr.txt` therefore covers the first run only and stands as recorded. Nothing in the re-check was reported as an error or crash.

### Conclusion

In the first walkthrough the Auto dewarping and the picture zone never took effect: the first file's `output-params` (the parameters the last render used) also said `off`, and no zone existed. In the re-check, with the GUI state confirmed by the user at each step (R2 status word, R3 zone outline, R5 after reopening), the app applied, saved and reloaded both correctly, and the re-saved file holds `auto` on the left half of page 02 and one zone on page 03. No app defect was found in this path; no `src/` file was touched and no launch fix was needed. Why the settings did not take effect in the first run is not established beyond that: the user's first-run reports could not tell the page split and Mixed mode's own picture detection apart from the two settings, and the user did not know the dewarping status word until it was explained in R2.

Verdict rule applied to the evidence after the re-check: steps 1 to 10 succeeded, with 4, 6 and 8 confirmed by R2 to R5; the app quit on Command+Q at step 11 and again at R5; every first-run TIFF opened in Preview (step 7); the depth lines are `ok` for all four files; the project file meets every expectation (`bw`, `colorOrGray`, `mixed`, `auto`, one zone). All conditions hold, so the verdict line at the top reads `BUILD-03 verdict: PASS (after re-check of steps 4, 6, 8; first run FAIL, see below)`.

Caveat: the two TIFFs re-rendered in the re-check were checked with `magick identify`, not re-opened in Preview.
