// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_mac_qt.h"

#include <ApplicationSettings.h>
#include <ColorSchemeFactory.h>
#include <ColorSchemeManager.h>
#include <IconProvider.h>
#include <StyledIconPack.h>

#include "MainWindow.h"
#include "SettingsDialog.h"

#include <QAction>
#include <QGuiApplication>
#include <QKeySequence>
#include <QMenu>
#include <QString>
#include <QWidget>
#include <QtTest/QtTest>
#include <memory>

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
