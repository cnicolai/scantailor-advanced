# Phase 5: Mac Menus and Shortcuts - Pattern Map

**Mapped:** 2026-11-14
**Files analyzed:** 16 new or modified
**Analogs found:** 14 / 16 (all analogs are git-tracked source under `src/` and `.planning/`)

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match |
|---|---|---|---|---|
| `src/app/CMakeLists.txt` (add `app_gui` static lib) | config | build | same file, `ui_snapshot` block (lines 56-70) | exact |
| `src/app/mac_check/main.cpp` | test harness (live) | request-response (event injection, verdicts) | `src/app/retina_check/main.cpp` | exact |
| `src/app/mac_check/CocoaProbe.{h,mm}` | test helper (Obj-C++) | event-driven | none | no analog |
| `src/qt_tests/tst_mac_qt.{h,cpp}` | test | request-response | `src/qt_tests/tst_hidpi_qt.{h,cpp}` | exact |
| `src/qt_tests/main.cpp`, `src/qt_tests/CMakeLists.txt` | test registration | batch | same files (TestHiDpiQt entry) | exact |
| `src/app/MainWindow.ui` / `.cpp` / `.h` | window / actions | event-driven | itself (`addAction`, `connect` blocks, lines 205-270) | exact |
| `src/core/zones/ZoneDefaultInteraction.{cpp,h}` | interaction handler | event-driven | itself (lines 30-40) plus `ZoomHandler.cpp` | exact |
| `src/core/zones/ZoneContextMenuInteraction.cpp` | interaction handler | event-driven | itself (lines 140-145, 200 ms workaround) | exact |
| `src/core/filters/select_content/ImageView.cpp` | view | event-driven | itself (lines 148-158) | exact |
| `src/core/filters/output/TabbedImageView.cpp` | view | event-driven | itself (lines 15-30) | exact |
| `src/app/RelinkingDialog.cpp` | dialog | request-response | itself (lines 62-76) | exact |
| `src/core/interaction/ZoomHandler.cpp` | handler (stays, keys unchanged) | event-driven | itself (lines 15-25) | reference |
| `.planning/phases/05-.../records/*.md` | records | doc | `.planning/phases/04-retina-rendering/records/live-page-view.md` | exact |

## Pattern Assignments

### `src/app/CMakeLists.txt` (config)
**Analog:** same file. Today `gui_only_sources` (lines 7-29, includes `main.cpp`) is compiled twice, into `scantailor-advanced` and, with `list(REMOVE_ITEM ... main.cpp)`, into `ui_snapshot`. `retina_check` links `core dewarping test_support` only and is the model for a harness that does not compile GUI sources.
```cmake
add_executable(${target_name} ${gui_only_sources} ${gui_only_ui_files} ${resource_files})
target_link_libraries(${target_name} PRIVATE core)
target_include_directories(${target_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    "$<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_INCLUDE_DIRECTORIES>")
...
if (BUILD_TESTS)
  set(ui_snapshot_sources ${gui_only_sources})
  list(REMOVE_ITEM ui_snapshot_sources main.cpp)
  add_executable(ui_snapshot ui_snapshot/main.cpp ${ui_snapshot_sources} ${gui_only_ui_files} ${resource_files})
  target_compile_definitions(ui_snapshot PRIVATE UI_SNAPSHOT_ROOT="${CMAKE_SOURCE_DIR}/build")
  add_executable(retina_check retina_check/main.cpp ${resource_files})
  target_compile_definitions(retina_check PRIVATE RETINA_CHECK_ROOT="${CMAKE_SOURCE_DIR}/build")
endif()
```
Plan: build `app_gui` (STATIC) from `gui_only_sources` minus `main.cpp` plus the `.ui` files (AUTOUIC is on at line 1), `PRIVATE core` plus PUBLIC include dir `${CMAKE_CURRENT_SOURCE_DIR}` and the TIFF include property (needed by consumers of `MainWindow.h`). The executable then holds `main.cpp` and `${resource_files}`. Resources (`.qrc`) stay with executables. Link `ui_snapshot`, `qt_tests` and `mac_check` against `app_gui`. `app_gui` must exist even when `BUILD_TESTS` is off (the app needs it). Add `mac_check` with `-DMAC_CHECK_ROOT="${CMAKE_SOURCE_DIR}/build"`; `.mm` needs `enable_language(OBJCXX)` inside `if (BUILD_TESTS)`.

### `src/app/mac_check/main.cpp` (live harness)
**Analog:** `src/app/retina_check/main.cpp` (1150 lines; header comment 1-14, `Summary` 98-125, `waitFor` ~128, `main` 1053-1150).

Header comment style (lines 1-14): purpose, `usage: retina_check <output-dir> [--section ...]`, "output directory must lie below build/", exits 3 on non-cocoa.

Summary collector (lines 98-125), copy verbatim: `Summary::add(key, value)` echoes `key: value` to stdout and stores; `save(path)` writes `summary.txt`.
```cpp
void add(const QString& key, const QString& value) {
  const QString line = key + ": " + value;
  m_lines.push_back(line);
  std::printf("%s\n", line.toLocal8Bit().constData());
  std::fflush(stdout);
}
```
Poll helper (line ~128): `template <typename Condition> bool waitFor(const Condition&, int limitMs)` using `QElapsedTimer` and event processing.

Argument parsing and path guard (lines 1053-1090):
```cpp
if (arg.startsWith("-Apple")) { ++i; }            // AppKit argument domain
else if (arg == "--section" && i + 1 < argc) { section = ...; }
else if (positional == 0 && !arg.startsWith("-")) { outArg = arg; ++positional; }
else { badArguments = true; }
...
const QString root = QDir::cleanPath(QString::fromUtf8(RETINA_CHECK_ROOT));   // use MAC_CHECK_ROOT
const QString out = QDir::cleanPath(QDir::current().absoluteFilePath(outArg));
if (out != root && !out.startsWith(root + "/")) { std::fprintf(stderr, "...not under..."); return 2; }
if (!QDir().mkpath(out + "/tmp") || !QDir().mkpath(out + "/settings")) { ...; return 2; }
```
Environment and settings isolation (lines 1091-1111), before the application object:
```cpp
qunsetenv("QT_QPA_PLATFORM");  qunsetenv("QT_SCALE_FACTOR"); qunsetenv("QT_SCREEN_SCALE_FACTORS"); ...
qputenv("LC_ALL", "en_US.UTF-8");
qputenv("TMPDIR", (out + "/tmp").toUtf8());
QCoreApplication::setOrganizationName("scantailor-tests");
QCoreApplication::setApplicationName("mac_check");
QSettings::setDefaultFormat(QSettings::IniFormat);
QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, out + "/settings");
Application app(argc, argv);
if (!QSettings().fileName().startsWith(out + "/settings/")) { ...; return 2; }
if (QGuiApplication::platformName() != "cocoa") { ...; return 3; }
```
Colour scheme and icons init (lines 1118-1123) is required before constructing `MainWindow` as well:
```cpp
std::unique_ptr<ColorScheme> scheme = ColorSchemeFactory().create(ApplicationSettings::getInstance().getColorScheme());
ColorSchemeManager::instance().setColorScheme(*scheme);
IconProvider::getInstance().setIconPack(StyledIconPack::createDefault());
```
Sections and verdicts (lines 1127-1150): `summary.add("platform"...)`, `qt_runtime`, `section`; each `runXSection(summary, out)` returns bool, emits `<prefix>_verdict: PASS|FAIL`; final `verdict`, `summary.save(out + "/summary.txt")`, `return pass ? 0 : 1`. Exit codes: 0 pass, 1 fail, 2 usage or path guard, 3 wrong platform. Sections for Phase 5 per RESEARCH: `menubar | dialogs | keys | zone | context | panels | all`. `ui_snapshot/main.cpp` (lines 1-50) shows constructing `MainWindow` with `#include "MainWindow.h"` (needs `app_gui` include dir) and `UI_SNAPSHOT_ROOT` guard of the same shape.

Obj-C++ helper `CocoaProbe.{h,mm}`: no analog in the repo (no `.mm` exists). Use RESEARCH.md "Standard Stack" (NSEvent synthesis, NSMenu dump, TIS layout). Keep it harness-only; no Accessibility permission.

### `src/qt_tests/tst_mac_qt.{h,cpp}` (test)
**Analog:** `src/qt_tests/tst_hidpi_qt.{h,cpp}` (header 44 lines, source 1407).

Header (copy shape; guard style `TST_HIDPI_QT_H` is used here):
```cpp
#ifndef TST_HIDPI_QT_H
#define TST_HIDPI_QT_H
#include <QObject>
class TestHiDpiQt : public QObject {
  Q_OBJECT
 private slots:
  void initTestCase();
  void testHqPixmapAtDeviceResolution();
  ...
};
#endif
```
New: `TST_MAC_QT_H`, `class TestMacQt`, slots `testMenuRoles`, `testShortcuts...`, `testViewMenu`, `testKeyNamesInStrings`, `testNoStdSystemInBatchPath`, `testNoDontUseNativeDialog`, etc. Same copyright first two lines as every file.

Source includes (lines 1-79): `#include "tst_hidpi_qt.h"`, module headers in angle brackets (`<ApplicationSettings.h>`, `<filters/output/Settings.h>`), then Qt, `<QtTest/QtTest>`, then std. Anonymous-namespace helpers before slots; tests use `QVERIFY`, `QTRY_COMPARE`, `QTest::qWaitForWindowExposed(&widget)`. Add `#include "MainWindow.h"` (via `app_gui` include dir).

Offscreen caveat (RESEARCH): `QKeySequence::Preferences` and `Quit` are empty offscreen, so compare with explicit `QKeySequence(Qt::CTRL | Qt::Key_Comma)`; `SaveAs` works. `qt_tests/main.cpp` sets settings under `TEST_TMP_ROOT` before `QApplication`; any MainWindow created in tests inherits that.

### `src/qt_tests/main.cpp` and `src/qt_tests/CMakeLists.txt` (registration)
**Analog:** the TestHiDpiQt entries.
```cpp
#include "tst_core_qt.h"
#include "tst_hidpi_qt.h"
#include "tst_ux_qt.h"
...
TestHiDpiQt tc3;
status |= QTest::qExec(&tc3, argc, argv);
return status;
```
Add `#include "tst_mac_qt.h"`, `TestMacQt tc4; status |= QTest::qExec(&tc4, argc, argv);` before `return`. Note main.cpp starts the offscreen platform with two screens (OneX, TwoX) via `QT_QPA_PLATFORM=offscreen:configfile=...`; it stays.
CMake:
```cmake
set(sources main.cpp tst_core_qt.cpp tst_hidpi_qt.cpp tst_ux_qt.cpp)   # add tst_mac_qt.cpp
target_link_libraries(qt_tests PRIVATE core dewarping foundation Qt6::Test Qt6::Core Qt6::Gui Qt6::Widgets)  # add app_gui
target_include_directories(qt_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/src")
```
`app_gui` must be defined before `qt_tests` (check `src/CMakeLists.txt` add_subdirectory order) or use target-existence-independent linking; the suite count stays 7 (no new `add_test`). The source-scan tests (no `std::system`, no `DontUseNativeDialog`, no "Ctrl"/"Alt" in strings) need a source-root definition; copy the `target_compile_definitions(qt_tests PRIVATE TEST_TMP_ROOT=...)` form with e.g. `SOURCE_ROOT="${CMAKE_SOURCE_DIR}/src"`.

### `src/app/MainWindow.{ui,cpp,h}` (actions, roles, View menu)
**Analog:** itself. Wiring blocks to extend:
- Window-level `addAction(...)` list, lines 205-228 (`addAction(actionMagnifyThumbnails); ... addAction(actionSwitchFilter6);`). Add new `actionZoomIn`, `actionZoomOut` here.
- Connects, lines 232-270:
```cpp
auto magnifyThumbnails = [this]() { scaleThumbnails(1); };
connect(actionMagnifyThumbnails, &QAction::triggered, magnifyThumbnails);
connect(actionDiminishThumbnails, &QAction::triggered, diminishThumbnails);
connect(actionSwitchFilter1, SIGNAL(triggered(bool)), SLOT(switchFilter1()));
```
- Role and shortcut code (Pattern 1 of RESEARCH.md lines 216-229): set `setMenuRole` immediately after `setupUi` and before the first `show()`; `setShortcuts({primary, secondary})` for stage actions (digit first, letter second).
- Quit connects to `close()` (RESEARCH cites lines 320, 328; `closeEvent` 544-551 defers via a 0 ms timer, so tests must not expect synchronous close).
- Beep, lines 1326-1337, replace the `external_alarm_cmd` and `std::system` branch:
```cpp
QApplication::alert(this);  // Flash the taskbar entry.
if (m_checkBeepWhenFinished()) {
  QString extPlayCmd("play /usr/share/sounds/freedesktop/stereo/bell.oga");
  QSettings settings;
  QString cmd = settings.value("main_window/external_alarm_cmd", extPlayCmd).toString();
  if (cmd.isEmpty()) { QApplication::beep(); } else { Q_UNUSED(std::system(cmd.toStdString().c_str())); }
}
```
  becomes `if (m_checkBeepWhenFinished()) { QApplication::beep(); }` and the stored value stays untouched (D-00d); drop `<cstdlib>` if unused.
- Zoom slot: `findPrimaryImageView` at lines 733-745; use RESEARCH Pattern 4 `zoomCurrentView` and enable or disable at the end of `setImageWidget` and `removeImageWidget`.
- `#ifdef ENABLE_DEBUG_FEATURES` block at lines 314-318 (hide the Debug Mode separator in the `#else` branch).
- `.ui`: Tools menu at lines 63-86, menus 87-109; `actionSwitchFilter1..6` texts need real stage names; add `menuView` between File and Tools in the menubar order.

### `src/core/zones/ZoneDefaultInteraction.{cpp,h}`
**Analog:** itself, lines 30-40:
```cpp
m_removeZoneShortcut = std::make_unique<QShortcut>(Qt::Key_Delete, &m_context.imageView());
m_removeVertexShortcut = std::make_unique<QShortcut>(Qt::Key_D, &m_context.imageView());
QObject::connect(m_removeZoneShortcut.get(), &QShortcut::activated, &m_context.imageView(),
                 std::bind(&ZoneDefaultInteraction::removeZoneUnderMouse, this));
```
Add `m_removeZoneBackspaceShortcut` (new `std::unique_ptr<QShortcut>` in the header) built the same way with `Qt::Key_Backspace` (Pattern 2, RESEARCH lines 236-242). Status tips at lines 19-34 use "Ctrl", "Shift+Ctrl", "Ctrl+Alt+Click", "Del"; rewrite with Apple glyphs (MAC-06), e.g. "Hold ⇧ to drag the zone or ⇧⌘ to copy. Press ⌫ to delete this zone." Check exact wording against 05-UI-SPEC.md.

### `src/core/zones/ZoneContextMenuInteraction.cpp`
**Analog:** itself, `menuAboutToHide()` (lines ~128-150): the `m_extraDelaysDone++ < 1` plus `QTimer::singleShot(200, this, SLOT(menuAboutToHide()))` workaround with the comment about OSX. Decision (keep, adjust, drop) comes from the live `context` harness section; the zone menu reads `QCursor::pos()`, so harness and tests must set the cursor position.

### `src/core/filters/select_content/ImageView.cpp`
**Analog:** itself, lines 148-158:
```cpp
QAction* create = m_noContentMenu->addAction(tr("Create Content Box"));
QAction* remove = m_haveContentMenu->addAction(tr("Remove Content Box"));
create->setShortcut(QKeySequence("Ins"));
remove->setShortcut(QKeySequence("Delete"));
addAction(create);
addAction(remove);
```
Change to `create->setShortcuts({QKeySequence(Qt::Key_B), QKeySequence(Qt::Key_Insert)}); remove->setShortcuts({QKeySequence(Qt::Key_Backspace), QKeySequence(Qt::Key_Delete)});`. Keep `addAction` and the `connect` lines. Update status tips in this file that mention Ctrl or Shift names (inventory in RESEARCH).

### `src/core/filters/output/TabbedImageView.cpp`
**Analog:** itself, lines 15-30:
```cpp
setStatusTip(tr("Use Ctrl+1..5 to switch the tabs."));
m_tabShortcuts[0] = new QShortcut(Qt::CTRL | Qt::Key_1, this);
...
for (int i = 0; i < 5; ++i) {
  m_tabShortcuts[i]->setAutoRepeat(false);
  connect(m_tabShortcuts[i], &QShortcut::activated, std::bind(&QTabWidget::setCurrentIndex, this, i));
}
```
Change keys to `Qt::CTRL | Qt::ALT | Qt::Key_N` and the tip to "Use ⌥⌘1..5 to switch the tabs." (wording per UI-SPEC). Must land in the same or an earlier commit than the stage actions' ⌘1-⌘5 (ambiguity).

### `src/app/RelinkingDialog.cpp`
**Analog:** itself, lines 62-76. Remove the trailing `nullptr, QFileDialog::DontUseNativeDialog` (file) and `QFileDialog::DontUseNativeDialog` (directory) arguments; for the file call pass `QString()` filter only. Delete the "So what's wrong with native dialogs?" comment (lines 72-75), replacing with a short note if the harness `panels` section shows a caveat. Native panels cannot pick a missing file or folder, so relinking via `getOpenFileName` on the missing path's parent dir needs the harness check (RESEARCH Open Questions).

### `ZoomHandler.cpp` (reference only)
Plain `+` and `-` stay (lines 18-19). Pattern for `QShortcut` + `std::bind` + `std::pow(2, 1.0 / 6)` step is reused by `MainWindow::zoomCurrentView`.

### Records (`.planning/phases/05-mac-menus-and-shortcuts/records/*.md`)
**Analog:** `.planning/phases/04-retina-rendering/records/live-page-view.md`. Format: H1 title naming the check and plan, one intro paragraph (date, command run, platform, what the subject is), a thresholds paragraph, then one `##` section per case with a fenced block of the raw `key: value` lines from `summary.txt` (e.g. `page_fit_verdict: PASS`) followed by an interpretation paragraph; image crops referenced under a `live/` subfolder. Phase 5 records must also state that no key or click was physically pressed (D-11).

## Shared Patterns

### File header and include style
Every new `.cpp/.h` starts with the two-line copyright (`// Copyright (C) 2019  Joseph Artsimovich ...` plus the GPLv3 line). Module headers in angle brackets, Qt next, std last; include guards `TST_MAC_QT_H` style.

### Settings isolation (tests and harness)
`qt_tests/main.cpp`: `QSettings::setDefaultFormat(IniFormat)`, `setPath(... tmpRoot + "/qt_tests-settings")`, assert the file name starts with the tmp root, return 2 otherwise. Same in `retina_check/main.cpp` 1091-1111. Neither may touch `~/.config` or `~/Library`.

### Explicit sequences and one owner per key
Use explicit `QKeySequence(Qt::CTRL | Qt::Key_X)` in code; `setShortcuts` (never two `setShortcut` calls); two `QShortcut` objects for two keys on a plain shortcut.

### Formatting gate
`xcrun clang-format -i <file>` and `--dry-run -Werror` on each touched C++ file; `.mm` included.

## No Analog Found

| File | Role | Data Flow | Reason |
|---|---|---|---|
| `src/app/mac_check/CocoaProbe.{h,mm}` | test helper | event-driven | No Objective-C++ exists in the repo; follow RESEARCH.md Standard Stack and its Open Questions. Needs `enable_language(OBJCXX)` and `-framework AppKit Carbon` on `mac_check` only. |
| `app_gui` static library | config | build | No static library with `.ui` files and AUTOUIC in the app tree; nearest pattern is the `core` library CMake in `src/core/CMakeLists.txt` (not re-read here). |

## Metadata

**Analog search scope:** `src/app`, `src/app/retina_check`, `src/app/ui_snapshot`, `src/qt_tests`, `src/core/zones`, `src/core/filters/select_content`, `src/core/filters/output`, `src/core/interaction`, `.planning/phases/04-retina-rendering/records`
**Files scanned:** about 14 opened or excerpted
**Pattern extraction date:** 2026-11-14
