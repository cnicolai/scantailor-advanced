<!-- GSD:project-start source:PROJECT.md -->

## Project

**ScanTailor Advanced (personal macOS fork)**

A personal fork (`cnicolai/scantailor-advanced`) of ScanTailor Advanced 1.2.1, the C++17/Qt desktop tool that turns raw page scans into clean output through a fixed six-stage pipeline (fix orientation → page split → deskew → select content → page layout → output). The fork exists to make it run well on a modern Apple Silicon Mac for one user's own scanning of books, flat documents and mixed text-and-image pages. Changes are not intended for upstream.

**Core Value:** The app builds from source and works reliably for daily scanning work on a current Apple Silicon Mac, with a test suite that makes later changes safe.

### Constraints

- **Platform**: macOS on Apple Silicon (arm64), current macOS release — the only target machine
- **Dependencies**: Homebrew-provided Qt 6, Boost, libjpeg, libpng, libtiff, zlib — what's available on the Mac without manual builds
- **Tech stack**: Keep C++17/Qt/CMake architecture and the six-stage pipeline — this is a port and hardening, not a rewrite
- **Behaviour**: Output for existing settings should not change unintentionally — the user relies on it for real scanning work
- **Test data**: Synthetic images only in the repository — no personal scans committed

<!-- GSD:project-end -->

<!-- GSD:stack-start source:codebase/STACK.md -->

## Technology Stack

## Languages

- C++17 - All application code under `src/` (core, imageproc, math, dewarping, foundation, app, plus the test code). Set in `CMakeLists.txt` via `set(CMAKE_CXX_STANDARD 17)`.
- CMake language - Build system in `CMakeLists.txt`, `CMakePresets.json`, `src/CMakeLists.txt`, per-module `CMakeLists.txt` files, and `cmake/ListItemsPrepend.cmake` and `cmake/SetDefaultBuildType.cmake` (the only two files left in `cmake/` after Phase 3).
- Qt UI definition files (`*.ui`) - Widget layouts such as `src/core/filters/page_layout/OptionsWidget.ui`, `src/app/AboutDialog.ui`, `src/app/OutOfMemoryDialog.ui`.
- Qt stylesheets and SVG icons - `src/resources/light_scheme/`, `src/resources/dark_scheme/`.
- Packaging and build helper scripts (`build-deb.sh`, `build-windows.sh`, `scripts/`) were removed in Phase 3. No shell or Python script remains outside `.planning/`.

## Runtime

- Native desktop application (no managed runtime). The only target is macOS on Apple Silicon (arm64); Phase 3 removed every other platform (code, build options and files).
- Multithreaded: worker thread pool in `src/core/WorkerThreadPool.h`.
- No language package manager. Native dependencies come from Homebrew and are resolved by CMake `find_package` (preset `CMAKE_PREFIX_PATH=/opt/homebrew`).
- Homebrew packages: `cmake ninja boost qtbase qtsvg jpeg-turbo libpng libtiff`.
- Lockfile: Not applicable.
- Dependabot and the other GitHub automation were removed in Phase 3 (no `.github/`).

## Frameworks

- Qt 6 only (Qt 5 support removed in Phase 3): `find_package(Qt6 6.5 COMPONENTS Core Gui Widgets Xml Network OpenGL Svg OpenGLWidgets Test CONFIG REQUIRED)`; targets link `Qt6::<Component>` directly.
- Boost 1.60+ - Header-heavy use: `boost/multi_index`, `boost/function`, `boost/bind`, `boost/lambda`, `boost/intrusive`, `boost/foreach`, `boost/scoped_array`. The `unit_test_framework` component is required when `BUILD_TESTS` is on and links dynamically (`BOOST_TEST_DYN_LINK`).
- Boost.Test (`unit_test_framework`) - Seven ctest suites: `foundation_tests`, `math_tests`, `imageproc_tests`, `core_tests`, `qt_tests`, `pipeline_tests`, `dewarping_tests`. Test sources live in `src/*/tests/`, `src/qt_tests/` and `src/pipeline_tests/`; the synthetic page generator and golden store are in `src/test_support/`; goldens are in `src/pipeline_tests/golden/{flat,book,mixed}/`.
- CTest - Test runner, enabled through the `BUILD_TESTS` option (default ON) in `CMakeLists.txt`.
- `src/app/ui_snapshot` - offscreen UI snapshot tool, built with the tests, not registered with ctest.
- See `TESTING.md` for run instructions.
- CMake 3.25+ (`cmake_minimum_required(VERSION 3.25 FATAL_ERROR)`) with `CMakePresets.json`: configure, build and test presets `macos-release`, `macos-debug` and `macos-asan` (Debug with `SANITIZE=address,undefined`). In-source builds are rejected.
- Ninja - the generator of every preset; binary directory `build/<preset>`.
- clang-format - Code formatting, configured in `.clang-format`. Phase 3 removed the CI check; formatting is the manual `xcrun clang-format` step documented in `TESTING.md`.
- Packaging (CPack with NSIS, Debian package, Flatpak) was removed in Phase 3, together with the install rules and the uninstall target. The app runs from `build/<preset>/scantailor-advanced`.

## Key Dependencies

- libjpeg (Homebrew `jpeg-turbo`) - JPEG image read/write. `find_package(JPEG REQUIRED)` in `CMakeLists.txt`; linked for all image I/O.
- libpng 1.6 - PNG read/write. `find_package(PNG REQUIRED)`.
- libtiff - TIFF read/write, including multi-page output. `find_package(TIFF REQUIRED)`; `tiffio.h` is included directly in source.
- zlib - Compression dependency for PNG and TIFF. `find_package(ZLIB REQUIRED)`.
- Boost - Containers and utility libraries described above.
- Qt 6 (Core, Gui, Widgets, Xml, Svg, OpenGL, OpenGLWidgets, Network) - Application framework. `QSettings` persists preferences and recent projects (`src/core/ApplicationSettings.cpp`, `src/core/RecentProjects.cpp`). `QDomDocument` handles XML project files (`src/core/` uses `QtXml`).
- `src/math/` (including `src/math/spfit/` and `src/math/adiff/` automatic differentiation) - numeric and spline fitting code.
- `src/imageproc/` - image processing primitives, including connected components (`ConnComp.h`, `Connectivity.h`) and the binarizer and eraser code.
- `src/dewarping/` - page dewarping.
- `src/foundation/` - base types and utilities.
- `src/test_support/` - test-only library: page generator, image comparison, truth metrics, golden store and manifest.
- `src/core/OpenGLSupport.cpp` - optional OpenGL context probing, used for the viewer (until Phase 6).

## Configuration

- `CMakeLists.txt` - Top-level build definition. Main options:
- `config.h.in` and `version.h.in` - CMake-configured headers. Current version is `1.2.1` (`version.h.in` template; `VERSION` macro).
- `cmake/SetDefaultBuildType.cmake` - Defaults the build type to Release.
- `.clang-format` - Formatting rules.
- `.gitignore` - Ignores `build/`, `build-*/`, `cmake-build-*/`, `out/`, and `CMakeLists.txt.user`.
- `QSettings` in INI format: `~/.config/scantailor-advanced/scantailor-advanced.ini`. No portable mode (removed in Phase 3).
- `QStandardPaths::AppDataLocation` for default parameter profiles: `~/Library/Application Support/scantailor-advanced/scantailor-advanced/profiles` (`src/core/DefaultParamsProfileManager.cpp`).
- Environment variables: the app reads none. The tests set `QT_QPA_PLATFORM=offscreen`, `LC_ALL` and `TMPDIR` themselves; the Wayland fallback variables were removed in Phase 3.
- None. The `.github/` directory with its workflows was removed in Phase 3; the gate is a local Release and Debug ctest run (`TESTING.md`).

## Platform Requirements

- Apple Clang (Xcode command line tools) with C++17 and CMake 3.25+
- Homebrew `cmake ninja boost qtbase qtsvg jpeg-turbo libpng libtiff` (Qt 6.5 or newer; Boost 1.60 or newer with `unit_test_framework`)
- zlib from the macOS SDK
- None. The fork is built from source and run from the build directory on the one target Mac. Flatpak, Debian, Windows installer, portable build and GitHub Releases were removed in Phase 3.

<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->

## Conventions

## Naming Patterns

- C++ sources and headers use PascalCase matching the primary class: `src/core/filters/deskew/Filter.cpp`, `src/core/filters/deskew/OptionsWidget.h`, `src/imageproc/Dpi.h`.
- Test sources use the `Test` prefix: `src/core/tests/TestPageId.cpp`, `src/imageproc/tests/TestDpi.cpp`.
- Qt Test sources use the `tst_` prefix: `src/qt_tests/tst_core_qt.cpp`, `src/qt_tests/tst_ux_qt.h`.
- Dialog and widget `.ui` files sit next to their class: `src/core/filters/deskew/ApplyDialog.ui`.
- Lowercase snake_case, one per filter or module: `namespace deskew`, `namespace select_content`, `namespace page_split`, `namespace imageproc`, `namespace output`, `namespace tests`.
- The filter `Filter` class lives inside its namespace: `deskew::Filter` in `src/core/filters/deskew/Filter.cpp`.
- PascalCase: `Filter`, `PageId`, `ImageId`, `OrderByDeviationProvider`, `CacheDrivenTask`.
- Forward declarations are grouped at the top of the header, after include guards: see `src/core/filters/deskew/Filter.h` (`class QString; class PageSelectionAccessor; ...`).
- lowerCamelCase for functions and methods: `getName()`, `preUpdateUI()`, `saveSettings()`, `performRelinking()`, `subPageToString()`.
- Qt-style accessors without `get` prefix for value getters: `imageId()`, `subPage()`, `isNull()`.
- `getName()`/`getView()` style is used for the filter interface; newer code uses bare nouns.
- Private data members use the `m_` prefix followed by lowerCamelCase: `m_settings`, `m_imageSettings`, `m_selectedPageOrder`, `m_optionsWidget`, `m_pageOrderOptions`. The prefix is used pervasively (around 2,780 occurrences in headers).
- Qt widget members are typically held through smart pointers: `m_optionsWidget.reset(new OptionsWidget(...))` in `src/core/filters/deskew/Filter.cpp`.
- lowerCamelCase: `pageSelectionAccessor`, `orderByDeviation`, `defaultOrder`.
- Constants and enumerators: UPPER_SNAKE_CASE for enum values: `PageId::SINGLE_PAGE`, `PageId::LEFT_PAGE`, `PageId::RIGHT_PAGE`, `PAGE_VIEW`.
- Boost suite names are CamelCase ending in `TestSuite`: `CorePageIdTestSuite`, `ImageProcDpiTestSuite`.
- Boost test case names are `test_` followed by snake_case: `test_default_constructor_is_null`, `test_equality`.

## Code Style

- `clang-format` with `BasedOnStyle: Chromium`, configured in `.clang-format` at the repository root.
- Indentation: 2 spaces. Line limit and brace wrapping follow the `.clang-format` file; read it before formatting by hand.
- Functions and control statements use braces on the same line: `if (...) {`, `void f() {`.
- Short single-line blocks are disallowed (`AllowShortBlocksOnASingleLine: false`, `AllowShortIfStatementsOnASingleLine: Never`). Lambdas may be single line (`AllowShortLambdasOnASingleLine: All`).
- `BinPackParameters: false`: when a parameter list wraps, each parameter goes on its own line.
- The CI check (`.github/workflows/lint.yml`) was removed in Phase 3. Formatting is a manual step: run `xcrun clang-format -i <file>` on every touched C++ file before committing and `xcrun clang-format --dry-run -Werror <file>` to check (`TESTING.md`). `src/dewarping/TextLineTracer.cpp` keeps two older violations.
- No dedicated static analyser configuration (no `.clang-tidy` in the repository root). `clang-format` is the only enforced gate.
- C++17 (`set(CMAKE_CXX_STANDARD 17)` in `CMakeLists.txt`). Use `std::make_shared`, `std::unique_ptr`, `auto`, and structured constructs already in use; do not introduce C++20 features.

## Import Organization

- Module headers are included by bare file name (`#include <PageId.h>`), because each module directory is an include root.
- Cross-filter headers are included with the filter path (`#include <filters/select_content/Task.h>`).
- No path aliases are configured.
- Headers use classic `#ifndef`/`#define` guards named after the file, e.g. `FILENAME_H`. `#pragma once` is not used anywhere in the tree.

## Copyright Header

- New source files start with the project copyright and license notice, matching existing files:

## Error Handling

- C++ exceptions are the main mechanism for recoverable failure inside workers and filters (about 160 `throw` sites in `src/`). Catch by const reference: `} catch (const std::runtime_error&) {` (`src/core/filters/output/OutputGenerator.cpp`).
- Cancellation uses a dedicated exception type: `catch (const TaskCancelException&)` in `src/core/filters/output/DespeckleView.cpp`. Throw and catch this type rather than returning sentinel values.
- Parsing and conversion functions report success through an out-parameter flag rather than throwing: `PageId::subPageFromString("left", &ok)` in `src/core/PageId.cpp`. Follow this pattern for new parse functions and check the flag at call sites.
- User-facing errors use modal dialogs with translated text: `QMessageBox::warning(this, tr("Error"), tr("DPI is too low!"));` in `src/core/filters/output/ChangeDpiDialog.cpp`.
- Do not swallow exceptions silently in new code. Log or propagate.
- Validate user input in the dialog before accepting it (see `ChangeDpiDialog.cpp`) rather than deep in the processing pipeline.

## Logging

- Keep diagnostic output out of normal paths. Roughly 19 `qDebug`/`qWarning`/`std::cerr` calls exist in `src/`; prefer that low count over adding more.
- Do not log file contents, user paths beyond what the existing code already prints, or secrets.

## Comments

- Comment non-obvious intent, algorithm references, and units (px, mm, DPI). Short inline comments are the norm; the codebase does not use long banner blocks.
- Do not restate what the code does.
- No Doxygen requirement is enforced. Where present, use `///` or `/** */` on public interface declarations in headers.
- User-visible strings are wrapped in `tr("...")` (in `QObject` subclasses) or `QCoreApplication::translate("namespace::Class", "...")` (in non-QObject code, e.g. `Filter::getName()` in `src/core/filters/deskew/Filter.cpp`). The translation catalogues in `src/translations/` were removed in Phase 3; the markers stay.

## Function Design

- No hard limit enforced. Keep functions focused; long per-page processing is split into `Task` and `CacheDrivenTask` classes per filter (`src/core/filters/deskew/Task.h`, `CacheDrivenTask.h`).
- Pass shared, read-only state as `const&` or `const std::shared_ptr<...>&`.
- Use `const` on parameters and locals wherever the value is not modified: `const PageId id;`, `const ImageId imageId(...)`.
- Keep one parameter per line when wrapping (`BinPackParameters: false`).
- Return by value for small Qt types (`QString`, `QDomElement`, `PageView`).
- Use `= default` for trivial destructors and special members: `Filter::~Filter() = default;`.

## Module Design

- Each filter is a directory under `src/core/filters/<name>/` with its own `CMakeLists.txt`, a `Filter.h/.cpp` implementing the filter interface, `OptionsWidget`, `Settings`, `Task`/`CacheDrivenTask`, `ProjectReader`/`ProjectWriter` hooks, and an optional `ApplyDialog` with a `.ui` form.
- Add new filters following this layout; mirror `src/core/filters/deskew/`.
- Not used. Include headers directly.
- `std::shared_ptr` is the dominant ownership model for shared settings and providers (about 200 uses). `std::unique_ptr` is used for owned widgets and tasks (about 200 files reference a smart pointer).
- Avoid raw `new` for new code; the few existing uses are for Qt widgets handed to a parent or reset into a member.

<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->

## Architecture

## System Overview

```text

```

## Pattern Overview

- The pipeline is fixed in order and defined in `src/core/StageSequence.cpp`: `fix_orientation`, `page_split`, `deskew`, `select_content`, `page_layout`, `output`. Each later stage consumes results of earlier ones.
- Every filter implements the `AbstractFilter` interface (`src/core/AbstractFilter.h`): `getName`, `getView`, `performRelinking`, `preUpdateUI`, `saveSettings`/`loadSettings`, `loadDefaultSettings`, and optional page-order hooks.
- Heavy per-page work runs off the GUI thread as `BackgroundTask` objects (`src/core/BackgroundTask.h`) scheduled via `BackgroundExecutor` and `WorkerThreadPool`. Results return to the GUI as events.
- Results are cached per page. Stages with no new computation use `CacheDrivenTask` (e.g. `src/core/filters/deskew/CacheDrivenTask.h`) to recompute only the cheap transformation or UI-facing data from cached parameters.
- Project state is persisted as XML (`ProjectWriter` / `ProjectReader`, root element `project`, with `filters`, `directories`, `files`, `images` sections).
- Settings live in per-filter `Settings` objects (e.g. `src/core/filters/deskew/Settings.h`) that map `PageId` to `Params`, guarded by a mutex-style interface, and in a global `ApplicationSettings` singleton (`src/core/ApplicationSettings.h`).

## Layers

- Purpose: Program entry, main window, dialogs, project create/open flows, file relinking, status and load-progress UI.
- Location: `src/app/`
- Contains: `main.cpp`, `MainWindow.{h,cpp,ui}`, `ProjectCreationContext`, `ProjectOpeningContext`, `ThumbnailSequence`, `RelinkingModel`/`RelinkingDialog`, `SettingsDialog`, `DefaultParamsDialog`.
- Depends on: `core`, Qt Widgets.
- Used by: End users (it is the executable).
- Purpose: Filter pipeline, page and project model, background tasks, image loading and writing, shared widgets.
- Location: `src/core/`
- Contains: the generic framework (`AbstractFilter.h`, `StageSequence`, `PageSequence`, `ProjectPages`, `FilterData`, `FilterResult.h`, `BackgroundTask`, `ProcessingTaskQueue`, `ImageLoader`, `TiffWriter`, `ProjectReader/Writer`), plus the stage implementations under `src/core/filters/` and the interactive zone editing under `src/core/zones/` and `src/core/interaction/`.
- Depends on: `imageproc`, `dewarping`, `math`, `foundation`, Qt.
- Used by: `src/app/`.
- Purpose: One processing stage each. Each directory is a self-contained mini-module.
- Location: `src/core/filters/{fix_orientation,page_split,deskew,select_content,page_layout,output}/`
- Contains: a standard file set (see Key Abstractions below).
- Depends on: `core` base classes, `imageproc`, and sometimes other stages via a `Dependencies` class (e.g. `src/core/filters/deskew/Dependencies.h`, `src/core/filters/page_split/Dependencies.h`, `src/core/filters/select_content/Dependencies.h`).
- Used by: `StageSequence` and the app GUI.
- Purpose: Pixel-level algorithms: binarization, connected components, morphology, skew detection, scaling, rotation, shear, transforms, and the debug-image support.
- Location: `src/imageproc/`
- Contains: `Binarize`, `ConnComp*`, `SkewFinder`, `Transform`, `Scale`, `OrthogonalRotation`, `BinaryImage`, `Morphology`, `SEDM`, `RastLineFinder`, and others. Tests in `src/imageproc/tests/`.
- Depends on: `foundation`, `math`.
- Used by: `core/filters/*` and `dewarping`.
- Purpose: Page-surface distortion modelling for curved book pages.
- Location: `src/dewarping/`
- Contains: `TextLineTracer`, `TopBottomEdgeTracer`, `CylindricalSurfaceDewarper`, `DistortionModel`, `DistortionModelBuilder`, `RasterDewarper`, `DewarpingPointMapper`, `Curve`.
- Depends on: `imageproc` (linked PUBLIC in `src/dewarping/CMakeLists.txt`).
- Used by: `core/filters/output/` (dewarping options and rendering).
- Purpose: Geometry and numerics; generic utilities.
- Location: `src/math/` (including `spfit/` spline fitting and `adiff/` automatic differentiation) and `src/foundation/`.
- Contains: `VecT.h`, `MatT.h`, `Grid.h`, `PriorityQueue.h`, `Property.h`/`PropertySet.h` (typed per-page parameter maps), `XmlMarshaller.cpp`/`XmlUnmarshaller.cpp`, `Span.h`, `StaticPool.h`.
- Depends on: nothing internal.
- Used by: all layers above.

## Data Flow

### Primary Request Path (opening a project and processing a page)

### Settings Change Flow

- Per-page parameters: stored in each filter's `Settings` object keyed by `PageId`, and serialized as XML.
- Global UI and app preferences: `ApplicationSettings::getInstance()` (singleton).
- Default parameter profiles: `DefaultParamsProvider` and `DefaultParamsProfileManager` (`src/core/DefaultParams*.h`).
- Page order and selection: `PageSequence`, `PageOrderProvider`, `PageSelectionAccessor`.

## Key Abstractions

- Purpose: Common contract for a pipeline stage.
- Examples: `src/core/AbstractFilter.h`; implemented by `src/core/filters/*/Filter.h`.
- Pattern: Virtual interface, `std::enable_shared_from_this`, held by `StageSequence`.
- Purpose: A unit of background work for one page, with cancellation via `TaskStatus`.
- Examples: `src/core/BackgroundTask.h`, `src/core/filters/deskew/Task.h`, `src/core/filters/output/Task.h`.
- Pattern: Command object returning `FilterResultPtr`, with a separate `CacheDrivenTask` for cached recomputation.
- Purpose: `Params` is an immutable per-page value object (`src/core/filters/deskew/Params.h`). `Settings` is the mutable, page-indexed store (`src/core/filters/deskew/Settings.h`).
- Pattern: Each stage follows the same split.
- Purpose: Identify a logical page (including sub-pages from page split) and its source image.
- Examples: `src/core/PageId.h`, `src/core/ImageId.h`, `src/core/PageInfo.h`.
- Purpose: Records the per-page geometric transform (rotation, crop, deskew, dewarp) so that later stages can map coordinates.
- Examples: `src/core/ImageTransformation.h`.
- Purpose: User-drawn regions (picture, fill, content, etc.) and curves, edited interactively.
- Examples: `src/core/zones/Zone.h`, `src/core/zones/EditableZoneSet.h`, `src/core/zones/EditableSpline.h`.
- Pattern: Interaction handlers under `src/core/interaction/` (`InteractionHandler`, `DragHandler`, `ZoomHandler`).
- Purpose: Light and dark theming and icon sets.
- Examples: `src/core/ColorScheme.h`, `src/core/LightColorScheme.cpp`, `src/core/DarkColorScheme.cpp`, `src/core/FontIconPack.h`, `src/core/StyledIconPack.h`.
- Pattern: Factory-created (`ColorSchemeFactory`) and managed by `ColorSchemeManager`.

## Entry Points

- Location: `src/app/main.cpp` (`int main(int argc, char* argv[])`).
- Triggers: User launch, or `scantailor-advanced <project.ScanTailor>` from the command line.
- Responsibilities: Set the application and organisation names, switch `QSettings` to INI format, apply the colour scheme, construct `MainWindow`, and enter the Qt event loop. Translation loading and the platform fallbacks were removed in Phase 3.
- Locations: `src/core/tests/main.cpp`, `src/imageproc/tests/main.cpp`, `src/foundation/tests/`, `src/math/tests/`, `src/dewarping/tests/`, `src/qt_tests/`, `src/pipeline_tests/` (seven suites).
- Triggers: `ctest --preset macos-release` (or `macos-debug`, `macos-asan`) or direct execution from `build/<preset>` (see `TESTING.md`).
- Location: `src/app/ui_snapshot/main.cpp`. Renders the main window and Settings dialog offscreen; built with the tests, not a ctest suite.

## Architectural Constraints

- **Threading:** The GUI runs on the Qt main thread. Page processing runs on `WorkerThreadPool` threads through `BackgroundExecutor`. Results are marshalled back via `WorkerThreadPool::TaskResultEvent` and `PayloadEvent.h`. UI code must not be touched from worker threads.
- **Global state:** `ApplicationSettings::getInstance()` (`src/core/ApplicationSettings.cpp`) and `ColorSchemeManager` are process-wide singletons. `Application` (`src/core/Application.h`) is the `QApplication` subclass.
- **Stage ordering:** Stages are hard-wired in `StageSequence`. Adding a stage requires editing that file, the `CMakeLists.txt` of `core`, and project serialization.
- **Circular imports:** The `Dependencies` headers link stages to each other (for example, `select_content` and `page_layout` depend on `page_split` and `deskew` outputs). Keep cross-stage includes one-directional where possible.
- **Header conventions:** Include guards use `SCANTAILOR_<MODULE>_<NAME>_H_`. Public headers are included without a path prefix (for example `#include "PageId.h"`), so each module directory is an include root.

## Anti-Patterns

### Copying filter boilerplate instead of using the shared base

### Calling UI from a worker task

## Error Handling

- `BackgroundTask::CancelledException` (`src/core/BackgroundTask.h`) signals cancellation of a running task.
- `OutOfMemoryHandler` (`src/core/OutOfMemoryHandler.h`) and `OutOfMemoryDialog` (`src/app/OutOfMemoryDialog.h`) handle `std::bad_alloc`.
- `ErrorWidget` (`src/core/ErrorWidget.h`) renders per-page processing errors inside the view.

## Cross-Cutting Concerns

<!-- GSD:architecture-end -->

<!-- GSD:skills-start source:skills/ -->

## Project Skills

No project skills found. Add skills to any of: `.claude/skills/`, `.agents/skills/`, `.cursor/skills/`, `.github/skills/`, or `.codex/skills/` with a `SKILL.md` index file.
<!-- GSD:skills-end -->

<!-- GSD:workflow-start source:GSD defaults -->

## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:
- `/gsd-fast` for a trivial task inline, with no subagents and no PLAN.md
- `/gsd-quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd-debug` for investigation and bug fixing
- `/gsd-execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->

<!-- GSD:profile-start -->

## Developer Profile

> Profile not yet configured. Run `/gsd-profile-user` to generate your developer profile.
> This section is managed by `generate-claude-profile` -- do not edit manually.
<!-- GSD:profile-end -->
