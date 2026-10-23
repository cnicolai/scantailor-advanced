# Project round trip: special-character names and non-default parameters in every stage (plan 02-08)

Measured with Qt 6.12.0 (Homebrew), presets `macos-release` and `macos-debug`, on the unmodified production code of commit `cde6825` (the only non-test path that differs is `src/CMakeLists.txt`, the two test `add_subdirectory` lines). Test: `ProjectRoundTripTestSuite` in `src/pipeline_tests/TestProjectRoundTrip.cpp`, test-code commit `a2b0653`.

Flow of every case: build a `ProjectPages` from the generated PNGs, build a `StageSequence`, install a `DefaultParams` profile with `DefaultParamsProvider::setParams`, call `loadDefaultSettings` of all six filters for every page, write with `ProjectWriter` (file 1), parse file 1 with `QDomDocument`, read it with `ProjectReader` into a second `StageSequence` (`readFilterSettings`), write again with a writer built from the reader's pages, selected page, disambiguator and output directory (file 2). Both project files live in the suite's own `QTemporaryDir` under `build/<preset>/test-tmp`, next to the input and output directories, so the paths in the files are relative. The temporary directory is removed at the end of each case (checked: `build/macos-release/test-tmp` and `build/macos-debug/test-tmp` are empty after the runs).

## Character set

Five image files in the input directory `in & <dir> "q" 'a'`; the output directory is `out & <x> "y"` (not an image directory, but written to the project file as `outputDirectory`). Each image is a generated 64 x 64 RGB32 PNG with 300 dpi in the metadata.

| # | Name | Unicode form | Characters | UTF-8 bytes |
|---|------|--------------|------------|-------------|
| 1 | `a&b <c> "d" 'e'.png` | ASCII (`&`, `<`, `>`, `"`, `'`, spaces) | 19 | 19 |
| 2 | `Seite_ä_NFC.png` | NFC, U+00E4 as one code point | 15 | 16 |
| 3 | `Seite_ä_NFD.png` | NFD, U+0061 U+0308 as two code points (the file contains the decomposed form) | 16 | 17 |
| 4 | `x.png` | ASCII, one-character base name | 5 | 5 |
| 5 | `ÜÜ…Ü.png` | NFC, 100 x U+00DC, base name 200 bytes | 104 | 204 |
| dir | `in & <dir> "q" 'a'` | ASCII | 18 | 18 |
| dir | `out & <x> "y"` | ASCII | 13 | 13 |

Names 2 and 3 differ in their suffix, because APFS treats the NFC and the NFD form of one name as the same file. In the written file the two names are different byte sequences (checked with `QByteArray::contains` on the UTF-8 forms).

## Non-default values per stage

All numbers are values the UI can enter (at most two decimals; checked per value with `QString::number(v, 'f', 2).toDouble() == v`). Defaults are those of `DefaultParams()`.

| Stage | Setting | Default | Chosen value | Where it shows in the file |
|-------|---------|---------|--------------|----------------------------|
| fix_orientation | image rotation | 0 degrees | 90 degrees clockwise | `<fix-orientation><image><rotation degrees="90"/>` (no `<image>` element at all with the default) |
| page_split | layout type | AUTO | `SINGLE_PAGE_UNCUT` | `<page-split><image layoutType="single-uncut"><params mode="auto">` (see finding F-08-1: only after the stage has run) |
| deskew | angle, mode | 0, auto | 1.25 degrees, manual (auto oblique true is not stored by the stage) | `<params angle="1.25" mode="manual">` |
| select_content | page rectangle, content detection, page detection, fine-tune corners | 210 x 297 mm, on, disabled, off | 215.9 x 279.4 mm, off, auto, on | `<params contentDetectionMode="disabled" fineTuneCorners="1" pageDetectionMode="auto">` and `<page-rect>` in pixels |
| page_layout | hard margins left / top / right / bottom (mm), alignment | 10 / 5 / 10 / 5, centre / centre | 7.5 / 8.25 / 9 / 10.5, top / left | `<hardMarginsMM bottom="10.5" left="7.5" right="9" top="8.25"/>`, `<alignment hor="left" vert="top"/>` |
| output | colour mode, DPI, despeckle level, dewarping, depth perception | black and white, 600, 1.0, off, 2.0 | mixed, 400, 2.0, auto, 2.5 | `colorMode="mixed"`, `<dpi horizontal="400" vertical="400"/>`, `despeckleLevel="2"`, `<dewarping-options mode="auto">`, `depthPerception="2.5"` |

Numbers checked for exact parse-back in file 1 (`test_numeric_values_parse_back_exactly`, every page): deskew angle 1.25, the four hard margins, depth perception 2.5, despeckle level 2, output DPI 400 horizontal and vertical, rotation 90. The file holds each as the shortest text that parses back to the same `double` (`QString::number(v, 'g', 16)`), for example `7.5`, `8.25`, `9`, `10.5`.

Not checked for exact parse-back: the select_content page rectangle. It is stored in pixels after a conversion with the integer dots-per-metre of the image (300 dpi is 11811 dpm, not 11811.02), so 215.9 x 279.4 mm is written as `2549.9949 x 3299.9934` px (the exact value would be 2550 x 3300). The deviation is 0.005 px and the value is stable over the round trip; it is not a defect of the project file.

## Results

| Case | Release | Debug |
|------|---------|-------|
| `test_round_trip_is_byte_identical` (5 pages, file 1 equals file 2, 18308 bytes) | pass | pass |
| `test_every_stage_carries_non_default_values` (all six `<filters>` children differ from the default-profile project) | pass | pass |
| `test_names_and_order_survive` (paths equal as code-unit sequences, NFC and NFD distinct, output directory unchanged, creation order) | pass | pass |
| `test_numeric_values_parse_back_exactly` | pass | pass |
| `test_single_page_round_trip` (page 1 only, file 1 equals file 2, 4220 bytes) | pass | pass |
| fixture teardown: `DefaultParamsProvider` profile name is the original again | pass | pass |

- Release: `build/macos-release/pipeline_tests --run_test=ProjectRoundTripTestSuite` printed `*** No errors detected`; `ctest --preset macos-release -R pipeline_tests` exit 0 (42 s for the whole executable).
- Debug: `build/macos-debug/pipeline_tests --run_test=ProjectRoundTripTestSuite` printed `*** No errors detected`; `ctest --preset macos-debug -R pipeline_tests --output-on-failure` exit 0 (206 s; ctest 4.x prints `100% tests passed out of 1`).
- `EnvironmentTestSuite` (expects profile `Default`) passes in the same process after this suite, also with `--random=1`.
- Mutation check: the second write was given an output directory with an `x` appended; `test_round_trip_is_byte_identical` and `test_single_page_round_trip` then failed (18308 vs 18309 bytes, 4220 vs 4221 bytes). The mutation was reverted before the commit.

## SAFE-02 disposition

The unmodified code round-trips every special-character file and directory name of this set; the SAFE-02 premise (names with &, <, " or non-ASCII characters break on save and reload) was not reproduced for paths. No expected-failure mark was added. Phase 6 decides what SAFE-02 fixes.

Evidence: file 1 and file 2 are byte-identical for the five-page project and the one-page project; the written file escapes the names (`name="a&amp;b &lt;c> &quot;d&quot; 'e'.png"`, `outputDirectory="out &amp; &lt;x> &quot;y&quot;"`); the page paths read back equal the original `QString`s code unit for code unit, NFC stays NFC and NFD stays NFD, and the two stay two pages. String fields other than paths (profile names, free text) are not covered: no stage stores user free text in the project file that the round trip can set through public API (research Open Question 1).

## Findings

### F-08-1: a layout type from the default profile is not written to the project file until page_split has run
- Check: `ProjectRoundTripTestSuite/test_every_stage_carries_non_default_values`, element `<page-split>` of the non-default project against the default one.
- Measured: after `loadDefaultSettings` alone (profile with `SINGLE_PAGE_UNCUT`) the written element is `<page-split defaultLayoutType="auto-detect"/>` with no `<image>` child, identical to the default-profile project. `page_split::Filter::writeImageSettings` appends an `<image>` element only inside `if (const Params* params = record.params())`, and `defaultLayoutType` is a `Settings`-level value that `DefaultParams` never sets. The application loads defaults at processing time (`MainWindow::startBatchProcessing` and the single-page path), so in use the params exist when the project is saved after the stage ran; a project saved before that loses the layout type chosen by the profile.
- Bound: the element differs from the default-profile project.
- Fixture: five pages, 64 x 64 px, 300 dpi, layout `SINGLE_PAGE_UNCUT` in the profile.
- Mark: none. The test runs the page_split stage for every page (real filters, fix_orientation task first, `SINGLE_PAGE_UNCUT` needs no detection), after which the element carries `layoutType="single-uncut"` and split parameters and the check passes. The behaviour is by design for a stage whose output is the parameters, so no expected failure was added; a project that is saved before page_split has run simply carries no layout type for the pages.
- Fixing requirement: none
- Backlog: pending (plan 02-10)
