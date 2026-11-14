// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_mac_qt.h"

#include <AbstractFilter.h>
#include <ApplicationSettings.h>
#include <BasicImageView.h>
#include <ColorSchemeFactory.h>
#include <ColorSchemeManager.h>
#include <IconProvider.h>
#include <PageId.h>
#include <PageRange.h>
#include <PageSelectionAccessor.h>
#include <PageSelectionProvider.h>
#include <PageSequence.h>
#include <ProjectPages.h>
#include <StageSequence.h>
#include <StyledIconPack.h>
#include <filters/output/ImageViewTab.h>
#include <filters/output/TabbedImageView.h>

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
#include <QSignalSpy>
#include <QStackedLayout>
#include <QStatusBar>
#include <QString>
#include <QStringList>
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
