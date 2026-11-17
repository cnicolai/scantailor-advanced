# Closing check of Phase 6 (plan 06-16)

Checked on 2026-11-17 on HEAD `c167b12a` (the gate record of `s23-docs`; the documentation commit is `26702f8e`). The last commit that changed anything under `src/` or a build file is `a9d17d1c` (`s22-split-project`), so the last gated source commit and the tree measured here are the same code. Phase 6 base (the planning base, whose `src/` equals the end of Phase 5): `e5319cd9`. No code was changed by this plan so far: the sanitizer run had no report in a file Phase 6 changed, so `s24-factor-fix` and `fix(06-16)` do not exist. Output is normalised: `{repo}` for the repository path, `~` for the home folder.

The application binary `scantailor-advanced` was not launched (it reads and writes `~/.config` and `~/Library`), and no real scan was used: every test and probe runs on synthetic pages. `UPDATE_GOLDENS` was never set. This is the automated part of the closing check. The two live runs (the D-08 memory run through the worker pool and the live `mac_check` confirmation of the Phase 5 menus and shortcuts) wait for the user's go at the blocking checkpoint of plan 06-16 (D-00b, D-08); the rows that depend on them say "pending the live runs (Task 3)".

Evidence is in this directory (`gate-log.md`, `safe-01.md`, `safe-02.md`, `safe-03.md`, `strip-07.md`, `clean-01.md`, `clean-02.md`, `clean-03.md`, `output-digests.md`), in the permanent tests named below and in the closing runs of this record (console output in `build/scratch/closing6/`).

## Automated closing runs at HEAD

```
$ ctest --preset macos-release      -> 100% tests passed out of 7    (66.5 s, build/scratch/closing6/ctest-release.txt)
$ ctest --preset macos-debug        -> 100% tests passed out of 7    (247.3 s, build/scratch/closing6/ctest-debug.txt)
$ git diff --quiet a796c2ae -- src/pipeline_tests/golden
exit status: 0                                                       (goldens unchanged)
```

Gate `s23-docs` on `26702f8e` (`build/scratch/gate/s23-docs/summary.txt`): Release and Debug configure, build and ctest 0, `100% tests passed out of 7` in both, `goldens: unchanged`, command diff 0 / 0, `result: PASS`.

Output digests (the decoded output pixels of seven pages through all six stages, `pipeline_tests --run_test=OutputDigestProbeTestSuite --log_level=message`, Release, `build/scratch/closing6/digests.txt`):

```
DIGEST flat 300 bw off - d76527cfd84d79f5784c6967e970efb3a5d76c87
DIGEST flat 600 gray off - 1e78a1c0424fd3ef129c9a840a0de3100104d49b
DIGEST mixed 300 mixed off - b7e3a693d73c03ebc79546385c601c537a7cac8b
DIGEST mixed 600 mixed off - 718260ea80a1b8883bf8945ea0e3a23694f7e587
DIGEST curved 300 gray auto - 0476631ce760feb5fb117efd9720e83df0772f8d
DIGEST curved 600 mixed auto - c48ff8a8d7a4be446c75355380ed9bf055675dd7
DIGEST curved 600 bw auto - dd346ae2c91df2c7529f26f9c2ba317d25fb4241
```

The seven lines equal the s17 block of `records/output-digests.md` (taken before any split) and `build/scratch/digests-s17.txt`, line for line (`diff` of the sorted sets is empty): the split of `OutputGenerator.cpp` did not change a decoded output pixel of any page, at HEAD.

## Success criteria

ROADMAP Phase 6, criteria 1 to 5.

| # | Verdict | Evidence |
|---|---------|----------|
| 1 | PASS | A TIFF whose header claims absurd dimensions is refused before any allocation and the page view says "too large to load" with the claimed size and the limit, not a crash and not `std::bad_alloc`: `CoreImageSizeLimitTestSuite` (14 cases; `test_forged_mono_tiff_is_refused_without_allocating`, `test_forged_rgb_tiff_is_refused_without_allocating`, `test_forged_wide_tiff_is_refused_without_allocating`, `test_rgb_tiff_at_the_ceiling_is_refused_without_allocating`, `test_tiff_side_boundary`, `test_tiff_total_boundary`, `test_multi_page_tiff_refuses_only_the_oversized_page`: peak growth below 64 MiB and under 1000 ms), `TestUxQt::testOversizedImageShowsTooLargeInPageView` (rows tiff, png, png-wide, jpeg, garbage), `testProjectFilesDialogMarksOversizedFile`, `testOversizedThumbnailFails`; PNG and JPEG follow the same limit (`testForgedPngIsRefused`, `testForgedJpegIsRefusedAndLargeJpegLoads`, `testLargeGreyPngLoads`, `testForgedPngMetadataReportsClaimedSize`). Mutation runs (refusal disabled makes the tests fail) and the int-arithmetic audit are in `records/safe-01.md`. Gate rows `s03-tiff-limit` to `s07-insert-thumbs`. The chosen ceiling is in "For the user". |
| 2 | PASS | Names with `&`, `<`, `"` and non-ASCII characters (NFC, NFD, emoji) round-trip: `test_names_and_order_survive` (Phase 2 case, unchanged), and the new `test_unusual_names_survive`; names with control characters, which made a project file unloadable before, now survive: `test_control_characters_survive` (written first and seen to fail on the unmodified writer: "project file is not well-formed XML"), `FoundationXmlEscapeTestSuite` (7 cases), `test_ordinary_projects_carry_no_escape_flag` (ordinary files keep their bytes). `ProjectRoundTripTestSuite` has no `XFAIL_CHECK` and the sanitizer run of it printed no `KNOWN DEFECT` line (0 matches in `asan-roundtrip.txt`). The backslash case sits in its own suite and passes with plain checks. Evidence: `records/safe-02.md`; gate rows `s08-name-escape`, `s09-name-matrix`. |
| 3 | pending the live runs (Task 3) | Cap by RAM, automated part: `CoreMemoryBudgetTestSuite` (8 cases, the cap table of `records/safe-03.md` including the D-08 page: 5 threads on this 64 GiB Mac, 1 on 16 GiB), `TestCoreQt::testWorkerPoolHonoursMemoryCap` (the pool runs `min(ideal, slider, cap)` tasks), `testStoredThreadCountClamps`, `TestUxQt::testSystemLoadToolTipShowsMemoryLimit` (the slider says "limited to n by memory"); `MainWindow::startBatchProcessing` calls `MemoryBudget::batchMemoryCap(sequence)` before the first `submitTask` (`MainWindowBatch.cpp`). Smoke runs of both probes are in `records/safe-03.md`. Gate rows `s10-thread-cap` to `s12-memory-probe`. The part of the criterion that needs the machine, "no growth in swap during the run", is the D-08 memory run, pending the user's go; its record will be `records/memory-run.md`. |
| 4 | PASS | The OpenGL option is gone from Settings (`TestMacQt::testSettingsHasNoOpenGlControl`), no source or CMake file names OpenGL (`testNoOpenGlInSourcesOrBuild`), and the Release binaries link no Qt OpenGL module (see "Static checks": `QtOpenGL` 0 in `otool -L` of eleven binaries and in both `build.ninja` files, with `QtGui` as the positive control). The colour-picker tests pass, unedited since the planning base: `TestHiDpiQt::testMedianColorRatioInvariant`, `testColorPickerGrabAtRatio2` (both ctest presets, `qt_tests` inside `100% tests passed out of 7`, and the sanitized `qt_tests`). Evidence: `records/strip-07.md`; gate rows `s13-opengl-option`, `s14-opengl-modules`. |
| 5 | PASS for the automated part; the live part is pending the live runs (Task 3) | Split: `OutputGenerator.cpp` 2814 to 268 lines (`OutputGeneratorProcessor.h`, `OutputGeneratorPixelOps.h`, `OutputGeneratorNoDewarp.cpp`, `OutputGeneratorDewarp.cpp`, `OutputGeneratorBinarize.cpp`), proven by pure-move checks, the seven equal digests above and goldens unchanged (`records/clean-01.md`, gate rows `s17-digest-probe`, `s18-split-nodewarp`, `s19-split-rest`); `MainWindow.cpp` 2319 to 1237 lines (`MainWindowBatch.cpp`, `MainWindowActions.cpp`, `MainWindowProject.cpp`, `MainWindowPages.cpp`; `records/clean-02.md`, gate rows `s20-split-batch` to `s22-split-project`). Save Project As is one flow: `ProjectSaveFlow` is the only caller of `QFileDialog::getSaveFileName` under `src` (`git grep -n getSaveFileName -- src` prints `src/app/ProjectSaveFlow.cpp:34` only), used by `MainWindow` and `OutOfMemoryDialog`; `TestMacQt::testProjectExtension`, `testSaveAsFlowFromOutOfMemoryDialog`, `testSaveAsFlowRunsTwice`; gate rows `s15-save-as-flow`, `s16-save-as-main`. No dead `#if 0` block and no `boost/foreach` include remain (static checks). Full `ctest` passes in both presets with baselines unchanged (above). "The Phase 5 menus and shortcuts behave exactly as before" is covered offscreen by `TestMacQt` (inside `qt_tests`, unchanged and passing after each split step) and, on the real Cocoa display, by the two live `mac_check` invocations of TESTING.md "Live Mac check", compared with the Phase 5 closing verdicts: pending the live runs (Task 3). |

## Requirements

| ID | Plans | Tests | Records |
|----|-------|-------|---------|
| SAFE-01 | 06-04, 06-05, 06-06 | `CoreImageSizeLimitTestSuite`, `testForgedPngIsRefused`, `testLargeGreyPngLoads`, `testForgedJpegIsRefusedAndLargeJpegLoads`, `testForgedPngMetadataReportsClaimedSize`, `testOversizedImageShowsTooLargeInPageView`, `testProjectFilesDialogMarksOversizedFile`, `testOversizedThumbnailFails` | `safe-01.md`; gate rows `s03` to `s07` |
| SAFE-02 | 06-07 | `FoundationXmlEscapeTestSuite`, `test_control_characters_survive`, `test_ordinary_projects_carry_no_escape_flag`, `test_unusual_names_survive`, `ProjectRoundTripBackslashTestSuite/test_backslash_in_file_name` | `safe-02.md`; gate rows `s08`, `s09` |
| SAFE-03 | 06-08, 06-09, 06-16 | `CoreMemoryBudgetTestSuite`, `testWorkerPoolHonoursMemoryCap`, `testStoredThreadCountClamps`, `testSystemLoadToolTipShowsMemoryLimit`, the two opt-in probes | `safe-03.md`, `memory-run.md` (pending the live runs); gate rows `s10` to `s12` |
| STRIP-07 | 06-10, 06-11 | `testSettingsHasNoOpenGlControl`, `testNoOpenGlInSourcesOrBuild`, `testMedianColorRatioInvariant`, `testColorPickerGrabAtRatio2` | `strip-07.md`; gate rows `s13`, `s14` |
| CLEAN-01 | 06-13, 06-14 | `OutputDigestProbeTestSuite` (opt-in, seven digests equal), the pipeline goldens | `clean-01.md`, `output-digests.md`, `move_check.py`; gate rows `s17` to `s19` |
| CLEAN-02 | 06-15, 06-16 | `TestMacQt` and the other `qt_tests` classes after each step; live `mac_check` pending the live runs | `clean-02.md`; gate rows `s20` to `s22` |
| CLEAN-03 | 06-01 | `git grep` for `#if 0` empty (static checks) | `clean-03.md`, `pp_equiv.py`; gate row `s01-if0` |
| CLEAN-04 | 06-02, 06-03 | `git grep -l boost/foreach -- src` empty (static checks) | gate rows `s02a-foreach`, `s02b-foreach` |
| CLEAN-05 | 06-12 | `testProjectExtension`, `testSaveAsFlowFromOutOfMemoryDialog`, `testSaveAsFlowRunsTwice` | gate rows `s15`, `s16` |

## Gate log

`records/gate-log.md` has 24 rows (`s01-if0` to `s23-docs`); every row reads `100% tests passed out of 7` for Release and for Debug and `unchanged` for the goldens (checked by script at the closing: 24 of 24 in each of the three columns, 0 rows without `unchanged`). The conditional commit `s24-factor-fix` does not exist: it is only made if the memory run measures more than the estimate, and that run has not happened yet. The generic GSD regression gate finds no test runner in this repository; the commands that count are the two ctest presets above.

## Static checks

Commands run from the repository root; output in `build/scratch/closing6/static-checks.txt`.

```
$ git diff --quiet a796c2ae -- src/pipeline_tests/golden
exit status: 0
$ L=$(git diff --name-only e5319cd9 -- src/pipeline_tests/golden src/imageproc src/dewarping 'src/core/filters/*/Task.cpp' 'src/core/filters/*/Task.h'); test -z "$(printf '%s\n' "$L" | grep -vxF -f records/output-path-allowlist.txt)"
12 files changed under those paths, all on the allowlist       (exit status 0)
$ git grep -n -E '^[[:space:]]*#[[:space:]]*if[[:space:]]+0' -- src
(no output)
$ git grep -c -E '^[[:space:]]*#[[:space:]]*if[[:space:]]+0' e5319cd9 -- src | wc -l      (positive control)
7
$ git grep -l 'boost/foreach' -- src
(no output)
$ git grep -l 'boost/foreach' e5319cd9 -- src | wc -l                                      (positive control)
26
$ grep -c QtOpenGL build/macos-release/build.ninja     -> 0     (macos-debug: 0; QtGui: 495, positive control)
$ otool -L build/macos-release/<binary> | grep -c QtOpenGL
scantailor-advanced 0, core_tests 0, foundation_tests 0, imageproc_tests 0, math_tests 0, dewarping_tests 0,
pipeline_tests 0, qt_tests 0, mac_check 0, retina_check 0, ui_snapshot 0      (QtGui: 1 in each, positive control)
$ grep -c OpenGL CMakeLists.txt src/core/CMakeLists.txt
CMakeLists.txt:0  src/core/CMakeLists.txt:0
$ git grep -n -i allocationlimit -- src       (outside src/qt_tests)
src/core/ImageLoader.cpp:37: QImageReader::setAllocationLimit(ImageSizeLimit::kQtAllocationLimitMb)   (2048; no call with 0)
```

D-00a holds: no output path changed except by moves (digests equal) and `UPDATE_GOLDENS` was never set. D-00c holds: `settings/enable_opengl` and `settings/batch_processing_threads` are not read, rewritten or removed by new code (`records/strip-07.md`, `records/safe-03.md`).

## Sanitizer run

The `macos-asan` preset (Debug, `SANITIZE=address,undefined`) was configured and built completely (`build/scratch/closing6/asan-configure.txt`, `asan-build.txt`, both exit 0). `ctest --preset macos-asan -R '^(core_tests|foundation_tests|qt_tests)$' -V` reported `100% tests passed out of 3` (`asan-ctest.txt`; `-V` shows the reports that ctest hides for passing tests): `qt_tests` Totals 14 + 15 + 28 + 33 passed, 0 failed, 0 skipped (`TestCoreQt`, `TestUxQt`, `TestHiDpiQt`, `TestMacQt`). `build/macos-asan/pipeline_tests --run_test=ProjectRoundTripTestSuite --log_level=test_suite` with `ASAN_OPTIONS=detect_leaks=0:halt_on_error=0` and `UBSAN_OPTIONS=print_stacktrace=1` ended `No errors detected` (8 test cases, `asan-roundtrip.txt`); the backslash suite likewise (`asan-roundtrip-backslash.txt`).

- `ERROR: AddressSanitizer` lines: 0 in all three files.
- `runtime error:` lines: 2, one in `core_tests` and one in `qt_tests`, both `{repo}/src/imageproc/BinaryImage.cpp:1044:37: member access within null pointer of type 'SharedData'`. 0 in the two round-trip runs.

Attribution (D-05, D-11): `BinaryImage.cpp` is not among the files Phase 6 changed (`git diff --name-only e5319cd9 -- src` does not list it), and the location is the known Phase 1 one in `.planning/phases/02-golden-baselines/records/sanitizer-findings.md`. It is older code, recorded, not fixed. No report is located in a file Phase 6 created or changed, so there is nothing to fix under `fix(06-16)`. The sanitized round trip also covers the new escape code of SAFE-02, and `qt_tests` the new code of SAFE-01, SAFE-03, STRIP-07 and CLEAN-05.

## For the user

- Image size ceiling (D-01): D-01 named 1 gigapixel as an example and left the number to the phase. The phase chose fewer than 2^29 pixels in total (`ImageSizeLimit::kMaxPixels` = 536870911, shown to the user as 537 megapixels) and at most 65535 pixels per side, so that the RGB32 byte count of the largest accepted image (4 x 536870911 = 2147483644) fits in a signed 32-bit `int`; 2^29 inclusive would overflow by one byte, and 2^30 would let `int` byte counts overflow. The int-arithmetic audit of `records/safe-01.md` found no pixel, byte or row-offset count above `INT_MAX` for an accepted image, and says what 2^30 would need. The 1200 dpi A3 reference page (14031 x 19843, 278 megapixels) loads; a 1200 dpi A2 (about 557 MP) and a 600 dpi A0 (about 558 MP) are refused by design. The ceiling is one constant to change (`ImageSizeLimit::kMaxPixels` in `src/core/ImageSizeLimit.h`, with the Qt reader limit `kQtAllocationLimitMb` that follows from it and the two `static_assert`s that say what else must hold) if you prefer another value. The limit bounds the source image as loaded, not derived images (deskew rotation, output dpi scaling).
- Names in project files (D-10): the premise was that tab, LF, CR and CRLF in names would be turned into spaces by attribute-value normalisation. Observed with Qt 6.12.0: QDom writes them as character references (`&#x9;`, `&#xa;`, `&#xd;`) and the parser restores them, in file names and folder names alike, so all four round-trip with no fix (`records/safe-02.md`). The real failing class was the other C0 control characters (U+0001 to U+001F except tab, LF and CR), which QDom wrote raw so that the file did not parse; those names are now escaped, and only those, with a flag attribute (`nameEscaped`, `pathEscaped`, `outputDirectoryEscaped`), so ordinary project files stay byte-identical. A project file with a flagged name read by an older build shows the percent-encoded text instead of failing. The backslash in a file name round-trips through the project file; backlog 999.8 (`RelinkablePath::normalize` during relinking) is untouched.
- Memory cap (SAFE-03): the worker pool runs at most `min(idealThreadCount, slider, cap)` pages at once, where the cap is half of the RAM divided by 40 bytes per pixel of the largest page of the batch. On this Mac (64 GiB, 10 processors) pages up to about 85 megapixels (the cap is 10 or more; for example 7016 x 9921, 600 dpi A3, 69.6 MP, gives 12) still run at full parallelism; larger pages are throttled (the 165.6 MP page of the D-08 run gets 5 threads). The slider tooltip says "limited to n by memory" when the cap binds. Limit of the estimator rule: it counts the output at 600 dpi at most, so a page whose output dpi you raise above 600 is estimated too small by (dpi / 600)^2 (research: a 300 dpi source with 1200 dpi output peaked at 26 bytes per larger pixel while the rule counts the 600 dpi output); revisit it if you set such a dpi.
- Optional check on your own scans (D-08): the memory run uses synthetic pages only. To see the cap on real pages, process a large batch in the app and watch Activity Monitor, tab Memory, the "Swap Used" figure at the bottom, before and during the run; it should not grow. Claude does not do this check (the app binary is never launched, D-00e).
- Backlog: no backlog item was added in Phase 6; 999.1 to 999.8 are as before (D-11), and no deferred review finding or UBSan report was fixed here.
- Decisions of the phase worth knowing: the three Phase 6 probes are opt-in and never run by ctest (TESTING.md "Opt-in probes"); `calcLocalWindowSize` and `from300dpi` in `OutputGeneratorBinarize.cpp` have no caller anywhere and were kept, not deleted (D-16); the Qt OpenGL modules are gone, the system `OpenGL.framework` is still linked by `QtGui` itself and cannot be removed from `otool -L` output.

## For the transition

D-11 leaves these notes out of date, and they are not edited here:

- `.planning/PROJECT.md` section Context and `.planning/STATE.md` section Blockers still call Phase 3 WR-01 (the GUI sources compiled twice) and Phase 4 WR-01 to WR-03 open. Phase 3 WR-01 was closed by the static library `app_gui` in Phase 5 (05-01); Phase 4 WR-01, WR-02 and WR-03 are `fixed` in `04-REVIEW-DISPOSITION.md`. PROJECT.md Context also still names the Phase 4 WR-01 and WR-02 as open findings.
- The OpenGL mentions are out of date: `.planning/codebase/STACK.md`, `.planning/codebase/INTEGRATIONS.md` and the generated stack text of `.claude/CLAUDE.md` still name the Qt OpenGL modules and `src/core/OpenGLSupport.cpp`. CLAUDE.md is regenerated through `generate-claude-md`, not edited by hand; the codebase map predates Phase 3's removals, Phase 5's targets (`app_gui`, `mac_check`) and Phase 6's new files (`ImageSizeLimit`, `MemoryBudget`, `ProjectSaveFlow`, the split files).
- The STATE.md line "Live `mac_check` and `retina_check` runs move the pointer and take focus: Phase 6 plans that include one must stop at a checkpoint until the user says go" can go once Task 3 has run.

