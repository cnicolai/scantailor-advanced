# Testing

The project has seven ctest suites. All of them run through the CMake presets, from the repository root, after the build described in the "Build" section of [README.md](README.md).

## Running the tests

```sh
ctest --preset macos-release
ctest --preset macos-debug
ctest --preset macos-asan
```

Each preset needs its own build first (`cmake --preset <preset>` and `cmake --build --preset <preset>`). A run is good when ctest prints `100% tests passed out of 7`. Release and Debug must both pass before a code change is committed; Debug is the slow one, mostly because of `pipeline_tests`.

Two suites create a Qt application, `pipeline_tests` and `qt_tests`. `pipeline_tests` runs on the single-screen offscreen platform. `qt_tests` runs offscreen with two screens, `OneX` at device pixel ratio 1 and `TwoX` at ratio 2: it writes the screen layout to `build/<preset>/test-tmp/offscreen-screens.json` and scales the screens with `QT_SCREEN_SCALE_FACTORS="OneX=1;TwoX=2"`. That variable is a test aid only; the app never sets a scale factor. Both suites set `LC_ALL=en_US.UTF-8` and `TMPDIR` themselves, and they keep their settings and temporary files in `build/<preset>/test-tmp`. No test touches the settings or profiles of the installed application.

A single suite runs with ctest's regular filter, for example `ctest --preset macos-release -R core_tests`, or directly from the build directory, for example `build/macos-release/core_tests --log_level=message`.

## Test suites

| Suite | Covers |
|-------|--------|
| `foundation_tests` | Constants, line intersection, distances between points and segments, locale-independent number-to-string conversion, and the escape of control characters in project-file attributes (`FoundationXmlEscapeTestSuite`) |
| `math_tests` | Automatic differentiation (Hessians), linear and quadratic functions, matrix calculations, sides of a line, lines clipped by a rectangle, squared-distance approximation |
| `imageproc_tests` | Binary images, binarization, morphology, raster operations, scaling, shear, transforms, rotation, connected components and their eraser, seed fill, distance maps, skew finder, line finder, histograms, polygon rasterizer, DPI |
| `core_tests` | Image and page identifiers, page ranges and sequence, margins, units, natural file-name ordering, deskew parameters, content span finder, oblique finder, select-content application, the image size ceiling (`CoreImageSizeLimitTestSuite`: forged TIFF headers are refused without allocating, JPEG metadata, multi-page files) and the memory budget of the worker threads (`CoreMemoryBudgetTestSuite`) |
| `qt_tests` | Qt Test cases: units, number formatting, file-name ordering and signals (`TestCoreQt`), and widget behaviour such as button clicks, check boxes, line edits, the collapsible group box and the error widget (`TestUxQt`), and the Retina tests (`TestHiDpiQt`): page view, thumbnails, zone mask and stage-list animation built at the device pixel ratio, moves between the 1x and the 2x screen, icon geometry and colour-picker sampling, and the Mac tests (`TestMacQt`): the application menu roles, the File, View and Tools menus, every window key (stages, Output tabs, zoom, thumbnails, Settings, Save As), the zone and content-box keys and their protection in text fields, the right-click menus, the scan that keeps PC key names (Ctrl, Alt and the like) out of user-visible text, and the source checks for the batch beep and the native file dialogs. Phase 6 added slots for: oversized images (the page view says "too large to load", the project file dialog marks the file, an oversized thumbnail fails without allocating), the PNG and JPEG limits (forged headers refused, large grey PNG and JPEG load), the worker pool and the slider honouring the memory cap, the stored thread count clamping, no OpenGL control in Settings and no OpenGL in sources or build files, and the shared Save Project As flow |
| `pipeline_tests` | The headless pipeline on generated pages: flat documents, book spreads, mixed pages, dewarping, project round trips (`ProjectRoundTripTestSuite`, including control characters and unusual file and folder names, and `ProjectRoundTripBackslashTestSuite`), a smoke run, and the test support itself (generator determinism, image comparison, truth metrics, the golden store, the comparator self-test). It also holds three opt-in probes that ctest never runs, see "Opt-in probes" |
| `dewarping_tests` | Text line tracer, distortion model, cylindrical dewarper |

Test data is generated in code with fixed seeds. No scan is stored in the repository.

## Golden baselines

`pipeline_tests` compares its outputs with golden files in `src/pipeline_tests/golden/{flat,book,mixed}/`. Each directory holds PNG and text goldens, a `fixtures.txt` and a `MANIFEST.txt`.

* A normal run never writes into the source tree.
* Only `UPDATE_GOLDENS=1` re-records goldens. The value must be exactly `1`.
* An update run rewrites only the goldens that fail, and it refreshes the manifest of every suite it touches.
* A golden update is its own commit, with the reason in the commit message.

```sh
UPDATE_GOLDENS=1 build/macos-release/pipeline_tests
```

Record goldens with the Release build only, and look at every changed golden before committing it.

## Failure artefacts

When a golden does not match, the test leaves its evidence in `build/<preset>/test-artifacts/<suite>/<name>/`: `actual.png`, `expected.png` and `diff.png` for images, `actual.txt`, `expected.txt` and `diff.txt` for text goldens. ctest prints the failing metric, its value, its limit and the folder. Each run clears the folder of a golden before it compares that golden again.

Every comparison also writes `GOLDEN-METRIC` lines (metric, value and limit, passing or not) to the Boost log; run `build/<preset>/pipeline_tests --log_level=message` to see them.

## Toolchain manifest

Each golden directory has a `MANIFEST.txt` with the toolchain the goldens were recorded with. The keys are `qt_runtime`, `qt_compile`, `boost`, `libtiff`, `libpng`, `libjpeg_turbo`, `apple_clang`, `xcode`, `cmake`, `macos`, `ffp_contract`, `build_type` and `source_commit`.

On the first comparison of a suite, the current toolchain is compared with the manifest. A difference never fails a test; it is logged as a `TOOLCHAIN DIFFERS` message (visible with `--log_level=message`) so that a failing golden can be read against a changed toolchain. `build_type` and `source_commit` are not compared, because they change legitimately between a recording and a check.

`qt_compile` is the Qt version found when CMake configured the build (`Qt6_VERSION`). `src/test_support/CMakeLists.txt` hands it to the code as `ST_QT_COMPILE_VERSION`, and the manifest code reads it from there.

## Expected failures

Defects found by the golden-baseline tests are recorded, not fixed. A check that cannot pass yet is wrapped in `XFAIL_CHECK(condition, "tag")` (`src/test_support/ExpectedFailure.h`): while the condition is false, the failure is only logged. Once the defect is fixed, the condition becomes true, the mark raises an error, and the test fails until the mark is removed. That trip-wire is deliberate. Each defect has a backlog item in `.planning/ROADMAP.md`; the findings are in `.planning/phases/02-golden-baselines/02-FINDINGS.md`.

| Test file | Defect | Backlog item |
|-----------|--------|--------------|
| `src/imageproc/tests/TestBinarize.cpp` | Global binarizers reject `Format_Grayscale8` | 999.1 |
| `src/dewarping/tests/TestTextLineTracer.cpp` | The distortion model can leave out the first or last text line | 999.2 |
| `src/pipeline_tests/TestFlatDocument.cpp` | Output of the 600 dpi flat page keeps about 0.1 degrees of residual skew | 999.3 |
| `src/pipeline_tests/TestBookSpread.cpp` (four marks) | `page_split` puts the cutter 28 to 30 px from the gutter centre | 999.4 |
| none (limitation of `src/test_support`) | `straightnessResidual` cannot measure the curved book page | 999.5 |
| `src/pipeline_tests/TestMixedPage.cpp` | `page_split` AUTO cuts the mixed page and keeps only a fragment of its picture | 999.6 |
| none (the test runs the stage first) | A layout type from the default profile is not written to the project file until `page_split` has run | 999.7 |

`src/pipeline_tests/TestGoldenStore.cpp` raises two marks on purpose to test the trip-wire itself; they do not refer to a defect.

## Opt-in probes

`pipeline_tests` holds three probes that are disabled by default. ctest runs `pipeline_tests` without `--run_test`, so Boost skips them and ctest never runs them; they run only when named. Build the Release preset first and run from the repository root.

**Output digests** (`OutputDigestProbeTestSuite`) prints seven `DIGEST` lines, one SHA-1 of the decoded output pixels for each of seven generated pages. It is the proof that splitting `OutputGenerator.cpp` moved code without changing a pixel: run it before and after a move and compare the lines. The reference lines are in `.planning/phases/06-hardening-and-cleanup/records/output-digests.md`.

```sh
build/macos-release/pipeline_tests --run_test=OutputDigestProbeTestSuite --log_level=message
```

**Peak memory of one page** (`PeakMemoryProbeTestSuite`) runs one generated mixed page through all six stages and prints one `PROBE` line: the pixel estimate, the estimate in MiB, the peak growth of the process footprint above its start, and the thread caps `MemoryBudget` gives for 16 GiB, for 64 GiB and for this Mac. It fails if the growth exceeds the estimate of 40 bytes per pixel. `ST_PROBE_DPI` (default 300) sets the page's dpi and `ST_PROBE_SCALE` (default 1) enlarges the page by repeating pixels and multiplying the dpi, so `ST_PROBE_DPI=1200 ST_PROBE_SCALE=3` builds a 12000 x 13800 page.

```sh
env ST_PROBE_DPI=<dpi> ST_PROBE_SCALE=<n> build/macos-release/pipeline_tests --run_test=PeakMemoryProbeTestSuite/test_print_peak_memory --log_level=message
```

**Batch through the worker pool** (`BatchMemoryProbeTestSuite`) pushes `ST_PROBE_PAGES` such pages (default 4) through the application's `WorkerThreadPool`, as `LoadFileTask`-wrapped stage chains, with the memory cap that the application computes (`MemoryBudget::batchMemoryCap`). It prints a `SAMPLE` line every 2 seconds (pages running, footprint, swap in use) and one `BATCH` line: the cap and the effective thread count, the most pages running at once, pages done and failed, the peak footprint above the start, and swap before, at its maximum and after. It fails on a page error, if the thread count differs from the cap's, if more pages run at once than threads, if swap grows during the run, or if the peak exceeds half of the RAM. The pages are hard links of one PNG in a temporary folder under `build/<preset>/test-tmp`, and the run stops submitting after 3600 seconds.

```sh
env ST_PROBE_DPI=<dpi> ST_PROBE_SCALE=<n> ST_PROBE_PAGES=<N> build/macos-release/pipeline_tests --run_test=BatchMemoryProbeTestSuite/test_batch_through_worker_pool --log_level=message
```

A large run of either memory probe uses gigabytes of memory (the 12000 x 13800 page peaks near 5 GB, and the batch near five times that), so start it only when the Mac is otherwise idle and other memory-heavy programs are closed. The records of the Phase 6 runs are in `.planning/phases/06-hardening-and-cleanup/records/` (`safe-03.md`, `memory-run.md`).

## Sanitizer build

The `macos-asan` preset is a Debug build with `SANITIZE=address,undefined`. Its test preset sets `ASAN_OPTIONS=detect_leaks=0:halt_on_error=0` and `UBSAN_OPTIONS=print_stacktrace=1`.

```sh
cmake --preset macos-asan
cmake --build --preset macos-asan
ctest --preset macos-asan
```

ctest hides the sanitizer reports of tests that pass. To see them, run `ctest --preset macos-asan -V`, or run a test executable directly with the same environment:

```sh
ASAN_OPTIONS=detect_leaks=0:halt_on_error=0 UBSAN_OPTIONS=print_stacktrace=1 build/macos-asan/pipeline_tests --log_level=test_suite
```

The reports known so far are in `.planning/phases/02-golden-baselines/records/sanitizer-findings.md`. They are recorded, not fixed.

## UI snapshot tool

`ui_snapshot` renders the main window and the Settings dialog offscreen and writes PNG and text files. It is built with the tests but is not a ctest suite. The output directory must lie below `build/`; the tool keeps its own settings and temporary files there, so it never reads or writes the settings of the installed application.

```sh
build/macos-release/ui_snapshot build/scratch/ui
build/macos-release/ui_snapshot build/scratch/ui --assert-no-language
```

With `--assert-no-language` it exits non-zero when the Settings dialog still has a language control.

The GUI sources are compiled once into the static library `app_gui`, which the app, `ui_snapshot`, `qt_tests` and `mac_check` link.

## Live Retina check

`retina_check` measures the Retina rendering on the real display. It is built with the tests but is not a ctest suite, because ctest runs offscreen. It runs on the Cocoa platform, so the display must be awake and the session unlocked.

```sh
build/macos-release/retina_check build/scratch/retina/all --section all -ApplePersistenceIgnoreState YES
```

* `--section` is `page`, `thumbnails`, `overlays`, `picker` or `all`.
* The output directory must lie below `build/`. The tool keeps its settings, temporary files, generated pages and thumbnails there, so it never reads or writes the settings of the installed application.
* It clears the scale-factor variables (`QT_SCALE_FACTOR`, `QT_SCREEN_SCALE_FACTORS` and the others) before the application exists, so it sees what the app sees on this display.
* Results go to `summary.txt` in the output directory as `key: value` lines, with a verdict per element and a final `verdict: PASS` or `verdict: FAIL`.
* Exit codes: 0 pass, 1 fail, 2 usage error or output path outside `build/`, 3 platform is not Cocoa.

The `qt_tests` cannot show how Cocoa behaves on a real display, and the live check cannot move a window between a 1x and a 2x display, so the two complement each other. The records of the Phase 4 runs are in `.planning/phases/04-retina-rendering/records/`.

## Live Mac check

`mac_check` checks the Mac menus, keys, context menus, dialogs and file panels on the real display. It is built with the tests but is not a ctest suite, because ctest runs offscreen. It runs on the Cocoa platform, so the display must be awake and the session unlocked, and it uses the current keyboard layout (the Phase 5 records were made with the German layout).

```sh
build/macos-release/mac_check build/scratch/mac/input --section menubar,layout,keys,zone,context -ApplePersistenceIgnoreState YES

D=build/scratch/mac/panels; mkdir -p "$D/home"; env HOME="$PWD/$D/home" CFFIXED_USER_HOME="$PWD/$D/home" build/macos-release/mac_check "$D" --section dialogs,panels -ApplePersistenceIgnoreState YES
```

* `--section` is `menubar`, `layout`, `keys`, `zone`, `context`, `dialogs`, `panels` or `identity`. It takes a comma-separated list or `all`. `all` leaves out `identity`, the storage-identity check of plan 05-02, which runs alone and only with the redirected home.
* The output directory must lie below `build/`. The tool keeps its settings, temporary files and `summary.txt` there, so it never reads or writes the settings of the installed application.
* `dialogs` and `panels` refuse to run unless `HOME` and `CFFIXED_USER_HOME` both name `<output-dir>/home`, because the file panels and the profile list would otherwise reach the home folder: without it the section opens nothing and ends with `home not redirected` and a FAIL verdict (`identity` exits 2 instead). The second command above sets both.
* All input is synthesised inside the tool's own process and sent to its own windows, so no Accessibility or screen-recording permission is needed. The mouse pointer does move during the run, and dialogs and file panels open and close for a few minutes; leave the mouse and keyboard alone while it runs.
* Results go to `summary.txt` in the output directory as `key: value` lines, with a verdict per section and a final `verdict: PASS` or `verdict: FAIL`.
* Exit codes: 0 pass, 1 fail, 2 usage error or output path outside `build/` (also `identity` without the redirected home), 3 platform is not Cocoa.

`qt_tests` (`TestMacQt`) cannot show what the native menu bar and the file panels do on Cocoa, and the live check does not run in ctest or in the Debug gate, so the two complement each other. The records of the Phase 5 runs are in `.planning/phases/05-mac-menus-and-shortcuts/records/`.

## Formatting

`.clang-format` is based on the Chromium style. No CI enforces it, so format every C++ file you touch before committing:

```sh
xcrun clang-format -i <file>
xcrun clang-format --dry-run -Werror <file>
```

The first command formats the file, the second checks it without changing it. `src/dewarping/TextLineTracer.cpp` keeps two older violations, at lines 317 and 328.
