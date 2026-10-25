# Phase 4: Retina Rendering - Pattern Map

**Mapped:** 2026-10-25
**Files analyzed:** 22 (2 new core files, 2 new test files, 18 modified)
**Analogs found:** 22 / 22 (all analogs are git-tracked; verified with `git ls-files`)

Only the new files need an external analog. Modified files are their own analog; the excerpts below are the "before" code the plan edits, with line numbers from the current tree. The exact "after" code is in 04-RESEARCH.md Patterns 1 to 8 and is not repeated here.

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `src/core/HiDpi.h` (new) | utility (pure geometry helpers) | transform | `src/core/PixmapRenderer.h` | role-match |
| `src/core/HiDpi.cpp` (new) | utility | transform | `src/core/PixmapRenderer.cpp` | role-match |
| `src/core/CMakeLists.txt` | config | n/a | itself (`common_sources`, line 4-) | exact |
| `src/qt_tests/tst_hidpi_qt.h` (new) | test | request-response | `src/qt_tests/tst_ux_qt.h` | exact |
| `src/qt_tests/tst_hidpi_qt.cpp` (new) | test | request-response | `src/qt_tests/tst_ux_qt.cpp` | exact |
| `src/qt_tests/main.cpp` | test harness | env setup + exec | itself | exact |
| `src/qt_tests/CMakeLists.txt` | config | n/a | itself | exact |
| `src/core/ImageViewBase.{h,cpp}` | component | event-driven + background task | itself (`HqTransformTask`, 902-968, 1015-1040) | exact |
| `src/core/filters/output/PictureZoneEditor.{h,cpp}` | component | event-driven + background task | `ImageViewBase::HqTransformTask` (same pattern) | exact |
| `src/core/ThumbnailBase.cpp` | component | request-response (paint) | itself (98-122) | exact |
| `src/core/ThumbnailPixmapCache.{h,cpp}` | service (cache) | file-I/O + CRUD | itself (606-633) | exact |
| `src/core/Utils.cpp` / `Utils.h` | utility | factory | itself (`createThumbnailCache`) | exact |
| `src/app/MainWindow.cpp` | controller | event-driven | itself (event filter ~516-520) | exact |
| `src/core/ApplicationSettings.cpp`, `src/app/SettingsDialog.ui` | config | CRUD | itself | exact |
| `src/core/StageListView.{h,cpp}` | component | event-driven | itself | exact |
| deskew/page_split `ImageView.cpp`, `page_split/Thumbnail.cpp`, `page_split/UnremoveButton.cpp`, `zones/ZoneContextMenuInteraction.cpp` | component | request-response (paint) | themselves | exact |
| `src/core/filters/output/ColorPickupInteraction.{h,cpp}` | component | request-response | itself (67-126) | exact |

## Pattern Assignments

### `src/core/HiDpi.h` and `src/core/HiDpi.cpp` (utility, transform)

**Analog:** `src/core/PixmapRenderer.{h,cpp}`: a small static, stateless, paint-geometry helper with forward-declared Qt types, the project copyright header and the `SCANTAILOR_CORE_<NAME>_H_` guard. (`PageRange.{h,cpp}` is the other small candidate, but it is a value class; PixmapRenderer is closer.)

**Header pattern** (`PixmapRenderer.h` lines 1-9, 28):
```cpp
// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_PIXMAPRENDERER_H_
#define SCANTAILOR_CORE_PIXMAPRENDERER_H_

class QPainter;
class QPixmap;
...
#endif
```
For HiDpi use guard `SCANTAILOR_CORE_HIDPI_H_` and the same two-line copyright header. The research proposes a `namespace hidpi { ... }` with free functions `deviceSize`, `toDevice`, `targetRect`, `toLogical`, `makePixmap` (04-RESEARCH.md Pattern 1). Forward declare `QSize`, `QPixmap` etc. only if the header needs no value types; `QTransform`/`QRectF` are returned/passed by value or const-ref, so include `<QTransform>`, `<QRectF>`, `<QPixmap>` as needed. Lower-case snake namespace matches convention (`namespace imageproc`).

**Source pattern** (`PixmapRenderer.cpp` lines 1-17): copyright, `#include "PixmapRenderer.h"` first, then Qt includes alphabetically one per line, then the function body. Copy this layout (`#include "HiDpi.h"` then `<QPixmap>`, `<QRectF>`, `<QTransform>`).

**CMake registration** (`src/core/CMakeLists.txt` lines 4-8, 73): one line per module, header and source together inside `set(common_sources ...)`:
```cmake
set(common_sources
    Application.cpp Application.h
    BackgroundExecutor.cpp BackgroundExecutor.h
    PixmapRenderer.cpp PixmapRenderer.h
    ...
    PageRange.cpp PageRange.h
```
Add `HiDpi.cpp HiDpi.h` next to `PixmapRenderer.cpp PixmapRenderer.h`. `core` is `add_library(core STATIC ${common_sources} ${common_ui_files})` (line 120), so `qt_tests` (links `core`) sees it automatically; include as `#include <HiDpi.h>` (bare name).

---

### `src/qt_tests/tst_hidpi_qt.h` / `tst_hidpi_qt.cpp` (test, request-response)

**Analog:** `src/qt_tests/tst_ux_qt.{h,cpp}` (widget-level tests, includes core headers by bare name, `QSignalSpy`, `QtTest`).

**Header pattern** (`tst_ux_qt.h` lines 1-19, whole file):
```cpp
// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef TST_UX_QT_H
#define TST_UX_QT_H

#include <QObject>

class TestUxQt : public QObject {
  Q_OBJECT
 private slots:
  void testButtonClickEmitsSignal();
  ...
};

#endif
```
Note: the `tst_*` headers use the short guard `TST_<NAME>_QT_H` (not `SCANTAILOR_...`); follow the neighbours: `TST_HIDPI_QT_H`, class `TestHiDpiQt`, slot names `testXxx` camelCase (this differs from the Boost `test_snake_case` rule; the Qt Test files use camelCase). Add `initTestCase()`/`cleanupTestCase()` if the two-screen setup needs them.

**Source pattern** (`tst_ux_qt.cpp` lines 1-17 and tail):
```cpp
#include "tst_ux_qt.h"

#include <CollapsibleGroupBox.h>
#include <ErrorWidget.h>

#include <QCheckBox>
#include <QSignalSpy>
#include <QWidget>
#include <QtTest/QtTest>

void TestUxQt::testErrorWidgetCreation() {
  ErrorWidget widget("Error de prueba", Qt::PlainText);
  QVERIFY(widget.findChildren<QWidget*>().size() >= 1);
}
```
No `QTEST_MAIN`; test classes are run manually from `main.cpp` (below).

**Registration in `main.cpp`** (lines 4-5 includes; 43-48 run block):
```cpp
#include "tst_core_qt.h"
#include "tst_ux_qt.h"
...
  TestCoreQt tc1;
  status |= QTest::qExec(&tc1, argc, argv);

  TestUxQt tc2;
  status |= QTest::qExec(&tc2, argc, argv);
```
Add `#include "tst_hidpi_qt.h"` and `TestHiDpiQt tc3; status |= QTest::qExec(&tc3, argc, argv);`.

**Env setup before `QApplication`** (`main.cpp` lines 21-26): this is where the two-screen offscreen setup goes (RESEARCH Pitfall 3 and Pattern 7). Existing code:
```cpp
  qputenv("QT_QPA_PLATFORM", "offscreen");
  qputenv("LC_ALL", "en_US.UTF-8");
  qputenv("TMPDIR", tmpRoot.toUtf8());
```
Change `QT_QPA_PLATFORM` to `offscreen:configfile=<json under tmpRoot>` (write the JSON with two named screens, no `dpr` key, using `QFile` and `TEST_TMP_ROOT`), and `qputenv("QT_SCREEN_SCALE_FACTORS", "OneX=1;TwoX=2")`. Caveat for the planner: ctest sets `QT_QPA_PLATFORM=offscreen` in the environment (`qt_tests/CMakeLists.txt` last lines), but `main.cpp` overrides it with `qputenv` before `QApplication`, so the in-code value wins; keep the CMake ENVIRONMENT line or align it. These two env vars apply to the whole `qt_tests` process, so existing tests (`TestCoreQt`, `TestUxQt`) run on a two-screen platform too; check they still pass.

**CMake** (`src/qt_tests/CMakeLists.txt` lines 1-4):
```cmake
set(sources
    main.cpp
    tst_core_qt.cpp
    tst_ux_qt.cpp)
```
Add `tst_hidpi_qt.cpp`. The target already links `core foundation Qt6::Test Qt6::Core Qt6::Gui Qt6::Widgets` and has `${CMAKE_SOURCE_DIR}/src` plus `TEST_TMP_ROOT`. Do not add a new ctest entry: the single `add_test(NAME qt_tests ...)` runs all classes, so the 7-suite count stays 7 (D-00b). If the test needs `test_support` (synthetic page generator), add it to `target_link_libraries`.

---

### `src/core/ImageViewBase.cpp` / `.h` (component, event-driven + background task)

**Analog:** itself. "Before" excerpts the plan edits (all from the current tree):

**validateHqPixmap** (lines 902-919): add DPR comparison before `return true`:
```cpp
  if (m_hqXform != m_imageToVirtual * m_virtualToWidget) {
    return false;
  }
  return true;
```

**initiateBuildingHqVersion** (lines 934-954): the task is built with `viewport()->size()` and state stored after:
```cpp
  const QTransform xform(m_imageToVirtual * m_virtualToWidget);
  const auto task = std::make_shared<HqTransformTask>(this, m_image, xform, viewport()->size());
  backgroundExecutor().enqueueTask(task);
  m_hqTransformTask = task;
  m_hqXform = xform;
  m_hqSourceId = m_image.cacheKey();
```

**hqVersionBuilt** (959-968): `m_hqPixmap = QPixmap::fromImage(image); m_hqPixmapPos = origin;` (`origin` is `QPoint`; becomes device origin divided by the image DPR).

**HqTransformTask** (1015-1040): constructor takes `(imageView, image, xform, targetSize)` and initialises members in the init list; `operator()` computes `targetRect`, calls `transform(...)`, then `convertToFormat(...)` with the comment about doing the conversion on the background thread, then `m_result->setData(targetRect.topLeft(), hqImage)`. Add a `double dpr` ctor parameter and member `m_dpr` (keep the `m_` style); `setDevicePixelRatio` goes after `convertToFormat` (it resets nothing, but the conversion returns a new image without relying on it).

**Event override** (new): `ImageViewBase` currently has no `event()` override of this kind; add `bool event(QEvent*) override` in the header's protected section next to the other event overrides. Check for subclass chains (`PictureZoneEditor`, which derives via `ZoneEditorBase`) so they call `ImageViewBase::event(e)`.

**Error handling / threading:** none to add. Cancellation uses `isCancelled()` already at the top of `operator()`.

---

### `src/core/filters/output/PictureZoneEditor.{h,cpp}` (component, event-driven + background task)

**Analog:** `ImageViewBase::HqTransformTask` (above): `MaskTransformTask` (lines ~301-324) has the same shape. Apply the same dpr plumbing (task ctor param, `hidpi::toDevice`/`targetRect`, `setDevicePixelRatio` on the mask image after `setAlphaChannel`), `QPoint m_screenPictureMaskOrigin` to `QPointF` (`.h:83`), add `double m_screenPictureMaskDpr` beside `m_screenPictureMaskXform` (`.h:84`), compare in `validateScreenPictureMask` (162-164), keep ratio in the per-frame copy in `onPaint` (137-148). See RESEARCH Pattern 3.

---

### `src/core/ThumbnailBase.cpp` (component, request-response paint)

**Analog:** itself. Excerpt to edit (lines 100-119):
```cpp
  QPixmap tempPixmap;
  const QString cacheKey(QString::fromLatin1("ThumbnailBase::tempPixmap"));
  if (!QPixmapCache::find(cacheKey, &tempPixmap) || (tempPixmap.width() < displayRect.width())
      || (tempPixmap.height() < displayRect.height())) {
    auto w = (int) displayRect.width();
    auto h = (int) displayRect.height();
    // Add some extra, to avoid rectreating the pixmap too often.
    w += w / 10;
    h += h / 10;

    tempPixmap = QPixmap(w, h);

    if (!tempPixmap.hasAlphaChannel()) {
      // This actually forces the alpha channel to be created.
      tempPixmap.fill(Qt::transparent);
    }

    QPixmapCache::insert(cacheKey, tempPixmap);
  }
```
Changes: DPR from `painter->device()->devicePixelRatioF()` in the cache key, comparison against `displayRect * dpr` (device pixels, since `tempPixmap.width()` is device pixels), allocation `QPixmap(w*dpr, h*dpr)` plus `setDevicePixelRatio(dpr)` before `fill`. Use `hidpi::makePixmap` if it fits the +10 % growth logic.

---

### `src/core/ThumbnailPixmapCache.{h,cpp}` (service, file-I/O + CRUD)

**Analog:** itself. Key excerpt: `getThumbFilePath` (606-633), file name built from `maxThumbSize.width()`:
```cpp
  const QString thumbnailQualityStr = QChar('q') + QString::number(maxThumbSize.width());
```
This stays; callers pass the effective size (Quality x DPR). The new `setDevicePixelRatio` copies the structure of `setMaxThumbSize` (RESEARCH cites 865-870: lock, compare, `removeLoadedItemsLocked()`, store). Also fix the unlocked `m_maxThumbSize` reads at `ensureThumbnailExists` (~448) and `recreateThumbnail` (~476) by using the local copy.

---

### `src/core/filters/output/ColorPickupInteraction.{h,cpp}` (component, request-response)

**Analog:** itself. Excerpt to extract into a pure `static QColor medianColor(const QImage&)` (lines 67-126, key part):
```cpp
    image = pixmap.toImage().convertToFormat(QImage::Format_RGB32);
  ...
  const int width = image.width();
  const int height = image.height();
  const int xCenter = width / 2;
  const int yCenter = height / 2;
  const int sqdistThreshold = xCenter * yCenter;
  ...
  std::vector<uint32_t> bitmixedColors;
  ...
      if (sqdist <= sqdistThreshold) {
        bitmixedColors.push_back(bitMixColor(color));
  ...
  auto halfPos(bitmixedColors.begin() + bitmixedColors.size() / 2);
  std::nth_element(bitmixedColors.begin(), halfPos, bitmixedColors.end());
  const QColor color(bitUnmixColor(*halfPos));
```
`takeColor()` keeps the `ScopedIncDec` guard, the `grab(rect)` and the OpenGL branch (lines 72-74, left alone per D-00d), then calls `medianColor`, then the unchanged property updates (`m_fillColorProp->setColor`, `defaultProps`, `m_zones.commit()`). The extracted function must not return early with empty input without a documented sentinel; the current early `return` on empty `bitmixedColors` can become an invalid `QColor` check at the call site. Declare it `static` and public (or a friend) so `TestHiDpiQt` can call it.

---

### Icon geometry fixes (deskew/page_split `ImageView.cpp`, `page_split/Thumbnail.cpp`, `page_split/UnremoveButton.cpp`, `zones/ZoneContextMenuInteraction.cpp`)

**Analog:** themselves; the exact line list and replacement (`deviceIndependentSize()`) is the table in RESEARCH Pattern 6. The one-line pattern to copy at every site:
```cpp
QRectF rect(QPointF(0, 0), m_handlePixmap.deviceIndependentSize());   // was QRectF(m_handlePixmap.rect())
0.5 * m_handlePixmap.deviceIndependentSize().width()                    // was 0.5 * m_handlePixmap.width()
```

---

### `src/core/StageListView.{h,cpp}`, `src/app/MainWindow.cpp`, `Utils.cpp`, `ApplicationSettings.cpp`, `SettingsDialog.ui`

**Analog:** themselves. Edit points and the replacement are specified in RESEARCH Patterns 4, 5 and 7:
- `StageListView`: `createBatchAnimationSequence` (259-277), allocation at line 272; add `bool event(QEvent*) override` and a `m_squareSide`-style member (remember the `m_` prefix).
- `MainWindow`: existing event filter on `thumbView` (installed ~516, handler ~520); add a `QEvent::DevicePixelRatioChange` case; `MainWindow.cpp:432` passes `devicePixelRatioF()` to `Utils::createThumbnailCache`.
- `ApplicationSettings.cpp:25` `DEFAULT_THUMBNAIL_QUALITY` 200 -> 250 (D-06); `SettingsDialog.ui:110` tooltip text.

## Shared Patterns

### The buffer rule (apply to every screen buffer)
**Source:** new `src/core/HiDpi.h`; spec in 04-RESEARCH.md Pattern 1. Logical coordinates everywhere; buffer = logical size x `devicePixelRatioF()` with `setDevicePixelRatio`; position and clip in logical coordinates; ratio read from the widget (or `painter->device()`), never hard-coded. At DPR 1 every helper must reproduce the old arithmetic exactly (keeps DPR-1 behaviour and `pipeline_tests` untouched).

### File header and guard
**Source:** `src/core/PixmapRenderer.h` lines 1-5. Two-line copyright comment, then `#ifndef SCANTAILOR_CORE_<NAME>_H_`; `#pragma once` is never used. Test headers use `TST_<NAME>_QT_H` instead.

### Includes
Bare module names (`#include <HiDpi.h>`, `#include <ImageViewBase.h>`); own header first with quotes in `.cpp`; Qt headers alphabetical, one per line.

### Idempotent DPR handlers
`QEvent::DevicePixelRatioChange` arrives up to twice for a top-level widget (RESEARCH Pitfall 5): compare the new ratio with the stored one (`qFuzzyCompare`) before cancelling tasks; the paint-time check in `validateHqPixmap` is the source of truth.

### Do not touch
`src/imageproc/Transform.cpp`, any `Task::process`, `ImageViewBase::createDownscaledImage`, goldens and manifests (D-00b). Proof of unchanged output: `ctest --preset macos-release` and `macos-debug` pass 7/7 via `.planning/phases/03-qt-6-and-macos-only/gate.sh`.

### Formatting
`xcrun clang-format -i <file>` on each touched C++ file; `--dry-run -Werror` to check. 2-space indent, one parameter per line when wrapping.

## No Analog Found

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| Two-screen offscreen JSON config written by `main.cpp` | test fixture | env setup | No existing multi-screen test; follow RESEARCH Pattern 7 and Pitfall 3/4. |
| Optional live-check harness (links `core` only) | tool | one-off | `src/app/ui_snapshot/main.cpp` is the nearest, but forces offscreen (line 127) and compiles GUI sources a second time (WR-01); RESEARCH recommends a separate small harness linking `core`. If committed, give it its own `CMakeLists.txt` entry rather than extending `ui_snapshot`. |

## Metadata

**Analog search scope:** `src/core/`, `src/qt_tests/`, `src/core/filters/output/`
**Files read:** `04-CONTEXT.md`, `04-RESEARCH.md` (lines 1-383 of 625; the remainder was not needed for file mapping), `src/qt_tests/{main.cpp,CMakeLists.txt,tst_ux_qt.h,tst_ux_qt.cpp,tst_core_qt.h}`, `src/core/{PixmapRenderer.h,PixmapRenderer.cpp,CMakeLists.txt}`, excerpts of `ImageViewBase.cpp`, `ThumbnailBase.cpp`, `ThumbnailPixmapCache.cpp`, `ColorPickupInteraction.cpp`
**Pattern extraction date:** 2026-10-25
