---
last_mapped_commit: 5eaac1884cdcabb6514bd632114f688631bd8dbc
last_mapped_at: 2026-10-18
---
# Testing Patterns

**Analysis Date:** 2026-10-18 (patched after Phases 2 and 3, D-10; not re-mapped)

## Test Framework

**Runner:**
- Boost.Test (`Boost::unit_test_framework`) for the core, foundation, math, imageproc, dewarping and pipeline suites.
- Qt Test (`QTest`, `QSignalSpy`) for Qt-facing logic and widgets, under `src/qt_tests/`.
- CTest is the driver: `add_test(NAME core_tests COMMAND core_tests --log_level=message)` in `src/core/tests/CMakeLists.txt`.
- Config: tests are enabled by the CMake option `BUILD_TESTS` (default ON). Seven ctest suites: `foundation_tests`, `math_tests`, `imageproc_tests`, `core_tests`, `qt_tests`, `pipeline_tests`, `dewarping_tests`.

**Assertion Library:**
- Boost: `BOOST_CHECK`, `BOOST_CHECK_EQUAL`, `BOOST_REQUIRE` (for fatal checks).
- Qt: `QCOMPARE`, `QVERIFY`.

**Run Commands:**

```bash
cmake --preset macos-release && cmake --build --preset macos-release
ctest --preset macos-release                          # all seven suites; good run: "100% tests passed out of 7"
ctest --preset macos-debug                            # after cmake --preset macos-debug and the build
ctest --preset macos-asan                             # Debug + address,undefined sanitizers
build/macos-release/core_tests --log_level=message    # one Boost suite with messages
ctest --preset macos-release -R core_tests            # one suite through ctest
```

Release and Debug must both pass before a code change is committed (`TESTING.md`). Watch mode and coverage targets are not configured in CMake.

**CI:**
- None. The `.github/` workflows were removed in Phase 3; the gate is the local Release and Debug ctest run.
- Formatting is checked by hand with `xcrun clang-format --dry-run -Werror <file>` (`TESTING.md`).

## Test File Organization

**Location:**
- Separate `tests/` directory inside each library: `src/core/tests/`, `src/foundation/tests/`, `src/math/tests/`, `src/imageproc/tests/`, `src/dewarping/tests/`.
- Qt tests live in `src/qt_tests/` (not under a library). Pipeline tests live in `src/pipeline_tests/`, with generated-page support code in `src/test_support/` and goldens in `src/pipeline_tests/golden/{flat,book,mixed}/`.
- Tests are not co-located with source files.

**Naming:**
- Test source files: `Test<Subject>.cpp`, e.g. `src/core/tests/TestPageId.cpp`, `src/imageproc/tests/TestConnCompEraserExt.cpp`.
- Each test directory has a `main.cpp` entry point, plus a `CMakeLists.txt` listing the sources in a `set(sources ...)` block.
- Qt tests: `tst_core_qt.cpp/.h`, `tst_ux_qt.cpp/.h`.

**Structure:**

```
src/core/tests/
├── CMakeLists.txt        # set(sources ...) + add_executable + add_test
├── main.cpp              # Boost test runner entry point
├── TestPageId.cpp
├── TestDeskewParams.cpp
└── ...
```

## Test Structure

**Suite Organization (Boost):**

```cpp
// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <PageId.h>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(CorePageIdTestSuite)

BOOST_AUTO_TEST_CASE(test_default_constructor_is_null) {
  const PageId id;
  BOOST_CHECK(id.isNull());
}

BOOST_AUTO_TEST_SUITE_END()
```

Source: `src/core/tests/TestPageId.cpp`, `src/imageproc/tests/TestDpi.cpp`.

**Patterns:**
- One suite per file, named `<Module><Subject>TestSuite`.
- Each case is a single behaviour: one constructor, one parse function, one operator.
- Setup is inline at the top of each case. There are no fixtures or `BOOST_FIXTURE_TEST_CASE` in the codebase.
- Teardown is not used; objects are scoped to the case.
- Assertions use `const` locals for inputs: `const ImageId img1("/a", 0);`.

**Qt Test pattern:**
- `QApplication` is required for widget tests. Test classes are `QObject` subclasses with `private slots:` methods named `testXxx` (e.g. `testButtonClickEmitsSignal`, `testLineEditAcceptsInput`, `testErrorWidgetCreation` in `src/qt_tests/tst_ux_qt.cpp`).
- Widgets are created without `show()`; state is checked directly.
- Signals are checked with `QSignalSpy`, and input is simulated with `QTest::mouseClick` and `QTest::keyClicks`.

## Mocking

**Framework:** None. No mocking library (gmock, trompeloeil, etc.) is linked.

**Patterns:**
- Test doubles are hand-written where needed. Few test files define stubs; search `src/imageproc/tests/TestSkewFinder.cpp` for the existing approach before adding one.
- Prefer constructing real value types (`ImageId`, `PageId`, `Dpi`, `Margins`) over stubbing them.

**What to Mock:**
- Hand-written interfaces only, where a dependency is expensive (file I/O, image loading). Keep stubs in the test file.

**What NOT to Mock:**
- Value types and pure math (`math/`, `foundation/`, `imageproc/` geometry and transforms). Test them directly.
- Qt widgets: create them, do not mock them. Use `QTest` to drive them.

## Fixtures and Factories

**Test Data:**
- Unit tests use inline literals. Example from `TestPageId.cpp`: `const ImageId img1("/a", 0);`. Paths like `"/path/to/image.tiff"` are used as opaque identifiers; no files are read.
- Image-processing tests build small in-memory buffers rather than loading fixture files.

**Location:**
- Unit tests keep data inline in the test file, or in a helper at the top of that file.
- `pipeline_tests` generate their pages in code with fixed seeds (`src/test_support/PageGenerator`). No scan is stored in the repository. Goldens are recorded with `UPDATE_GOLDENS=1` on the Release build only (`TESTING.md`).
- Known defects are recorded with `XFAIL_CHECK(condition, "tag")` (`src/test_support/ExpectedFailure.h`), one backlog item each in `.planning/ROADMAP.md`.

## Coverage

**Requirements:** None enforced. No coverage threshold appears in CMake or CI.

**View Coverage:**

```bash

# Manual gcov/lcov flow (compiler-dependent; not part of the presets and not described in TESTING.md any more)

lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage-html
```

## Test Types

**Unit Tests:**
- Boost suites for `core`, `foundation`, `math`, `imageproc`: pure functions, value types, parsers, serialization (XML round-trips for `Margins`, `Dpi`), and algorithm helpers (`ContentSpanFinder`, `ObliqueFinder`, `Binarize`, `Morphology`, `ConnCompEraser`).
- Boost test cases in six executables (the count is not tracked here; run `ctest` for the suites).

**Qt Tests:**
- `test*` slots across `src/qt_tests/` (`TestCoreQt`, `TestUxQt`). Cover `Units`, `foundation::Utils`, `SmartFilenameOrdering`, and a set of UX widgets (`CollapsibleGroupBox`, `ErrorWidget`).

**Integration Tests:**
- `pipeline_tests` (Phase 2) run the six stages headlessly on generated flat, book and mixed pages, round-trip a project file and compare the results with goldens.

**UI snapshot:**
- `src/app/ui_snapshot` renders the main window and Settings dialog offscreen to PNG and text. Built with the tests, not a ctest suite.

**E2E Tests:**
- Not used.

## Common Patterns

**Equality and ordering (value types):**

```cpp
BOOST_AUTO_TEST_CASE(test_equality) {
  const ImageId img1("/a", 0);
  const ImageId img2("/b", 0);
  const PageId p1(img1, PageId::SINGLE_PAGE);
  const PageId p2(img1, PageId::SINGLE_PAGE);
  BOOST_CHECK(p1 == p2);
  BOOST_CHECK(p1 != PageId(img2, PageId::SINGLE_PAGE));
}
```

**Parse functions with out-parameter flag:**

```cpp
bool ok = false;
BOOST_CHECK(PageId::subPageFromString("left", &ok) == PageId::LEFT_PAGE);
BOOST_CHECK(ok);
```

**Floating-point comparisons:**
- Use `BOOST_CHECK_CLOSE` or an explicit tolerance for geometry and math results. Exact `BOOST_CHECK_EQUAL` is acceptable only for integers and enums.

**Async Testing:**
- Not used. Worker tasks are not exercised by the test suites.

**Error Testing:**
- Assert the failure flag rather than catching: `PageId::subPageFromString("invalid", &ok); BOOST_CHECK(!ok);`.
- For exception paths, use `BOOST_CHECK_THROW(expr, ExceptionType)`.

## Adding a New Test

1. Create `src/<module>/tests/Test<Subject>.cpp` with the copyright header.
2. Add the file to the `set(sources ...)` list in `src/<module>/tests/CMakeLists.txt`.
3. Wrap cases in `BOOST_AUTO_TEST_SUITE(<Module><Subject>TestSuite)` and name cases `test_<behaviour>`.
4. Run `xcrun clang-format -i` on the new file.
5. Build with the `macos-release` preset and run the suite directly with `--log_level=message` before relying on `ctest`.

---

*Testing analysis: 2026-10-18; patched after Phases 2 and 3*
