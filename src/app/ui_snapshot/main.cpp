// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
//
// Offscreen snapshot of the main window and the Settings dialog for Phase 3 (D-12).
// usage: ui_snapshot <output-dir> [--assert-no-language]
// The output directory must lie below the checkout's build/ directory. QSettings and temporary files are kept
// inside it, so a run never touches the user's real settings or profiles.

#include <core/Application.h>
#include <core/ApplicationSettings.h>
#include <core/ColorSchemeFactory.h>
#include <core/ColorSchemeManager.h>
#include <core/IconProvider.h>
#include <core/StyledIconPack.h>

#include <QAbstractButton>
#include <QAction>
#include <QByteArray>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGroupBox>
#include <QLabel>
#include <QPixmap>
#include <QSettings>
#include <QString>
#include <QTextStream>
#include <cstdio>
#include <memory>

#include "MainWindow.h"
#include "SettingsDialog.h"

namespace {

void printLine(const char* prefix, const QString& text) {
  std::printf("%s%s\n", prefix, text.toLocal8Bit().constData());
}

bool savePng(const QPixmap& pixmap, const QString& path) {
  if (!pixmap.save(path, "PNG")) {
    std::fprintf(stderr, "ui_snapshot: cannot write %s\n", path.toLocal8Bit().constData());
    return false;
  }
  printLine("wrote ", path);
  return true;
}

bool writeTexts(const QString& path, const MainWindow& window, const SettingsDialog& dialog) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    std::fprintf(stderr, "ui_snapshot: cannot write %s\n", path.toLocal8Bit().constData());
    return false;
  }
  QTextStream out(&file);

  for (const QLabel* label : dialog.findChildren<QLabel*>()) {
    out << "label: " << label->text() << "\n";
  }
  for (const QAbstractButton* button : dialog.findChildren<QAbstractButton*>()) {
    out << "button: " << button->text() << "\n";
  }
  for (const QGroupBox* group : dialog.findChildren<QGroupBox*>()) {
    out << "group: " << group->title() << "\n";
  }
  for (const QComboBox* combo : dialog.findChildren<QComboBox*>()) {
    for (int i = 0; i < combo->count(); ++i) {
      out << "combo " << combo->objectName() << ": " << combo->itemText(i) << "\n";
    }
  }
  for (const QAction* action : window.findChildren<QAction*>()) {
    if (!action->text().isEmpty()) {
      out << "action: " << action->text() << "\n";
    }
  }
  return true;
}

bool hasLanguageControl(const SettingsDialog& dialog) {
  if (dialog.findChild<QWidget*>("languageBox") || dialog.findChild<QWidget*>("languageLabel")) {
    return true;
  }
  for (const QLabel* label : dialog.findChildren<QLabel*>()) {
    if (label->text().contains("Language")) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main(int argc, char* argv[]) {
  QString outArg;
  bool assertNoLanguage = false;
  int positional = 0;
  for (int i = 1; i < argc; ++i) {
    const QString arg = QString::fromLocal8Bit(argv[i]);
    if (arg == "--assert-no-language") {
      assertNoLanguage = true;
    } else if (positional == 0) {
      outArg = arg;
      ++positional;
    } else {
      positional = 2;
    }
  }
  if (positional != 1) {
    std::fprintf(stderr, "usage: ui_snapshot <output-dir> [--assert-no-language]\n");
    return 2;
  }

  const QString root = QDir::cleanPath(QString::fromUtf8(UI_SNAPSHOT_ROOT));
  const QString out = QDir::cleanPath(QDir::current().absoluteFilePath(outArg));
  if (out != root && !out.startsWith(root + "/")) {
    std::fprintf(stderr, "ui_snapshot: %s is not under %s\n", out.toLocal8Bit().constData(),
                 root.toLocal8Bit().constData());
    return 2;
  }
  if (!QDir().mkpath(out + "/tmp") || !QDir().mkpath(out + "/settings")) {
    std::fprintf(stderr, "ui_snapshot: cannot create %s\n", out.toLocal8Bit().constData());
    return 2;
  }

  // Everything below is set before the application object exists, so no setting is read from the user's account.
  qputenv("QT_QPA_PLATFORM", "offscreen");
  qputenv("LC_ALL", "en_US.UTF-8");
  qputenv("TMPDIR", (out + "/tmp").toUtf8());

  QCoreApplication::setOrganizationName("scantailor-tests");
  QCoreApplication::setApplicationName("ui_snapshot");
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, out + "/settings");

  Application app(argc, argv);

  const QString settingsFile = QSettings().fileName();
  if (!settingsFile.startsWith(out + "/settings/")) {
    std::fprintf(stderr, "ui_snapshot: settings file %s is not under %s/settings\n",
                 settingsFile.toLocal8Bit().constData(), out.toLocal8Bit().constData());
    return 2;
  }
  printLine("settings file: ", settingsFile);

  {
    std::unique_ptr<ColorScheme> scheme
        = ColorSchemeFactory().create(ApplicationSettings::getInstance().getColorScheme());
    ColorSchemeManager::instance().setColorScheme(*scheme);
  }
  IconProvider::getInstance().setIconPack(StyledIconPack::createDefault());

  MainWindow window;
  window.resize(1280, 800);
  window.show();
  QApplication::processEvents();
  if (!savePng(window.grab(), out + "/main-window.png")) {
    return 2;
  }

  SettingsDialog dialog(&window);
  dialog.show();
  QApplication::processEvents();
  if (!savePng(dialog.grab(), out + "/settings-dialog.png")) {
    return 2;
  }

  const QString textsPath = out + "/texts.txt";
  if (!writeTexts(textsPath, window, dialog)) {
    return 2;
  }
  printLine("wrote ", textsPath);

  const bool languagePresent = hasLanguageControl(dialog);
  std::printf("language control present: %s\n", languagePresent ? "yes" : "no");
  return (assertNoLanguage && languagePresent) ? 1 : 0;
}
