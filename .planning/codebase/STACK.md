---
last_mapped_commit: 5eaac1884cdcabb6514bd632114f688631bd8dbc
last_mapped_at: 2026-10-18
---
# Technology Stack

**Analysis Date:** 2026-10-18 (patched after the Phase 3 strip, D-10; not re-mapped)

## Languages

**Primary:**
- C++17 - All application code under `src/` (core, imageproc, math, dewarping, foundation, app, plus the test code). Set in `CMakeLists.txt` via `set(CMAKE_CXX_STANDARD 17)`.

**Secondary:**
- CMake language - Build system in `CMakeLists.txt`, `CMakePresets.json`, `src/CMakeLists.txt`, per-module `CMakeLists.txt` files, and `cmake/ListItemsPrepend.cmake` and `cmake/SetDefaultBuildType.cmake` (the only two files left in `cmake/` after Phase 3).
- Qt UI definition files (`*.ui`) - Widget layouts such as `src/core/filters/page_layout/OptionsWidget.ui`, `src/app/AboutDialog.ui`, `src/app/OutOfMemoryDialog.ui`.
- Qt stylesheets and SVG icons - `src/resources/light_scheme/`, `src/resources/dark_scheme/`.
- Packaging and build helper scripts (`build-deb.sh`, `build-windows.sh`, `scripts/`) were removed in Phase 3. No shell or Python script remains outside `.planning/`.

## Runtime

**Environment:**
- Native desktop application (no managed runtime). The only target is macOS on Apple Silicon (arm64); Phase 3 removed every other platform (code, build options and files).
- Multithreaded: worker thread pool in `src/core/WorkerThreadPool.h`.

**Package Manager:**
- No language package manager. Native dependencies come from Homebrew and are resolved by CMake `find_package` (preset `CMAKE_PREFIX_PATH=/opt/homebrew`).
- Homebrew packages: `cmake ninja boost qtbase qtsvg jpeg-turbo libpng libtiff`.
- Lockfile: Not applicable.
- Dependabot and the other GitHub automation were removed in Phase 3 (no `.github/`).

## Frameworks

**Core:**
- Qt 6 only (Qt 5 support removed in Phase 3): `find_package(Qt6 6.5 COMPONENTS Core Gui Widgets Xml Network OpenGL Svg OpenGLWidgets Test CONFIG REQUIRED)`; targets link `Qt6::<Component>` directly.
  - Modules used: Core, Gui, Widgets, Xml, Network, OpenGL, OpenGLWidgets, Svg, Test. The OpenGL modules stay until Phase 6 (STRIP-07).
  - No LinguistTools: the translation catalogues and their CMake support were removed in Phase 3. `tr()` markers and `.ui` strings stay (D-06).
- Boost 1.60+ - Header-heavy use: `boost/multi_index`, `boost/function`, `boost/bind`, `boost/lambda`, `boost/intrusive`, `boost/foreach`, `boost/scoped_array`. The `unit_test_framework` component is required when `BUILD_TESTS` is on and links dynamically (`BOOST_TEST_DYN_LINK`).

**Testing:**
- Boost.Test (`unit_test_framework`) - Seven ctest suites: `foundation_tests`, `math_tests`, `imageproc_tests`, `core_tests`, `qt_tests`, `pipeline_tests`, `dewarping_tests`. Test sources live in `src/*/tests/`, `src/qt_tests/` and `src/pipeline_tests/`; the synthetic page generator and golden store are in `src/test_support/`; goldens are in `src/pipeline_tests/golden/{flat,book,mixed}/`.
- CTest - Test runner, enabled through the `BUILD_TESTS` option (default ON) in `CMakeLists.txt`.
- `src/app/ui_snapshot` - offscreen UI snapshot tool, built with the tests, not registered with ctest.
- See `TESTING.md` for run instructions.

**Build/Dev:**
- CMake 3.25+ (`cmake_minimum_required(VERSION 3.25 FATAL_ERROR)`) with `CMakePresets.json`: configure, build and test presets `macos-release`, `macos-debug` and `macos-asan` (Debug with `SANITIZE=address,undefined`). In-source builds are rejected.
- Ninja - the generator of every preset; binary directory `build/<preset>`.
- clang-format - Code formatting, configured in `.clang-format`. Phase 3 removed the CI check; formatting is the manual `xcrun clang-format` step documented in `TESTING.md`.
- Packaging (CPack with NSIS, Debian package, Flatpak) was removed in Phase 3, together with the install rules and the uninstall target. The app runs from `build/<preset>/scantailor-advanced`.

## Key Dependencies

**Critical (imaging and math):**
- libjpeg (Homebrew `jpeg-turbo`) - JPEG image read/write. `find_package(JPEG REQUIRED)` in `CMakeLists.txt`; linked for all image I/O.
- libpng 1.6 - PNG read/write. `find_package(PNG REQUIRED)`.
- libtiff - TIFF read/write, including multi-page output. `find_package(TIFF REQUIRED)`; `tiffio.h` is included directly in source.
- zlib - Compression dependency for PNG and TIFF. `find_package(ZLIB REQUIRED)`.
- Boost - Containers and utility libraries described above.

**Critical (UI):**
- Qt 6 (Core, Gui, Widgets, Xml, Svg, OpenGL, OpenGLWidgets, Network) - Application framework. `QSettings` persists preferences and recent projects (`src/core/ApplicationSettings.cpp`, `src/core/RecentProjects.cpp`). `QDomDocument` handles XML project files (`src/core/` uses `QtXml`).

**In-tree libraries (not external dependencies):**
- `src/math/` (including `src/math/spfit/` and `src/math/adiff/` automatic differentiation) - numeric and spline fitting code.
- `src/imageproc/` - image processing primitives, including connected components (`ConnComp.h`, `Connectivity.h`) and the binarizer and eraser code.
- `src/dewarping/` - page dewarping.
- `src/foundation/` - base types and utilities.
- `src/test_support/` - test-only library: page generator, image comparison, truth metrics, golden store and manifest.

**Infrastructure:**
- `src/core/OpenGLSupport.cpp` - optional OpenGL context probing, used for the viewer (until Phase 6).

## Configuration

**Build configuration:**
- `CMakeLists.txt` - Top-level build definition. Main options:
  - `BUILD_TESTS` (default ON)
  - `DEVELOPER_VERSION` (enables internal debug features; default ON for Debug builds)
  - `SANITIZE` (value for `-fsanitize=`, empty disables; used by `macos-asan`)
  - `CMAKE_POSITION_INDEPENDENT_CODE` (ON)
  - The `PORTABLE_VERSION` and `WIN_XP` options and the Windows `DEPENDENCIES_ROOT` prefixes were removed in Phase 3.
- `config.h.in` and `version.h.in` - CMake-configured headers. Current version is `1.2.1` (`version.h.in` template; `VERSION` macro).
- `cmake/SetDefaultBuildType.cmake` - Defaults the build type to Release.
- `.clang-format` - Formatting rules.
- `.gitignore` - Ignores `build/`, `build-*/`, `cmake-build-*/`, `out/`, and `CMakeLists.txt.user`.

**Runtime configuration:**
- `QSettings` in INI format: `~/.config/scantailor-advanced/scantailor-advanced.ini`. No portable mode (removed in Phase 3).
- `QStandardPaths::AppDataLocation` for default parameter profiles: `~/Library/Application Support/scantailor-advanced/scantailor-advanced/profiles` (`src/core/DefaultParamsProfileManager.cpp`).
- Environment variables: the app reads none. The tests set `QT_QPA_PLATFORM=offscreen`, `LC_ALL` and `TMPDIR` themselves; the Wayland fallback variables were removed in Phase 3.

**Packaging and CI:**
- None. The `.github/` directory with its workflows was removed in Phase 3; the gate is a local Release and Debug ctest run (`TESTING.md`).

## Platform Requirements

**Development (macOS on Apple Silicon, per `README.md`):**
- Apple Clang (Xcode command line tools) with C++17 and CMake 3.25+
- Homebrew `cmake ninja boost qtbase qtsvg jpeg-turbo libpng libtiff` (Qt 6.5 or newer; Boost 1.60 or newer with `unit_test_framework`)
- zlib from the macOS SDK

**Production:**
- None. The fork is built from source and run from the build directory on the one target Mac. Flatpak, Debian, Windows installer, portable build and GitHub Releases were removed in Phase 3.

---

*Stack analysis: 2026-10-18; patched after the Phase 3 strip*
