# Phase 3: Qt 6 and macOS Only - Pattern Map

**Mapped:** 2026-10-24
**Files analyzed:** 14 groups (new or rewritten) plus deletion lists
**Analogs found:** 12 / 14 (all analog paths verified with `git ls-files`)

Phase 3 is mostly deletion. Only the entries below need analogs. All paths are git-tracked.

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match |
|---|---|---|---|---|
| `src/app/ui_snapshot/main.cpp` (new) | test tool (executable main) | request-response (offscreen render to PNG) | `src/qt_tests/main.cpp` | role-match |
| `src/app/CMakeLists.txt` (BUILD_TESTS block for ui_snapshot, plus Win32 deletion) | config | build | `src/qt_tests/CMakeLists.txt`, `src/pipeline_tests/CMakeLists.txt` | role-match |
| `src/test_support/Manifest.cpp` (D-15) | utility | transform | itself (lines 62-77) and `ST_*` definitions in `src/test_support/CMakeLists.txt` | exact |
| `src/test_support/CMakeLists.txt` (D-15) | config | build | itself (`target_compile_definitions` block) | exact |
| `CMakeLists.txt` (Qt find, `Qt_*_lib` removal) | config | build | itself lines 240-276 | exact |
| `src/core/CMakeLists.txt`, `src/foundation/CMakeLists.txt`, `src/qt_tests/CMakeLists.txt`, `src/pipeline_tests/CMakeLists.txt`, `src/imageproc/tests/CMakeLists.txt`, `src/dewarping/tests/CMakeLists.txt` | config | build | the Qt6 branch of `src/core/CMakeLists.txt:122-129` | exact |
| `README.md` (new top) | doc | n/a | existing README Features section (kept) | none for the top |
| `TESTING.md` (English rewrite) | doc | n/a | `.planning/phases/02-golden-baselines/records/ctest-phase2.md` for facts | partial |
| `.planning/phases/03-qt-6-and-macos-only/records/*.md` | record | n/a | `.planning/phases/02-golden-baselines/records/` | exact |
| `src/app/main.cpp`, `SettingsDialog.{cpp,ui}`, `Application.{h,cpp}`, `ApplicationSettings.cpp` | edit-by-deletion | n/a | n/a | n/a |

## Pattern Assignments

### `src/app/ui_snapshot/main.cpp` (new executable, offscreen)

**Analog:** `src/qt_tests/main.cpp` (lines 1-40). Copy the environment-before-QApplication pattern and the settings isolation guard. The tool must not touch the user's real settings.

```cpp
// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
const QString tmpRoot = QString::fromUtf8(TEST_TMP_ROOT);
QDir().mkpath(tmpRoot);
qputenv("QT_QPA_PLATFORM", "offscreen");
qputenv("LC_ALL", "en_US.UTF-8");
qputenv("TMPDIR", tmpRoot.toUtf8());
QCoreApplication::setOrganizationName("scantailor-tests");
QCoreApplication::setApplicationName("qt_tests");
QSettings::setDefaultFormat(QSettings::IniFormat);
QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tmpRoot + "/qt_tests-settings");
QApplication app(argc, argv);
const QString settingsFile = QSettings().fileName();
if (!settingsFile.startsWith(tmpRoot)) { /* fprintf to stderr; return 2 */ }
```

Use a distinct application name and settings dir (for example `ui_snapshot` / `ui_snapshot-settings`). Then construct `MainWindow` and `SettingsDialog`, call `QWidget::grab().save(path, "PNG")`, and take the output directory from argv or a `SNAPSHOT_DIR` compile definition. Use `main.cpp` of the app (`src/app/main.cpp:1-34`) as the analog for how to build `Application`, the colour scheme (`ColorSchemeFactory`, `ColorSchemeManager`, `IconProvider`) and `MainWindow`, so the screenshot shows the real styling. Includes use bare module paths (`<core/Application.h>`, `"MainWindow.h"`). Keep the repository copyright header and clang-format style.

Gotcha: `MainWindow.cpp` and `SettingsDialog.cpp` are compiled into the `scantailor-advanced` executable (`gui_only_sources`), not a library. The new target needs those sources. Either list the shared sources in a variable reused by both targets, or make a small static library. The `main.cpp` entry must be excluded from the shared list. Set `CMAKE_AUTOUIC`/`AUTORCC` (already ON in this file) and the same `resource_files`.

### `src/app/CMakeLists.txt` (ui_snapshot block under BUILD_TESTS)

**Analog:** `src/qt_tests/CMakeLists.txt` (whole file) for the offscreen test properties, `src/pipeline_tests/CMakeLists.txt` for definitions.

```cmake
add_executable(qt_tests ${sources})
target_link_libraries(qt_tests PRIVATE core foundation ${Qt_Test_lib} ... ${EXTRA_LIBS})
target_include_directories(qt_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/src")
target_compile_definitions(qt_tests PRIVATE TEST_TMP_ROOT="${CMAKE_BINARY_DIR}/test-tmp")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test-tmp")
add_test(NAME qt_tests COMMAND qt_tests)
set_tests_properties(qt_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LC_ALL=en_US.UTF-8;TMPDIR=${CMAKE_BINARY_DIR}/test-tmp")
```

Wrap in `if (BUILD_TESTS)` inside `src/app/CMakeLists.txt` (the top-level `src/CMakeLists.txt:10-15` gates the other test dirs the same way). Per D-12 the language-control and no-`.qm` assertion can be a `Qt Test` case in `src/qt_tests/tst_ux_qt.cpp` (analog: existing `tst_ux_qt.{h,cpp}`) or an `add_test` script. Do not add the new test as an eighth ctest suite unless the suite count (7) is deliberately changed in `TESTING.md`.

Deletion in this file: the `win32_resource_file` block (lines ~50-80, `if (WIN32)` through `endif()`), `WIN32` keyword and `${win32_resource_file}` in `add_executable`, the `install(TARGETS ...)` line, the `if (Qt6_FOUND AND WIN32)` windeployqt block, and the `translation_sources(...)` call. Keep everything else. Final shape: `add_executable(${target_name} ${gui_only_sources} ${gui_only_ui_files} ${resource_files})`. Note: on macOS `MACOSX_BUNDLE` is not set today, so do not add it.

### `src/test_support/Manifest.cpp` and `CMakeLists.txt` (D-15)

**Analog:** the existing `ST_*` pattern. Compile definitions are fed from CMake and read as string literals:

```cmake
target_compile_definitions(
    test_support
    PRIVATE ST_CMAKE_VERSION="${CMAKE_VERSION}" ... ST_FP_CONTRACT="${st_fp_contract}")
```
```cpp
values["qt_compile"] = nonEmpty(QString::fromLatin1(QT_VERSION_STR));   // Manifest.cpp:73, replace
values["xcode"]      = nonEmpty(QString::fromLatin1(ST_XCODE_VERSION)); // pattern to copy
```

Change: add `ST_QT_VERSION="${Qt6_VERSION}"` to the definitions and use `ST_QT_VERSION` at line 73. The string must equal `QT_VERSION_STR` (for example `6.12.0`; verify `Qt6_VERSION` matches the header's value exactly, since the goldens' `MANIFEST.txt` must not change). Manifest compare is warn-only (Phase 2 D-09), so a mismatch would warn rather than fail; check the warning is absent. Also check that no other file in `src/` or the test directories contains `QT_VERSION` (grep must be empty, including `#include <QtGlobal>` use only for this token).

### Qt find and `Qt_*_lib` replacement (`CMakeLists.txt:240-276`)

**Analog:** the Qt6 branch, which stays:

```cmake
find_package(Qt6 COMPONENTS Core Gui Widgets Xml Network LinguistTools OpenGL Svg OpenGLWidgets Test CONFIG)
...
set(Qt_Core_lib Qt::Core) ... set(Qt_Test_lib Qt::Test)
```

Target: `find_package(Qt6 <floor> REQUIRED COMPONENTS Core Gui Widgets Xml Network OpenGL Svg OpenGLWidgets Test CONFIG)`. Drop `LinguistTools` (D-05), `qt_min_version`, `QT_BINDIR` and the whole Qt5 `else()`. Delete the `Qt_*_lib` variables and replace each use with the `Qt::X` or `Qt6::X` target (the Qt6 branch already uses `Qt::`; `Qt6::OpenGLWidgets` appears at line 410). Uses to rewrite (from grep): `src/foundation/CMakeLists.txt:34`, `src/test_support/CMakeLists.txt:6`, `src/qt_tests/CMakeLists.txt:9`, `src/pipeline_tests/CMakeLists.txt:6`, `src/imageproc/tests/CMakeLists.txt:25`, `src/dewarping/tests/CMakeLists.txt:6`. Rewrite `src/core/CMakeLists.txt:122-137` to the single Qt6 `target_link_libraries` call (the `if(Qt6_FOUND)` side) and remove `translation_sources(scantailor ...)` at line 139. Check for further `Qt_*_lib` uses in filters' CMake files with `git grep -n "Qt_.*_lib" -- '*.txt' '*.cmake'` before deleting. `OpenGL`, `OpenGLWidgets` and `Svg` stay in this phase (STRIP-07 is Phase 6). `cmake/ListItemsPrepend.cmake` stays (used by `src/app/CMakeLists.txt:resource_files`); `cmake/SetDefaultBuildType.cmake` needs a grep for its include before deciding.

### `.planning/phases/03-qt-6-and-macos-only/records/*.md`

**Analog:** `.planning/phases/02-golden-baselines/records/` (tracked: `closing-check.md`, `ctest-phase2.md`, `sanitizer-findings.md`, `access-approval.md`, `round-trip.md`, `test-cases-asan.txt`, others). Pattern from `closing-check.md`:

- Title naming the plan; a sentence on the commit checked and normalisation (`{repo}` for the repository path, `~` for home).
- Each command in a fenced block as `$ cmd`, output, then `exit status: N`.
- Tables for per-suite results (as in `ctest-phase2.md`: `# | Test | Result | Time`, plus the line `100% tests passed out of 7`).
- `sanitizer-findings.md` format for the D-14 comparison (12 UBSan, 0 ASan, 9 project locations).

Suggested records: `ctest-per-commit.md`, `leftover-grep.md` (criterion 1 search, expected empty), `ui-screenshots.md` (D-12 verdict plus PNG paths), `fresh-clone.md` (D-13, clone under gitignored `build/`), `sanitizer-compare.md` (D-14), `backlog-999.8.md` or similar (D-03; check next free number with the GSD backlog command).

### `README.md` (new top) and `TESTING.md`

No code analog for the README top; use D-08 and `CMakePresets.json` (`macos-release`, `macos-debug`, `macos-asan` configure/build/test presets). Keep the Features section (about lines 109-440) byte-for-byte. For `TESTING.md` take facts from `.planning/phases/02-golden-baselines/records/ctest-phase2.md` (seven suites, offscreen environment, `build/<preset>/test-tmp`), `sanitizer-findings.md` and the existing `src/pipeline_tests/CMakeLists.txt` definitions (`ARTIFACT_DIR` is `${CMAKE_BINARY_DIR}/test-artifacts`). Write in English with British spelling.

## Edit-by-deletion notes (resolve with `unifdef`, read every diff)

| File | Goes | Stays |
|---|---|---|
| `src/app/main.cpp:20-34` | whole `#if QT_VERSION_MAJOR == 5` block (xcb fallback, HighDPI attributes, rounding policy) | Qt 6 side (nothing inside, verify the `#else`) |
| `src/app/main.cpp:36-39` | `#ifdef _WIN32` setLibraryPaths | n/a |
| `src/app/main.cpp:47-51` | portable `setPath` branch; `app.isPortableVersion()` | `setDefaultFormat(IniFormat)` |
| `src/dewarping/DistortionModelBuilder.cpp:9`, `DetectVertContentBounds.cpp:11` | inverted gate `QT_VERSION_MAJOR > 5 \|\| QT_VERSION_MINOR > 9` | the TRUE side on Qt 6 (the `> 5` branch). Verify polarity against dewarp goldens |
| other 17 files with `QT_VERSION` | the Qt 5 side (`MAJOR == 5 && MINOR < 14`) | the Qt 6 side (`KeepEmptyParts` and similar) |
| `src/core/Utils.cpp:13,21` | `_WIN32` `MoveFileExW` | `rename()` |
| `src/imageproc/ByteOrder.h:7` | `winsock2.h` | `netinet/in.h` |
| `src/core/{Dark,Light}ColorScheme.cpp:75` | Windows base 13 | 16 |
| `src/core/RelinkablePath.cpp:23` | labelled `_WIN32` branch only | rest of `normalize` (D-03) |
| `src/core/zones/ZoneContextMenuInteraction.{h,cpp}` | non-`Q_OS_MAC` side | Mac side |
| `src/app/MainWindow.cpp:1327` | `#else` of `Q_OS_UNIX` | `play ...` command (D-04) |
| `src/core/ImageViewBase.cpp` top | `QGLWidget` macro hack | n/a (do after CMake Qt 5 removal, per Pitfall 7) |
| `src/core/Application.{h,cpp}`, `ApplicationSettings.cpp:210-216`, `SettingsDialog.{cpp,ui}`, `main.cpp:53`, `MainWindow.cpp:1537` | translation and language code (D-05) | `tr()` calls (D-06) |
| `config.h.in` | `PORTABLE_CONFIG_DIR`, `TRANSLATION_DIRS`, `PLUGIN_DIRS` | other defines. Remove the matching `CMakeLists.txt` variables and the `src/resources/unix/...desktop.in` `configure_file` BEFORE deleting `src/resources/unix/` (Pitfall 14) |
| `CMakeLists.txt` | Qt 5 branch, `WIN32`/`MSVC`/`MINGW`/`WIN_XP`/`DEPENDENCIES_ROOT`, `PORTABLE_VERSION`, `CMAKE_COMPILER_IS_GNUCC` block with `-march=native`, translations (~355-381), CPack (~502-534), `install()` and uninstall | Clang block byte-for-byte (`-ffp-contract=off`, warnings). BUILD_TESTS option (also fix its stale description text) |

## Files to delete (tracked, verified)

- `.github/` (6 files: `dependabot.yml`, `workflows/{ci,cmake,flatpak,lint,release}.yml`)
- `flatpak/org.scantailor.Advanced.json`
- `build-deb.sh`, `build-windows.sh`, `scripts/post_issue_followups.py`
- `cmake/AddDynamicLibraryLocations.cmake`, `CopyToBuildDir.cmake`, `UpdateTranslations.cmake`, `cmake_uninstall.cmake.in`, `copy_to_build_dir.cmake.in`
- `src/resources/unix/mime/scantailor-project.xml`, `src/resources/unix/scantailor-advanced.desktop.in`
- `src/resources/win32/resources.rc.in`, `src/resources/win32/scantailor-advanced.ico`
- `src/resources/scantailor-advanced.png`
- `src/translations/*.ts` (list with `git ls-files src/translations`)

Keep `cmake/ListItemsPrepend.cmake`; decide `cmake/SetDefaultBuildType.cmake` after grepping its include. `src/resources/appicon.svg` stays.

## Shared Patterns

### Per-commit gate (D-11)
**Apply to:** every strip commit. Pattern from `ctest-phase2.md`: `cmake --preset <p> && cmake --build --preset <p> && ctest --preset <p> --output-on-failure` for `macos-release` and `macos-debug`, result 7/7, then `git status --porcelain -- src` and `git diff --stat -- 'src/pipeline_tests/golden'` empty. `UPDATE_GOLDENS` unset.

### Formatting and headers
New C++ files: the two-line copyright header shown above, 2-space indent, `xcrun clang-format -i <file>` (the plain binary may be missing), include guards `FILENAME_H` style for headers (no new headers expected).

### Test environment
`QT_QPA_PLATFORM=offscreen;LC_ALL=en_US.UTF-8;TMPDIR=${CMAKE_BINARY_DIR}/test-tmp` set both in code before `QApplication` and in `set_tests_properties`, as in `src/qt_tests/`.

## No Analog Found

| File | Reason |
|---|---|
| `README.md` new top | Prose; follow D-08 and `CMakePresets.json` |
| `src/app/ui_snapshot/` as a separate GUI-linked tool | No existing test executable links the app's GUI sources (`MainWindow`, `SettingsDialog`); the source-sharing approach must be designed in the plan |

## Metadata

**Analog search scope:** `src/qt_tests`, `src/pipeline_tests`, `src/test_support`, `src/app`, `src/core/CMakeLists.txt`, top-level `CMakeLists.txt`, `cmake/`, Phase 2 records.
**Pattern extraction date:** 2026-10-24
