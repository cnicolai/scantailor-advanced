# Phase 6: Hardening and Cleanup - Pattern Map

**Mapped:** 2026-11-16
**Files analyzed:** 41 new/modified (grouped where the pattern is the same)
**Analogs found:** 38 / 41 (all analog paths verified git-tracked under `src/` and `.planning/`)

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `src/core/ImageSizeLimit.{h,cpp}` | utility (pure policy) | transform | `src/core/ImageMetadataLoader.h` (status/guard style); `src/core/HiDpi.h` (small free-function helper) | role-match |
| `src/core/MemoryBudget.{h,cpp}` | utility (pure fn + sysctl) | transform | `src/core/WorkerThreadPool.cpp:79-90` (the code it replaces) | role-match |
| `src/core/tests/TestImageSizeLimit.cpp` | test (Boost) | request-response (in-memory QBuffer) | `src/core/tests/TestImageId.cpp` | exact (structure) |
| `src/core/tests/TestMemoryBudget.cpp` | test (Boost) | transform | `src/core/tests/TestImageId.cpp` | exact |
| `src/core/tests/CMakeLists.txt` | config | - | itself (`set(sources ...)`) | exact |
| `src/core/TiffReader.cpp`, `JpegMetadataLoader.cpp`, `ImageLoader.cpp` | service (reader) | file-I/O | themselves (insert check before alloc) | exact |
| `src/core/LoadFileTask.{h,cpp}` (ErrorResult 3rd state) | component (error widget) | request-response | `LoadFileTask.cpp:111-135` | exact |
| `src/app/ProjectFilesDialog.cpp` (`STATUS_TOO_LARGE`) | component (model) | CRUD | `ProjectFilesDialog.cpp:21,534-538,565-590,611` | exact |
| `src/app/MainWindow.cpp` insert flow (`:1970`) | controller | request-response | `ProjectFilesDialog::FileList::loadNextFile` | role-match |
| `src/core/WorkerThreadPool.{h,cpp}`, `src/app/SystemLoadWidget.cpp` | service/component | event-driven | `WorkerThreadPool.cpp:79-90`, `SystemLoadWidget.cpp:12-47` | exact |
| `src/foundation/XmlMarshaller/XmlUnmarshaller` escape helpers + `src/foundation/tests/TestXmlEscape.cpp` | utility + test | transform | `src/foundation/tests/CMakeLists.txt` + `TestUtils.cpp` | role-match |
| `src/foundation/XmlUnmarshaller.cpp:14` | utility (comment) | - | itself | exact |
| `src/pipeline_tests/TestProjectRoundTrip.cpp` (extended names) | test (Boost) | file-I/O round trip | itself (`nameWithMarkup()`, `RoundTripFixture`) | exact |
| `src/qt_tests/tst_ux_qt.{h,cpp}` new slots (ErrorResult too-large, ProjectFilesDialog status) | test (Qt Test) | request-response | `TestUxQt::testErrorWidgetCreation` (`tst_ux_qt.cpp:74`) | exact |
| `src/qt_tests/tst_mac_qt.{h,cpp}` new slots (no-OpenGL scans, ProjectSaveFlow, updated `testBatchAlarmUsesBeep`) | test (source scan) | file-I/O | `testNoDontUseNativeDialog` (`tst_mac_qt.cpp:1474`), `testBatchAlarmUsesBeep` (`:1416`) | exact |
| `src/qt_tests/tst_hidpi_qt.cpp` | test | - | unchanged; run before/after (`testMedianColorRatioInvariant`, `testColorPickerGrabAtRatio2`) | exact |
| `src/app/ProjectSaveFlow.{h,cpp}` | utility (static helpers) | request-response | `MainWindow.cpp:1425-1458` + `OutOfMemoryDialog.cpp:50-92` (identical bodies) | exact |
| `src/app/CMakeLists.txt` (`gui_only_sources`) | config | - | itself lines 12-51 | exact |
| `src/app/MainWindow{Actions,Batch,Project,Pages}.cpp` | controller (same-class TUs) | event-driven | `MainWindow.cpp` | exact (pure move) |
| `src/core/filters/output/OutputGenerator*.cpp/.h` split | service | transform | `OutputGenerator.cpp` | exact (pure move) |
| `src/core/filters/output/CMakeLists.txt` (`sources`) | config | - | itself lines 11-30 | exact |
| STRIP-07 edits (`ImageViewBase`, `ColorPickupInteraction`, `ApplicationSettings`, `SettingsDialog.{cpp,ui}`, 2 CMake files) | mixed | deletion | themselves | exact |
| CLEAN-03/04 edits (7 + 26 files) | mixed | deletion | none needed | n/a |
| `.planning/phases/06-hardening-and-cleanup/records/gate-log.md` | docs | - | `.planning/phases/05-mac-menus-and-shortcuts/records/gate-log.md` | exact |
| `records/*.md` (measurements, memory-run, closing-check) | docs | - | `records/closing-check.md` (Phase 5) | exact |

## Pattern Assignments

### `src/core/tests/TestImageSizeLimit.cpp`, `TestMemoryBudget.cpp` (test, Boost, in-memory)

**Analog:** `src/core/tests/TestImageId.cpp` (Boost suite, no fixtures, no files).

**Header / includes / suite naming** (`TestImageId.cpp:1-12`):
```cpp
// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ImageId.h>

#include <QFileInfo>
#include <QString>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(CoreImageIdTestSuite)

BOOST_AUTO_TEST_CASE(test_default_constructor_is_null) {
  const ImageId id;
  BOOST_CHECK(id.isNull());
```
Conventions: bare-name module includes with `<...>`, Qt, then `<boost/test/unit_test.hpp>`; suite `Core<Thing>TestSuite` (research uses `ImageSizeLimitTestSuite` / `MemoryBudgetTestSuite` as `--run_test` names; either keep the `Core` prefix and adjust the command, or name them as research says, pick one and use it in the plan commands); cases `test_` + snake_case; close with `BOOST_AUTO_TEST_SUITE_END()`.

**Registration** (`src/core/tests/CMakeLists.txt`, whole file is a single list): add the file names to `set(sources ...)` (alphabetical-ish, one per line), e.g. `TestImageSizeLimit.cpp`, `TestMemoryBudget.cpp`. `main.cpp` is `BOOST_AUTO_TEST_MAIN` only (`src/core/tests/main.cpp`), so no other registration. Linking is `core Boost::unit_test_framework`; if TIFF forging uses libtiff, add `TIFF::TIFF ZLIB::ZLIB` to `target_link_libraries(core_tests PRIVATE ...)`. ctest entry is already `add_test(NAME core_tests COMMAND core_tests --log_level=message)`, so no ctest change.

**Forge pattern:** `TiffReader::readImage(QIODevice&, int)` and `ImageLoader::load(QIODevice&, int)` accept a `QBuffer` (no file, no /tmp). Footprint guard via mach `task_info` `phys_footprint` before/after (research Code Examples, lines 316-324).

**XFAIL not needed here.** For boundary table use `BOOST_CHECK_EQUAL`/`BOOST_CHECK` on `exceeds()`.

---

### `src/qt_tests/tst_ux_qt.{h,cpp}` new slots (ErrorResult "too large", ProjectFilesDialog status)

**Analog:** `src/qt_tests/tst_ux_qt.h` + `tst_ux_qt.cpp:74-77`.

**Registration (Qt Test slots)** - declare in the class, define out-of-class; `main.cpp` already `qExec`s each class once, so new slots in an existing class need no `main.cpp` change:
```cpp
// tst_ux_qt.h
class TestUxQt : public QObject {
  Q_OBJECT
 private slots:
  void testButtonClickEmitsSignal();
  ...
  void testErrorWidgetCreation();
};
```
```cpp
// tst_ux_qt.cpp:74
void TestUxQt::testErrorWidgetCreation() {
  ErrorWidget widget("Error de prueba", Qt::PlainText);
  QVERIFY(widget.findChildren<QWidget*>().size() >= 1);
}
```
Includes pattern: `#include <ErrorWidget.h>` (bare name), Qt includes, `<QtTest/QtTest>` last. For `LoadFileTask::ErrorResult::updateUI` needs a stub `FilterUiInterface` (check `src/core/FilterUiInterface.h` for pure virtuals; none exist in tests yet - planner must write the stub). `main.cpp` order: TestCoreQt, TestUxQt, TestHiDpiQt, TestMacQt, `status |= QTest::qExec(&tcN, argc, argv)`. A brand-new test class would additionally need `#include`, an instance and a `qExec` line in `src/qt_tests/main.cpp` and an entry in `src/qt_tests/CMakeLists.txt` `set(sources ...)`; prefer slots in existing classes.

---

### `src/qt_tests/tst_mac_qt.{h,cpp}` source-scanning tests (STRIP-07 no-OpenGL scan, updated beep test)

**Analog:** `testNoDontUseNativeDialog` (`tst_mac_qt.cpp:1474-1520`) and `testBatchAlarmUsesBeep` (`:1416-1472`).

**Scan helpers** (`tst_mac_qt.cpp:321-352`; `SOURCE_ROOT` is a compile definition set in `src/qt_tests/CMakeLists.txt`):
```cpp
QString readUtf8(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) { return QString(); }
  return QString::fromUtf8(file.readAll());
}
QString sourceRoot() { return QString::fromUtf8(SOURCE_ROOT); }
// scanScopeFiles(suffix): files below SOURCE_ROOT/src ending in suffix, relative paths, sorted;
// excludes src/qt_tests/, src/pipeline_tests/, src/test_support/, src/app/{ui_snapshot,retina_check,mac_check}/ and any "tests" dir
```
**Core pattern** - forbidden-word list with a control that proves the search finds them, then loop over `.cpp/.h/.mm`, assert `fileCount >= 500` so an empty scan cannot pass:
```cpp
const QStringList forbidden{"DontUseNativeDialog", ...};
const auto firstForbidden = [&forbidden](const QString& text) { for (...) if (text.contains(word)) return word; return QString(); };
QCOMPARE(firstForbidden(QStringLiteral("getExistingDirectory(this, t, d, QFileDialog::DontUseNativeDialog)")), QStringLiteral("DontUseNativeDialog"));  // control
QCOMPARE(firstForbidden(QStringLiteral("getOpenFileName(this, t, d)")), QString());
...
QVERIFY2(fileCount >= 500, qPrintable(QStringLiteral("only %1 C++ files read").arg(fileCount)));
QVERIFY2(hits.isEmpty(), qPrintable("...:\n" + hits.join('\n')));
```
For STRIP-07 copy this verbatim with `forbidden{"QOpenGL", "OpenGLSupport", "isOpenGlEnabled"}`; also read `CMakeLists.txt` and `src/core/CMakeLists.txt` via `root.filePath(...)` and assert no `OpenGL`. Note `scanScopeFiles` excludes test dirs, so these scans do not read themselves.

**Must change in the CLEAN-02 commit** (`tst_mac_qt.cpp:1433-1452`): `readUtf8(root.filePath("src/app/MainWindow.cpp"))` is the only file read for `m_batchQueue->allProcessed()`; change to read the file that now holds `filterResult` (e.g. `MainWindowBatch.cpp`), keep the same alert/`m_checkBeepWhenFinished()`/beep ordering assertions. The `forbidden` shell-command scan over `MainWindow.cpp` should iterate `src/app/MainWindow*.cpp`.

**Registering new slots:** declare in `tst_mac_qt.h` `private slots:` and define as `void TestMacQt::testX()`. `TestMacQt` builds windows with `makeShownMainWindow` (see `testSettingsShortcutOpensSettings`); for the Settings-dialog no-OpenGL check use the control-string pattern from `hasLanguageControl` in `src/app/ui_snapshot/main.cpp:80-90` (not read by this mapper; verify the lines).

---

### `src/qt_tests/tst_hidpi_qt.cpp` (STRIP-07 regression guard, unchanged)

**Analog:** itself. `testMedianColorRatioInvariant` (~line 1329) and `testColorPickerGrabAtRatio2` (line 1369): builds a `BasicImageView`, `placeOn(view, screenNamed("TwoX"/"OneX"), QSize(400,300))`, `waitForHq(view, dpr)`, `view.grab(rect)` then `ColorPickupInteraction::medianColor`. Record PASS lines before and after the removal; no edit. After deleting `sampleImageAtWidgetRect` these must still compile (they use only `grab` and `medianColor`).

---

### `src/core/ImageSizeLimit.{h,cpp}`, `TiffReader.cpp`, `JpegMetadataLoader.cpp`, `ImageLoader.cpp` (reader checks)

**Analog for header style:** `src/core/ImageMetadataLoader.h` (guard, forward declarations, status enum with doc comments).

**Copyright header + guard convention** (`ImageMetadataLoader.h:1-12`):
```cpp
// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_IMAGEMETADATALOADER_H_
#define SCANTAILOR_CORE_IMAGEMETADATALOADER_H_
...
#endif
```
New guards: `SCANTAILOR_CORE_IMAGESIZELIMIT_H_`, `SCANTAILOR_CORE_MEMORYBUDGET_H_`, `SCANTAILOR_APP_PROJECTSAVEFLOW_H_`, `SCANTAILOR_OUTPUT_OUTPUTGENERATORPROCESSOR_H_` (check which `SCANTAILOR_<MODULE>_` token output/app headers use: `grep -n "ifndef" src/core/filters/output/OutputGenerator.h src/app/MainWindow.h`). New files use the same two-line copyright comment (2019 text is what every existing file carries; no per-year variant found).

**Status enum:** `ImageMetadataLoader::Status { LOADED, NO_IMAGES, FORMAT_NOT_RECOGNIZED, GENERIC_ERROR }` (UPPER_SNAKE enumerators, `/**< ... */` trailing docs). Research recommends per-page `ImageSizeLimit::exceeds()` rather than a new status; if a status value is added, follow this enum style.

**ImageLoader insertion point** (`ImageLoader.cpp:28-46`; `std::call_once` for `QImageReader::setAllocationLimit(2048)` goes here, check after `reader.size()`, before `reader.read()`):
```cpp
QImage ImageLoader::load(QIODevice& ioDev, const int pageNum) {
  if (TiffReader::canRead(ioDev)) {
    return TiffReader::readImage(ioDev, pageNum);
  }
  ...
```
Failure contract is a null `QImage()` (not an exception, so `OutOfMemoryHandler`/`std::bad_alloc` never fires).

**JPEG insertion point** (`JpegMetadataLoader.cpp:195-206`): between `jpeg_read_header` (`assert(headerStatus == JPEG_HEADER_OK);`) and `if (!jpeg_start_decompress(cinfo.ptr()))`; the size is read right after (`const QSize size(cinfo->image_width, cinfo->image_height);`). Emit the metadata and return `LOADED` without starting decompression when `exceeds(size)`.

**TiffReader:** `TiffInfo` reads `uint32` tags into `int` (lines ~84, 121); check goes before the `if (info.mapsToBinaryOrIndexed8())` branch at ~275 and before the `QImage(info.width, ...)` at 279. Not re-read here; planner must open it.

---

### `src/core/LoadFileTask.{h,cpp}` ErrorResult third state

**Analog:** `LoadFileTask.cpp:111-135` (full excerpt):
```cpp
LoadFileTask::ErrorResult::ErrorResult(const QString& filePath)
    : m_filePath(QDir::toNativeSeparators(filePath)), m_fileExists(QFile::exists(filePath)) {}

void LoadFileTask::ErrorResult::updateUI(FilterUiInterface* ui) {
  class ErrWidget : public ErrorWidget { ... linkActivated -> relinkingDialogRequester ... };

  QString errMsg;
  Qt::TextFormat fmt = Qt::AutoText;
  if (m_fileExists) {
    errMsg = tr("The following file could not be loaded:\n%1").arg(m_filePath);
    fmt = Qt::PlainText;
  } else {
    errMsg = tr("The following file doesn't exist:<br>%1<br>...").arg(m_filePath.toHtmlEscaped());
    fmt = Qt::RichText;
  }
```
Add a branch before `m_fileExists`: `Qt::PlainText` message with dimensions (`tr("The following file is too large to load (%1 x %2 px):\n%3")`). Add the claimed size as a constructor parameter or member (`QSize m_tooLargeSize`). `tr` here resolves to `LoadFileTask::ErrorResult` context (QObject subclass).

---

### `src/app/ProjectFilesDialog.cpp` (`STATUS_TOO_LARGE`)

**Analog:** itself. Status enum at line 21: `enum Status { STATUS_DEFAULT, STATUS_LOAD_OK, STATUS_LOAD_FAILED };`; display switch at 534-538 (`case Item::STATUS_LOAD_FAILED:` ... red text); sort at 611-612 (`lhs.status() == Item::STATUS_LOAD_FAILED`); loader at 565-590:
```cpp
const ImageMetadataLoader::Status st = ImageMetadataLoader::load(
    filePath, [&](const ImageMetadata& metadata) { perPageMetadata.push_back(metadata); });
LoadStatus status;
if (st == ImageMetadataLoader::LOADED) {
  status = LOAD_OK;
  item.perPageMetadata().swap(perPageMetadata);
  item.setStatus(Item::STATUS_LOAD_OK);
} else {
  status = LOAD_FAILED;
  item.setStatus(Item::STATUS_LOAD_FAILED);
}
const QModelIndex idx(index(itemIdx, 0));
emit dataChanged(idx, idx);
```
Add `STATUS_TOO_LARGE`; after a `LOADED` result, if any page `exceeds(metadata.size())` set it, return `LOAD_FAILED`, and give the item display text the claimed size. Update the sort comparator at 611-612 so too-large sorts with failed. `MainWindow.cpp:1970-1982` follows the same load-then-check shape and feeds `LoadFilesStatusDialog`.

---

### `src/core/WorkerThreadPool.{h,cpp}`, `src/app/SystemLoadWidget.cpp` (SAFE-03)

**Analog:** `WorkerThreadPool.cpp:79-90` (the function being changed):
```cpp
void WorkerThreadPool::updateNumberOfThreads() {
  int maxThreads = QThread::idealThreadCount();
  if (sizeof(void*) <= 4) {            // dead on arm64, may go (D-07)
    maxThreads = std::min(maxThreads, 2);
  }
  int numThreads = m_settings.value("settings/batch_processing_threads", maxThreads).toInt();
  numThreads = std::min(numThreads, maxThreads);
  m_pool->setMaxThreadCount(numThreads);
}
```
Add `numThreads = std::min(numThreads, m_memoryCap)`; called from the ctor and on every `submitTask` (line 56), so the cap must be a stored int pushed in by `setMemoryCap(int)` from `MainWindow::startBatchProcessing` (not a scan per call). `SystemLoadWidget` (`SystemLoadWidget.cpp:12-47`) reads the same key, and its tooltip is `QString("%1/%2").arg(threads).arg(m_maxThreads)`; extend for the memory-limited text. Settings key must remain consistent (CONTEXT: ideally one shared function, e.g. `MemoryBudget::effectiveThreads(ideal, slider, cap)` used by both).

---

### `src/app/ProjectSaveFlow.{h,cpp}` (CLEAN-05)

**Analog:** `MainWindow.cpp:1425-1458` (`saveProjectAsTriggered`) and `OutOfMemoryDialog.cpp:50-92` (identical bodies except post-success line) plus `MainWindow.cpp:1879-1887` / `OutOfMemoryDialog.cpp:85-92` (`saveProjectWithFeedback`). Not re-read in full by this mapper; the planner must open both and copy verbatim (research lines 622-625 list the exact calls: `QFileDialog::getSaveFileName(this, QString(), projectDir, tr("Scan Tailor Projects") + " (*.ScanTailor)")`, case-insensitive `.ScanTailor` suffix, `QSettings` `project/lastDir`, `RecentProjects rp; rp.read(); rp.setMostRecent(...); rp.write();`, and `QMessageBox::warning(this, tr("Error"), tr("Error saving the project file!"))`).
Add to `gui_only_sources` in `src/app/CMakeLists.txt` (list is `Name.cpp Name.h` pairs, one per line, lines 12-50). `testNoDontUseNativeDialog` scans the new file automatically (keep the native panel call). Use `Q_DECLARE_TR_FUNCTIONS(ProjectSaveFlow)` for `tr` in a non-QObject class (CLAUDE.md allows `QCoreApplication::translate` form too).
Tests: `qt_tests` settings are redirected to `TEST_TMP_ROOT/qt_tests-settings` (`src/qt_tests/main.cpp:37-51`), so `remember()` can assert `project/lastDir` safely.

---

### `src/foundation` XML escape helpers + `src/foundation/tests/`

**Analog for tests:** `src/foundation/tests/CMakeLists.txt` (same shape as core: `set(sources main.cpp TestConstants.cpp ...)`, `target_link_libraries(foundation_tests PRIVATE foundation Boost::unit_test_framework)`); add `TestXmlEscape.cpp` to `sources`. Test file shape = `TestImageId.cpp` above. Helper call sites: `ProjectWriter.cpp:87,120,135` and `ProjectReader.cpp:29,107,140` (attribute `outputDirectory`, directory `path`, file `name`). Escape-only-when-needed with flag attribute `nameEscaped="1"` keeps ordinary files byte-identical.
**`XmlUnmarshaller.cpp:14`:** replace `return el.text();  // FIXME: this needs unescaping, but Qt doesn't provide such functionality` with a one-line comment that `QDomElement::text()` already returns unescaped text.

---

### `src/pipeline_tests/TestProjectRoundTrip.cpp` (SAFE-02 extended names, XFAIL)

**Analog:** itself. Name builders (lines 62-82) are free functions returning `QString`; the fixture creates PNGs from a `names` list (line 165):
```cpp
QString nameWithMarkup() { return QStringLiteral("a&b <c> \"d\" 'e'.png"); }
QString nameNfd() { return QString::fromUtf16(u"Seite_ä_NFD.png"); }
...
const QStringList names{nameWithMarkup(), nameNfc(), nameNfd(), nameOneCharacter(), nameTwoHundredBytes()};
for (const QString& name : names) {
  QImage image(64, 64, QImage::Format_RGB32);
  ...
  BOOST_REQUIRE_MESSAGE(image.save(path, "PNG"), "cannot write " << path.toStdString());
```
Add a second name set builder (tab, LF, CR, leading/trailing space, `it's`, `a>b`, emoji `QString::fromUtf16(u"\U0001F4C4.png")`-style, U+0001, U+001F, backslash) in the same style. Pipeline_tests CMake uses GLOB with `CONFIGURE_DEPENDS` (no CMake edit for new test files there).

**XFAIL usage** (`src/test_support/ExpectedFailure.h`, header-only, `#include <ExpectedFailure.h>` or as the existing test includes it):
```cpp
XFAIL_CHECK(condition_that_should_hold_after_fix, "backslash in file names (backlog 999.8)");
```
While the condition is false: logs `KNOWN DEFECT <tag>` and passes. When it becomes true: `BOOST_ERROR("expected failure now passes, remove the mark: ...")`. Observe first, wrap only if it really fails (research A6).

---

### `src/app/MainWindow*.cpp` split (CLEAN-02) and `OutputGenerator*` split (CLEAN-01)

**Analog:** the files themselves; pure moves. Registration: new `.cpp` into `gui_only_sources` (`src/app/CMakeLists.txt:12`; the library is `add_library(app_gui STATIC ${gui_only_sources} ${gui_only_ui_files})`, line 51) and into `set(sources ...)` of `src/core/filters/output/CMakeLists.txt` (existing entry style `OutputGenerator.cpp OutputGenerator.h`). The class `OutputGenerator::Processor` is defined in the .cpp (lines 97-253) and must move verbatim into an internal header `OutputGeneratorProcessor.h`. Proof tools (digest table, chunk-multiset script) are in research lines 565-578; copy the scratch harness into `.planning/phases/06-hardening-and-cleanup/records/` before CLEAN-01 since `build/scratch` is gitignored. Retire the Phase 5 `git diff f6cc4457 -- ... OutputGenerator.cpp` guard for this file only.

---

### STRIP-07 edits

No new analog needed; edit list and line refs are in research lines 515-529. `CMakeLists.txt:105` `find_package(Qt6 6.5 COMPONENTS Core Gui Widgets Xml Network OpenGL Svg OpenGLWidgets Test CONFIG REQUIRED)` drops `OpenGL`, `OpenGLWidgets`. `src/core/CMakeLists.txt:67` (`OpenGLSupport.cpp OpenGLSupport.h` in `common_sources`) and `:127` (link line). Link check must grep `QtOpenGL` only, not `-i opengl` (system `OpenGL.framework` stays).

---

### Gate log and records (`.planning/phases/06-hardening-and-cleanup/records/`)

**Analog:** `.planning/phases/05-mac-menus-and-shortcuts/records/gate-log.md`. Preamble paragraph, then one table with exactly these columns:
```markdown
| Label | Commit | Subject | Release ctest | Debug ctest | Goldens | Command diff (release / debug) | Expected diff met |
| m02-menu-roles | 47a22164 | feat(05-01): ... | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 0 / 0 | yes: empty (edits only) |
```
Labels are short (`m01-tracer`); the last column states why a non-empty diff is expected (e.g. "only the `-DSOURCE_ROOT=...` flag on the nine `qt_tests` compile lines"). Phase 6 expected non-empty: SAFE-01/03 (new sources), STRIP-07 (QtOpenGL flags vanish), CLEAN-01/02/05 (new sources). Per-gate command: `bash .planning/phases/03-qt-6-and-macos-only/gate.sh check <label>` (run `snapshot` once first; commit before `check`; output in `build/scratch/gate/<label>/summary.txt`).
**Closing-check record analog:** `records/closing-check.md` (Phase 5): header with date, HEAD, last gated commit; statement of what was and was not launched (app binary not launched; harness stands in); fenced command/output blocks for each live run (`mac_check ... verdict: PASS (exit 0)`).

## Shared Patterns

### Copyright header and guards
**Source:** `src/core/ImageMetadataLoader.h:1-12`, `src/core/tests/TestImageId.cpp:1-2`
**Apply to:** every new `.h/.cpp` (use `#ifndef SCANTAILOR_<MODULE>_<NAME>_H_`; the CLAUDE.md guide's older `FILENAME_H` form appears in `tst_ux_qt.h` (`TST_UX_QT_H`) for test headers only).

### Failure by null/status, not exception
**Source:** `ImageMetadataLoader::Status`; `TiffReader::readImage` ("or a null image in case of failure"); `PageId::subPageFromString(..., &ok)`
**Apply to:** `ImageSizeLimit`, reader checks, `MemoryBudget`. Never throw `std::bad_alloc` for the oversized case.

### Source-scan tests with a positive control
**Source:** `tst_mac_qt.cpp:1416-1520`
**Apply to:** STRIP-07 scan, updated beep test, any CLEAN-02 scan. Always include a control string and a `fileCount >= 500` guard.

### Test environment isolation
**Source:** `src/qt_tests/main.cpp:21-51`, `src/qt_tests/CMakeLists.txt` (`TEST_TMP_ROOT`, `SOURCE_ROOT` defines; ctest env `QT_QPA_PLATFORM=offscreen;LC_ALL=en_US.UTF-8;TMPDIR=build/.../test-tmp`)
**Apply to:** all new qt_tests slots (settings and temp files only under `build/`); Boost tests that need temp files use the project temp dir fixture pattern in `TestProjectRoundTrip.cpp` (`m_tempDir`), never `/tmp`.

### Formatting
`xcrun clang-format -i` then `--dry-run -Werror` on every touched C++ file (Chromium style, 2 spaces, `BinPackParameters: false`).

## No Analog Found

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| `makeMinimalLittleEndianTiff` forge helper (in `TestImageSizeLimit.cpp`) | test helper | file-I/O | No test builds TIFF bytes today; use libtiff via `TIFF::TIFF` as the scratch measurements did, or hand-build IFD (research A3) |
| Mach `phys_footprint` guard (`task_info`) | test helper | transform | No memory-measuring test exists |
| `FilterUiInterface` stub for `ErrorResult::updateUI` | test helper | request-response | No existing stub in `qt_tests`; check `src/core/FilterUiInterface.h` pure virtuals |
| Chunk-multiset script, digest harness | records tooling | batch | Only in gitignored `build/scratch/phase6`; copy into `records/` |
| `sysctlbyname("hw.memsize")` wrapper | utility | transform | No existing sysctl use found in `src/` |

## Metadata

**Analog search scope:** `src/core`, `src/core/tests`, `src/qt_tests`, `src/pipeline_tests`, `src/foundation/tests`, `src/test_support`, `src/app`, `.planning/phases/05-mac-menus-and-shortcuts/records`
**Files read:** about 14 (several partially; unread sections flagged above for the planner: `TiffReader.cpp`, `MainWindow.cpp:1425-1458`, `OutOfMemoryDialog.cpp:50-92`, `ui_snapshot/main.cpp:80-90`)
**Pattern extraction date:** 2026-11-16
