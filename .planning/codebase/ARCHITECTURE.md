---
last_mapped_commit: 5eaac1884cdcabb6514bd632114f688631bd8dbc
last_mapped_at: 2026-10-18
---
<!-- refreshed: 2026-10-18 -->

# Architecture

**Analysis Date:** 2026-10-18 (patched after Phases 2 and 3, D-10; not re-mapped)

## System Overview

```text
┌─────────────────────────────────────────────────────────────┐
│                  Qt GUI application (src/app)                │
│  main.cpp → MainWindow, dialogs, thumbnails, project panels  │
├──────────────────┬──────────────────┬───────────────────────┤
│  Filter UIs      │  Project I/O     │  Batch / background    │
│ `core/filters/*` │ `core/ProjectRead│  `core/BackgroundExec- │
│  OptionsWidget   │  ProjectWrite`   │  utor`, `WorkerThread- │
│  ImageView       │                  │  Pool`, `ProcessingTask│
└────────┬─────────┴────────┬─────────┴──────────┬────────────┘
         │                  │                     │
         ▼                  ▼                     ▼
┌─────────────────────────────────────────────────────────────┐
│     Filter pipeline (core/StageSequence, AbstractFilter)     │
│  fix_orientation → page_split → deskew → select_content →    │
│  page_layout → output                                        │
│  `src/core/filters/<stage>/` (Filter, Task, CacheDrivenTask, │
│   Params, Settings, OptionsWidget, ImageView, Thumbnail)     │
└────────┬───────────────────────────────────┬────────────────┘
         │                                   │
         ▼                                   ▼
┌──────────────────────────────┐   ┌──────────────────────────┐
│ Image processing             │   │ Dewarping                 │
│ `src/imageproc/`             │   │ `src/dewarping/`          │
└──────────────┬───────────────┘   └──────────────────────────┘
               ▼
┌─────────────────────────────────────────────────────────────┐
│ Math and foundation: `src/math/`, `src/foundation/`          │
│ (vectors, matrices, splines, property sets, XML marshalling) │
└─────────────────────────────────────────────────────────────┘
```

Top-level CMake (`CMakeLists.txt`) adds subdirectories in this order: `foundation`, `math`, `imageproc`, `dewarping`, `core`, `app`; with `BUILD_TESTS` it adds `qt_tests`, `test_support`, `pipeline_tests` and `dewarping/tests`. Each is built as a library consumed by the layers above it.

## Pattern Overview

**Overall:** Layered desktop application with a linear, per-page filter pipeline. Each pipeline stage ("filter") owns its settings, a per-page task, a UI, and thumbnail generation. Stages communicate through a shared project model rather than calling each other directly.

**Key Characteristics:**
- The pipeline is fixed in order and defined in `src/core/StageSequence.cpp`: `fix_orientation`, `page_split`, `deskew`, `select_content`, `page_layout`, `output`. Each later stage consumes results of earlier ones.
- Every filter implements the `AbstractFilter` interface (`src/core/AbstractFilter.h`): `getName`, `getView`, `performRelinking`, `preUpdateUI`, `saveSettings`/`loadSettings`, `loadDefaultSettings`, and optional page-order hooks.
- Heavy per-page work runs off the GUI thread as `BackgroundTask` objects (`src/core/BackgroundTask.h`) scheduled via `BackgroundExecutor` and `WorkerThreadPool`. Results return to the GUI as events.
- Results are cached per page. Stages with no new computation use `CacheDrivenTask` (e.g. `src/core/filters/deskew/CacheDrivenTask.h`) to recompute only the cheap transformation or UI-facing data from cached parameters.
- Project state is persisted as XML (`ProjectWriter` / `ProjectReader`, root element `project`, with `filters`, `directories`, `files`, `images` sections).
- Settings live in per-filter `Settings` objects (e.g. `src/core/filters/deskew/Settings.h`) that map `PageId` to `Params`, guarded by a mutex-style interface, and in a global `ApplicationSettings` singleton (`src/core/ApplicationSettings.h`).

## Layers

**Application / GUI shell (`src/app/`):**
- Purpose: Program entry, main window, dialogs, project create/open flows, file relinking, status and load-progress UI.
- Location: `src/app/`
- Contains: `main.cpp`, `MainWindow.{h,cpp,ui}`, `ProjectCreationContext`, `ProjectOpeningContext`, `ThumbnailSequence`, `RelinkingModel`/`RelinkingDialog`, `SettingsDialog`, `DefaultParamsDialog`.
- Depends on: `core`, Qt Widgets.
- Used by: End users (it is the executable).

**Core domain and pipeline (`src/core/`):**
- Purpose: Filter pipeline, page and project model, background tasks, image loading and writing, shared widgets.
- Location: `src/core/`
- Contains: the generic framework (`AbstractFilter.h`, `StageSequence`, `PageSequence`, `ProjectPages`, `FilterData`, `FilterResult.h`, `BackgroundTask`, `ProcessingTaskQueue`, `ImageLoader`, `TiffWriter`, `ProjectReader/Writer`), plus the stage implementations under `src/core/filters/` and the interactive zone editing under `src/core/zones/` and `src/core/interaction/`.
- Depends on: `imageproc`, `dewarping`, `math`, `foundation`, Qt.
- Used by: `src/app/`.

**Filter stages (`src/core/filters/<stage>/`):**
- Purpose: One processing stage each. Each directory is a self-contained mini-module.
- Location: `src/core/filters/{fix_orientation,page_split,deskew,select_content,page_layout,output}/`
- Contains: a standard file set (see Key Abstractions below).
- Depends on: `core` base classes, `imageproc`, and sometimes other stages via a `Dependencies` class (e.g. `src/core/filters/deskew/Dependencies.h`, `src/core/filters/page_split/Dependencies.h`, `src/core/filters/select_content/Dependencies.h`).
- Used by: `StageSequence` and the app GUI.

**Image processing (`src/imageproc/`):**
- Purpose: Pixel-level algorithms: binarization, connected components, morphology, skew detection, scaling, rotation, shear, transforms, and the debug-image support.
- Location: `src/imageproc/`
- Contains: `Binarize`, `ConnComp*`, `SkewFinder`, `Transform`, `Scale`, `OrthogonalRotation`, `BinaryImage`, `Morphology`, `SEDM`, `RastLineFinder`, and others. Tests in `src/imageproc/tests/`.
- Depends on: `foundation`, `math`.
- Used by: `core/filters/*` and `dewarping`.

**Dewarping (`src/dewarping/`):**
- Purpose: Page-surface distortion modelling for curved book pages.
- Location: `src/dewarping/`
- Contains: `TextLineTracer`, `TopBottomEdgeTracer`, `CylindricalSurfaceDewarper`, `DistortionModel`, `DistortionModelBuilder`, `RasterDewarper`, `DewarpingPointMapper`, `Curve`.
- Depends on: `imageproc` (linked PUBLIC in `src/dewarping/CMakeLists.txt`).
- Used by: `core/filters/output/` (dewarping options and rendering).

**Math and foundation (`src/math/`, `src/foundation/`):**
- Purpose: Geometry and numerics; generic utilities.
- Location: `src/math/` (including `spfit/` spline fitting and `adiff/` automatic differentiation) and `src/foundation/`.
- Contains: `VecT.h`, `MatT.h`, `Grid.h`, `PriorityQueue.h`, `Property.h`/`PropertySet.h` (typed per-page parameter maps), `XmlMarshaller.cpp`/`XmlUnmarshaller.cpp`, `Span.h`, `StaticPool.h`.
- Depends on: nothing internal.
- Used by: all layers above.

## Data Flow

### Primary Request Path (opening a project and processing a page)

1. `main()` creates `Application` and `MainWindow`, optionally opening a project passed on the command line (`src/app/main.cpp`).
2. `MainWindow` holds the `ProjectPages` (`m_pages`) and `StageSequence` (`m_stages`) (`src/app/MainWindow.cpp`, lines ~104–105). Opening a project deserializes via `ProjectReader` and rebuilds the stages.
3. Loading a page's image creates a `LoadFileTask` (`src/core/LoadFileTask.h`), which produces an `ImageInfo` and a thumbnail.
4. When a user changes a parameter or a stage is run, a stage's `Task` (e.g. `src/core/filters/deskew/Task.h`, method `process(TaskStatus, FilterData)`) is queued through `ProcessingTaskQueue` and executed by the `BackgroundExecutor` / `WorkerThreadPool`.
5. The task returns a `FilterResult` (`src/core/FilterResult.h`). The result is delivered back to the GUI thread, which updates the `ImageView` and `Thumbnail` for the stage (`src/core/filters/<stage>/ImageView.h`, `Thumbnail.h`).
6. For pages whose settings have not changed, `CacheDrivenTask` recomputes only the cheap transform/UI data from the stored `Params` and `ImageTransformation` instead of re-running the full `Task`.
7. The `output` stage (`src/core/filters/output/Task.h`, `OutputGenerator.h`, `OutputImageBuilder.h`) composes the final image and writes it using `TiffWriter` or the image writers.

### Settings Change Flow

1. A stage's `OptionsWidget` (`src/core/filters/<stage>/OptionsWidget.h`) edits parameters for the selected page or a page range.
2. `Settings::setPageParams` / `setDegrees` (per-stage `Settings.h`) stores the new `Params` keyed by `PageId`.
3. `StageSequence` and dependent stages are invalidated; downstream stages re-run their `Task` or `CacheDrivenTask` for affected pages.
4. Changes are serialized in the next project save by `ProjectWriter` through each filter's `saveSettings`.

**State Management:**
- Per-page parameters: stored in each filter's `Settings` object keyed by `PageId`, and serialized as XML.
- Global UI and app preferences: `ApplicationSettings::getInstance()` (singleton).
- Default parameter profiles: `DefaultParamsProvider` and `DefaultParamsProfileManager` (`src/core/DefaultParams*.h`).
- Page order and selection: `PageSequence`, `PageOrderProvider`, `PageSelectionAccessor`.

## Key Abstractions

**`AbstractFilter`:**
- Purpose: Common contract for a pipeline stage.
- Examples: `src/core/AbstractFilter.h`; implemented by `src/core/filters/*/Filter.h`.
- Pattern: Virtual interface, `std::enable_shared_from_this`, held by `StageSequence`.

**`BackgroundTask` / `AbstractCommand<FilterResultPtr>`:**
- Purpose: A unit of background work for one page, with cancellation via `TaskStatus`.
- Examples: `src/core/BackgroundTask.h`, `src/core/filters/deskew/Task.h`, `src/core/filters/output/Task.h`.
- Pattern: Command object returning `FilterResultPtr`, with a separate `CacheDrivenTask` for cached recomputation.

**`Params` / `Settings` pair:**
- Purpose: `Params` is an immutable per-page value object (`src/core/filters/deskew/Params.h`). `Settings` is the mutable, page-indexed store (`src/core/filters/deskew/Settings.h`).
- Pattern: Each stage follows the same split.

**`PageId` / `ImageId` / `PageInfo`:**
- Purpose: Identify a logical page (including sub-pages from page split) and its source image.
- Examples: `src/core/PageId.h`, `src/core/ImageId.h`, `src/core/PageInfo.h`.

**`ImageTransformation`:**
- Purpose: Records the per-page geometric transform (rotation, crop, deskew, dewarp) so that later stages can map coordinates.
- Examples: `src/core/ImageTransformation.h`.

**`Zone` / `EditableZoneSet` / `EditableSpline`:**
- Purpose: User-drawn regions (picture, fill, content, etc.) and curves, edited interactively.
- Examples: `src/core/zones/Zone.h`, `src/core/zones/EditableZoneSet.h`, `src/core/zones/EditableSpline.h`.
- Pattern: Interaction handlers under `src/core/interaction/` (`InteractionHandler`, `DragHandler`, `ZoomHandler`).

**`ColorScheme` / `IconPack`:**
- Purpose: Light and dark theming and icon sets.
- Examples: `src/core/ColorScheme.h`, `src/core/LightColorScheme.cpp`, `src/core/DarkColorScheme.cpp`, `src/core/FontIconPack.h`, `src/core/StyledIconPack.h`.
- Pattern: Factory-created (`ColorSchemeFactory`) and managed by `ColorSchemeManager`.

## Entry Points

**Executable:**
- Location: `src/app/main.cpp` (`int main(int argc, char* argv[])`).
- Triggers: User launch, or `scantailor-advanced <project.ScanTailor>` from the command line.
- Responsibilities: Set the application and organisation names, switch `QSettings` to INI format, apply the colour scheme, construct `MainWindow`, and enter the Qt event loop. Translation loading and the platform fallbacks were removed in Phase 3.

**Test binaries:**
- Locations: `src/core/tests/main.cpp`, `src/imageproc/tests/main.cpp`, `src/foundation/tests/`, `src/math/tests/`, `src/dewarping/tests/`, `src/qt_tests/`, `src/pipeline_tests/` (seven suites).
- Triggers: `ctest --preset macos-release` (or `macos-debug`, `macos-asan`) or direct execution from `build/<preset>` (see `TESTING.md`).

**UI snapshot tool:**
- Location: `src/app/ui_snapshot/main.cpp`. Renders the main window and Settings dialog offscreen; built with the tests, not a ctest suite.

## Architectural Constraints

- **Threading:** The GUI runs on the Qt main thread. Page processing runs on `WorkerThreadPool` threads through `BackgroundExecutor`. Results are marshalled back via `WorkerThreadPool::TaskResultEvent` and `PayloadEvent.h`. UI code must not be touched from worker threads.
- **Global state:** `ApplicationSettings::getInstance()` (`src/core/ApplicationSettings.cpp`) and `ColorSchemeManager` are process-wide singletons. `Application` (`src/core/Application.h`) is the `QApplication` subclass.
- **Stage ordering:** Stages are hard-wired in `StageSequence`. Adding a stage requires editing that file, the `CMakeLists.txt` of `core`, and project serialization.
- **Circular imports:** The `Dependencies` headers link stages to each other (for example, `select_content` and `page_layout` depend on `page_split` and `deskew` outputs). Keep cross-stage includes one-directional where possible.
- **Header conventions:** Include guards use `SCANTAILOR_<MODULE>_<NAME>_H_`. Public headers are included without a path prefix (for example `#include "PageId.h"`), so each module directory is an include root.

## Anti-Patterns

### Copying filter boilerplate instead of using the shared base

**What happens:** New or changed stage code duplicates logic that already exists in `src/core/` (for example, cache handling or thumbnail creation).
**Why it's wrong:** Divergent behaviour between stages and harder maintenance; the six stages already rely on shared base classes.
**Do this instead:** Extend `AbstractFilter`, `BackgroundTask`, `CacheDrivenTask` and reuse `src/core/filters/<stage>/Utils.h` helpers. Compare with `src/core/filters/deskew/` as the reference implementation.

### Calling UI from a worker task

**What happens:** Code in `Task.cpp` that touches widgets or `ImageView` directly.
**Why it's wrong:** Qt widgets are not thread-safe; this causes crashes that are hard to reproduce.
**Do this instead:** Return data in the `FilterResult` and let `Filter` or `ImageView` apply it on the GUI thread (see the `UiUpdater` pattern in `src/core/filters/deskew/Task.h`).

## Error Handling

**Strategy:** Exceptions for cancellation and allocation failure, and `QMessageBox`-based reporting for user-facing errors.

**Patterns:**
- `BackgroundTask::CancelledException` (`src/core/BackgroundTask.h`) signals cancellation of a running task.
- `OutOfMemoryHandler` (`src/core/OutOfMemoryHandler.h`) and `OutOfMemoryDialog` (`src/app/OutOfMemoryDialog.h`) handle `std::bad_alloc`.
- `ErrorWidget` (`src/core/ErrorWidget.h`) renders per-page processing errors inside the view.

## Cross-Cutting Concerns

**Logging:** Not centralised. Diagnostics use `DebugImages` (`src/imageproc/DebugImages.h`, `src/core/DebugImagesImpl.h`) to save intermediate images on demand.
**Validation:** Parameters are validated when read in `loadSettings` and `Params` deserialization (XML unmarshaller in `src/foundation/XmlUnmarshaller.h`).
**Authentication:** Not applicable (local desktop application).

---

*Architecture analysis: 2026-10-18; patched after Phases 2 and 3*
