---
phase: 03-qt-6-and-macos-only
reviewed: 2026-10-24T00:00:00Z
depth: standard
files_reviewed: 52
files_reviewed_list:
  - .claude/CLAUDE.md
  - CMakeLists.txt
  - CMakePresets.json
  - README.md
  - TESTING.md
  - config.h.in
  - src/app/CMakeLists.txt
  - src/app/MainWindow.cpp
  - src/app/RelinkablePathVisualization.cpp
  - src/app/SettingsDialog.cpp
  - src/app/SettingsDialog.ui
  - src/app/main.cpp
  - src/app/ui_snapshot/main.cpp
  - src/core/Application.cpp
  - src/core/Application.h
  - src/core/ApplicationSettings.cpp
  - src/core/ApplicationSettings.h
  - src/core/BackgroundTask.h
  - src/core/CMakeLists.txt
  - src/core/DarkColorScheme.cpp
  - src/core/DefaultParamsProfileManager.cpp
  - src/core/ImageViewBase.cpp
  - src/core/ImageViewBase.h
  - src/core/LightColorScheme.cpp
  - src/core/RelinkablePath.cpp
  - src/core/Utils.cpp
  - src/core/filters/output/DewarpingView.cpp
  - src/core/filters/page_split/ImageView.cpp
  - src/core/filters/page_split/PageLayout.cpp
  - src/core/filters/page_split/PageLayoutAdapter.cpp
  - src/core/interaction/CMakeLists.txt
  - src/core/interaction/ZoomHandler.cpp
  - src/core/tests/CMakeLists.txt
  - src/core/zones/CMakeLists.txt
  - src/core/zones/ZoneContextMenuInteraction.cpp
  - src/core/zones/ZoneContextMenuInteraction.h
  - src/dewarping/Curve.cpp
  - src/dewarping/CylindricalSurfaceDewarper.cpp
  - src/dewarping/DetectVertContentBounds.cpp
  - src/dewarping/DistortionModelBuilder.cpp
  - src/dewarping/TextLineTracer.cpp
  - src/dewarping/tests/CMakeLists.txt
  - src/foundation/CMakeLists.txt
  - src/foundation/Proximity.cpp
  - src/foundation/tests/CMakeLists.txt
  - src/imageproc/ByteOrder.h
  - src/imageproc/tests/CMakeLists.txt
  - src/math/PolylineIntersector.cpp
  - src/math/tests/CMakeLists.txt
  - src/pipeline_tests/CMakeLists.txt
  - src/pipeline_tests/main.cpp
  - src/qt_tests/CMakeLists.txt
  - src/test_support/CMakeLists.txt
  - src/test_support/Manifest.cpp
findings:
  critical: 0
  warning: 2
  info: 7
  total: 9
status: issues_found
---

# Phase 3: Code Review Report

**Reviewed:** 2026-10-24
**Depth:** standard
**Files Reviewed:** 52 (`.claude/CLAUDE.md` was read as project context and checked only for factual claims)
**Status:** issues_found

## Summary

I reviewed the Phase 3 changes (`git diff 72e7724e HEAD`) in every listed file and checked what they leave behind with repository-wide greps.

**Conditional resolution.** Every `#if` / `#ifdef` the phase resolved kept the macOS and Qt 6 side:
- `Q_OS_UNIX` (the `play` alarm), `Q_OS_MAC` (`ZoneContextMenuInteraction`, 3 sites) and the `#else` of `_WIN32` (the colour-scheme `px` to `em` base 16, `Utils::overwritingRename`, `ByteOrder.h`) are all resolved to the macOS side.
- `RelinkablePath::normalize` and the `play` alarm are unchanged in behaviour.

**Build system.** The build-system rewrite is consistent:
- No reference to `EXTRA_LIBS`, `Qt_*_lib`, `translation_sources`, `QT_BINDIR`, `PORTABLE_*`, `TRANSLATION_DIRS`, `PLUGIN_DIRS`, `install(`, `CPack` or a removed `cmake/*.cmake` file remains.
- Every `Qt6::` target is spelled out.
- No `.qrc` entry points to a deleted resource file.
- The preset `binaryDir` (`build/<preset>`) matches the `UI_SNAPSHOT_ROOT` (`<source>/build`) assumption.
- `config.h.in` no longer defines the removed macros, and no source uses them.
- The Boost test define `BOOST_TEST_DYN_LINK` resolves to the same value it had on macOS before.

**Formatting.** `clang-format --dry-run -Werror` is clean on every changed C++ file except the two older violations in `TextLineTracer.cpp` that `TESTING.md` already documents.

I found no bug that changes macOS behaviour. What remains is a new test tool that is built unconditionally and doubles the GUI compile cost, a comment the phase orphaned, and leftover dead code and stale comments from the strip.

## Warnings

### WR-01: `ui_snapshot` recompiles the entire GUI sources and is built by default

**File:** `src/app/CMakeLists.txt:61-75`
**Issue:** `ui_snapshot` lists every `gui_only_sources` file (except `main.cpp`) and every `.ui` and `.qrc` file again as its own sources. It lives under `if (BUILD_TESTS)`, which defaults to ON, so every normal `cmake --build` compiles MainWindow, the dialogs, the models, AUTOMOC, AUTOUIC and AUTORCC output a second time. The preset builds go through this path, and so does the Debug and ASan gate. The tool is described in its own header and in the CMake comment as a harness "for Phase 3 D-12", the one-off before/after comparison of the Settings dialog. It has no ctest registration, so nothing exercises it after this phase, and `--assert-no-language` checks a one-time condition. As written it is a permanent build cost, and it will rot silently when `MainWindow` or `SettingsDialog` change.
**Fix:** Either remove the target now that the Phase 3 records exist, or keep it and stop paying for it twice:
```cmake
# one compile of the GUI sources, shared by both executables
add_library(app_gui OBJECT ${gui_only_sources_without_main} ${gui_only_ui_files} ${resource_files})
target_link_libraries(app_gui PUBLIC core)
...
add_executable(${target_name} main.cpp $<TARGET_OBJECTS:app_gui>)
add_executable(ui_snapshot EXCLUDE_FROM_ALL ui_snapshot/main.cpp $<TARGET_OBJECTS:app_gui>)
```
If it stays a regular target, add `EXCLUDE_FROM_ALL`. Then update the `TESTING.md` sentence "It is built with the tests" to say it is built on request.

### WR-02: Orphaned comment in the top-level CMakeLists refers to a block this phase deleted

**File:** `CMakeLists.txt:38`
**Issue:** The comment reads "Clang counterpart of the GNU warning set above (-Wpedantic left out, D-08)". The GNU block (`-Wall -Wextra -Wpedantic -Werror=return-type -Wno-unused`, plus `-march=native`) was removed from lines 26-35 in this phase, so "above" points at nothing. A reader cannot tell what "left out" is relative to, or that `-march=native` was deliberately not carried over. The clang flags are now the only warning configuration, and this comment is the only record of why `-Wpedantic` is absent.
**Fix:** Rewrite it to stand alone:
```cmake
# Warning set for Apple Clang. -Wpedantic is left out on purpose (D-08); -Wno-unused keeps the old fork's noise level.
```

## Info

### IN-01: Unused includes left behind by the removal of the language and portable code

**File:** `src/app/SettingsDialog.cpp:13`, `src/app/MainWindow.cpp:24`, `src/core/ApplicationSettings.cpp:8`
**Issue:** The last uses of `Application` (the `static_cast<Application*>(qApp)` calls, now deleted) were in `SettingsDialog.cpp` and `MainWindow.cpp`. Both still `#include "Application.h"`, and `ApplicationSettings.cpp` still includes `<QLocale>`, which was only needed for `DEFAULT_LANGUAGE`. `Application.h` also still includes `<QTimer>`, and `src/app/main.cpp` uses `QTimer` without including it itself, so it depends on that header staying.
**Fix:** Delete the three unused includes. Add `#include <QTimer>` (and `<memory>`) to `src/app/main.cpp` so it does not rely on `Application.h`.

### IN-02: `MainWindow::changeEvent` still handles `QEvent::LanguageChange`

**File:** `src/app/MainWindow.cpp:2142-2145`
**Issue:** `QEvent::LanguageChange` is only sent after a `QTranslator` is installed or removed. This phase removed `Application::installLanguage` and the only translator, so the branch (`retranslateUi(this); updateWindowTitle();`) can no longer run. It is dead code that suggests a language switch still exists.
**Fix:** Remove the `changeEvent` override, or at least the `LanguageChange` case, if nothing else in `MainWindow` relies on the override. Check the declaration in `MainWindow.h` first.

### IN-03: Windows-specific comments and a dead UNC branch survive in the touched files

**File:** `src/app/main.cpp:43`, `src/app/RelinkablePathVisualization.cpp:66`, `src/app/RelinkablePathVisualization.cpp:266`, `src/core/ImageViewBase.cpp:136-137`, `src/core/AtomicFileOverwriter.cpp:39`
**Issue:** The phase's stated aim is macOS only, and the leftover grep records presumably covered code but not comments:
- `main.cpp:43` still says "Doesn't work for Windows".
- `ImageViewBase.cpp:136-137` explains the `WA_OpaquePaintEvent` workaround as a Windows first-show problem.
- `RelinkablePathVisualization.cpp:66` handles `//host` as "the Windows `\\host\path` thing".
- `RelinkablePathVisualization.cpp:266` mentions the Windows XP button style.

The branch at line 66 can never be taken: `RelinkablePath::normalize` collapses leading `//` to `/` once the `_WIN32` clause is gone, as it already did on Unix before this phase. The backslash handling in `normalize` is deliberately kept (backlog 999.8) and is not part of this finding.
**Fix:** Drop or reword the comments. Delete the dead `startsWith("//")` branch, or record it in backlog 999.8 together with the backslash handling so it is not forgotten.

### IN-04: `ui_snapshot` confines output with a lexical prefix check only

**File:** `src/app/ui_snapshot/main.cpp:114-124`
**Issue:** The "output must lie below `build/`" rule is enforced with `QDir::cleanPath` and `startsWith(root + "/")`. A symlink inside `build/` that points elsewhere passes the check, and the tool then creates `tmp/` and `settings/` and writes PNG files through it. The root is also hard-wired to `${CMAKE_SOURCE_DIR}/build` rather than the actual binary directory, so a build tree outside `build/` still gets the same root. This is a developer-only tool run by the one user on their own machine, so the risk is low, but the header claims the run "never touches" anything outside. `QStandardPaths::AppDataLocation` is also not redirected. `scantailor-tests/ui_snapshot` under `~/Library/Application Support` is only read here, so nothing is written, but the claim is stronger than the code.
**Fix:** Compare `QFileInfo(out).canonicalFilePath()` after `mkpath` with the canonical root, or call `QStandardPaths::setTestModeEnabled(true)`. Soften the header and `TESTING.md` wording if neither is done.

### IN-05: `ui_snapshot` argument parsing accepts unknown options as the output directory

**File:** `src/app/ui_snapshot/main.cpp:98-108`
**Issue:** Any argument other than `--assert-no-language` counts as the positional output directory, so a typo such as `--assert-no-lang` becomes the path. It fails the root check with an unhelpful "not under" message. The `positional = 2` assignment is used as an error flag and reads as a bug at first glance.
**Fix:** Reject arguments that start with `--` with the usage message, and use a separate `bool tooManyArgs`.

### IN-06: `qt_compile` in the golden manifest is now the configure-time Qt version, and a miswired definition reads as "unknown"

**File:** `src/test_support/Manifest.cpp:29-31,78`, `src/test_support/CMakeLists.txt:43`
**Issue:** The key used to be the compile-time `QT_VERSION_STR` of the headers actually compiled against (D-15 changed this). It is now `Qt6_VERSION` as found at CMake configure time and passed in as a compile definition. The two agree in a normal build, and `TESTING.md` documents the change. They can differ if the build tree is not re-configured after the headers change, for example a manual `brew switch`. The `#ifndef ST_QT_COMPILE_VERSION` fallback to `"unknown"` also turns a broken definition into a manifest difference instead of a build error. `TestGoldenStore.cpp:254` only checks that the key exists.
**Fix:** If the reason for D-15 is only to avoid `QT_VERSION_STR`, `QT_VERSION_STR` is still valid in Qt 6, and `QT_VERSION_MAJOR` / `QT_VERSION_MINOR` / `QT_VERSION_PATCH` are the macros Qt documents. Otherwise keep the definition, but drop the fallback in `Manifest.cpp` so a missing definition fails to compile.

### IN-07: Windows-DLL leftovers outside the strip's reach, plus stale ignore rules

**File:** `src/imageproc/tests/CMakeLists.txt:21`, `.gitignore:127-140`
**Issue:** `remove_definitions(-DBUILDING_IMAGEPROC)` removes a definition that nothing defines anymore. It was the `dllexport` switch of the Windows build, and no `BUILDING_IMAGEPROC` or `__declspec` remains anywhere in `src/`. `.gitignore` still lists `*.deb`, `*.rpm`, `*.msi`, `*.AppImage`, `*.flatpak` and the flatpak-builder directories, although all Linux and Windows packaging was removed in this phase.
**Fix:** Delete the `remove_definitions` line and the packaging and flatpak ignore blocks.

---

_Reviewed: 2026-10-24_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
