# Architecture Research

**Domain:** Porting and hardening a C++17/Qt desktop image-processing pipeline (ScanTailor Advanced 1.2.1) for macOS / Apple Silicon, with a headless regression harness
**Researched:** 2026-10-18
**Confidence:** HIGH for the test seams and the build-order logic (verified by reading `src/`); MEDIUM for Retina and Homebrew/CMake details (reasoned from code plus general Qt knowledge, not yet built on this Mac)

Scope of this file: how the target changes (Qt 6 only, platform pruning, macOS build, Retina, Mac menus, synthetic-image regression harness, safe file splitting) fit into the existing layered architecture, where the headless test seams are, how to build a golden-image harness, and the safest ordering.

---

## Standard Architecture

### System Overview (existing layers, with the test seams marked)

```
                         Qt event loop / GUI thread
┌──────────────────────────────────────────────────────────────────────────┐
│ app/   MainWindow, dialogs, ThumbnailSequence, project create/open        │  GUI-only
│        MainWindow::createCompositeTask() chains the Tasks                 │  (no automated test;
├──────────────────────────────────────────────────────────────────────────┤   manual smoke only)
│ core/  Filter (owns Settings + OptionsWidget)  -- needs QApplication      │
│        UiUpdater::updateUI(FilterUiInterface*) -- GUI thread only         │  SEAM C (offscreen
│        ImageViewBase / ThumbnailBase / ThumbnailPixmapCache / IconPacks   │  QApplication)
├──────────────────────────────────────────────────────────────────────────┤
│ core/  LoadFileTask -> fix_orientation::Task -> page_split::Task ->       │
│        deskew::Task -> select_content::Task -> page_layout::Task ->       │  SEAM B (headless:
│        output::Task          each: process(TaskStatus, FilterData)        │  Task chain, filter=
│        m_filter is dereferenced ONLY inside UiUpdater::updateUI           │  nullptr)
│        Settings (per stage, PageId -> Params), widget-free                │
├──────────────────────────────────────────────────────────────────────────┤
│ core/filters/output/OutputGenerator::process(...)  (2814 lines)           │  SEAM A2 (headless,
│        input: FilterData + ImageTransformation + ZoneSets + Settings      │  no Qt app)
├───────────────────────────────────┬──────────────────────────────────────┤
│ dewarping/  (links imageproc)     │ imageproc/  QImage-based, no widgets │  SEAM A1 (headless,
│  DistortionModelBuilder, Tracers  │  Binarize, SkewFinder, Transform ... │  no Qt app)
├───────────────────────────────────┴──────────────────────────────────────┤
│ math/ , foundation/   pure C++ (+QtCore types), no Qt app                │  existing Boost tests
└──────────────────────────────────────────────────────────────────────────┘
```

### Verified facts that shape the design (read from source)

| Fact | Where verified | Consequence |
|------|----------------|-------------|
| Each stage `Task::process(const TaskStatus&, FilterData[, extras])` is a plain function call chained through `m_nextTask`. `m_filter` is only dereferenced in `UiUpdater::updateUI` (`m_filter->optionsWidget()`), never in `process`. | `grep m_filter filters/*/Task.cpp` | The whole six-stage chain runs headlessly with `filter = nullptr`; no `OptionsWidget`, no `QApplication` needed. |
| The last Task in a chain returns a `UiUpdater` (a `FilterResult`). Constructing it only does `ImageViewBase::createDownscaledImage` (QImage transform). `updateUI` is never called by the harness. | `output/Task.cpp:437`, `deskew/Task.cpp:217` | Harness simply discards the `FilterResultPtr`. Real output is on disk (output TIFF) and in the `Settings` objects. |
| `Filter` constructors build `OptionsWidget` (`m_optionsWidget.reset(new OptionsWidget(...))`), so `StageSequence` needs a `QApplication`. | `filters/*/Filter.cpp`, `StageSequence.cpp` | Use `StageSequence` only for the full-fidelity e2e test and project-file round trip; run under `QT_QPA_PLATFORM=offscreen`. |
| `TaskStatus` is an interface; `NullTaskStatus.h` already exists in `core/`. `LoadFileTask` is itself a `BackgroundTask` (and so a `TaskStatus`). | `core/TaskStatus.h` (foundation), `NullTaskStatus.h`, `BackgroundTask.h` | Skip `LoadFileTask`/`BackgroundExecutor`/`WorkerThreadPool` in tests; call `fix_orientation::Task::process(NullTaskStatus, FilterData(img))` directly. A small throwing `TaskStatus` stub covers cancellation paths. |
| `output::Task` calls `m_thumbnailCache->recreateThumbnail(...)` unconditionally; `ThumbnailPixmapCache` runs a `QThread` and asserts `QCoreApplication::instance()->thread()` in places. | `output/Task.cpp:425`, `ThumbnailPixmapCache.cpp:350,529,674` | A real cache pointed at a temp dir is the simplest stub (needs an application object). Do not add an interface just for this unless the cache proves flaky in tests. |
| `output::Task` writes real files: output TIFF, `foreground/`, `background/`, `cache/automask`, `cache/speckles`. `QDir().mkdir(automaskDir)` deliberately fails if `$OUT/cache` does not exist. | `output/Task.cpp`, `output/Utils.cpp:17-27` | Harness must create `$OUT/cache` up front, or mixed-mode automask writes fail and `invalidateParams` silently removes the stored output params. |
| `ApplicationSettings` is a process-wide singleton wrapping a default-constructed `QSettings`; it is read inside Tasks (`deskew/Task.cpp:282` black-on-white detection, `OutputGenerator.cpp:1230`). `main.cpp` sets org/app name and INI format; tests do not. | `ApplicationSettings.cpp`, `app/main.cpp:43-51` | Harness fixture must set org/app name, `QSettings::setDefaultFormat(IniFormat)` and `QSettings::setPath` to a temp dir before first `getInstance()`, otherwise tests read the developer's real preferences and are non-deterministic. |
| `OutputGenerator::process` takes everything as arguments (`FilterData`, `ZoneSet`s, `DistortionModel&`, `DepthPerception`, optional mask outputs, `DebugImages*`, `PageId`, `shared_ptr<Settings>`). `output::Settings` includes no widget headers. | `OutputGenerator.h`, `output/Settings.cpp` | `OutputGenerator` is directly unit-testable without `Task`, files, thumbnails or Qt app. This is the primary seam for the 2814-line file. |
| Dewarping RANSAC is seeded (`QRandomGenerator prng(0)`); the active Qt 6 branch is the `#else` of `#if QT_VERSION_MAJOR == 5 && MINOR <= 9`. | `dewarping/DistortionModelBuilder.cpp:162-177`, `DetectVertContentBounds.cpp:250-270` | Removing the Qt 5 branch is behaviour-preserving, and dewarp output is reproducible on a given Qt version (but see Pitfall G3 about Qt upgrades). |
| No `QtConcurrent`, `std::thread` or OpenMP in `imageproc/`, `dewarping/`, `OutputGenerator`; threading exists only in `core/WorkerThreadPool`, `BackgroundExecutor`, `ThumbnailPixmapCache`. | `grep` | Pipeline algorithms are single-threaded and deterministic for a fixed build; goldens are stable run to run. |
| Retina-relevant code is view-layer only: `ImageViewBase::HqTransformTask` renders in logical widget pixels (`viewport()->size()`, `QPixmap::fromImage` without `setDevicePixelRatio`); `ThumbnailBase` allocates `QPixmap(w,h)` in logical pixels; `main.cpp` sets `AA_EnableHighDpiScaling` / `AA_UseHighDpiPixmaps` (no-ops in Qt 6). | `ImageViewBase.cpp:947-1050`, `ThumbnailBase.cpp:100-125`, `app/main.cpp:27-31` | Retina work never touches `imageproc/`, `dewarping/` or any `Task::process`; golden images are unaffected by it, which makes goldens a clean proof of that. |
| macOS shortcuts in `MainWindow.ui` are written as `Ctrl+S`, `Ctrl+N`, `Ctrl+O`, `Ctrl+W`, `Ctrl+Q`, ... ; Qt maps `Ctrl` to Cmd on macOS automatically. Only existing `Q_OS_MAC` code is `ZoneContextMenuInteraction`. | `MainWindow.ui`, grep | Mac menu work is mostly about `QAction::MenuRole` (About/Preferences/Quit), the native menu bar, `Ctrl+W`/Close semantics and status-tip text that says "Ctrl+", not about rewriting shortcuts. |

### Component Responsibilities (existing plus new)

| Component | Responsibility | Status / Notes |
|-----------|----------------|----------------|
| `foundation`, `math` | Value types, geometry, XML marshalling | Existing; Boost tests (`foundation_tests`, `math_tests`). No change except Qt-version cleanup in `foundation/Proximity.cpp`, `math/PolylineIntersector.cpp`. |
| `imageproc` | Pixel algorithms on `QImage`/`BinaryImage`/`GrayImage` | Existing; `imageproc_tests`. `TestBinarize.cpp` is `#if 0` and must be revived with synthetic input. |
| `dewarping` | Text-line tracing, distortion model, raster dewarp | Existing; **no tests**. Add `dewarping_tests` (new executable, no Qt app). |
| `core` (framework) | `Task` chain contracts, `Settings`, `ProjectPages`, I/O (`ImageLoader`, `TiffWriter`), project XML | Existing; `core_tests`, `qt_tests`. |
| `core/filters/<stage>` | Stage `Task` + `Settings` + `Params` + UI | Existing; `Task`/`Settings` are the headless seam, `Filter`/`OptionsWidget`/`ImageView`/`UiUpdater::updateUI` are GUI. |
| `core/filters/output/OutputGenerator` | The single biggest unit of pixel logic | Existing; split mechanically AFTER goldens exist (see Build Order). |
| `app` | `MainWindow`, dialogs, menus | Existing; Mac menu/shortcut work lands here. |
| **`src/test_support/`** (new static lib) | Synthetic page generator, image comparison metrics, golden store, temp-dir/QSettings fixture helpers | Depends on `Qt::Gui` + `imageproc` only (NOT on `core`), so `imageproc_tests` and `dewarping_tests` can use it without breaking layering. |
| **`src/pipeline_tests/`** (new executable) | Headless Task-chain builder, e2e scenarios, golden tests, XML round trip, `OutputGenerator` tests | Depends on `core` + `test_support`; Boost.Test with a global fixture creating `QApplication` (offscreen). |
| **`src/dewarping/tests/`** (new executable) | Dewarp tests on synthetically warped pages | Depends on `dewarping` + `test_support`; no Qt app. |

---

## Recommended Project Structure

```
src/
├── test_support/                  # NEW: pure QImage-level helpers, no core dependency
│   ├── CMakeLists.txt             # static lib `test_support` -> imageproc, Qt::Gui
│   ├── SyntheticPage.{h,cpp}      # deterministic page builder (flat, spread, mixed, curved)
│   ├── BitmapFont.{h,cpp}         # built-in 5x7 glyph table (no system fonts!)
│   ├── Warp.{h,cpp}               # analytic page-curl warp, independent of dewarping/
│   ├── ImageCompare.{h,cpp}       # metrics: mismatch ratio, edge-tolerant XOR, MAE, PSNR, p99.9
│   └── GoldenStore.{h,cpp}        # load/compare/update, writes actual+diff PNG on failure
├── pipeline_tests/                # NEW
│   ├── CMakeLists.txt             # add_test(... ENVIRONMENT QT_QPA_PLATFORM=offscreen)
│   ├── main.cpp                   # BOOST_TEST_MODULE + global fixture (QApplication, QSettings, tmp dir)
│   ├── HeadlessPipeline.{h,cpp}   # builds the Task chain with filter=nullptr (mirror of createCompositeTask)
│   ├── TestStageFixOrientation.cpp / TestStagePageSplit.cpp / TestStageDeskew.cpp
│   ├── TestStageSelectContent.cpp / TestStagePageLayout.cpp
│   ├── TestOutputGenerator.cpp    # direct OutputGenerator::process calls, per colour mode
│   ├── TestPipelineScenarios.cpp  # flat doc / book spread / mixed / curved, golden-compared
│   ├── TestProjectRoundTrip.cpp   # StageSequence + ProjectWriter/Reader incl. special-character paths
│   └── golden/                    # small lossless PNGs + MANIFEST.txt (Qt version, commit, date)
├── dewarping/tests/               # NEW: TestDewarpRoundTrip.cpp, TestDistortionModelBuilder.cpp
├── imageproc/tests/               # EXISTING: revive TestBinarize.cpp, add TestSkewFinder on synthetic
├── core/tests/                    # EXISTING: add TIFF-dimension-limit and cancellation tests (no Qt app)
├── qt_tests/                      # EXISTING: add ENVIRONMENT=offscreen to its add_test
└── core/filters/output/
    ├── OutputGenerator.cpp        # after split: public API + Processor orchestration
    ├── OutputGeneratorProcessor.h # private header: `class OutputGenerator::Processor` declaration
    ├── OutputGeneratorDewarp.cpp  # Processor members: processWithDewarping, dewarp, build*DistortionModel
    ├── OutputGeneratorBinarize.cpp# Processor members: binarize*, threshold, smoothing, despeckle
    ├── OutputGeneratorPictures.cpp# Processor members: detectPictures, processPictureZones, segment/posterize
    └── OutputGeneratorUtils.{h,cpp} # the ~850 lines of anonymous-namespace free functions, now testable
```

### Structure Rationale

- **`test_support` below `core`:** `imageproc` and `dewarping` must stay free of `core`. A support lib that depends only on `Qt::Gui` + `imageproc` lets lower layers share the synthetic generator and the comparison metrics without inverting the layer order (foundation -> math -> imageproc -> dewarping -> core -> app).
- **`pipeline_tests` as a separate binary:** `core_tests` has a bare Boost `main` with no `QApplication`; adding a global `QApplication` fixture there would change the environment for all existing tests. A new binary isolates the offscreen-platform requirement and the temp-dir/QSettings fixture.
- **Goldens inside `pipeline_tests/golden/`:** next to the tests that own them, small (synthetic ~1200x1700 px, PNG, a few KB for bi-level), so they are cheap to commit and diff.
- **`OutputGenerator*` split keeps one class:** the first split moves member-function definitions across translation units while `Processor` stays one class declared in a private header. No signature, no ownership, no behaviour changes, which is the only kind of split safe enough for a 2814-line file whose members share ~20 fields.

---

## Architectural Patterns

### Pattern 1: Headless Task chain (filter = nullptr)

**What:** A `HeadlessPipeline` helper constructs the six `Task`s exactly like `MainWindow::createCompositeTask` (`MainWindow.cpp:2059-2101`) but passes `nullptr` for the `Filter`, and owns the `Settings` objects so tests can assert on them.
**When to use:** Every stage-level and pipeline-level test. The only place `Filter` is required is the project round trip.
**Trade-offs:** `Filter::createTask` wiring (settings sharing) is re-implemented in the helper; mitigated by one e2e test that uses a real `StageSequence` and asserts identical output to the helper path.

```cpp
// pipeline_tests/HeadlessPipeline.h (sketch)
struct HeadlessPipeline {
  std::shared_ptr<ProjectPages> pages;
  std::shared_ptr<fix_orientation::Settings> fixOrientation;
  std::shared_ptr<ImageSettings> imageSettings;
  std::shared_ptr<page_split::Settings> pageSplit;
  std::shared_ptr<deskew::Settings> deskew;
  std::shared_ptr<select_content::Settings> selectContent;
  std::shared_ptr<page_layout::Settings> pageLayout;
  std::shared_ptr<output::Settings> output;
  std::shared_ptr<ThumbnailPixmapCache> thumbs;   // real cache in tmp dir
  OutputFileNameGenerator outNames;               // outDir = tmp/out, tmp/out/cache pre-created

  // Runs stages [0..lastStageIdx]; returns the terminal FilterResultPtr (discarded by most tests).
  FilterResultPtr run(const QImage& scan, const PageInfo& page, int lastStageIdx);
};

// Build order mirrors createCompositeTask: output -> page_layout -> ... -> fix_orientation
auto outputTask = std::make_shared<output::Task>(nullptr, output, thumbs, id, outNames, TAB_OUTPUT, /*batch*/true, /*debug*/false);
auto layoutTask = std::make_shared<page_layout::Task>(nullptr, pageLayout, id, outputTask, true, false);
// ... select_content, deskew, page_split(nullptr, pageSplit, pages, deskewTask, pageInfo, ...), fix_orientation
fixTask->process(NullTaskStatus(), FilterData(scan));
```

Use `batch = true` so UI-only branches (e.g. `needPictureEditor`, `needSpecklesImage`) follow the batch path; run a second variant with `batch = false` for mixed mode to exercise the automask/speckles files.

### Pattern 2: Ground-truth-first assertions, goldens second

**What:** The synthetic generator knows the truth (rotation angle, gutter x, content rectangle, ink mask, curl amplitude). Assert against truth with tolerances first; use committed golden images only as a regression net for pixel output that has no analytic truth (colour processing, despeckle, picture/text mix).
**When to use:** Always. Truth assertions survive Qt upgrades and compiler flag changes; goldens do not necessarily.
**Trade-offs:** More assertion code per scenario; but a failing truth assertion is a real bug, while a failing golden might be benign churn.

| Stage | Truth from generator | Assertion (tolerance) |
|-------|----------------------|-----------------------|
| deskew | rotation theta applied by generator | `abs(detected + theta) <= 0.15 deg` for theta in {-3, -1.5, 0.5, 2}; read from `deskew::Settings::getPageParams` |
| page_split | gutter x of a two-page spread | split line within +/- 0.5% of page width; layout type = two-page |
| select_content | bounding rect of text/picture blocks | each edge within +/- 3 px at 300 dpi, or IoU >= 0.97 |
| page_layout | margins in mm requested | resulting page rect exact to 0.01 mm (pure arithmetic) |
| output size | derived from page rect and DPI | exact `QSize` equality |
| binarize | generator's ink mask | pixel mismatch <= 0.5% of ink bounding area; ink-IoU >= 0.9 |
| dewarp | generator's analytic warp | text-baseline straightness: max residual of baselines from best-fit lines <= 1.5 px at 300 dpi |

### Pattern 3: Edge-tolerant golden comparison with explicit metrics per image kind

**What:** One `GoldenStore::compare(actual, goldenName, Tolerance)` function with three metric families, chosen per scenario; on failure it writes `actual.png` and `diff.png` into `${CMAKE_BINARY_DIR}/test_artifacts/<test>/` and prints the metric values.
**When to use:** All golden checks.

| Image kind | Metric | Starting tolerance (tune on first run, then freeze) |
|------------|--------|------------------------------------------------------|
| Bi-level (B/W output, masks) | Edge-tolerant XOR: a differing pixel counts only if the opposite colour is absent within a 1 px neighbourhood in the other image | <= 0.1% of pixels; plus foreground connected-component count within +/- 2% |
| Greyscale / colour | Mean absolute error and 99.9th-percentile absolute difference | MAE <= 1.5/255; p99.9 <= 24/255; fraction with diff > 16 <= 0.5% |
| Dewarped / resampled | PSNR plus the geometric truth assertion above | PSNR >= 30 dB; geometry assertion is the real gate |
| Geometry (sizes, rects, DPI) | Exact, or numeric tolerance as in the table above | exact for integers; <= 1e-6 relative for doubles from pure arithmetic |

Rules: compare after normalising both images to `Format_Grayscale8` or `Format_RGB32`; assert DPI/size separately (never inside the pixel metric); never compare TIFF bytes (libtiff version, strip layout and compression make bytes unstable).

### Pattern 4: Deterministic synthetic pages without fonts

**What:** `SyntheticPage` composes pages from primitives it fully controls: a built-in 5x7 bitmap font scaled to a configurable x-height for "text lines" (guarantees line structure for `SkewFinder`, `TextLineTracer`, `ContentBoxFinder`), filled rectangles and a gradient-plus-noise block for pictures (so mixed-mode picture detection triggers), a paper background with a smooth illumination gradient, optional dark scanner border, optional gutter shadow, noise from an explicit `std::mt19937(seed)`.
**Why:** `QPainter::drawText` depends on installed fonts and Qt text shaping, which differs across macOS versions and Qt releases; `rand()` and `QRandomGenerator::global()` are not reproducible. Independent rotation and curl use `QTransform` and `Warp.cpp` (not `imageproc::transform` and not `dewarping::`), so the code under test is never used to build its own input.
**Scenarios (match the "books, flat documents, mixed pages" requirement):** S1 flat page rotated 1.5 deg with scanner border; S2 two-page spread, 1 deg tilt, gutter shadow; S3 text plus picture block (mixed), light illumination gradient; S4 curved page (analytic curl, 2 text columns); S5 colour page with tinted paper. Each runs under B/W-Otsu, B/W-Sauvola, B/W-Wolf, greyscale and colour where meaningful. Keep inputs around 1200x1700 px at 300 dpi to bound runtime.

### Pattern 5: Zero-behaviour-change proof for mechanical edits

**What:** For purely preprocessor-driven edits (dropping the Qt 5 branches, Windows branches), compare the compiler's preprocessed output before and after for each touched translation unit: `clang++ -E -P <flags> file.cpp`. If the output is byte-identical, the edit provably cannot change behaviour on this machine.
**When to use:** The ~19 files with `QT_VERSION` checks and the ~8 files with Windows branches; run in the same phase, no goldens needed for that proof (goldens remain the backstop).
**Trade-offs:** Needs the build's `compile_commands.json` (`-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`); header-only changes are covered because `-E` expands includes.

### Pattern 6: Split by relocating member definitions first, extracting classes later

**What:** Step 1 moves definitions into new `.cpp` files without touching declarations; Step 2 (optional, only where a real boundary exists) extracts a class. Suggested cut lines in `OutputGenerator.cpp` (from the current function map):

| New file | Source lines (current) | Content |
|----------|------------------------|---------|
| `OutputGeneratorUtils.{h,cpp}` | 365-1213 | Free functions currently in anonymous namespaces: `reserveBlackAndWhite`, `fillExcept`, `fillMarginsInPlace`, `applyFillZonesInPlace`, `findRectAreas`, `applyAffineTransform`, `smoothToGrayscale`, `from300dpi`/`to300dpi`, spline-point helpers. Pure and directly unit-testable once exposed in `namespace output::detail`. |
| `OutputGeneratorDewarp.cpp` | 1532-1892, 2038-2107, 2608-2813 | `processWithDewarping`, `setupTrivialDistortionModel`, `createDewarper`, `dewarp`, `buildAutoDistortionModel`, `buildMarginalDistortionModel` |
| `OutputGeneratorBinarize.cpp` | 1928-2035, 2186-2500 | `estimateBinarizationMask`, `modifyBinarizationMask`, `adjustThreshold`, `calcBinarizationThreshold`, `morphologicalSmoothInPlace`, `binarize*`, `maybeDespeckleInPlace` |
| `OutputGeneratorPictures.cpp` | 2108-2185, 2502-2582 | `detectPictures`, `segmentImage`, `posterizeImage`, `processPictureZones`, `getForegroundType` |
| `OutputGenerator.cpp` (rest) | 1-364, 1215-1531 | ctor, `process`, `Processor` setup, `processWithoutDewarping` (~260 lines) |

The same approach applies to `MainWindow.cpp` (2245 lines) and `ThumbnailSequence.cpp` (1712), but only if the Mac-menu or Retina work touches them; they have no golden safety net, so keep those moves strictly mechanical and review by `git diff --color-moved`.

---

## Data Flow

### Production flow (unchanged by this work)

```
image file
   |  ImageLoader::load (TIFF/PNG/JPEG)
   v
LoadFileTask (BackgroundTask, worker thread)
   | convertToSupportedFormat, overrideDpi, ThumbnailPixmapCache::ensureThumbnailExists
   v   FilterData(QImage)                         (carries origImage, grayImage, xform, bw threshold)
fix_orientation::Task  -- rotation/trim -------> xform.preRotation / preCropArea
   v   FilterData(data, xform)
page_split::Task       -- PageLayoutEstimator --> page_split::Settings (layout, split line)
   v
deskew::Task           -- SkewFinder ----------> deskew::Settings (angle) ; xform.postRotation
   v
select_content::Task   -- PageFinder, ContentBoxFinder -> select_content::Settings (page/content rect)
   v   (data, pageRect, contentRect)
page_layout::Task      -- margins, alignment ---> page_layout::Settings ; xform.postCropArea
   v   (data, contentRectPhys)
output::Task           -- OutputGenerator::process -> TIFF(s) in $OUT, output::Settings (OutputParams)
   v
UiUpdater (FilterResult) --post to GUI thread--> updateUI(FilterUiInterface*)   <-- GUI only
```

Stages communicate through (a) the `FilterData`/`ImageTransformation` value passed down the chain and (b) the per-stage `Settings` objects. The harness taps both: `Settings` for per-stage assertions, the output TIFF for the pixel assertions.

### Test flow

```
SyntheticPage(scenario, seed)  ----> QImage + Truth{angle, gutterX, contentRect, inkMask, curl}
        |  (written to tmp/in/page.tif via TiffWriter, so ImageLoader path is exercised)
        v
HeadlessPipeline.run(img, page, lastStage)
        |                      \
        v                       v
 stage Settings (angles,     tmp/out/page.tif  (ImageLoader::load)
 rects, layout)                  |
        |                        v
        v                  GoldenStore::compare(actual, "flat_bw_sauvola.png", tol)
 truth assertions                |  fail -> test_artifacts/<test>/{actual,diff}.png
```

### Where a Qt application object is actually needed

| Code under test | Needs | Reason |
|-----------------|-------|--------|
| `math`, `foundation`, `imageproc`, `dewarping`, `ImageLoader`, `TiffWriter`/`TiffReader`, `ProjectPages`, stage `Settings`/`Params`, `OutputGenerator::process` | Nothing (plain `main`) | `QImage` and `QPainter` on a `QImage` work without an application; only fonts/`QPixmap`/widgets need one. |
| Stage `Task::process` with `filter = nullptr` | Nothing, except `ApplicationSettings` needs QSettings isolation | See fixture. |
| `ThumbnailPixmapCache`, hence `output::Task` and `LoadFileTask` | `QCoreApplication` at minimum (thread-affinity asserts); use the same `QApplication` as below | Constructed in GUI thread, spawns `QThread`. |
| `StageSequence`/`Filter`/`OptionsWidget`, `ProjectWriter`/`ProjectReader` tests, `ErrorWidget`, `qt_tests` | `QApplication` with `QT_QPA_PLATFORM=offscreen` | Widgets are constructed. |
| `ImageViewBase`, thumbnails, icon packs, Retina logic | `QApplication` offscreen for construction only; real rendering is manual | `offscreen` has DPR 1 unless `QT_SCALE_FACTOR=2` is set (LOW confidence it exercises true HiDPI paths; treat as smoke only). |
| `MainWindow`, menus | Manual | Not worth automating for a one-user fork. |

### Test fixture contract (global)

```cpp
// pipeline_tests/main.cpp (sketch)
struct GlobalFixture {
  GlobalFixture() {
    qputenv("QT_QPA_PLATFORM", "offscreen");               // before QApplication; CTest ENVIRONMENT also sets it
    static int argc = 1; static char arg0[] = "pipeline_tests"; static char* argv[] = {arg0, nullptr};
    app = std::make_unique<QApplication>(argc, argv);
    QCoreApplication::setOrganizationName("scantailor-tests");
    QCoreApplication::setApplicationName("pipeline_tests");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tmp.path());  // BEFORE ApplicationSettings::getInstance()
    // pin the settings the Tasks read: black-on-white detection etc.
  }
  QTemporaryDir tmp; std::unique_ptr<QApplication> app;
};
BOOST_TEST_GLOBAL_FIXTURE(GlobalFixture);
```

---

## Integration of each target change

| Change | Files / boundary | Interaction with other changes | Protection |
|--------|------------------|--------------------------------|------------|
| **Qt 6 only** | 19 files with `QT_VERSION` checks (`app/main.cpp`, `core/Application.cpp`, `BackgroundTask.h`, `ImageViewBase.{h,cpp}`, `ZoomHandler.cpp`, `RelinkablePath.cpp`, `page_split/{ImageView,PageLayout,PageLayoutAdapter}.cpp`, `output/DewarpingView.cpp`, `dewarping/{Curve,CylindricalSurfaceDewarper,DetectVertContentBounds,DistortionModelBuilder,TextLineTracer}.cpp`, `foundation/Proximity.cpp`, `math/PolylineIntersector.cpp`, `app/RelinkablePathVisualization.cpp`) plus top-level and `core/CMakeLists.txt` (`if(Qt6_FOUND)` duplicates). Delete the Qt 5 side of each conditional, delete the `Qt_*_lib` indirection and `qt_min_version`. | Shares files with Retina work (`ImageViewBase`, `main.cpp`) and with `dewarping/` that the goldens cover; must land BEFORE Retina to avoid editing the same hunks twice. | Pattern 5 (`-E` diff) plus goldens (dewarping RANSAC branch is the one place a wrong deletion would change numeric output). |
| **Remove Linux/Windows** | `.github/workflows/*`, `flatpak/`, `build-deb.sh`, `build-windows.sh`, `src/resources/{unix,win32}`, `cmake/{CopyToBuildDir,AddDynamicLibraryLocations}.cmake`, big `if (WIN32)` blocks and CPack/NSIS in top-level `CMakeLists.txt`, `WIN_XP`, `DEBUG_CLI`, Windows branches in `app/main.cpp`, `MainWindow.cpp`, `core/Utils.cpp`, `RelinkablePath.cpp`, `{Dark,Light}ColorScheme.cpp`, `imageproc/ByteOrder.h`, `app/CMakeLists.txt` (win32 rc, windeployqt). | On macOS the preprocessor already ignores these branches, so behaviour is invariant. Keep `UNIX`-style CMake that also works on Darwin; drop `desktop`/mime installs (Linux). Do not delete `src/resources/unix/mime` until it is clear nothing references it. Delete CI first (zero risk). | Pattern 5 (`-E` identity), full build, all tests. |
| **macOS build via Homebrew** | Top-level `CMakeLists.txt`: bump `cmake_minimum_required` (3.9 is below what current Homebrew CMake supports without policy warnings), handle CMake 3.30+ `FindBoost` policy (CMP0167, use `CONFIG` mode `find_package(Boost CONFIG ...)`), `CMAKE_PREFIX_PATH` for Homebrew Qt/Boost/libtiff/libjpeg, `add_definitions(-DBOOST_TEST_DYN_LINK)` for dynamic Boost.Test, `TIFF`/`JPEG` imported targets, `-Wall -Wextra` only applied for GCC (AppleClang is skipped, consider enabling for clang). | Needed first; everything else assumes a green local build. The earlier failed attempt means the first build will surface unknown breakage (MEDIUM: expect Boost.Test/CMake-policy and deprecation issues; not verified). | `ctest` baseline run. |
| **Retina** | `ImageViewBase` (`HqTransformTask` target size and transform scaled by `devicePixelRatioF()`, result `QPixmap::setDevicePixelRatio`, position in logical coords, `validateHqPixmap` compares logical transform), `ThumbnailBase` (`tempPixmap` and the painter in device pixels, DPR set), `ThumbnailPixmapCache` (thumbnail `QImage` rendered at `maxSize * DPR`, DPR set on the pixmap; thumb file path is keyed by size so the old cache is simply orphaned), `PixmapRenderer::drawPixmap` (uses `painter.viewport()` vs pixmap rect; check with DPR), icon packs (`StyledIconPack`, `FontIconPack`, `IconProvider`: SVG/vector or `@2x` assets, `QIcon::pixmap(size, dpr)`), `main.cpp` (delete `AA_EnableHighDpiScaling`/`AA_UseHighDpiPixmaps`, keep `PassThrough` rounding policy). | Strictly view layer; no `Task::process` or `imageproc` edits. Must come after Qt6-only (same hunks in `ImageViewBase`, `main.cpp`). Extract the geometry decision into a pure function (e.g. `hqTargetRect(xform, imageRect, targetSizeLogical, dpr)`) so it can be unit-tested in `qt_tests` without rendering. | Pure-function unit tests; goldens prove pipeline output unchanged; visual check on the Retina display. |
| **Mac menus and shortcuts** | `MainWindow.ui` / `MainWindow.cpp`: `QAction::setMenuRole` for About / Preferences (SettingsDialog) / Quit, `Qt::AA_DontUseNativeMenuBar` must not be set, `Ctrl+W` close semantics (Cmd+W closes project vs window), remove literal "Ctrl+" text in tooltips (`MainWindow.ui` lines ~425, 482, 549) or format with `QKeySequence::toString(NativeText)`, `ZoneContextMenuInteraction` `Q_OS_MAC` branches keep. | Depends on the pruned `MainWindow.cpp` (Windows branches gone) and benefits from a smaller file; independent of Retina. | Manual checklist; `qt_tests` can assert `QAction::shortcut()`/`menuRole()` for the action set if `MainWindow` is constructible offscreen (MEDIUM; verify). |
| **Split oversized files** | See Pattern 6. | `OutputGenerator.cpp` after goldens; `MainWindow.cpp` after Mac menus (otherwise the menu diff is buried in a move); `ThumbnailSequence.cpp`, `ContentBoxFinder.cpp` only if touched. | Goldens (output), `git diff --color-moved` (GUI). |
| **Harness** | New dirs above; `src/CMakeLists.txt` adds `test_support`, `pipeline_tests`, `dewarping/tests` under `BUILD_TESTS`. | Must precede refactors; depends only on a green baseline build. | n/a |

---

## Suggested Build Order

Dependency graph (arrow = "must come before"):

```
P0 baseline build + CI removal
   -> P1 test_support + L0/L1 tests (imageproc, dewarping, I/O)   [no pipeline code touched]
      -> P2 pipeline harness + goldens generated on UNCHANGED code (Qt5 branches still present)
         -> P3 mechanical pruning: Qt6-only, Windows/Linux removal  [proof: -E identity + goldens]
            -> P4 Mac view work: Retina, then Mac menus/shortcuts   [proof: goldens unchanged + manual]
            -> P5 safe splits: OutputGenerator (golden-guarded), MainWindow (after P4 menus)
               -> P6 debt fixes that need tests: std::system alarm, TIFF dimension cap, XML special chars
```

P4 and P5(OutputGenerator) are independent and can be reordered; P5(MainWindow) must follow P4 menus.

### P0 - Baseline: build green, CI off, record state
- Fix only what blocks a build and `ctest` on Apple Silicon with Homebrew Qt 6/Boost (CMake policy, Boost.Test linkage, include paths). Do NOT prune Qt 5 or Windows code yet.
- Delete `.github/workflows/*` now (no code interaction).
- Run `ctest` and record which tests pass or fail today; known: `qt_tests` needs a display, so set `ENVIRONMENT QT_QPA_PLATFORM=offscreen` on its `add_test`. Record failures as baseline, do not fix unrelated ones silently.
- Export `compile_commands.json` and save the `-E` outputs of the to-be-touched translation units for Pattern 5.
- Why first: every later claim of "unchanged" needs a known-good reference build; the user's earlier macOS attempt failed, so the unknowns are largest here.

### P1 - Test support library and the no-GUI test layers
- `test_support` (SyntheticPage, BitmapFont, Warp, ImageCompare, GoldenStore).
- Revive `TestBinarize.cpp` (replace the `#if 0` block with synthetic ground-truth tests for Otsu, Sauvola, Wolf).
- `SkewFinder` tests on rotated synthetic text; `dewarping_tests` (build a warped synthetic page with `Warp`, run `DistortionModelBuilder`/`TextLineTracer`, assert baseline straightness after `RasterDewarper`).
- `core_tests` additions needing no app: TIFF reader dimension cap (security debt), cancellation (`TaskStatus` stub throwing after N checks).
- Why before the pipeline harness: it validates the generator and metrics at the cheapest level, where failures are diagnosable; the pipeline harness then reuses trusted components.

### P2 - Pipeline harness and goldens on untouched code
- `HeadlessPipeline`, global fixture, per-stage truth tests, `TestOutputGenerator` (direct calls, each colour mode, with and without picture/fill zones, dewarping auto/manual/off), `TestPipelineScenarios` S1-S5, `TestProjectRoundTrip` with paths containing `&`, `<`, quotes, non-ASCII.
- Generate goldens on the P0 build in the Release configuration; commit with `MANIFEST.txt` (Qt version, Homebrew Qt build string, commit hash, config).
- Why before pruning and splitting: these goldens are characterisation tests of today's behaviour, which the PROJECT constraint ("output must not change unintentionally") requires; every later phase is verified against them.

### P3 - Mechanical pruning (Qt 6 only, Windows/Linux)
- Do Qt6-only and platform pruning in two commits, each verified by `-E` identity per translation unit, then full `ctest`.
- Why after P2: deleting the wrong half of `#if QT_VERSION_MAJOR == 5 && MINOR <= 9 ... #else` in `DistortionModelBuilder.cpp` or `DetectVertContentBounds.cpp` is exactly the error the dewarp tests and goldens catch.
- Why before P4: same files (`ImageViewBase.{h,cpp}`, `main.cpp`, `MainWindow.cpp`) are edited by Retina/menu work; doing the dead-branch removal first keeps those diffs small and reviewable.

### P4 - Mac view-layer work
- Retina first (rendering correctness is the more visible problem), then menus/shortcuts.
- After each: `ctest` unchanged, goldens unchanged (they must not move at all, any change is a bug here), then manual check on the Retina display.

### P5 - Safe file splits
- `OutputGenerator.cpp` in the order: (1) free functions to `OutputGeneratorUtils` unchanged, (2) `Processor` declaration to a private header, (3) member definitions moved to the three topic files, (4) only then consider extracting real classes. Run goldens after each step.
- `MainWindow.cpp` split only after P4 menus; mechanical moves only.

### P6 - Debt that benefits from the new tests
- Replace `std::system` on the alarm string (use `QProcess` with argument list or drop the feature); unbounded TIFF dimensions (cap before allocation); XML special-character handling found by the round-trip test.

---

## Anti-Patterns

### Anti-Pattern 1: Generating goldens after the refactor
**What people do:** Write the harness and golden images in the same phase as (or after) the pruning/splitting.
**Why it's wrong:** The goldens then encode whatever the refactor produced, including mistakes; they stop being a regression net.
**Do this instead:** P2 goldens come from the P0 code; any intentional output change later is a separate, reviewed golden update.

### Anti-Pattern 2: Testing through `MainWindow`, `BackgroundExecutor` or `WorkerThreadPool`
**What people do:** Drive the pipeline by constructing the window or queueing `BackgroundTask`s and waiting for events.
**Why it's wrong:** Event-loop timing, thread scheduling and widget construction make tests slow and flaky; the logic under test does not depend on them.
**Do this instead:** Call `Task::process` directly with `NullTaskStatus`; test the threading code separately, if at all.

### Anti-Pattern 3: Real fonts, global RNG and the code-under-test in the generator
**What people do:** `drawText` for "text", `qrand()`, and `imageproc::transform` to make the skewed input.
**Why it's wrong:** Results vary with installed fonts and Qt releases, and a bug in `transform` hides itself.
**Do this instead:** Pattern 4 (bitmap font, seeded `std::mt19937`, independent `QTransform`/`Warp.cpp`).

### Anti-Pattern 4: Byte-exact image goldens
**What people do:** Compare output file hashes or exact pixel equality.
**Why it's wrong:** Qt resampling, FMA contraction (arm64 clang contracts by default), libtiff upgrades and Debug vs Release can move a handful of pixels; the suite becomes a nuisance and gets disabled.
**Do this instead:** Edge-tolerant metrics with frozen, documented tolerances (Pattern 3); generate and compare in Release only.

### Anti-Pattern 5: Letting tests read the user's real preferences
**What people do:** Run `Task`s without initialising QSettings.
**Why it's wrong:** `ApplicationSettings` feeds `deskew::Task` and `OutputGenerator`; results then differ per machine and per user.
**Do this instead:** The global fixture above, plus an assertion at fixture start that `ApplicationSettings` values equal known defaults.

### Anti-Pattern 6: Re-architecting while splitting
**What people do:** Extract `Processor` into independent classes with new ownership while moving code.
**Why it's wrong:** `Processor` shares ~20 members (`m_settings`, `m_dbg`, `m_status`, DPI/area state); extraction changes lifetime and ordering semantics in a file that has no unit tests today.
**Do this instead:** Pattern 6, step 1 only until the harness covers each path; extract classes only where a seam is proven by a test.

### Anti-Pattern 7: Changing the PRNG in dewarping "for Qt independence" during this work
**What people do:** Swap `QRandomGenerator(0)` for `std::mt19937` to escape Qt-version dependence.
**Why it's wrong:** Changes dewarp output for existing settings, contradicting the "no unintentional output change" constraint, and is indistinguishable from a regression in the middle of other changes.
**Do this instead:** Leave as is, record the Qt build string in `golden/MANIFEST.txt`, and rely on the geometric truth assertion if a Qt upgrade moves the pixels.

---

## Scaling Considerations (test runtime rather than users)

| Concern | Now (about 200 Boost cases) | With harness | If it grows |
|---------|------------------------------|--------------|--------------|
| Runtime | Seconds | Each pipeline scenario at 1200x1700 px, 300 dpi: expected 0.2-2 s per page per colour mode (Release); Debug can be 10x slower | Tag slow scenarios (`--run_test=!slow`), keep default `ctest` under about 1 minute |
| Repo size | Negligible | About 20-40 PNG goldens, each a few KB to ~100 KB | Keep inputs generated, never store them; store only outputs |
| Flakiness | None | Offscreen plugin availability; thumbnail `QThread` shutdown | Destroy `ThumbnailPixmapCache` before `QApplication`; avoid event-loop waits |

### Scaling Priorities
1. **First bottleneck:** Debug builds (asserts plus no optimisation) make `OutputGenerator` tests slow; define a `RelWithDebInfo` test config and document that goldens are Release-only.
2. **Second bottleneck:** Golden churn after `brew upgrade qt`; mitigate with truth-first assertions and the `SCANTAILOR_UPDATE_GOLDEN=1` regenerate flow (rewrites goldens, prints metrics, requires manual `git diff` review).

---

## Integration Points

### External dependencies relevant to the seams

| Dependency | Integration pattern | Notes |
|------------|---------------------|-------|
| Qt 6 (Homebrew) | `find_package(Qt6 COMPONENTS Core Gui Widgets Xml Network OpenGL OpenGLWidgets Svg Test LinguistTools)` | After pruning, collapse `Qt_*_lib` variables to `Qt6::...` targets. `OpenGLSupport.cpp` uses `QOpenGLContext`; macOS OpenGL is deprecated but still functional; keep, and do not call it in tests. |
| Boost.Test (Homebrew) | Dynamic library plus `BOOST_TEST_DYN_LINK` | Existing `BOOST_AUTO_TEST_MAIN` define in each `main.cpp` is old style; new binaries use `BOOST_TEST_MODULE` with `#include <boost/test/unit_test.hpp>`. Keep existing ones as they are unless they fail to build. |
| libtiff / libpng / libjpeg | Imported targets | `TiffWriter` is the output path under test; compare via `ImageLoader::load`, not bytes. |
| `QSettings` | INI format under test temp path | See fixture. |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| `test_support` -> `imageproc` | Direct link | Must not link `core` (layering). |
| `pipeline_tests` -> `core` | Direct link of stage `Task`/`Settings` headers (`filters/<stage>/Task.h`, `Settings.h`) | Stage headers are already include roots; no new public API except possibly `Filter::settings()` accessors, which are NOT needed because the harness owns the `Settings` objects. |
| `Task` -> `Filter` | `shared_ptr<Filter>` held only for `UiUpdater` | Harness passes `nullptr`; do not add null checks to production code. If a future `Task::process` starts using `m_filter`, the headless tests will crash, which is the desired early warning. |
| `output::Task` -> filesystem | Direct `QFile`/`QDir`/`TiffWriter` | Tests use per-test `QTemporaryDir`; no mocking layer. |
| `OutputGenerator` -> `output::Settings` | `shared_ptr<Settings>` passed in and mutated (`setParams`, `setPictureZones`, `setDewarpingOptions`, `setOutputProcessingParams`) | Tests must assert on those side effects too (auto picture zones, black-on-white flag, generated distortion model). |
| `app` -> `core` | `MainWindow::createCompositeTask` is the only production assembler of the chain | `HeadlessPipeline` duplicates its order; add a comment in both places pointing at each other. |

---

## Sources

- Source reading (HIGH): `src/core/filters/*/Task.{h,cpp}`, `src/core/filters/output/{Task.cpp,OutputGenerator.{h,cpp},Utils.cpp,Settings.cpp}`, `src/core/{FilterData.h,FilterResult.h,BackgroundTask.h,NullTaskStatus.h,LoadFileTask.cpp,StageSequence.{h,cpp},ThumbnailPixmapCache.{h,cpp},ImageViewBase.{h,cpp},ThumbnailBase.cpp,PixmapRenderer.cpp,ApplicationSettings.cpp}`, `src/app/{MainWindow.cpp:2059-2150,main.cpp,CMakeLists.txt}`, `src/dewarping/{DistortionModelBuilder.cpp,DetectVertContentBounds.cpp,CMakeLists.txt}`, `src/{core,imageproc,math,qt_tests}/tests/CMakeLists.txt`, `src/imageproc/tests/{main.cpp,TestBinarize.cpp,Utils.h}`, top-level `CMakeLists.txt`, `MainWindow.ui` shortcuts.
- Project context: `.planning/PROJECT.md`, `.planning/codebase/{ARCHITECTURE,STRUCTURE,TESTING}.md`.
- Not verified on this machine (MEDIUM/LOW): Homebrew Boost/CMake policy behaviour, `offscreen` platform HiDPI behaviour with `QT_SCALE_FACTOR`, whether `MainWindow` constructs under `offscreen` for action-level tests, runtime of scenarios, exact tolerance values (starting points to be tuned on the first golden run).

---
*Architecture research for: macOS port and test hardening of ScanTailor Advanced*
*Researched: 2026-10-18*
