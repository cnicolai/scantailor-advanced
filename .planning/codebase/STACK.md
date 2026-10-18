---
last_mapped_commit: 5eaac1884cdcabb6514bd632114f688631bd8dbc
last_mapped_at: 2026-10-18
---
# Technology Stack

**Analysis Date:** 2026-10-18

## Languages

**Primary:**
- C++17 - All application code under `src/` (core, imageproc, math, dewarping, foundation, app). Set in `CMakeLists.txt` via `set(CMAKE_CXX_STANDARD 17)`.

**Secondary:**
- CMake language - Build system in `CMakeLists.txt`, `src/CMakeLists.txt`, per-module `CMakeLists.txt` files, and `cmake/*.cmake`.
- Qt UI definition files (`*.ui`) - Widget layouts such as `src/core/filters/page_layout/OptionsWidget.ui`, `src/app/AboutDialog.ui`, `src/app/OutOfMemoryDialog.ui`.
- Qt stylesheets and SVG icons - `src/resources/light_scheme/`, `src/resources/dark_scheme/`.
- Bash - Packaging and build helpers: `build-deb.sh`, `build-windows.sh`, `scripts/`.
- Python - `scripts/post_issue_followups.py` (repository maintenance script, not part of the application).

## Runtime

**Environment:**
- Native desktop application (no managed runtime). Targets Linux, Windows (MSVC and MinGW), and macOS-style Unix builds via CMake.
- Linux runtime is the KDE Flatpak platform: `org.kde.Platform` 5.15-24.08 with `org.kde.Sdk` (`flatpak/org.scantailor.Advanced.json`).
- Windows is built with MSVC or MinGW; the optional `WIN_XP` option is MSVC-only (`CMakeLists.txt`).
- Multithreaded: worker thread pool in `src/core/WorkerThreadPool.h`.

**Package Manager:**
- No language package manager. Native dependencies are resolved by CMake `find_package`.
- Lockfile: Not applicable.
- Dependabot tracks GitHub Actions only (`.github/dependabot.yml`).

## Frameworks

**Core:**
- Qt - GUI, widgets, XML, networking module linked, SVG, OpenGL. Qt 6 is preferred when found; Qt 5 (minimum 5.9) is the fallback (`CMakeLists.txt`, `find_package(Qt6 ...)` then `find_package(Qt5 5.9 ...)`).
  - Modules used: Core, Gui, Widgets, Xml, Network, OpenGL, OpenGLWidgets (Qt 6 only), Svg, Test, LinguistTools.
  - Qt Linguist translations: `src/translations/`, built with `Qt_add_translation` / `Qt5_add_translation`.
- Boost 1.60+ - Header-heavy use: `boost/multi_index`, `boost/function`, `boost/bind`, `boost/lambda`, `boost/intrusive`, `boost/foreach`, `boost/scoped_array`. Static libraries are the default (`Boost_USE_STATIC_LIBS ON`).

**Testing:**
- Boost.Test (`unit_test_framework`) - Test executables: `math_tests`, `imageproc_tests`, `core_tests`, `foundation_tests`, `qt_tests`. Test sources live in `src/*/tests/` and `src/qt_tests/`.
- CTest - Test runner, enabled through the `BUILD_TESTS` option (default ON) in `CMakeLists.txt`.
- See `TESTING.md` for run instructions.

**Build/Dev:**
- CMake 3.9+ (`cmake_minimum_required(VERSION 3.9 FATAL_ERROR)`) - Build system. In-source builds are rejected.
- Ninja - Used by the Flatpak build (`"buildsystem": "cmake-ninja"` in `flatpak/org.scantailor.Advanced.json`).
- clang-format - Code formatting, configured in `.clang-format`, enforced by `.github/workflows/lint.yml`.
- CPack - Packaging (NSIS for Windows, DEB via `build-deb.sh`, Flatpak via `flatpak/`).
- Qt Linguist (`lupdate`/`lrelease` via `cmake/UpdateTranslations.cmake`) - Translation maintenance.

## Key Dependencies

**Critical (imaging and math):**
- libjpeg - JPEG image read/write. `find_package(JPEG REQUIRED)` in `CMakeLists.txt`; linked for all image I/O.
- libpng 1.6 - PNG read/write. `find_package(PNG REQUIRED)`.
- libtiff - TIFF read/write, including multi-page output. `find_package(TIFF REQUIRED)`; `tiffio.h` is included directly in source.
- zlib - Compression dependency for PNG and TIFF. `find_package(ZLIB REQUIRED)`.
- Boost - Containers and utility libraries described above.

**Critical (UI):**
- Qt (Core, Gui, Widgets, Xml, Svg, OpenGL, Network) - Application framework. `QSettings` persists preferences and recent projects (`src/core/ApplicationSettings.cpp`, `src/core/RecentProjects.cpp`). `QDomDocument` handles XML project files (`src/core/` uses `QtXml`).

**In-tree libraries (not external dependencies):**
- `src/math/` (including `src/math/spfit/` and `src/math/adiff/` automatic differentiation) - numeric and spline fitting code.
- `src/imageproc/` - image processing primitives, including connected components (`ConnComp.h`, `Connectivity.h`) and the binarizer and eraser code.
- `src/dewarping/` - page dewarping.
- `src/foundation/` - base types and utilities.

**Infrastructure:**
- `src/core/OpenGLSupport.cpp` - optional OpenGL context probing, used for the viewer.

## Configuration

**Build configuration:**
- `CMakeLists.txt` - Top-level build definition. Main options:
  - `BUILD_TESTS` (default ON)
  - `PORTABLE_VERSION` (default ON)
  - `DEVELOPER_VERSION` (enables internal debug features; default depends on build type)
  - `WIN_XP` (MSVC only)
  - `CMAKE_POSITION_INDEPENDENT_CODE` (ON on UNIX)
- Windows dependency roots: `DEPENDENCIES_ROOT` (default `../libs`) with per-library prefixes for JPEG, ZLIB, PNG, TIFF, Qt, and Boost (`CMakeLists.txt`).
- `config.h.in` and `version.h.in` - CMake-configured headers. Current version is `1.2.1` (`version.h.in` template; `VERSION` macro).
- `cmake/SetDefaultBuildType.cmake` - Defaults the build type to Release.
- `.clang-format` - Formatting rules.
- `.gitignore` - Ignores `build/`, `build-*/`, `cmake-build-*/`, `out/`, and `CMakeLists.txt.user`.

**Runtime configuration:**
- `QSettings` (platform-native store) for application settings and recent projects.
- `QStandardPaths::AppDataLocation` for default parameter profiles, `<AppData>/profiles` (`src/core/DefaultParamsProfileManager.cpp`).
- Environment variables:
  - `SCANTAILOR_NO_XCB_FALLBACK` - disables the forced X11 fallback on Wayland under Qt5 (see `README.md`).
  - `QT_QPA_PLATFORM` - standard Qt platform override, documented in `README.md`.
  - `XDG_SESSION_TYPE` - read by the Wayland fallback logic.

**Packaging and CI:**
- `.github/workflows/ci.yml` - Ubuntu build with Qt 5, Boost, and libjpeg/libpng/libtiff/zlib from apt.
- `.github/workflows/cmake.yml` - Ubuntu CMake build.
- `.github/workflows/lint.yml` - clang-format check.
- `.github/workflows/flatpak.yml` - Flatpak build on ubuntu-22.04.
- `.github/workflows/release.yml` - Release artifacts (DEB, Flatpak, Windows) and GitHub release publishing.

## Platform Requirements

**Development (Linux, per `README.md` and `ci.yml`):**
- C++17 compiler (GCC or Clang) and CMake 3.9+
- Qt 5.9+ or Qt 6 development packages (including Svg, Xml, Network, LinguistTools)
- Boost 1.60+ development headers, plus `unit_test_framework` for tests
- libjpeg, libpng, libtiff, zlib development headers

**Development (Windows):**
- MSVC or MinGW toolchain
- Prebuilt JPEG, ZLIB, PNG, TIFF, Qt, and Boost under `DEPENDENCIES_ROOT`
- `build-windows.sh` for scripted builds; MXE is documented in `README.md` for cross-compiling from Linux.

**Production:**
- Desktop distribution through:
  - Flatpak (`flatpak/org.scantailor.Advanced.json`, KDE 5.15 runtime)
  - Debian package (`build-deb.sh`)
  - Windows installer (CPack NSIS) and portable build (`PORTABLE_VERSION`)
  - GitHub Releases (`.github/workflows/release.yml`)

---

*Stack analysis: 2026-10-18*
