// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_mac_qt.h"

#include <ApplicationSettings.h>
#include <ColorSchemeFactory.h>
#include <ColorSchemeManager.h>
#include <IconProvider.h>
#include <StyledIconPack.h>

#include <config.h>

#include "MainWindow.h"
#include "SettingsDialog.h"

#include <QAction>
#include <QGuiApplication>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QtTest/QtTest>
#include <memory>
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

  const QStringList expectedEntries{utf8("New Project…"), utf8("Open Project…"),    "--",
                                    "Save Project",       utf8("Save Project As…"), "--",
                                    "Close Project",      utf8("Settings…"),        "Quit"};
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
