// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#define BOOST_TEST_MODULE pipeline_tests

#include <Application.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QString>
#include <QTemporaryDir>
#include <boost/test/unit_test.hpp>
#include <cstdio>
#include <cstdlib>
#include <memory>

namespace {

/**
 * Creates the core Application (not a plain QApplication: DefaultParamsProfileManager casts qApp to it)
 * offscreen, with settings and temporary files redirected into the build tree.  Everything the tests
 * need from the environment is set here with qputenv before the application object exists, so a direct
 * run of the binary behaves like a ctest run.
 */
class GlobalFixture {
 public:
  GlobalFixture() {
    const QString tmpRoot = QString::fromUtf8(TEST_TMP_ROOT);
    QDir().mkpath(tmpRoot);

    qputenv("TMPDIR", tmpRoot.toUtf8());
    qputenv("QT_QPA_PLATFORM", "offscreen");
    qputenv("LC_ALL", "en_US.UTF-8");
    qputenv("QT_LOGGING_RULES", "qt.svg=false");

    m_tempDir = std::make_unique<QTemporaryDir>(tmpRoot + "/pipeline_tests-XXXXXX");
    if (!m_tempDir->isValid()) {
      std::fprintf(stderr, "pipeline_tests: cannot create a temporary directory under %s\n", TEST_TMP_ROOT);
      std::abort();
    }

    QCoreApplication::setOrganizationName("scantailor-tests");
    QCoreApplication::setApplicationName("pipeline_tests");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_tempDir->path() + "/settings");

    static int argc = 1;
    static char arg0[] = "pipeline_tests";
    static char* argv[] = {arg0, nullptr};
    m_app = std::make_unique<Application>(argc, argv);

    // Nothing may touch ApplicationSettings before this check has passed.
    const QString settingsFile = QSettings().fileName();
    if (!settingsFile.startsWith(m_tempDir->path())) {
      std::fprintf(stderr, "pipeline_tests: settings file %s is not under the temporary directory %s\n",
                   settingsFile.toLocal8Bit().constData(), m_tempDir->path().toLocal8Bit().constData());
      std::abort();
    }
  }

 private:
  // The application object must go before the temporary directory: members are destroyed in reverse order.
  std::unique_ptr<QTemporaryDir> m_tempDir;
  std::unique_ptr<Application> m_app;
};

BOOST_GLOBAL_FIXTURE(GlobalFixture);

}  // namespace
