// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_core_qt.h"
#include "tst_hidpi_qt.h"
#include "tst_mac_qt.h"
#include "tst_ux_qt.h"

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
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

  // Two offscreen screens with device pixel ratio 1 and 2 for the Retina tests; a test aid only (D-00c). The
  // per-screen factors set the window ratio; the "dpr" key of the JSON file would not.
  const QString screensFile = tmpRoot + "/offscreen-screens.json";
  {
    const QByteArray screens = R"({"screens":[{"name":"OneX","x":0,"y":0,"width":800,"height":600},)"
                               R"({"name":"TwoX","x":800,"y":0,"width":1600,"height":1200}]})";
    QFile file(screensFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(screens) != screens.size()) {
      std::fprintf(stderr, "qt_tests: cannot write %s\n", screensFile.toLocal8Bit().constData());
      return 2;
    }
  }
  qputenv("QT_QPA_PLATFORM", ("offscreen:configfile=" + screensFile).toUtf8());
  qputenv("QT_SCREEN_SCALE_FACTORS", "OneX=1;TwoX=2");
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

  TestHiDpiQt tc3;
  status |= QTest::qExec(&tc3, argc, argv);

  TestMacQt tc4;
  status |= QTest::qExec(&tc4, argc, argv);

  return status;
}
