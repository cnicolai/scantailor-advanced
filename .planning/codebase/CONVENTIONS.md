---
last_mapped_commit: 5eaac1884cdcabb6514bd632114f688631bd8dbc
last_mapped_at: 2026-10-18
---
# Coding Conventions

**Analysis Date:** 2026-10-18 (patched after the Phase 3 strip, D-10; not re-mapped)

## Naming Patterns

**Files:**
- C++ sources and headers use PascalCase matching the primary class: `src/core/filters/deskew/Filter.cpp`, `src/core/filters/deskew/OptionsWidget.h`, `src/imageproc/Dpi.h`.
- Test sources use the `Test` prefix: `src/core/tests/TestPageId.cpp`, `src/imageproc/tests/TestDpi.cpp`.
- Qt Test sources use the `tst_` prefix: `src/qt_tests/tst_core_qt.cpp`, `src/qt_tests/tst_ux_qt.h`.
- Dialog and widget `.ui` files sit next to their class: `src/core/filters/deskew/ApplyDialog.ui`.

**Namespaces:**
- Lowercase snake_case, one per filter or module: `namespace deskew`, `namespace select_content`, `namespace page_split`, `namespace imageproc`, `namespace output`, `namespace tests`.
- The filter `Filter` class lives inside its namespace: `deskew::Filter` in `src/core/filters/deskew/Filter.cpp`.

**Classes and types:**
- PascalCase: `Filter`, `PageId`, `ImageId`, `OrderByDeviationProvider`, `CacheDrivenTask`.
- Forward declarations are grouped at the top of the header, after include guards: see `src/core/filters/deskew/Filter.h` (`class QString; class PageSelectionAccessor; ...`).

**Functions and methods:**
- lowerCamelCase for functions and methods: `getName()`, `preUpdateUI()`, `saveSettings()`, `performRelinking()`, `subPageToString()`.
- Qt-style accessors without `get` prefix for value getters: `imageId()`, `subPage()`, `isNull()`.
- `getName()`/`getView()` style is used for the filter interface; newer code uses bare nouns.

**Member variables:**
- Private data members use the `m_` prefix followed by lowerCamelCase: `m_settings`, `m_imageSettings`, `m_selectedPageOrder`, `m_optionsWidget`, `m_pageOrderOptions`. The prefix is used pervasively (around 2,780 occurrences in headers).
- Qt widget members are typically held through smart pointers: `m_optionsWidget.reset(new OptionsWidget(...))` in `src/core/filters/deskew/Filter.cpp`.

**Local variables and parameters:**
- lowerCamelCase: `pageSelectionAccessor`, `orderByDeviation`, `defaultOrder`.
- Constants and enumerators: UPPER_SNAKE_CASE for enum values: `PageId::SINGLE_PAGE`, `PageId::LEFT_PAGE`, `PageId::RIGHT_PAGE`, `PAGE_VIEW`.

**Test names:**
- Boost suite names are CamelCase ending in `TestSuite`: `CorePageIdTestSuite`, `ImageProcDpiTestSuite`.
- Boost test case names are `test_` followed by snake_case: `test_default_constructor_is_null`, `test_equality`.

## Code Style

**Formatting:**
- `clang-format` with `BasedOnStyle: Chromium`, configured in `.clang-format` at the repository root.
- Indentation: 2 spaces. Line limit and brace wrapping follow the `.clang-format` file; read it before formatting by hand.
- Functions and control statements use braces on the same line: `if (...) {`, `void f() {`.
- Short single-line blocks are disallowed (`AllowShortBlocksOnASingleLine: false`, `AllowShortIfStatementsOnASingleLine: Never`). Lambdas may be single line (`AllowShortLambdasOnASingleLine: All`).
- `BinPackParameters: false`: when a parameter list wraps, each parameter goes on its own line.

**Enforcement:**
- The CI check (`.github/workflows/lint.yml`) was removed in Phase 3. Formatting is a manual step: run `xcrun clang-format -i <file>` on every touched C++ file before committing and `xcrun clang-format --dry-run -Werror <file>` to check (`TESTING.md`). `src/dewarping/TextLineTracer.cpp` keeps two older violations.

**Linting:**
- No dedicated static analyser configuration (no `.clang-tidy` in the repository root). `clang-format` is the only enforced gate.

**Language standard:**
- C++17 (`set(CMAKE_CXX_STANDARD 17)` in `CMakeLists.txt`). Use `std::make_shared`, `std::unique_ptr`, `auto`, and structured constructs already in use; do not introduce C++20 features.

## Import Organization

**Order (observed in `src/core/filters/deskew/Filter.cpp`):**
1. The matching header first: `#include "Filter.h"`.
2. Project headers in angle brackets for other modules: `#include <OrderByDeviationProvider.h>`, `#include <filters/select_content/CacheDrivenTask.h>`.
3. Standard library headers: `#include <utility>`.
4. Local headers in quotes, sorted alphabetically: `"AbstractRelinker.h"`, `"CacheDrivenTask.h"`, `"FilterUiInterface.h"`, `"OptionsWidget.h"`, `"ProjectReader.h"`, `"ProjectWriter.h"`, `"Task.h"`, `"Utils.h"`.

**Path conventions:**
- Module headers are included by bare file name (`#include <PageId.h>`), because each module directory is an include root.
- Cross-filter headers are included with the filter path (`#include <filters/select_content/Task.h>`).
- No path aliases are configured.

**Include guards:**
- Headers use classic `#ifndef`/`#define` guards named after the file, e.g. `FILENAME_H`. `#pragma once` is not used anywhere in the tree.

## Copyright Header

- New source files start with the project copyright and license notice, matching existing files:

```cpp
// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
```

## Error Handling

**Patterns:**
- C++ exceptions are the main mechanism for recoverable failure inside workers and filters (about 160 `throw` sites in `src/`). Catch by const reference: `} catch (const std::runtime_error&) {` (`src/core/filters/output/OutputGenerator.cpp`).
- Cancellation uses a dedicated exception type: `catch (const TaskCancelException&)` in `src/core/filters/output/DespeckleView.cpp`. Throw and catch this type rather than returning sentinel values.
- Parsing and conversion functions report success through an out-parameter flag rather than throwing: `PageId::subPageFromString("left", &ok)` in `src/core/PageId.cpp`. Follow this pattern for new parse functions and check the flag at call sites.
- User-facing errors use modal dialogs with translated text: `QMessageBox::warning(this, tr("Error"), tr("DPI is too low!"));` in `src/core/filters/output/ChangeDpiDialog.cpp`.

**Guidelines:**
- Do not swallow exceptions silently in new code. Log or propagate.
- Validate user input in the dialog before accepting it (see `ChangeDpiDialog.cpp`) rather than deep in the processing pipeline.

## Logging

**Framework:** Qt debug output (`qDebug`, `qWarning`) and occasional `std::cerr`. No logging library is used.

**Patterns:**
- Keep diagnostic output out of normal paths. Roughly 19 `qDebug`/`qWarning`/`std::cerr` calls exist in `src/`; prefer that low count over adding more.
- Do not log file contents, user paths beyond what the existing code already prints, or secrets.

## Comments

**When to Comment:**
- Comment non-obvious intent, algorithm references, and units (px, mm, DPI). Short inline comments are the norm; the codebase does not use long banner blocks.
- Do not restate what the code does.

**Doc comments:**
- No Doxygen requirement is enforced. Where present, use `///` or `/** */` on public interface declarations in headers.

**Translation markers (kept after the Phase 3 strip, D-06):**
- User-visible strings are wrapped in `tr("...")` (in `QObject` subclasses) or `QCoreApplication::translate("namespace::Class", "...")` (in non-QObject code, e.g. `Filter::getName()` in `src/core/filters/deskew/Filter.cpp`). The translation catalogues in `src/translations/` were removed in Phase 3; the markers stay.

## Function Design

**Size:**
- No hard limit enforced. Keep functions focused; long per-page processing is split into `Task` and `CacheDrivenTask` classes per filter (`src/core/filters/deskew/Task.h`, `CacheDrivenTask.h`).

**Parameters:**
- Pass shared, read-only state as `const&` or `const std::shared_ptr<...>&`.
- Use `const` on parameters and locals wherever the value is not modified: `const PageId id;`, `const ImageId imageId(...)`.
- Keep one parameter per line when wrapping (`BinPackParameters: false`).

**Return values:**
- Return by value for small Qt types (`QString`, `QDomElement`, `PageView`).
- Use `= default` for trivial destructors and special members: `Filter::~Filter() = default;`.

## Module Design

**Exports:**
- Each filter is a directory under `src/core/filters/<name>/` with its own `CMakeLists.txt`, a `Filter.h/.cpp` implementing the filter interface, `OptionsWidget`, `Settings`, `Task`/`CacheDrivenTask`, `ProjectReader`/`ProjectWriter` hooks, and an optional `ApplyDialog` with a `.ui` form.
- Add new filters following this layout; mirror `src/core/filters/deskew/`.

**Barrel Files:**
- Not used. Include headers directly.

**Memory:**
- `std::shared_ptr` is the dominant ownership model for shared settings and providers (about 200 uses). `std::unique_ptr` is used for owned widgets and tasks (about 200 files reference a smart pointer).
- Avoid raw `new` for new code; the few existing uses are for Qt widgets handed to a parent or reset into a member.

---

*Convention analysis: 2026-10-18; patched after the Phase 3 strip*
