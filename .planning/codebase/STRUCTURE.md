---
last_mapped_commit: 5eaac1884cdcabb6514bd632114f688631bd8dbc
last_mapped_at: 2026-10-18
---
# Codebase Structure

**Analysis Date:** 2026-10-18 (patched after the Phase 3 strip, D-10; not re-mapped)

## Directory Layout

```
scantailor-advanced/
├── src/                    # All C++ source code
│   ├── app/                # Executable: main window, dialogs, project flows
│   ├── core/               # Pipeline framework, page/project model, shared widgets
│   │   ├── filters/        # One sub-directory per processing stage
│   │   │   ├── fix_orientation/
│   │   │   ├── page_split/
│   │   │   ├── deskew/
│   │   │   ├── select_content/
│   │   │   ├── page_layout/
│   │   │   └── output/
│   │   ├── interaction/    # Mouse/drag/zoom interaction handlers
│   │   ├── zones/          # Zone and spline model for interactive editing
│   │   └── tests/          # core_tests suite
│   ├── imageproc/          # Pixel-level image algorithms (+ tests/)
│   ├── dewarping/          # Page-surface dewarping library (+ tests/, the dewarping_tests suite)
│   ├── math/               # Geometry and numerics (adiff/, spfit/, tests/)
│   ├── foundation/         # Generic utilities, XML marshalling, properties (tests/)
│   ├── qt_tests/           # Qt-dependent test suite
│   ├── pipeline_tests/     # Headless pipeline tests on generated pages (+ golden/{flat,book,mixed}/)
│   ├── test_support/       # Test-only library: page generator, image compare, truth metrics, golden store
│   └── resources/          # Icons, stylesheets (light_scheme/, dark_scheme/), icon tables
├── cmake/                  # CMake helper modules (ListItemsPrepend.cmake, SetDefaultBuildType.cmake)
├── .planning/codebase/     # Codebase analysis documents (this directory), patched not re-mapped after Phase 3
├── CMakeLists.txt          # Top-level build; adds foundation, math, imageproc, dewarping, core, app (+ test dirs)
├── CMakePresets.json       # Presets macos-release, macos-debug, macos-asan (configure, build, test)
├── config.h.in             # CMake template for config.h
├── version.h.in            # CMake template for version.h
├── .clang-format           # C++ formatting rules
├── README.md               # Build and run instructions for the Mac
├── TESTING.md              # Test instructions (English): suites, goldens, sanitizer build, ui_snapshot, formatting
└── LICENSE                 # GPLv3
```

## Directory Purposes

**`src/app/`:**
- Purpose: The GUI executable and everything that is only needed by the main window.
- Contains: `main.cpp`, `MainWindow.{h,cpp,ui}`, `NewOpenProjectPanel`, `ProjectCreationContext`, `ProjectOpeningContext`, `ThumbnailSequence`, relinking classes (`Relinking*.{h,cpp}`), settings and default-parameter dialogs, status bar and system load widgets, `.ui` files.
- Also holds `src/app/ui_snapshot/main.cpp`, the offscreen UI snapshot tool: it compiles the app sources without `main.cpp`, is built whenever `BUILD_TESTS` is on and is not registered with ctest.
- Key files: `src/app/main.cpp`, `src/app/MainWindow.cpp`, `src/app/CMakeLists.txt`.

**`src/core/`:**
- Purpose: Framework and domain logic shared by all stages.
- Contains: Base abstractions (`AbstractFilter.h`, `AbstractCommand.h`, `AbstractRelinker.h`, `AbstractIconPack.h`), pipeline (`StageSequence.{h,cpp}`, `FilterData`, `FilterResult.h`, `ProcessingTaskQueue`, `BackgroundTask`, `BackgroundExecutor`, `WorkerThreadPool`), page model (`PageSequence`, `PageInfo`, `PageId`, `ImageId`, `ProjectPages`, `PageOrderProvider`), persistence (`ProjectReader`, `ProjectWriter`, `RecentProjects`, `ApplicationSettings`), image I/O (`ImageLoader`, `ImageMetadataLoader`, `TiffReader`, `TiffWriter`, `JpegMetadataLoader`, `PngMetadataLoader`), view widgets (`BasicImageView`, `ImageViewBase`, `ThumbnailBase`, `ZoneEditorBase`), theming (`*ColorScheme*`, `FontIconPack`, `StyledIconPack`), and the `Units*` family.
- Key files: `src/core/Application.h`, `src/core/StageSequence.cpp`, `src/core/AbstractFilter.h`, `src/core/ProjectWriter.cpp`, `src/core/ProjectReader.cpp`, `src/core/ImageSettings.h`.

**`src/core/filters/<stage>/`:**
- Purpose: One processing stage per directory.
- Standard file set (present in each stage unless noted):
  - `Filter.{h,cpp}`: implements `AbstractFilter`, owns the stage's `Settings`.
  - `Task.{h,cpp}`: full per-page computation (`BackgroundTask`).
  - `CacheDrivenTask.{h,cpp}`: fast recomputation from cached parameters.
  - `Params.{h,cpp}`: immutable per-page parameters.
  - `Settings.{h,cpp}`: page-indexed store of `Params`.
  - `OptionsWidget.{h,cpp,ui}`: right-hand panel UI.
  - `ImageView.{h,cpp}`: central image view for the stage.
  - `Thumbnail.{h,cpp}`: thumbnail rendering.
  - `Utils.{h,cpp}`: helpers.
  - `ApplyDialog.{h,cpp,ui}`: "apply to pages" dialog.
  - `Dependencies.{h,cpp}` (where present): cross-stage inputs.
  - `CMakeLists.txt`: sources for the stage library.
- Stage-specific extras:
  - `deskew/`: `ObliqueFinder.{h,cpp}`.
  - `fix_orientation/`: `ImageTrim.{h,cpp}`, `DeskewExternalToolSpike.h`.
  - `output/`: large set including `OutputGenerator`, `OutputImageBuilder`, `OutputImage*`, `Binarization` options (`OptionsWidgetBinarization{Otsu,Sauvola,Wolf}`), `DewarpingOptions`/`DewarpingView`, `Despeckle*`, `Picture*`/`FillZone*` zone editors, `ColorPickupInteraction`, `ApplyColorsDialog`.
  - `page_split/`: `PageLayout*`, `VertLineFinder`, `SplitModeDialog`, `UnremoveButton`.
  - `page_layout/`: `Alignment`, `Guide`, `OrderByHeightProvider`, `OrderByWidthProvider`, `ApplyMarginsDialog`.
  - `select_content/`: `ContentBoxFinder`, `PageFinder`, `PhysSizeCalc`.

**`src/core/zones/`:**
- Purpose: Spline and zone data model used for picture, fill and content zones.
- Key files: `Zone.h`, `EditableZoneSet.h`, `EditableSpline.h`, `SerializableSpline.h`, `ZoneCreationInteraction.h`, `ZoneDefaultInteraction.h`.

**`src/core/interaction/`:**
- Purpose: Reusable input-handling state machines.
- Key files: `InteractionHandler.h`, `DragHandler.h`, `ObjectDragHandler.h`, `DraggablePoint.h`, `DraggablePolygon.h`, `InteractiveXSpline.h`, `ZoomHandler.h`.

**`src/imageproc/`:**
- Purpose: Image-processing primitives, independent of Qt widgets.
- Contains: binarization (`Binarize*`, `BinaryThreshold*`), connected components (`ConnComp*`, `ConnectivityMap*`, `ColorSegmenter*`), morphology and distance transforms (`Morphology*`, `SEDM*`, `SeedFill*`), skew and line detection (`SkewFinder*`, `RastLineFinder*`, `LineIntersect*`), geometry transforms (`Transform*`, `Scale*`, `Shear*`, `OrthogonalRotation*`, `PolygonRasterizer*`, `RasterOp*`), colour (`Grayscale*`, `ColorMixer.h`, `ColorInterpolation*`, `BackgroundColorCalculator*`), and debugging (`DebugImages.h`).
- Tests: `src/imageproc/tests/` (`imageproc_tests`).

**`src/dewarping/`:**
- Purpose: Curved-page dewarping.
- Key files: `CylindricalSurfaceDewarper.h`, `DistortionModelBuilder.h`, `TextLineTracer.h`, `TopBottomEdgeTracer.h`, `RasterDewarper.h`, `DewarpingPointMapper.h`.

**`src/math/`:**
- Purpose: Numerics and geometry.
- Subdirectories: `adiff/` (automatic differentiation), `spfit/` (spline fitting, 19 files), `tests/` (`math_tests`).

**`src/foundation/`:**
- Purpose: Generic containers, utilities, and XML marshalling.
- Key files: `VecT.h`, `MatT.h`, `Grid.h`, `Property.h`, `PropertySet.h`, `XmlMarshaller.h`, `XmlUnmarshaller.h`, `Span.h`, `FastQueue.h`, `PriorityQueue.h`, `StaticPool.h`, `DynamicPool.h`, `Proximity.h`, `Constants.h`.
- Tests: `src/foundation/tests/` (`foundation_tests`).

**`src/qt_tests/`:**
- Purpose: Tests that need a running Qt application (`qt_tests` binary).

**`src/pipeline_tests/` and `src/test_support/`:**
- Purpose: `pipeline_tests` runs the headless pipeline (`HeadlessPipeline.{h,cpp}`) on generated pages (flat document, book spread, mixed page, dewarping, project round trip, smoke run) and compares the output with goldens in `src/pipeline_tests/golden/{flat,book,mixed}/` (PNG and text goldens, `fixtures.txt`, `MANIFEST.txt`). `test_support` is the library behind it: `PageGenerator`, `ImageCompare`, `TruthMetrics`, `GoldenStore`, `Manifest`, `ExpectedFailure.h`.

**`src/dewarping/tests/`:**
- Purpose: The `dewarping_tests` suite (text line tracer, distortion model, cylindrical dewarper).

**`src/resources/`:**
- Purpose: Assets compiled into the app.
- Subdirectories: `icons/` (about 65 files), `light_scheme/` and `dark_scheme/` (stylesheets and their own `icons/`). The `unix/` desktop entry and `mime/` files and the `win32/` resources were removed in Phase 3.
- Top-level files: `resources.qrc`, `icons.xml`, `font-icons.xml`, `appicon.svg`, `GPLv3.html`.

**`src/translations/` (removed in Phase 3):**
- Removed in Phase 3 (STRIP-06), together with `cmake/UpdateTranslations.cmake`. `tr()` markers and `.ui` strings stay (D-06).

**`cmake/`:**
- Purpose: Helper scripts included from `CMakeLists.txt`.
- Key files: `ListItemsPrepend.cmake`, `SetDefaultBuildType.cmake`. `CopyToBuildDir.cmake`, `AddDynamicLibraryLocations.cmake`, `UpdateTranslations.cmake` and the uninstall script were removed in Phase 3.

**`CMakePresets.json`:**
- Purpose: Configure, build and test presets `macos-release`, `macos-debug` and `macos-asan`; binary directory `build/<preset>`.

**`.github/` (removed in Phase 3):**
- The workflows and the Dependabot file are gone in Phase 3; there is no CI. The gate is a local Release and Debug ctest run (`TESTING.md`).

## Key File Locations

**Entry Points:**
- `src/app/main.cpp`: process entry point.
- `src/app/MainWindow.cpp`: main window and project lifecycle.

**Configuration:**
- `CMakeLists.txt`: top-level build options (`BUILD_TESTS`, `DEVELOPER_VERSION`, `SANITIZE`). `CMakePresets.json`: the three macOS presets.
- `config.h.in`, `version.h.in`: generated headers.
- `.clang-format`: formatting rules.
- `src/app/CMakeLists.txt`, `src/core/CMakeLists.txt`: module build definitions.

**Core Logic:**
- `src/core/StageSequence.cpp`: stage order.
- `src/core/AbstractFilter.h`: stage contract.
- `src/core/filters/deskew/Task.cpp`, `src/core/filters/output/Task.cpp`: examples of per-page processing.
- `src/core/ProjectReader.cpp`, `src/core/ProjectWriter.cpp`: project XML persistence.

**Testing:**
- `src/core/tests/`: core unit tests (for example `TestDeskewParams.cpp`, `TestPageSequence.cpp`, `TestContentSpanFinder.cpp`).
- `src/imageproc/tests/`: image algorithm tests.
- `src/foundation/tests/`, `src/math/tests/`: utility and numeric tests.
- `src/qt_tests/`: Qt-based tests.
- `src/pipeline_tests/`, `src/test_support/`, `src/dewarping/tests/`: pipeline tests with goldens, their support library and the dewarping suite.
- `src/app/ui_snapshot/`: offscreen UI snapshot tool (not a ctest suite).
- `TESTING.md`: how to build and run tests (seven suites, presets `macos-release`, `macos-debug`, `macos-asan`).

## Naming Conventions

**Files:**
- C++ classes use PascalCase file names matching the class: `DeskewParams` in `Params.h` within its stage folder, or `ImageTransformation.h` at the root of `core`. Shared classes in `core` use PascalCase, for example `StageSequence.cpp`.
- Stage directories use `snake_case`: `fix_orientation`, `page_split`, `select_content`, `page_layout`.
- Per-stage standard names are fixed: `Filter`, `Task`, `CacheDrivenTask`, `Params`, `Settings`, `OptionsWidget`, `ImageView`, `Thumbnail`, `Utils`.
- Qt Designer forms share the class name: `OptionsWidget.ui` pairs with `OptionsWidget.h`.
- Test files use the `Test` prefix: `src/core/tests/TestDeskewParams.cpp`, `src/imageproc/tests/TestBinarize.cpp`.
- Multi-variant classes use a suffix: `OutputImagePlain`, `OutputImageWithForegroundMask`, `OptionsWidgetBinarizationOtsu`.

**Directories:**
- Module directories are lowercase single words: `app`, `core`, `imageproc`, `dewarping`, `math`, `foundation`.
- Stage directories are `snake_case` and match the stage name shown in the GUI, for example `page_split` is "Split Pages".
- Tests are in a `tests/` subdirectory of the module they cover.

**Include style:**
- Headers are included without a path prefix within a module (for example `#include "PageId.h"` in `src/core/`). Cross-module includes use the module name as root (for example `#include <core/Application.h>` in `src/app/main.cpp`).

## Where to Add New Code

**New processing stage:**
- Primary code: `src/core/filters/<new_stage>/` with the standard file set listed above.
- Register it in `src/core/StageSequence.cpp` (add a member, an index, and an `emplace_back` in the order you want).
- Add its `CMakeLists.txt` to `src/core/CMakeLists.txt` and include it in the `core` sources list.
- Tests: `src/core/tests/Test<NewStage>*.cpp` and add the file to `src/core/tests/CMakeLists.txt`.
- Reference implementation: `src/core/filters/deskew/`.

**New image algorithm:**
- Implementation: `src/imageproc/<Algorithm>.{h,cpp}` and add to `src/imageproc/CMakeLists.txt`.
- Tests: `src/imageproc/tests/Test<Algorithm>.cpp`.

**New dewarping logic:**
- Implementation: `src/dewarping/`.

**New math or geometry helper:**
- Implementation: `src/math/` (numerical) or `src/foundation/` (generic containers and utilities).

**New reusable widget or interaction:**
- Core widget: `src/core/<WidgetName>.{h,cpp}`.
- Interaction state machine: `src/core/interaction/`.
- Zone-related editing: `src/core/zones/`.

**New dialog or main-window feature:**
- Implementation: `src/app/<Dialog>.{h,cpp,ui}`, and add the `.ui` to `src/app/CMakeLists.txt`.

**Utilities:**
- Generic: `src/foundation/Utils.{h,cpp}`.
- Core-specific: `src/core/Utils.{h,cpp}`.
- Stage-specific: `src/core/filters/<stage>/Utils.{h,cpp}`.

## Special Directories

**`.planning/codebase/`:**
- Purpose: Generated codebase analysis documents consumed by GSD commands.
- Generated: Yes (by `/gsd-map-codebase`).
- Committed: Yes.

**`build/` (not tracked; created by the presets as `build/<preset>`):**
- Purpose: Out-of-source CMake build tree. In-source builds are rejected by `CMakeLists.txt`. Test settings, temporary files and failure artefacts also live here (`build/<preset>/test-tmp`, `test-artifacts`).
- Generated: Yes.
- Committed: No (ignored via `.gitignore`).

**`src/resources/icons/`, `src/resources/light_scheme/icons/`, `src/resources/dark_scheme/icons/`:**
- Purpose: Image assets bundled into the executable.
- Generated: No.
- Committed: Yes.

---

*Structure analysis: 2026-10-18; patched after the Phase 3 strip*
