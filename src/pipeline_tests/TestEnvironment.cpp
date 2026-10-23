// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <DefaultParamsProvider.h>

#include <QByteArray>
#include <QDir>
#include <QGuiApplication>
#include <QSettings>
#include <QString>
#include <boost/test/unit_test.hpp>
#include <cstdlib>

BOOST_AUTO_TEST_SUITE(EnvironmentTestSuite)

BOOST_AUTO_TEST_CASE(test_settings_and_temp_are_isolated) {
  const QString root = QString::fromUtf8(TEST_TMP_ROOT);

  const QString settingsFile = QSettings().fileName();
  BOOST_CHECK_MESSAGE(settingsFile.startsWith(root),
                      "settings file " << settingsFile.toStdString() << " is not under " << root.toStdString());

  const QString tempPath = QDir::tempPath();
  BOOST_CHECK_MESSAGE(tempPath.startsWith(root),
                      "temporary path " << tempPath.toStdString() << " is not under " << root.toStdString());

  BOOST_CHECK_EQUAL(DefaultParamsProvider::getInstance().getProfileName().toStdString(), "Default");

  // The one place that calls the C parser on purpose: it proves the locale pin.  Everything else parses
  // numbers with Qt.
  BOOST_CHECK_EQUAL(QByteArray("1.5").toDouble(), 1.5);
  BOOST_CHECK_EQUAL(std::strtod("1.5", nullptr), 1.5);

  BOOST_CHECK_EQUAL(QGuiApplication::platformName().toStdString(), "offscreen");
}

BOOST_AUTO_TEST_SUITE_END()
