# Phase 2: Golden Baselines - Pattern Map

**Mapped:** 2026-10-23
**Files analyzed:** 22 new/modified
**Analogs found:** 19 / 22 (all analog paths verified git-tracked via `git ls-files`)

Project-wide rules for every new file: copyright header copied from an existing file (first two lines of `src/imageproc/tests/main.cpp`), include guards `SCANTAILOR_<MODULE>_<NAME>_H_` (no `#pragma once`), C++17 only, clang-format Chromium by hand, module dirs as include roots (`#include <PageId.h>`), suite names `<Module><Subject>TestSuite`, cases `test_<snake_case>`.

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|---|---|---|---|---|
| `src/CMakeLists.txt` (modify: 3 `add_subdirectory` in `if (BUILD_TESTS)`) | config | n/a | same file, lines 10-12 | exact |
| `src/test_support/CMakeLists.txt` | config | n/a | `src/imageproc/tests/CMakeLists.txt` (library variant: `src/qt_tests/CMakeLists.txt` for include dirs) | role-match |
| `src/test_support/Rng.h`, `BitmapFont.{h,cpp}`, `PageGenerator.{h,cpp}` | utility | transform | none in repo (`src/imageproc/tests/Utils.{h,cpp}` only for shape; uses `rand()`, do not copy) | partial |
| `src/test_support/ImageCompare.{h,cpp}` | utility | transform | `src/imageproc/tests/Utils.cpp` (`fuzzyCompare` style) | partial |
| `src/test_support/GoldenStore.{h,cpp}`, `Manifest`, `TextGolden`, `TestEnvironment` | utility | file-I/O | none | no analog |
| `src/pipeline_tests/CMakeLists.txt` | config | n/a | `src/core/tests/CMakeLists.txt` | exact |
| `src/pipeline_tests/main.cpp` (global fixture + Application) | test | event-driven | `src/qt_tests/main.cpp` (app creation), `src/app/main.cpp:42-60` (settings setup), `src/core/tests/main.cpp` | role-match |
| `src/pipeline_tests/HeadlessPipeline.{h,cpp}` | service | request-response | `src/app/MainWindow.cpp:2059-2101` (`createCompositeTask`) | role-match |
| `src/pipeline_tests/TestFlatDocument.cpp`, `TestBookSpread.cpp`, `TestMixedPage.cpp` | test | request-response | `src/core/tests/TestPageId.cpp` (structure) | role-match |
| `src/pipeline_tests/TestGeneratorDeterminism.cpp`, `TestComparatorSelfTest.cpp`, `TestGoldenStore.cpp` | test | transform | `src/core/tests/TestPageId.cpp` | role-match |
| `src/pipeline_tests/TestProjectRoundTrip.cpp` | test | file-I/O | `src/core/tests/TestPageSequence.cpp` (ProjectPages usage) | partial |
| `src/pipeline_tests/golden/*` (PNG, txt, MANIFEST.txt) | data | file-I/O | none | no analog |
| `src/dewarping/tests/CMakeLists.txt`, `main.cpp` | config/test | n/a | `src/imageproc/tests/CMakeLists.txt`, `src/imageproc/tests/main.cpp` | exact |
| `src/dewarping/tests/Test{DistortionModel,CylindricalDewarper,TextLineTracer}.cpp` | test | transform | `src/imageproc/tests/TestBinarize.cpp` (namespace + suite) | role-match |
| `src/imageproc/tests/TestBinarize.cpp` (modify) | test | transform | itself (skeleton) + `TestScale.cpp` | exact |
| `src/imageproc/tests/TestScale.cpp` (modify, test code only) | test | transform | itself lines 45-73 | exact |
| `src/imageproc/tests/CMakeLists.txt` (modify: link `test_support`) | config | n/a | itself | exact |
| `CMakePresets.json` (modify test env, optional) | config | n/a | itself | exact |

## Pattern Assignments

### `src/CMakeLists.txt` (config)

**Analog:** same file. Current block (lines 10-12):
```cmake
if (BUILD_TESTS)
  add_subdirectory(qt_tests)
endif()
```
Add exactly three lines inside it (`test_support` first, then `pipeline_tests`, `dewarping/tests`). Nothing else in non-test CMake may change (D-16, P13). Note `add_subdirectory(dewarping/tests)` is wired here so `src/dewarping/CMakeLists.txt` stays untouched.

### `src/pipeline_tests/CMakeLists.txt` and `src/dewarping/tests/CMakeLists.txt` (config)

**Analog:** `src/core/tests/CMakeLists.txt` and `src/imageproc/tests/CMakeLists.txt`.
```cmake
remove_definitions(-DBUILDING_IMAGEPROC)          # imageproc/tests only; dewarping tests need the analogous remove for its own BUILDING_ define if present
add_executable(imageproc_tests ${sources})
target_link_libraries(
    imageproc_tests
    PRIVATE imageproc math foundation ${Qt_Widgets_lib} ${Qt_Xml_lib}
    Boost::unit_test_framework ${EXTRA_LIBS})

add_test(NAME imageproc_tests COMMAND imageproc_tests --log_level=message)
```
core variant: `PRIVATE core Boost::unit_test_framework ${EXTRA_LIBS}`.
Additions: `dewarping_tests` links `dewarping imageproc math foundation test_support ${Qt_Xml_lib} ${Qt_Gui_lib} ${Qt_Core_lib}` (Xml mandatory, RESEARCH Pattern 6). `pipeline_tests` links `core test_support Boost::unit_test_framework ${EXTRA_LIBS}`. After each `add_test` add
`set_tests_properties(<name> PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LC_ALL=en_US.UTF-8;QT_LOGGING_RULES=qt.svg=false")`. Also add it for the existing `qt_tests` registration (`src/qt_tests/CMakeLists.txt`, `add_test(NAME qt_tests COMMAND qt_tests)`) per TEST-11. Pass golden/artifact dirs via `target_compile_definitions(... GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden" ARTIFACT_DIR="${CMAKE_BINARY_DIR}/test-artifacts")`.

### `src/test_support/CMakeLists.txt` (config, static lib)

**Analog:** `src/qt_tests/CMakeLists.txt` for link/include style:
```cmake
target_link_libraries(
    qt_tests
    PRIVATE core foundation ${Qt_Test_lib} ${Qt_Core_lib} ${Qt_Gui_lib} ${Qt_Widgets_lib} ${EXTRA_LIBS})
target_include_directories(qt_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/src")
```
Use `add_library(test_support STATIC ...)` with `PUBLIC ${Qt_Core_lib} ${Qt_Gui_lib}` and `target_include_directories(test_support PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")` so consumers include `<PageGenerator.h>` by bare name. Must not depend on `core`.

### `src/pipeline_tests/main.cpp` (test, global fixture)

**Analogs:** `src/core/tests/main.cpp` / `src/imageproc/tests/main.cpp` (old style):
```cpp
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define BOOST_AUTO_TEST_MAIN

#include <boost/test/unit_test.hpp>
```
New executable uses `BOOST_TEST_MODULE pipeline_tests` + `BOOST_GLOBAL_FIXTURE` (RESEARCH "Global fixture" example, section Code Examples) instead, creating `Application` from `core/Application.h` (not `QApplication`; `src/qt_tests/main.cpp` shows the plain-QApplication pattern NOT to copy).

**Settings setup order to mirror** (`src/app/main.cpp:42-52`):
```cpp
Application::setApplicationName(APPLICATION_NAME);
Application::setOrganizationName(ORGANIZATION_NAME);
QSettings::setDefaultFormat(QSettings::IniFormat);
if (app.isPortableVersion()) {
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, app.getPortableConfigPath());
}
```
Tests instead call `QSettings::setPath(IniFormat, UserScope, tmp)` before constructing `Application`, and assert `QSettings().fileName()` starts with tmp before the first `ApplicationSettings::getInstance()`.

### `src/pipeline_tests/HeadlessPipeline.{h,cpp}` (service, request-response)

**Analog:** `src/app/MainWindow.cpp:2059-2101` `createCompositeTask`: build tasks last stage to first, each receiving the next:
```cpp
outputTask = m_stages->outputFilter()->createTask(page.id(), m_thumbnailCache, m_outFileNameGen, batch, debug);
pageLayoutTask = m_stages->pageLayoutFilter()->createTask(page.id(), outputTask, batch, debug);
selectContentTask = m_stages->selectContentFilter()->createTask(page.id(), pageLayoutTask, batch, debug);
deskewTask = m_stages->deskewFilter()->createTask(page.id(), selectContentTask, batch, debug);
pageSplitTask = m_stages->pageSplitFilter()->createTask(page, deskewTask, batch, debug);
fixOrientationTask = m_stages->fixOrientationFilter()->createTask(page.id(), pageSplitTask, batch);
```
Headless version constructs the `Task` classes directly (`filter = nullptr`, own `Settings`), signatures in RESEARCH Pattern 2, entry `fixTask->process(NullTaskStatus(), FilterData(image))`. Mandatory default loading per stage before processing (Pattern 2 block: `applyRotation`, `updatePage`, `setPageParams` x3, `setParams`). Pre-create `out/cache`. Real `ThumbnailPixmapCache` in a temp dir, destroyed before `Application`. Set output DPI explicitly.

### Test files (`Test*.cpp` in pipeline_tests, dewarping/tests)

**Analog:** `src/core/tests/TestPageId.cpp`:
```cpp
#include <ImageId.h>
#include <PageId.h>

#include <QString>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(CorePageIdTestSuite)

BOOST_AUTO_TEST_CASE(test_default_constructor_is_null) {
  const PageId id;
  BOOST_CHECK(id.isNull());
}
...
BOOST_AUTO_TEST_SUITE_END()
```
imageproc/dewarping files additionally wrap in `namespace <module> { namespace tests { ... } }` (see `TestBinarize.cpp` below). Use names like `FlatDocumentTestSuite`, `ComparatorSelfTestSuite`, `GeneratorDeterminismTestSuite`, `ProjectRoundTripTestSuite`, `GoldenStoreTestSuite`, `BookSpreadTestSuite`, `MixedPageTestSuite`. Parse out-parameter style from `PageId::subPageFromString("left", &ok)` for any new parse helper. Expected failures: `XFAIL_CHECK` trip-wire macro (RESEARCH Pattern 5), not a bare `expected_failures` decorator.

### `src/imageproc/tests/TestBinarize.cpp` (modify)

**Analog:** itself. Keep header/namespace/suite skeleton, delete the `#if 0` block:
```cpp
#include <Binarize.h>
#include <BinaryImage.h>

#include <QImage>
#include <QSize>
#include <boost/test/unit_test.hpp>

#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

BOOST_AUTO_TEST_SUITE(BinarizeTestSuite)
...
BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc
```
Inputs come from `test_support::PageGenerator`, not `Utils.h` (rand-based). Add `test_support` to `src/imageproc/tests/CMakeLists.txt` `PRIVATE` link list (`imageproc_tests` target, shown above).

### `src/imageproc/tests/TestScale.cpp` (modify, test code only)

**Analog:** itself lines 45-73 (`checkScale` compares `scaleToGray` with `QImage::scaled(..., Qt::SmoothTransformation)`, `test_random_image` uses `rand() % 256`). Per D-17/P12: replace the Qt reference with an own box-average reference for 50x50 (tolerance 1), replace the 80x80 check with a mean-error bound; replace `rand()` with test_support `Rng`. No production edit.

### `src/test_support/*` (utility)

No codebase analog for generator, golden store, manifest, text golden (see below). For `ImageCompare` model function style on `fuzzyCompare` in `src/imageproc/tests/Utils.cpp` (not read in full; planner should `Read` it before writing). Use RESEARCH Pattern 1 (splitmix64), Pattern 3/4 for metrics, store, manifest.

## Shared Patterns

### Test registration
**Source:** `src/imageproc/tests/CMakeLists.txt` last line. **Apply to:** every new executable.
`add_test(NAME x COMMAND x --log_level=message)` plus `set_tests_properties(... ENVIRONMENT ...)` as above.

### Settings and locale isolation
**Source:** `src/app/main.cpp:42-52` (what production sets) and RESEARCH global fixture. **Apply to:** `pipeline_tests` only (dewarping and imageproc tests need no Application).
Pin `LC_ALL=en_US.UTF-8` via CTest ENVIRONMENT; never `atof`/`printf` for numbers in tests.

### Stage default loading
**Source:** RESEARCH Pattern 2 (each stage's `Utils::buildDefault*`). **Apply to:** HeadlessPipeline and all scenario tests. Add a sanity assertion (content rect valid, output not blank) before recording any golden (D-05).

### Golden update gating
**Source:** none in repo; RESEARCH Pattern 4. **Apply to:** all image and text golden comparisons: rewrite only when `UPDATE_GOLDENS=1`, comparison failed, sanity passed; refresh `MANIFEST.txt` each update run; artifacts to `ARTIFACT_DIR/<test>/` on failure.

### Non-test diff proof (TEST-04)
Only these non-test paths may differ from `cde6825`: the three `add_subdirectory` lines in `src/CMakeLists.txt` (plus `CMakePresets.json` test settings). No `.cpp/.h/.ui/.qrc` change outside `tests/`, `test_support/`, `pipeline_tests/`, `qt_tests/`.

## No Analog Found

| File | Role | Data Flow | Reason |
|---|---|---|---|
| `src/test_support/PageGenerator`, `BitmapFont`, `Rng.h` | utility | transform | Only `imageproc/tests/Utils` exists and uses `rand()`; use RESEARCH Pattern 1 |
| `src/test_support/GoldenStore`, `Manifest`, `TextGolden`, `TestEnvironment` | utility | file-I/O | No golden/fixture infrastructure exists; use RESEARCH Patterns 3-5 |
| `src/pipeline_tests/golden/*` | data | file-I/O | No committed golden data exists |
| `TestProjectRoundTrip.cpp` core body | test | file-I/O | No existing `ProjectWriter`/`ProjectReader`/`StageSequence` test; `TestPageSequence.cpp` only shows `ProjectPages` construction; use RESEARCH "Project round trip" shape |

## Metadata

**Analog search scope:** `src/*/tests/`, `src/qt_tests/`, `src/CMakeLists.txt`, `src/app/main.cpp`, `src/app/MainWindow.cpp`.
**Files scanned:** ~12 read, ~50 listed (all verified tracked).
**Not read in full (planner should read before implementing):** `src/imageproc/tests/Utils.{h,cpp}`, `src/core/tests/TestPageSequence.cpp`, `src/foundation/tests/CMakeLists.txt`, `src/math/tests/CMakeLists.txt`, `CMakePresets.json`.
**Pattern extraction date:** 2026-10-23
