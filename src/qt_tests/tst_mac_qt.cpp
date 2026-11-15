// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_mac_qt.h"

#include <AbstractFilter.h>
#include <ApplicationSettings.h>
#include <BasicImageView.h>
#include <BinaryImage.h>
#include <ColorSchemeFactory.h>
#include <ColorSchemeManager.h>
#include <ContentMask.h>
#include <Dpi.h>
#include <IconProvider.h>
#include <ImageId.h>
#include <ImagePixmapUnion.h>
#include <ImageTransformation.h>
#include <InteractionState.h>
#include <PageId.h>
#include <PageRange.h>
#include <PageSelectionAccessor.h>
#include <PageSelectionProvider.h>
#include <PageSequence.h>
#include <ProjectPages.h>
#include <SerializableSpline.h>
#include <StageSequence.h>
#include <StyledIconPack.h>
#include <filters/output/ImageViewTab.h>
#include <filters/output/PictureZoneEditor.h>
#include <filters/output/Settings.h>
#include <filters/output/TabbedImageView.h>
#include <filters/select_content/ImageView.h>

#include <config.h>

#include "MainWindow.h"
#include "SettingsDialog.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QCursor>
#include <QFrame>
#include <QGuiApplication>
#include <QImage>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLayoutItem>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPolygonF>
#include <QSignalSpy>
#include <QSpinBox>
#include <QStackedLayout>
#include <QStatusBar>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <QtTest/QtTest>
#include <cmath>
#include <memory>
#include <set>
#include <vector>

namespace {

std::unique_ptr<MainWindow> makeShownMainWindow() {
  auto window = std::make_unique<MainWindow>();
  window->resize(1280, 800);
  window->show();
  QTest::qWaitForWindowActive(window.get());
  return window;
}

QAction* findAction(const QWidget& window, const char* objectName) {
  return window.findChild<QAction*>(objectName);
}

QMenu* findMenu(const QWidget& window, const char* objectName) {
  return window.findChild<QMenu*>(objectName);
}

// Texts of the visible actions of a menu, "--" for a visible separator and the title for a submenu.
QStringList visibleEntries(const QMenu& menu) {
  QStringList entries;
  for (const QAction* action : menu.actions()) {
    if (!action->isVisible()) {
      continue;
    }
    if (action->isSeparator()) {
      entries.append("--");
    } else if (action->menu() != nullptr) {
      entries.append(action->menu()->title());
    } else {
      entries.append(action->text());
    }
  }
  return entries;
}

QString utf8(const char* text) {
  return QString::fromUtf8(text);
}

// A page selection without pages, enough to build a StageSequence (as in tst_hidpi_qt.cpp).
class EmptySelectionProvider : public PageSelectionProvider {
 public:
  PageSequence allPages() const override { return PageSequence(); }

  std::set<PageId> selectedPages() const override { return std::set<PageId>(); }

  std::vector<PageRange> selectedRanges() const override { return std::vector<PageRange>(); }
};

QStackedLayout* imageFrameLayout(MainWindow& window) {
  QFrame* const frame = window.findChild<QFrame*>("imageViewFrame");
  return frame != nullptr ? qobject_cast<QStackedLayout*>(frame->layout()) : nullptr;
}

// Removes every widget from the main area (the widgets stay children of the frame, hidden) so that a test can put
// its own view in. Deletes the layout items, not the widgets.
void emptyImageFrame(MainWindow& window) {
  QStackedLayout* const layout = imageFrameLayout(window);
  while (layout != nullptr && layout->count() > 0) {
    QLayoutItem* const item = layout->takeAt(0);
    if (item->widget() != nullptr) {
      item->widget()->hide();
    }
    delete item;
  }
}

QImage greyImage(int width, int height) {
  QImage image(width, height, QImage::Format_RGB32);
  image.fill(QColor(128, 128, 128));
  return image;
}

QAbstractItemView* stageList(MainWindow& window) {
  return window.findChild<QAbstractItemView*>("filterList");
}

int selectedStageRow(MainWindow& window) {
  const QModelIndexList rows = stageList(window)->selectionModel()->selectedRows();
  return rows.isEmpty() ? -1 : rows.first().row();
}

const char* const kStageActionNames[] = {"actionSwitchFilter1", "actionSwitchFilter2", "actionSwitchFilter3",
                                         "actionSwitchFilter4", "actionSwitchFilter5", "actionSwitchFilter6"};

// Zone editor of an empty picture mask over a white page, as in tst_hidpi_qt.cpp.
std::unique_ptr<output::PictureZoneEditor> makeZoneEditor(const QImage& page) {
  QImage maskImage(page.size(), QImage::Format_RGB32);
  maskImage.fill(Qt::black);
  return std::make_unique<output::PictureZoneEditor>(
      page, ImagePixmapUnion(), imageproc::BinaryImage(maskImage), QTransform(), QPolygonF(QRectF(page.rect())),
      PageId(ImageId(QStringLiteral("page.png"))), std::make_shared<output::Settings>());
}

QImage zonePage() {
  QImage page(1200, 900, QImage::Format_RGB32);
  page.fill(Qt::white);
  page.setDotsPerMeterX(11811);
  page.setDotsPerMeterY(11811);
  return page;
}

// Adds a rectangular zone (image pixels) the way the creation interaction does, without committing.
EditableSpline::Ptr addRectZone(output::PictureZoneEditor& editor, const QRectF& imageRect) {
  QPolygonF polygon;
  polygon << imageRect.topLeft() << imageRect.topRight() << imageRect.bottomRight() << imageRect.bottomLeft();
  auto spline = std::make_shared<EditableSpline>(SerializableSpline(polygon));
  editor.zones().addZone(spline);
  return spline;
}

int zoneCount(const output::PictureZoneEditor& editor) {
  int count = 0;
  for (const EditableZoneSet::Zone& zone : editor.zones()) {
    Q_UNUSED(zone);
    ++count;
  }
  return count;
}

bool hasZoneAt(const output::PictureZoneEditor& editor, const QPointF& imagePoint) {
  for (const EditableZoneSet::Zone& zone : editor.zones()) {
    if (zone.spline()->toPolygon().containsPoint(imagePoint, Qt::WindingFill)) {
      return true;
    }
  }
  return false;
}

// Moves the cursor to a point of the viewport (viewport coordinates) and sends the mouse move that the interaction
// handlers track. The zone menu reads QCursor::pos(), so both are needed.
void pointAt(QWidget* viewport, const QPoint& localPos) {
  QCursor::setPos(viewport->mapToGlobal(localPos));
  QTest::mouseMove(viewport, localPos);
}

QPoint viewportPointOf(const output::PictureZoneEditor& editor, const QPointF& imagePoint) {
  return editor.imageToWidget().map(imagePoint).toPoint();
}

// The views are scroll areas: the focus sits on the view or on its viewport.
bool focusIsIn(const QWidget& view) {
  const QWidget* const focus = QApplication::focusWidget();
  return focus != nullptr && (focus == &view || view.isAncestorOf(focus));
}

// A key to the widget that has the keyboard focus: the shortcut override of a focused field is then what Qt asks.
void keyToFocusWidget(Qt::Key key) {
  QWidget* const focus = QApplication::focusWidget();
  if (focus == nullptr) {
    QFAIL("no widget has the keyboard focus");
  }
  QTest::keyClick(focus, key);
}

// A top-level window with a zone editor and, optionally, a spin box and a line edit below it. The window is declared
// first so that it is destroyed last.
struct ZoneWindow {
  explicit ZoneWindow(const bool withFields) {
    editor = makeZoneEditor(zonePage());
    auto* const layout = new QVBoxLayout(&window);
    layout->addWidget(editor.get(), 1);
    if (withFields) {
      spinBox = new QSpinBox(&window);
      spinBox->setValue(5);
      lineEdit = new QLineEdit(QStringLiteral("abc"), &window);
      layout->addWidget(spinBox);
      layout->addWidget(lineEdit);
    }
    window.resize(700, 760);
    window.show();
    QTest::qWaitForWindowActive(&window);
  }

  void focusEditor() { editor->viewport()->setFocus(); }

  QWidget window;
  std::unique_ptr<output::PictureZoneEditor> editor;
  QSpinBox* spinBox = nullptr;
  QLineEdit* lineEdit = nullptr;
};

const QRectF kZoneA(100, 100, 400, 700);  // left half of the page, image pixels
const QRectF kZoneB(700, 100, 400, 700);  // right half

// A top-level window with a Select Content view and a spin box below it. The window is declared first so that it is
// destroyed last.
struct ContentWindow {
  explicit ContentWindow(const QRectF& contentRect) {
    QImage page(800, 600, QImage::Format_RGB32);
    page.fill(Qt::white);
    view = std::make_unique<select_content::ImageView>(page, page, ContentMask(),
                                                       ImageTransformation(QRectF(page.rect()), Dpi(300, 300)),
                                                       contentRect, QRectF(), false);
    auto* const layout = new QVBoxLayout(&window);
    layout->addWidget(view.get(), 1);
    spinBox = new QSpinBox(&window);
    spinBox->setValue(5);
    layout->addWidget(spinBox);
    window.resize(700, 600);
    window.show();
    QTest::qWaitForWindowActive(&window);
  }

  void focusView() { view->viewport()->setFocus(); }

  QWidget window;
  std::unique_ptr<select_content::ImageView> view;
  QSpinBox* spinBox = nullptr;
};

// The visible QMenu that is a child of the view, if any.
QMenu* visibleMenuOf(QWidget& view) {
  for (QMenu* const menu : view.findChildren<QMenu*>()) {
    if (menu->isVisible()) {
      return menu;
    }
  }
  return nullptr;
}

// A right-button click delivered through the window, as the platform does it: QWidgetWindow turns the press into the
// QContextMenuEvent that the views handle. QTest::mouseClick(widget, ...) would skip that step.
void rightClickThroughWindow(QWidget& window, QWidget* child, const QPoint& localPos) {
  QTest::mouseClick(window.windowHandle(), Qt::RightButton, Qt::NoModifier, child->mapTo(&window, localPos));
}

QStringList actionTexts(const QMenu& menu) {
  QStringList texts;
  for (const QAction* const action : menu.actions()) {
    texts << action->text();
  }
  return texts;
}

QAction* actionWithText(const QWidget& view, const QString& text) {
  for (QAction* const action : view.actions()) {
    if (action->text() == text) {
      return action;
    }
  }
  return nullptr;
}

}  // namespace

void TestMacQt::initTestCase() {
  QCOMPARE(QGuiApplication::platformName(), QString("offscreen"));

  {
    const std::unique_ptr<ColorScheme> scheme
        = ColorSchemeFactory().create(ApplicationSettings::getInstance().getColorScheme());
    ColorSchemeManager::instance().setColorScheme(*scheme);
  }
  IconProvider::getInstance().setIconPack(StyledIconPack::createDefault());
}

void TestMacQt::testSettingsInFileWithPreferencesRole() {
  const std::unique_ptr<MainWindow> window = makeShownMainWindow();

  QAction* const settings = findAction(*window, "actionSettings");
  QVERIFY(settings != nullptr);
  QMenu* const fileMenu = findMenu(*window, "menuFile");
  QMenu* const toolsMenu = findMenu(*window, "menuDebug");
  QVERIFY(fileMenu != nullptr);
  QVERIFY(toolsMenu != nullptr);

  QVERIFY(fileMenu->actions().contains(settings));
  QVERIFY(!toolsMenu->actions().contains(settings));

  QCOMPARE(settings->menuRole(), QAction::PreferencesRole);
  QCOMPARE(settings->shortcut(), QKeySequence(Qt::CTRL | Qt::Key_Comma));
  QCOMPARE(settings->shortcut().toString(QKeySequence::NativeText), QString::fromUtf8("⌘,"));
  QCOMPARE(settings->text(), QString::fromUtf8("Settings…"));
}

void TestMacQt::testSettingsShortcutOpensSettings() {
  const std::unique_ptr<MainWindow> window = makeShownMainWindow();
  QVERIFY(window->findChild<SettingsDialog*>() == nullptr);

  QTest::keyClick(window.get(), Qt::Key_Comma, Qt::ControlModifier);

  QTRY_VERIFY(window->findChild<SettingsDialog*>() != nullptr);
  SettingsDialog* const dialog = window->findChild<SettingsDialog*>();
  QVERIFY(dialog->isVisible());

  // The dialog deletes itself on close (WA_DeleteOnClose).
  dialog->close();
  QTRY_VERIFY(window->findChild<SettingsDialog*>() == nullptr);
}

void TestMacQt::testMenuRoles() {
  const std::unique_ptr<MainWindow> window = makeShownMainWindow();

  QStringList rolesSeen;
  int menusSeen = 0;
  for (const QAction* menuAction : window->menuBar()->actions()) {
    const QMenu* const menu = menuAction->menu();
    if (menu == nullptr) {
      continue;
    }
    ++menusSeen;
    for (const QAction* action : menu->actions()) {
      if (action->isSeparator() || action->menu() != nullptr) {
        continue;
      }
      const QString name = action->objectName();
      QAction::MenuRole expected = QAction::NoRole;
      if (name == "actionAbout") {
        expected = QAction::AboutRole;
        rolesSeen.append(name);
      } else if (name == "actionSettings") {
        expected = QAction::PreferencesRole;
        rolesSeen.append(name);
      } else if (name == "actionQuit") {
        expected = QAction::QuitRole;
        rolesSeen.append(name);
      }
      QVERIFY2(action->menuRole() == expected, qPrintable(name + " in " + menu->objectName() + " has the wrong role"));
    }
  }
  QVERIFY(menusSeen >= 2);

  rolesSeen.sort();
  QCOMPARE(rolesSeen, QStringList({"actionAbout", "actionQuit", "actionSettings"}));
}

void TestMacQt::testFileMenuTextsAndKeys() {
  const std::unique_ptr<MainWindow> window = makeShownMainWindow();
  QMenu* const fileMenu = findMenu(*window, "menuFile");
  QVERIFY(fileMenu != nullptr);

  const QStringList expectedEntries{utf8("New Project…"),
                                    utf8("Open Project…"),
                                    "--",
                                    "Save Project",
                                    utf8("Save Project As…"),
                                    "--",
                                    "Close Project",
                                    "About",
                                    utf8("Settings…"),
                                    "Quit"};
  QCOMPARE(visibleEntries(*fileMenu), expectedEntries);

  struct Binding {
    const char* objectName;
    QKeySequence sequence;
    Qt::Key key;
    Qt::KeyboardModifiers modifiers;
    QString nativeText;
  };
  const std::vector<Binding> bindings{
      {"actionNewProject", QKeySequence(Qt::CTRL | Qt::Key_N), Qt::Key_N, Qt::ControlModifier, utf8("⌘N")},
      {"actionOpenProject", QKeySequence(Qt::CTRL | Qt::Key_O), Qt::Key_O, Qt::ControlModifier, utf8("⌘O")},
      {"actionSaveProject", QKeySequence(Qt::CTRL | Qt::Key_S), Qt::Key_S, Qt::ControlModifier, utf8("⌘S")},
      {"actionSaveProjectAs", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), Qt::Key_S,
       Qt::ControlModifier | Qt::ShiftModifier, utf8("⇧⌘S")},
      {"actionCloseProject", QKeySequence(Qt::CTRL | Qt::Key_W), Qt::Key_W, Qt::ControlModifier, utf8("⌘W")},
      {"actionQuit", QKeySequence(Qt::CTRL | Qt::Key_Q), Qt::Key_Q, Qt::ControlModifier, utf8("⌘Q")},
  };

  for (const Binding& binding : bindings) {
    QAction* const action = findAction(*window, binding.objectName);
    QVERIFY2(action != nullptr, binding.objectName);
    QCOMPARE(action->shortcut(), binding.sequence);
    QCOMPARE(action->shortcut().toString(QKeySequence::NativeText), binding.nativeText);

    // The production slots open panels or close the window; only the key path is under test here.
    QObject::disconnect(action, &QAction::triggered, window.get(), nullptr);
    action->setEnabled(true);
    QSignalSpy spy(action, &QAction::triggered);
    QTest::keyClick(window.get(), binding.key, binding.modifiers);
    QVERIFY2(spy.count() == 1, binding.objectName);

    if (QString(binding.objectName) == "actionSaveProjectAs") {
      QTest::keyClick(window.get(), binding.key, binding.modifiers);
      QCOMPARE(spy.count(), 2);
    }
  }
}

void TestMacQt::testToolsMenuWithoutSettings() {
  const std::unique_ptr<MainWindow> window = makeShownMainWindow();
  QMenu* const toolsMenu = findMenu(*window, "menuDebug");
  QVERIFY(toolsMenu != nullptr);
  QVERIFY(!toolsMenu->actions().contains(findAction(*window, "actionSettings")));

  QStringList expected{
      utf8("Fix DPI…"), utf8("Relinking…"), "Reverse Two-Page Spread Order", "--", utf8("Default Parameters…"),
      "Units"};
#ifdef ENABLE_DEBUG_FEATURES
  expected << "--" << "Debug Mode";
#endif
  QCOMPARE(visibleEntries(*toolsMenu), expected);
  QVERIFY(visibleEntries(*toolsMenu).last() != "--");
}

void TestMacQt::testQuitClosesWindow() {
  const std::unique_ptr<MainWindow> window = makeShownMainWindow();
  QVERIFY(window->isVisible());

  QAction* const quit = findAction(*window, "actionQuit");
  QVERIFY(quit != nullptr);
  quit->trigger();

  // closeEvent ignores the first close and finishes it from a zero-delay timer.
  QTRY_VERIFY(!window->isVisible());
}

void TestMacQt::testProjectActionsDisabledWithoutProject() {
  const std::unique_ptr<MainWindow> window = makeShownMainWindow();

  for (const char* name : {"actionSaveProject", "actionSaveProjectAs", "actionCloseProject", "actionFixDpi",
                           "actionRelinking", "actionReverseTwoPageOrder"}) {
    const QAction* const action = findAction(*window, name);
    QVERIFY2(action != nullptr, name);
    QVERIFY2(!action->isEnabled(), name);
  }
  for (const char* name :
       {"actionNewProject", "actionOpenProject", "actionSettings", "actionAbout", "actionQuit", "actionDefaults"}) {
    const QAction* const action = findAction(*window, name);
    QVERIFY2(action != nullptr, name);
    QVERIFY2(action->isEnabled(), name);
  }

  // Shift-Command-S with no project does nothing: the disabled action cannot trigger.
  QAction* const saveAs = findAction(*window, "actionSaveProjectAs");
  QSignalSpy spy(saveAs, &QAction::triggered);
  QTest::keyClick(window.get(), Qt::Key_S, Qt::ControlModifier | Qt::ShiftModifier);
  QCOMPARE(spy.count(), 0);
}

void TestMacQt::testStageActions() {
  const std::unique_ptr<MainWindow> window = makeShownMainWindow();

  const auto provider = std::make_shared<EmptySelectionProvider>();
  const StageSequence stages(std::make_shared<ProjectPages>(), PageSelectionAccessor(provider));
  const Qt::Key digits[] = {Qt::Key_1, Qt::Key_2, Qt::Key_3, Qt::Key_4, Qt::Key_5, Qt::Key_6};
  const Qt::Key letters[] = {Qt::Key_O, Qt::Key_E, Qt::Key_N, Qt::Key_V, Qt::Key_K, Qt::Key_P};

  // Nothing is loaded: the six actions are disabled and the keys do nothing.
  for (int n = 1; n <= 6; ++n) {
    QAction* const action = findAction(*window, kStageActionNames[n - 1]);
    QVERIFY2(action != nullptr, kStageActionNames[n - 1]);
    QCOMPARE(action->text(), stages.filterAt(n - 1)->getName());
    QCOMPARE(action->toolTip(), "Switch to " + stages.filterAt(n - 1)->getName());
    QCOMPARE(action->menuRole(), QAction::NoRole);
    const QList<QKeySequence> expected{QKeySequence(Qt::CTRL | digits[n - 1]), QKeySequence(letters[n - 1])};
    QCOMPARE(action->shortcuts(), expected);
    QCOMPARE(action->shortcuts().at(0).toString(QKeySequence::NativeText), utf8("⌘") + QString::number(n));
    QVERIFY2(!action->isEnabled(), kStageActionNames[n - 1]);

    QSignalSpy spy(action, &QAction::triggered);
    QTest::keyClick(window.get(), digits[n - 1], Qt::ControlModifier);
    QTest::keyClick(window.get(), letters[n - 1]);
    QCOMPARE(spy.count(), 0);
  }
  QCOMPARE(selectedStageRow(*window), 0);

  // With a project loaded the actions are enabled; the production slot selects stage n in the stage list.
  for (int n = 1; n <= 6; ++n) {
    QAction* const action = findAction(*window, kStageActionNames[n - 1]);
    action->setEnabled(true);
    QSignalSpy spy(action, &QAction::triggered);

    QTest::keyClick(window.get(), digits[n - 1], Qt::ControlModifier);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(selectedStageRow(*window), n - 1);

    if (n == 3) {
      // Twice is twice, never four times (menu and window shortcut), and the letter is the same action.
      QTest::keyClick(window.get(), Qt::Key_3, Qt::ControlModifier);
      QCOMPARE(spy.count(), 2);
      QCOMPARE(selectedStageRow(*window), 2);
      QTest::keyClick(window.get(), Qt::Key_N);
      QCOMPARE(spy.count(), 3);
      QCOMPARE(selectedStageRow(*window), 2);
    }
  }

  // The letter types into a focused line edit and triggers no stage; the Command digit still switches the stage.
  QLineEdit* const edit = new QLineEdit;
  window->findChild<QStatusBar*>("statusBar")->addWidget(edit);
  edit->setFocus();
  QTRY_VERIFY(QApplication::focusWidget() == edit);

  QAction* const stage1 = findAction(*window, "actionSwitchFilter1");
  QSignalSpy spy1(stage1, &QAction::triggered);
  QTest::keyClick(QApplication::focusWidget(), Qt::Key_O);
  QCOMPARE(spy1.count(), 0);
  QCOMPARE(edit->text(), QString("o"));
  QTest::keyClick(QApplication::focusWidget(), Qt::Key_1, Qt::ControlModifier);
  QCOMPARE(spy1.count(), 1);
  QCOMPARE(edit->text(), QString("o"));
  QCOMPARE(selectedStageRow(*window), 0);
}

void TestMacQt::testViewMenuStages() {
  const std::unique_ptr<MainWindow> window = makeShownMainWindow();

  QStringList topLevel;
  for (const QAction* menuAction : window->menuBar()->actions()) {
    if (menuAction->menu() != nullptr) {
      topLevel.append(menuAction->menu()->objectName());
    }
  }
  QCOMPARE(topLevel, QStringList({"menuFile", "menuView", "menuDebug"}));

  QMenu* const viewMenu = findMenu(*window, "menuView");
  QVERIFY(viewMenu != nullptr);
  QCOMPARE(viewMenu->title(), QString("View"));
  QVERIFY(viewMenu->actions().size() >= 6);
  for (int n = 0; n < 6; ++n) {
    QVERIFY2(viewMenu->actions().at(n) == findAction(*window, kStageActionNames[n]), kStageActionNames[n]);
  }
}

void TestMacQt::testOutputTabKeys() {
  const std::unique_ptr<MainWindow> window = makeShownMainWindow();
  emptyImageFrame(*window);
  QStackedLayout* const layout = imageFrameLayout(*window);
  QVERIFY(layout != nullptr);

  auto* const tabs = new output::TabbedImageView;
  tabs->addTab(new BasicImageView(greyImage(200, 150)), "Output", output::TAB_OUTPUT);
  tabs->addTab(new BasicImageView(greyImage(200, 150)), "Picture Zones", output::TAB_PICTURE_ZONES);
  tabs->addTab(new BasicImageView(greyImage(200, 150)), "Fill Zones", output::TAB_FILL_ZONES);
  layout->addWidget(tabs);
  layout->setCurrentWidget(tabs);
  QTRY_VERIFY(tabs->isVisible());
  tabs->currentWidget()->setFocus();
  QCOMPARE(tabs->currentIndex(), 0);

  QCOMPARE(tabs->statusTip(), utf8("Use ⌥⌘1 to ⌥⌘5 to switch the tabs."));

  QAction* const stage1 = findAction(*window, "actionSwitchFilter1");
  QObject::disconnect(stage1, &QAction::triggered, window.get(), nullptr);
  stage1->setEnabled(true);
  QSignalSpy spy(stage1, &QAction::triggered);

  // Command-1 is the stage key; it changes no tab.
  QTest::keyClick(window.get(), Qt::Key_1, Qt::ControlModifier);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(tabs->currentIndex(), 0);

  // Option-Command-n is the tab key; it triggers no stage action.
  QTest::keyClick(window.get(), Qt::Key_2, Qt::ControlModifier | Qt::AltModifier);
  QCOMPARE(tabs->currentIndex(), 1);
  QCOMPARE(spy.count(), 1);
  QTest::keyClick(window.get(), Qt::Key_3, Qt::ControlModifier | Qt::AltModifier);
  QCOMPARE(tabs->currentIndex(), 2);
  QCOMPARE(spy.count(), 1);
  QTest::keyClick(window.get(), Qt::Key_1, Qt::ControlModifier | Qt::AltModifier);
  QCOMPARE(tabs->currentIndex(), 0);
  QCOMPARE(spy.count(), 1);
}

void TestMacQt::testZoomActions() {
  const std::unique_ptr<MainWindow> window = makeShownMainWindow();
  emptyImageFrame(*window);
  QStackedLayout* const layout = imageFrameLayout(*window);
  QVERIFY(layout != nullptr);

  auto* const view = new BasicImageView(greyImage(1200, 900));
  layout->addWidget(view);
  layout->setCurrentWidget(view);
  QTRY_VERIFY(view->isVisible());
  view->setFocus();

  QAction* const zoomIn = findAction(*window, "actionZoomIn");
  QAction* const zoomOut = findAction(*window, "actionZoomOut");
  QVERIFY(zoomIn != nullptr);
  QVERIFY(zoomOut != nullptr);
  QTRY_VERIFY(zoomIn->isEnabled());
  QVERIFY(zoomOut->isEnabled());

  const QList<QKeySequence> expectedIn{QKeySequence(Qt::CTRL | Qt::Key_Plus), QKeySequence(Qt::CTRL | Qt::Key_Equal)};
  QCOMPARE(zoomIn->shortcuts(), expectedIn);
  QCOMPARE(zoomIn->shortcuts().at(0).toString(QKeySequence::NativeText), utf8("⌘+"));
  const QList<QKeySequence> expectedOut{QKeySequence(Qt::CTRL | Qt::Key_Minus)};
  QCOMPARE(zoomOut->shortcuts(), expectedOut);
  QCOMPARE(zoomOut->shortcuts().at(0).toString(QKeySequence::NativeText), utf8("⌘-"));

  // The cursor is away from the view: the view centre is the focus (the cursor rule is a manual check).
  QCursor::setPos(QPoint(5000, 5000));
  QVERIFY(!view->rect().contains(view->mapFromGlobal(QCursor::pos())));

  QSignalSpy larger(findAction(*window, "actionMagnifyThumbnails"), &QAction::triggered);
  QSignalSpy smaller(findAction(*window, "actionDiminishThumbnails"), &QAction::triggered);
  const double step = std::pow(2.0, 1.0 / 6);

  QCOMPARE(view->zoomLevel(), 1.0);
  QTest::keyClick(window.get(), Qt::Key_Plus, Qt::ControlModifier);
  QVERIFY(qFuzzyCompare(view->zoomLevel(), step));
  QTest::keyClick(window.get(), Qt::Key_Equal, Qt::ControlModifier);
  QVERIFY(qFuzzyCompare(view->zoomLevel(), step * step));
  QTest::keyClick(window.get(), Qt::Key_Minus, Qt::ControlModifier);
  QVERIFY(qFuzzyCompare(view->zoomLevel(), step));
  for (int i = 0; i < 3; ++i) {
    QTest::keyClick(window.get(), Qt::Key_Minus, Qt::ControlModifier);
  }
  QCOMPARE(view->zoomLevel(), 1.0);

  // The menu items do the same through trigger().
  zoomIn->trigger();
  QVERIFY(qFuzzyCompare(view->zoomLevel(), step));
  QCOMPARE(larger.count(), 0);
  QCOMPARE(smaller.count(), 0);

  // The plain plus key of ZoomHandler is unchanged.
  QTest::keyClick(view, Qt::Key_Plus);
  QVERIFY(qFuzzyCompare(view->zoomLevel(), step * step));
  QCOMPARE(larger.count(), 0);
}

void TestMacQt::testZoomDisabledWithoutView() {
  const std::unique_ptr<MainWindow> window = makeShownMainWindow();

  QAction* const zoomIn = findAction(*window, "actionZoomIn");
  QAction* const zoomOut = findAction(*window, "actionZoomOut");
  QVERIFY(zoomIn != nullptr);
  QVERIFY(zoomOut != nullptr);
  QVERIFY(!zoomIn->isEnabled());
  QVERIFY(!zoomOut->isEnabled());

  QSignalSpy spy(zoomIn, &QAction::triggered);
  QTest::keyClick(window.get(), Qt::Key_Plus, Qt::ControlModifier);
  QCOMPARE(spy.count(), 0);

  emptyImageFrame(*window);
  QStackedLayout* const layout = imageFrameLayout(*window);
  auto* const view = new BasicImageView(greyImage(400, 300));
  layout->addWidget(view);
  layout->setCurrentWidget(view);
  QVERIFY(zoomIn->isEnabled());
  QVERIFY(zoomOut->isEnabled());

  layout->removeWidget(view);
  QVERIFY(!zoomIn->isEnabled());
  QVERIFY(!zoomOut->isEnabled());
  delete view;
}

void TestMacQt::testThumbnailKeys() {
  const std::unique_ptr<MainWindow> window = makeShownMainWindow();

  QAction* const larger = findAction(*window, "actionMagnifyThumbnails");
  QAction* const smaller = findAction(*window, "actionDiminishThumbnails");
  QAction* const zoomIn = findAction(*window, "actionZoomIn");
  QVERIFY(larger != nullptr);
  QVERIFY(smaller != nullptr);
  QVERIFY(zoomIn != nullptr);
  QCOMPARE(larger->text(), QString("Larger Thumbnails"));
  QCOMPARE(smaller->text(), QString("Smaller Thumbnails"));

  const QList<QKeySequence> expectedLarger{QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Plus),
                                           QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Equal)};
  const QList<QKeySequence> expectedSmaller{QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Minus)};
  QCOMPARE(larger->shortcuts(), expectedLarger);
  QCOMPARE(smaller->shortcuts(), expectedSmaller);
  QCOMPARE(larger->shortcuts().at(0).toString(QKeySequence::NativeText), utf8("⌥⌘+"));
  QCOMPARE(smaller->shortcuts().at(0).toString(QKeySequence::NativeText), utf8("⌥⌘-"));

  QAbstractButton* const largerButton = window->findChild<QAbstractButton*>("magnifyThumbnailsBtn");
  QAbstractButton* const smallerButton = window->findChild<QAbstractButton*>("diminishThumbnailsBtn");
  QVERIFY(largerButton != nullptr);
  QVERIFY(smallerButton != nullptr);
  QCOMPARE(largerButton->toolTip(), utf8("Larger thumbnails.  (⌥⌘+)"));
  QCOMPARE(smallerButton->toolTip(), utf8("Smaller thumbnails.  (⌥⌘-)"));

  // The production slots are lambdas whose receiver is the action itself; only the key path is under test here.
  for (QAction* const action : {larger, smaller}) {
    QObject::disconnect(action, &QAction::triggered, action, nullptr);
    QObject::disconnect(action, &QAction::triggered, window.get(), nullptr);
  }
  QSignalSpy largerSpy(larger, &QAction::triggered);
  QSignalSpy smallerSpy(smaller, &QAction::triggered);
  QSignalSpy zoomSpy(zoomIn, &QAction::triggered);

  const Qt::KeyboardModifiers modifiers = Qt::ControlModifier | Qt::AltModifier;
  QTest::keyClick(window.get(), Qt::Key_Plus, modifiers);
  QCOMPARE(largerSpy.count(), 1);
  QCOMPARE(zoomSpy.count(), 0);
  QTest::keyClick(window.get(), Qt::Key_Equal, modifiers);
  QCOMPARE(largerSpy.count(), 2);
  QTest::keyClick(window.get(), Qt::Key_Minus, modifiers);
  QCOMPARE(smallerSpy.count(), 1);
  QCOMPARE(zoomSpy.count(), 0);

  // Command-plus belongs to the page zoom; with no page view it does nothing, and never reaches the thumbnails.
  QTest::keyClick(window.get(), Qt::Key_Plus, Qt::ControlModifier);
  QCOMPARE(largerSpy.count(), 2);
  QCOMPARE(smallerSpy.count(), 1);
}

void TestMacQt::testViewMenuContents() {
  const std::unique_ptr<MainWindow> window = makeShownMainWindow();
  QMenu* const viewMenu = findMenu(*window, "menuView");
  QVERIFY(viewMenu != nullptr);

  const QStringList expected{
      "Fix Orientation", "Split Pages", "Deskew", "Select Content",    "Margins",           "Output", "--",
      "Zoom In",         "Zoom Out",    "--",     "Larger Thumbnails", "Smaller Thumbnails"};
  QCOMPARE(visibleEntries(*viewMenu), expected);
}

void TestMacQt::testZoneRemoveKeys() {
  ZoneWindow zw(false);
  output::PictureZoneEditor& editor = *zw.editor;
  addRectZone(editor, kZoneA);
  addRectZone(editor, kZoneB);
  QCOMPARE(zoneCount(editor), 2);

  zw.focusEditor();
  QTRY_VERIFY(focusIsIn(editor));

  // Backspace removes the zone under the pointer and keeps the other one.
  pointAt(editor.viewport(), viewportPointOf(editor, kZoneA.center()));
  keyToFocusWidget(Qt::Key_Backspace);
  QCOMPARE(zoneCount(editor), 1);
  QVERIFY(!hasZoneAt(editor, kZoneA.center()));
  QVERIFY(hasZoneAt(editor, kZoneB.center()));

  // A second press over the same spot removes nothing more.
  keyToFocusWidget(Qt::Key_Backspace);
  QCOMPARE(zoneCount(editor), 1);
  QVERIFY(hasZoneAt(editor, kZoneB.center()));

  // Forward delete removes a zone as well.
  addRectZone(editor, kZoneA);
  QCOMPARE(zoneCount(editor), 2);
  pointAt(editor.viewport(), viewportPointOf(editor, kZoneA.center() + QPointF(20, 20)));
  keyToFocusWidget(Qt::Key_Delete);
  QCOMPARE(zoneCount(editor), 1);
  QVERIFY(hasZoneAt(editor, kZoneB.center()));

  pointAt(editor.viewport(), viewportPointOf(editor, kZoneB.center()));
  keyToFocusWidget(Qt::Key_Backspace);
  QCOMPARE(zoneCount(editor), 0);

  // With no zone left the keys do nothing.
  keyToFocusWidget(Qt::Key_Backspace);
  keyToFocusWidget(Qt::Key_Delete);
  QCOMPARE(zoneCount(editor), 0);
}

void TestMacQt::testZoneKeysIgnoredInTextFields() {
  ZoneWindow zw(true);
  output::PictureZoneEditor& editor = *zw.editor;
  addRectZone(editor, kZoneA);
  QCOMPARE(zoneCount(editor), 1);

  zw.focusEditor();
  QTRY_VERIFY(focusIsIn(editor));
  pointAt(editor.viewport(), viewportPointOf(editor, kZoneA.center()));

  // A spin box takes both keys for itself.
  zw.spinBox->setFocus();
  QTRY_VERIFY(zw.spinBox->isAncestorOf(QApplication::focusWidget()));
  keyToFocusWidget(Qt::Key_Backspace);
  keyToFocusWidget(Qt::Key_Delete);
  QCOMPARE(zoneCount(editor), 1);

  // A line edit edits its text and leaves the zone alone.
  zw.lineEdit->setFocus();
  QTRY_VERIFY(QApplication::focusWidget() == zw.lineEdit);
  zw.lineEdit->setCursorPosition(3);
  keyToFocusWidget(Qt::Key_Backspace);
  QCOMPARE(zw.lineEdit->text(), QString("ab"));
  QCOMPARE(zoneCount(editor), 1);
  keyToFocusWidget(Qt::Key_Delete);
  QCOMPARE(zw.lineEdit->text(), QString("ab"));
  QCOMPARE(zoneCount(editor), 1);

  // Positive control: with the editor focused the same key removes the zone.
  zw.focusEditor();
  QTRY_VERIFY(focusIsIn(editor));
  keyToFocusWidget(Qt::Key_Backspace);
  QCOMPARE(zoneCount(editor), 0);
}

void TestMacQt::testZoneStatusTips() {
  ZoneWindow zw(false);
  output::PictureZoneEditor& editor = *zw.editor;

  QCOMPARE(editor.interactionState().defaultStatusTip(),
           utf8("Click to start creating a new zone. ⌥⌘-click to copy the latest created zone. Use Z, X and C keys "
                "to switch zone creation mode."));

  addRectZone(editor, kZoneA);
  pointAt(editor.viewport(), viewportPointOf(editor, kZoneA.center()));
  QTRY_VERIFY(editor.interactionState().statusTip().contains(utf8("Press ⌫ to delete this zone.")));
  QVERIFY(editor.interactionState().statusTip().contains(utf8("Hold ⇧ to drag the zone or ⇧⌘ to copy.")));
}

void TestMacQt::testContentBoxKeys() {
  ContentWindow cw{QRectF()};
  select_content::ImageView& view = *cw.view;
  QSignalSpy spy(&view, &select_content::ImageView::manualContentRectSet);

  cw.focusView();
  QTRY_VERIFY(focusIsIn(view));

  // B creates the box once; a second B changes nothing.
  keyToFocusWidget(Qt::Key_B);
  QCOMPARE(spy.count(), 1);
  QVERIFY(!spy.last().at(0).toRectF().isEmpty());
  keyToFocusWidget(Qt::Key_B);
  QCOMPARE(spy.count(), 1);

  // Backspace removes it; a second one changes nothing.
  keyToFocusWidget(Qt::Key_Backspace);
  QCOMPARE(spy.count(), 2);
  QVERIFY(spy.last().at(0).toRectF().isEmpty());
  keyToFocusWidget(Qt::Key_Backspace);
  QCOMPARE(spy.count(), 2);

  // The PC keys stay as second keys.
  keyToFocusWidget(Qt::Key_Insert);
  QCOMPARE(spy.count(), 3);
  keyToFocusWidget(Qt::Key_Delete);
  QCOMPARE(spy.count(), 4);

  // While a drag has the interaction captured, none of the keys acts.
  {
    InteractionState::Captor captor;
    view.interactionState().capture(captor);
    keyToFocusWidget(Qt::Key_B);
    QCOMPARE(spy.count(), 4);
    keyToFocusWidget(Qt::Key_Insert);
    QCOMPARE(spy.count(), 4);
    captor.release();
  }
  keyToFocusWidget(Qt::Key_B);
  QCOMPARE(spy.count(), 5);
  {
    InteractionState::Captor captor;
    view.interactionState().capture(captor);
    keyToFocusWidget(Qt::Key_Backspace);
    keyToFocusWidget(Qt::Key_Delete);
    QCOMPARE(spy.count(), 5);
    captor.release();
  }

  // A spin box with the focus keeps B, Backspace and Delete for itself.
  cw.spinBox->setFocus();
  QTRY_VERIFY(focusIsIn(*cw.spinBox));
  keyToFocusWidget(Qt::Key_B);
  keyToFocusWidget(Qt::Key_Backspace);
  keyToFocusWidget(Qt::Key_Delete);
  QCOMPARE(spy.count(), 5);

  // Positive control: with the view focused again Backspace removes the box.
  cw.focusView();
  QTRY_VERIFY(focusIsIn(view));
  keyToFocusWidget(Qt::Key_Backspace);
  QCOMPARE(spy.count(), 6);
}

void TestMacQt::testContentMenuShowsKeys() {
  ContentWindow cw{QRectF()};

  const QAction* const create = actionWithText(*cw.view, QStringLiteral("Create Content Box"));
  QVERIFY(create != nullptr);
  QCOMPARE(create->shortcuts().size(), 2);
  QCOMPARE(create->shortcuts().at(0), QKeySequence(Qt::Key_B));
  QCOMPARE(create->shortcuts().at(0).toString(QKeySequence::NativeText), QString("B"));
  QCOMPARE(create->shortcuts().at(1), QKeySequence(Qt::Key_Insert));

  const QAction* const remove = actionWithText(*cw.view, QStringLiteral("Remove Content Box"));
  QVERIFY(remove != nullptr);
  QCOMPARE(remove->shortcuts().size(), 2);
  QCOMPARE(remove->shortcuts().at(0), QKeySequence(Qt::Key_Backspace));
  QCOMPARE(remove->shortcuts().at(0).toString(QKeySequence::NativeText), utf8("⌫"));
  QCOMPARE(remove->shortcuts().at(1), QKeySequence(Qt::Key_Delete));
}

void TestMacQt::testZoneMenuDismissRestoresDefaultAtOnce() {
  ZoneWindow zw(false);
  output::PictureZoneEditor& editor = *zw.editor;
  addRectZone(editor, kZoneA);
  QCOMPARE(zoneCount(editor), 1);

  const QPoint centre = viewportPointOf(editor, kZoneA.center());
  pointAt(editor.viewport(), centre);
  rightClickThroughWindow(zw.window, editor.viewport(), centre);
  QTRY_VERIFY(visibleMenuOf(editor) != nullptr);

  // Dismissed without a click: the interaction, and with it the menu, is gone well before the 200 ms the removed
  // deferral used to wait.
  visibleMenuOf(editor)->hide();
  QTRY_COMPARE_WITH_TIMEOUT(editor.findChildren<QMenu*>().size(), 0, 100);
  QCOMPARE(zoneCount(editor), 1);
}

void TestMacQt::testRightClickOpensZoneMenu() {
  {
    ZoneWindow zw(false);
    output::PictureZoneEditor& editor = *zw.editor;
    addRectZone(editor, kZoneA);
    QCOMPARE(zoneCount(editor), 1);

    const QPoint centre = viewportPointOf(editor, kZoneA.center());
    pointAt(editor.viewport(), centre);
    rightClickThroughWindow(zw.window, editor.viewport(), centre);

    QTRY_VERIFY(visibleMenuOf(editor) != nullptr);
    QMenu* const menu = visibleMenuOf(editor);
    const QStringList texts = actionTexts(*menu);
    QVERIFY(texts.contains(QStringLiteral("Properties")));
    QVERIFY(texts.contains(QStringLiteral("Delete")));
    menu->hide();
    // The interaction restores the default one after a deferred check.
    QTest::qWait(300);
    QCOMPARE(zoneCount(editor), 1);
  }

  {
    ContentWindow cw{QRectF(150, 100, 500, 400)};
    select_content::ImageView& view = *cw.view;
    const QPoint centre = view.rect().center();
    pointAt(view.viewport(), centre);
    rightClickThroughWindow(cw.window, view.viewport(), centre);

    QTRY_VERIFY(visibleMenuOf(view) != nullptr);
    QMenu* const menu = visibleMenuOf(view);
    QVERIFY(actionTexts(*menu).contains(QStringLiteral("Remove Content Box")));
    menu->hide();
  }
}
