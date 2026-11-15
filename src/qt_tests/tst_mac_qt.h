// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef TST_MAC_QT_H
#define TST_MAC_QT_H

#include <QObject>

/**
 * Offscreen checks of the Mac menus and shortcuts (Phase 5): a real MainWindow is built, shown and driven with
 * synthesised key events. The offscreen platform has no native menu bar, so these tests cover roles, texts, key
 * sequences and enabled states; the live Mac check covers the native application menu.
 */
class TestMacQt : public QObject {
  Q_OBJECT
 private slots:
  void initTestCase();
  void testSettingsInFileWithPreferencesRole();
  void testSettingsShortcutOpensSettings();
  void testMenuRoles();
  void testFileMenuTextsAndKeys();
  void testToolsMenuWithoutSettings();
  void testQuitClosesWindow();
  void testProjectActionsDisabledWithoutProject();
  void testStageActions();
  void testViewMenuStages();
  void testOutputTabKeys();
  void testZoomActions();
  void testZoomDisabledWithoutView();
  void testThumbnailKeys();
  void testViewMenuContents();
  void testZoneRemoveKeys();
  void testZoneKeysIgnoredInTextFields();
  void testZoneStatusTips();
};

#endif
