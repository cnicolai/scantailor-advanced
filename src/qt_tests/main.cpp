// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_core_qt.h"
#include "tst_ux_qt.h"

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QString>
#include <QtTest/QtTest>
#include <cstdio>

int main(int argc, char* argv[]) {
  // Everything the tests need from the environment is set before the application object exists, so that a
  // direct run behaves like a ctest run: offscreen, a fixed locale, and settings and temporary files inside the
  // build tree (the widgets under test read and write QSettings with the default constructor).
  const QString tmpRoot = QString::fromUtf8(TEST_TMP_ROOT);
  QDir().mkpath(tmpRoot);
  qputenv("QT_QPA_PLATFORM", "offscreen");
  qputenv("LC_ALL", "en_US.UTF-8");
  qputenv("TMPDIR", tmpRoot.toUtf8());

  QCoreApplication::setOrganizationName("scantailor-tests");
  QCoreApplication::setApplicationName("qt_tests");
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, tmpRoot + "/qt_tests-settings");

  QApplication app(argc, argv);

  // Nothing may read or write settings before this check has passed.
  const QString settingsFile = QSettings().fileName();
  if (!settingsFile.startsWith(tmpRoot)) {
    std::fprintf(stderr, "qt_tests: settings file %s is not under %s\n", settingsFile.toLocal8Bit().constData(),
                 tmpRoot.toLocal8Bit().constData());
    return 2;
  }

  int status = 0;

  TestCoreQt tc1;
  status |= QTest::qExec(&tc1, argc, argv);

  TestUxQt tc2;
  status |= QTest::qExec(&tc2, argc, argv);

  return status;
}
