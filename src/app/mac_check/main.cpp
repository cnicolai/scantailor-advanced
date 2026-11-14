// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
//
// Live Mac check for Phase 5 (D-12): shows the real MainWindow on the default Cocoa platform and reads the native
// menu bar (application menu, Help menu, application name), sends synthesised key events to a probe window to find
// which chords fire on the German keyboard layout, and measures the app's storage identity (names and paths) the way
// src/app/main.cpp sets it up.
// usage: mac_check <output-dir> [--section menubar|layout|all, comma-separated | --section identity]
// The output directory must lie below the checkout's build/ directory. QSettings and temporary files are kept inside
// it, so a run never touches the user's real settings. The section identity runs alone and needs HOME and
// CFFIXED_USER_HOME set to <output-dir>/home.
// Exit codes: 0 pass, 1 fail, 2 usage, path or home not redirected, 3 not Cocoa.
// All input is synthesised inside this process and sent to its own windows; nothing is physically pressed and no other
// application receives an event.

#include <config.h>
#include <core/Application.h>
#include <core/ApplicationSettings.h>
#include <core/ColorSchemeFactory.h>
#include <core/ColorSchemeManager.h>
#include <core/IconProvider.h>
#include <core/StyledIconPack.h>
#include <pwd.h>
#include <unistd.h>

#include <QAction>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QKeySequence>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QThread>
#include <QWidget>
#include <QWindow>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

#include "CocoaProbe.h"
#include "MainWindow.h"

namespace {

const char* const kUsage
    = "usage: mac_check <output-dir> [--section menubar|layout|all, comma-separated | --section identity]\n";

// Collects the "key: value" result lines, echoes them to stdout and writes them to summary.txt.
class Summary {
 public:
  void add(const QString& key, const QString& value) {
    const QString line = key + ": " + value;
    m_lines.push_back(line);
    std::printf("%s\n", line.toLocal8Bit().constData());
    std::fflush(stdout);
  }

  bool save(const QString& path) const {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      return false;
    }
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    for (const QString& line : m_lines) {
      out << line << "\n";
    }
    return true;
  }

 private:
  QStringList m_lines;
};

QString yesNo(const bool value) {
  return value ? "yes" : "no";
}

QString orNone(const QString& value) {
  return value.isEmpty() ? QString("none") : value;
}

// Processes events until the condition holds or the limit in milliseconds has passed.
template <typename Condition>
bool waitFor(const Condition& condition, const int limitMs) {
  QElapsedTimer timer;
  timer.start();
  while (!condition()) {
    if (timer.elapsed() > limitMs) {
      return false;
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    QThread::msleep(10);
  }
  return true;
}

// Processes events for the given time.
void pump(const int ms) {
  QElapsedTimer timer;
  timer.start();
  while (timer.elapsed() < ms) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    QThread::msleep(5);
  }
}

QString accountHome() {
  const struct passwd* const entry = getpwuid(getuid());
  return entry != nullptr ? QDir::cleanPath(QString::fromLocal8Bit(entry->pw_dir)) : QString();
}

// Writes a path into a summary line: the output directory becomes {out}, then the account's home directory ~.
QString shown(QString path, const QString& out) {
  path.replace(out, "{out}");
  const QString home = accountHome();
  if (!home.isEmpty()) {
    path.replace(home, "~");
  }
  return path;
}

bool isBelow(const QString& path, const QString& root) {
  const QString cleaned = QDir::cleanPath(path);
  return !cleaned.isEmpty() && (cleaned == root || cleaned.startsWith(root + "/"));
}

QString stripAmpersand(QString text) {
  text.remove('&');
  return text;
}

QString modifierGlyphs(const unsigned modifiers) {
  QString glyphs;
  if ((modifiers & cocoaprobe::Control) != 0) {
    glyphs += QString::fromUtf8("\xE2\x8C\x83");  // control
  }
  if ((modifiers & cocoaprobe::Option) != 0) {
    glyphs += QString::fromUtf8("\xE2\x8C\xA5");  // option
  }
  if ((modifiers & cocoaprobe::Shift) != 0) {
    glyphs += QString::fromUtf8("\xE2\x87\xA7");  // shift
  }
  if ((modifiers & cocoaprobe::Command) != 0) {
    glyphs += QString::fromUtf8("\xE2\x8C\x98");  // command
  }
  return glyphs;
}

QString keyText(const cocoaprobe::MenuItemInfo& item) {
  return modifierGlyphs(item.modifiers) + item.keyEquivalent;
}

bool isRoleAction(const QAction& action) {
  const QAction::MenuRole role = action.menuRole();
  return role == QAction::AboutRole || role == QAction::PreferencesRole || role == QAction::QuitRole;
}

// Section menubar -----------------------------------------------------------------------------------------------

bool runMenubarSection(Summary& summary) {
  MainWindow window;
  window.resize(1280, 800);
  window.show();
  window.activateWindow();
  window.raise();
  cocoaprobe::activate(window.windowHandle());
  const bool focused = waitFor([&window] { return QGuiApplication::focusWindow() == window.windowHandle(); }, 5000);
  pump(500);
  const bool appActive = cocoaprobe::activate(window.windowHandle()) && focused;
  summary.add("app_active", yesNo(appActive));
  summary.add("window_dpr", QString::number(window.devicePixelRatioF()));
  summary.add("localized_name", orNone(cocoaprobe::localizedAppName()));
  summary.add("bundle_name", orNone(cocoaprobe::bundleName()));

  const std::vector<cocoaprobe::MenuInfo> menus = cocoaprobe::mainMenu();

  // The native top-level menus after the application menu against the Qt menus that keep a visible item once the
  // role items have moved to the application menu.
  QStringList nativeTopLevel;
  for (size_t i = 1; i < menus.size(); ++i) {
    nativeTopLevel << menus[i].title;
  }
  QStringList qtTopLevel;
  for (const QAction* const barAction : window.menuBar()->actions()) {
    const QMenu* const menu = barAction->menu();
    if (menu == nullptr) {
      continue;
    }
    bool keepsItem = false;
    for (const QAction* const action : menu->actions()) {
      if (action->isVisible() && !action->isSeparator() && !isRoleAction(*action)) {
        keepsItem = true;
      }
    }
    if (keepsItem) {
      qtTopLevel << stripAmpersand(menu->title());
    }
  }
  summary.add("menubar_top_level", nativeTopLevel.join(" | "));
  summary.add("qt_top_level", qtTopLevel.join(" | "));

  QStringList appMenuItems;
  QStringList aboutTitles;
  QStringList settingsTitles;
  QStringList quitTitles;
  QString aboutTitle;
  QString settingsTitle;
  QString quitTitle;
  QString settingsKey;
  QString quitKey;
  bool settingsEnabled = false;
  if (!menus.empty()) {
    for (const cocoaprobe::MenuItemInfo& item : menus[0].items) {
      if (item.separator || item.hidden) {
        continue;
      }
      appMenuItems << item.title;
      if (item.title.startsWith("About")) {
        aboutTitles << item.title;
      } else if (item.title.startsWith("Settings") || item.title.startsWith("Preferences")) {
        settingsTitles << item.title;
        if (settingsKey.isEmpty()) {
          settingsKey = keyText(item);
          settingsEnabled = item.enabled;
        }
      } else if (item.title.startsWith("Quit")) {
        quitTitles << item.title;
        if (quitKey.isEmpty()) {
          quitKey = keyText(item);
        }
      }
    }
  }
  aboutTitle = aboutTitles.value(0);
  settingsTitle = settingsTitles.value(0);
  quitTitle = quitTitles.value(0);
  summary.add("app_menu_items", appMenuItems.join(" | "));
  summary.add("about_item_title", orNone(aboutTitle));
  summary.add("settings_item_title", orNone(settingsTitle));
  summary.add("quit_item_title", orNone(quitTitle));
  summary.add("settings_item_key", orNone(settingsKey));
  summary.add("quit_item_key", orNone(quitKey));
  summary.add("settings_item_enabled", yesNo(settingsEnabled));
  summary.add("about_items", QString::number(aboutTitles.size()));
  summary.add("settings_items", QString::number(settingsTitles.size()));
  summary.add("quit_items", QString::number(quitTitles.size()));

  // Visible items of the other native menus that repeat one of the three role actions.
  const QAction* const aboutAction = window.findChild<QAction*>("actionAbout");
  const QAction* const settingsAction = window.findChild<QAction*>("actionSettings");
  const QAction* const quitAction = window.findChild<QAction*>("actionQuit");
  QStringList roleTexts;
  for (const QAction* const action : {aboutAction, settingsAction, quitAction}) {
    if (action != nullptr) {
      roleTexts << stripAmpersand(action->text());
    }
  }
  int outside = 0;
  bool helpShown = false;
  for (size_t i = 1; i < menus.size(); ++i) {
    if (menus[i].title == "Help") {
      helpShown = true;
    }
    for (const cocoaprobe::MenuItemInfo& item : menus[i].items) {
      if (!item.hidden && !item.separator && roleTexts.contains(stripAmpersand(item.title))) {
        ++outside;
      }
    }
  }
  summary.add("role_items_outside_app_menu", QString::number(outside));
  summary.add("help_menu_shown", yesNo(helpShown));

  const bool standardKeys
      = QKeySequence::keyBindings(QKeySequence::Preferences).contains(QKeySequence(Qt::CTRL | Qt::Key_Comma))
        && QKeySequence::keyBindings(QKeySequence::Quit).contains(QKeySequence(Qt::CTRL | Qt::Key_Q))
        && QKeySequence::keyBindings(QKeySequence::SaveAs).contains(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
  summary.add("standard_keys_match", yesNo(standardKeys));

  // One line per native menu, hidden items in brackets, for the record.
  for (size_t i = 0; i < menus.size(); ++i) {
    QStringList entries;
    for (const cocoaprobe::MenuItemInfo& item : menus[i].items) {
      if (item.separator) {
        entries << "--";
        continue;
      }
      QString entry = item.title;
      if (!item.keyEquivalent.isEmpty()) {
        entry += " (" + keyText(item) + ")";
      }
      if (item.hidden) {
        entry = "[" + entry + "]";
      }
      entries << entry;
    }
    summary.add(QString("native_menu_%1").arg(i), menus[i].title + ": " + entries.join(" | "));
  }

  const bool pass = appActive && nativeTopLevel == qtTopLevel && aboutTitles.size() == 1 && settingsTitles.size() == 1
                    && quitTitles.size() == 1 && settingsKey == QString::fromUtf8("\xE2\x8C\x98,")
                    && quitKey == QString::fromUtf8("\xE2\x8C\x98q") && settingsEnabled && outside == 0 && standardKeys;
  summary.add("menubar_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

// Section layout ------------------------------------------------------------------------------------------------

struct Chord {
  QString name;
  QKeySequence sequence;
  QChar character;  // the key to look up in the layout; unused for a special key
  bool special = false;
  cocoaprobe::SpecialKey specialKey = cocoaprobe::SpecialKey::Backspace;
  unsigned modifiers = 0;  // cocoaprobe::Modifier bits; Shift is added for the equal key when the layout needs it
  bool required = false;   // part of the layout verdict
};

Chord characterChord(const QString& name,
                     const QKeySequence& sequence,
                     const QChar character,
                     const unsigned modifiers,
                     const bool required) {
  Chord chord;
  chord.name = name;
  chord.sequence = sequence;
  chord.character = character;
  chord.modifiers = modifiers;
  chord.required = required;
  return chord;
}

Chord specialChord(const QString& name,
                   const QKeySequence& sequence,
                   const cocoaprobe::SpecialKey key,
                   const bool required) {
  Chord chord;
  chord.name = name;
  chord.sequence = sequence;
  chord.special = true;
  chord.specialKey = key;
  chord.required = required;
  return chord;
}

std::vector<Chord> makeChords() {
  const unsigned cmd = cocoaprobe::Command;
  const unsigned optCmd = cocoaprobe::Command | cocoaprobe::Option;
  const unsigned ctrlCmd = cocoaprobe::Command | cocoaprobe::Control;
  std::vector<Chord> chords;
  for (int digit = 1; digit <= 6; ++digit) {
    const Qt::Key key = static_cast<Qt::Key>(Qt::Key_1 + digit - 1);
    const QChar character = QChar('0' + digit);
    chords.push_back(characterChord(QString("cmd_%1").arg(digit), QKeySequence(Qt::CTRL | key), character, cmd, true));
  }
  for (int digit = 1; digit <= 5; ++digit) {
    const Qt::Key key = static_cast<Qt::Key>(Qt::Key_1 + digit - 1);
    const QChar character = QChar('0' + digit);
    chords.push_back(characterChord(QString("opt_cmd_%1").arg(digit), QKeySequence(Qt::CTRL | Qt::ALT | key), character,
                                    optCmd, false));
  }
  for (int digit = 1; digit <= 5; ++digit) {
    const Qt::Key key = static_cast<Qt::Key>(Qt::Key_1 + digit - 1);
    const QChar character = QChar('0' + digit);
    chords.push_back(characterChord(QString("ctrl_cmd_%1").arg(digit), QKeySequence(Qt::CTRL | Qt::META | key),
                                    character, ctrlCmd, false));
  }
  chords.push_back(characterChord("cmd_plus", QKeySequence(Qt::CTRL | Qt::Key_Plus), '+', cmd, true));
  chords.push_back(characterChord("cmd_minus", QKeySequence(Qt::CTRL | Qt::Key_Minus), '-', cmd, true));
  chords.push_back(characterChord("cmd_equal", QKeySequence(Qt::CTRL | Qt::Key_Equal), '=', cmd, false));
  chords.push_back(characterChord("opt_cmd_plus", QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Plus), '+', optCmd, false));
  chords.push_back(
      characterChord("opt_cmd_minus", QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Minus), '-', optCmd, false));
  chords.push_back(
      characterChord("opt_cmd_equal", QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Equal), '=', optCmd, false));
  chords.push_back(
      characterChord("ctrl_cmd_plus", QKeySequence(Qt::CTRL | Qt::META | Qt::Key_Plus), '+', ctrlCmd, false));
  chords.push_back(
      characterChord("ctrl_cmd_minus", QKeySequence(Qt::CTRL | Qt::META | Qt::Key_Minus), '-', ctrlCmd, false));
  chords.push_back(specialChord("backspace", QKeySequence(Qt::Key_Backspace), cocoaprobe::SpecialKey::Backspace, true));
  chords.push_back(
      specialChord("forward_delete", QKeySequence(Qt::Key_Delete), cocoaprobe::SpecialKey::ForwardDelete, true));
  chords.push_back(characterChord("b", QKeySequence(Qt::Key_B), 'b', cocoaprobe::NoModifier, true));
  return chords;
}

QString codeText(const bool found, const int code) {
  return found ? QString::number(code) : QString("none");
}

bool runLayoutSection(Summary& summary) {
  const std::vector<Chord> chords = makeChords();

  QWidget probe;
  probe.setFocusPolicy(Qt::StrongFocus);
  probe.resize(300, 200);
  std::vector<int> counts(chords.size(), 0);
  for (size_t i = 0; i < chords.size(); ++i) {
    auto* const action = new QAction(&probe);
    action->setShortcut(chords[i].sequence);
    action->setShortcutContext(Qt::WindowShortcut);
    probe.addAction(action);
    int* const counter = &counts[i];
    QObject::connect(action, &QAction::triggered, &probe, [counter] { ++*counter; });
  }
  probe.show();
  probe.activateWindow();
  probe.raise();
  probe.setFocus();
  cocoaprobe::activate(probe.windowHandle());
  waitFor([&probe] { return QGuiApplication::focusWindow() == probe.windowHandle(); }, 5000);
  pump(300);

  const QString layoutId = cocoaprobe::keyboardLayoutId();

  // Key codes of the layout, looked up with UCKeyTranslate.
  int code = 0;
  for (int digit = 1; digit <= 6; ++digit) {
    const bool found = cocoaprobe::keyCodeFor(QChar('0' + digit), false, &code);
    summary.add(QString("keycode_%1").arg(digit), codeText(found, code));
  }
  summary.add("keycode_plus", codeText(cocoaprobe::keyCodeFor('+', false, &code), code));
  summary.add("keycode_minus", codeText(cocoaprobe::keyCodeFor('-', false, &code), code));
  int equalCode = 0;
  bool equalShift = false;
  const bool equalFound = cocoaprobe::keyCodeFor('=', false, &equalCode)
                          || (equalShift = true, cocoaprobe::keyCodeFor('=', true, &equalCode));
  summary.add("keycode_equal", codeText(equalFound, equalCode));
  summary.add("keycode_equal_shift", yesNo(equalFound && equalShift));
  summary.add("keycode_b", codeText(cocoaprobe::keyCodeFor('b', false, &code), code));

  int crossFires = 0;
  int crossFiresRequired = 0;
  bool requiredOk = true;
  std::vector<int> fired(chords.size(), 0);
  for (size_t i = 0; i < chords.size(); ++i) {
    const Chord& chord = chords[i];
    summary.add("sequence_" + chord.name, chord.sequence.toString(QKeySequence::NativeText));
    int keyCode = 0;
    unsigned modifiers = chord.modifiers;
    bool haveKey = true;
    if (chord.special) {
      keyCode = cocoaprobe::specialKeyCode(chord.specialKey);
    } else if (chord.character == QChar('=')) {
      haveKey = equalFound;
      keyCode = equalCode;
      if (equalShift) {
        modifiers |= cocoaprobe::Shift;
      }
    } else {
      haveKey = cocoaprobe::keyCodeFor(chord.character, false, &keyCode);
    }

    std::fill(counts.begin(), counts.end(), 0);
    bool sent = false;
    if (haveKey) {
      sent = cocoaprobe::sendKey(probe.windowHandle(), keyCode, modifiers);
      pump(300);
    }
    int own = 0;
    int others = 0;
    for (size_t j = 0; j < counts.size(); ++j) {
      (j == i ? own : others) += counts[j];
    }
    fired[i] = own;
    crossFires += others;
    if (chord.required) {
      crossFiresRequired += others;
      if (own != 1) {
        requiredOk = false;
      }
    }
    summary.add("chord_" + chord.name, QString::number(own));
    if (!haveKey || !sent) {
      summary.add("chord_" + chord.name + "_note", haveKey ? "event not sent" : "no key code in this layout");
    }
    if (others != 0) {
      summary.add("chord_" + chord.name + "_cross", QString::number(others));
    }
  }
  summary.add("cross_fires", QString::number(crossFires));
  summary.add("cross_fires_required", QString::number(crossFiresRequired));

  auto countOf = [&chords, &fired](const QString& name) {
    for (size_t i = 0; i < chords.size(); ++i) {
      if (chords[i].name == name) {
        return fired[i];
      }
    }
    return 0;
  };
  auto allOne = [&countOf](const QStringList& names) {
    for (const QString& name : names) {
      if (countOf(name) != 1) {
        return false;
      }
    }
    return true;
  };
  const bool optionDigits = allOne({"opt_cmd_1", "opt_cmd_2", "opt_cmd_3", "opt_cmd_4", "opt_cmd_5"});
  const bool optionPlusMinus = allOne({"opt_cmd_plus", "opt_cmd_minus"});
  const bool controlDigits = allOne({"ctrl_cmd_1", "ctrl_cmd_2", "ctrl_cmd_3", "ctrl_cmd_4", "ctrl_cmd_5"});
  const bool controlPlusMinus = allOne({"ctrl_cmd_plus", "ctrl_cmd_minus"});
  auto works = [](const bool value) { return value ? QString("works") : QString("fails"); };
  summary.add("option_command_digits", works(optionDigits));
  summary.add("option_command_plus_minus", works(optionPlusMinus));
  summary.add("control_command_digits", works(controlDigits));
  summary.add("control_command_plus_minus", works(controlPlusMinus));
  auto group = [](const bool option, const bool control) {
    return option ? QString("option-command") : control ? QString("control-command") : QString("none");
  };
  const QString outputTabsGroup = group(optionDigits, controlDigits);
  const QString thumbnailsGroup = group(optionPlusMinus, controlPlusMinus);
  summary.add("output_tabs_group", outputTabsGroup);
  summary.add("thumbnails_group", thumbnailsGroup);
  summary.add("command_equal_note",
              QString("cmd_equal fired %1 time(s); on the German layout = is Shift-0, so Cmd-= is a US-layout binding "
                      "(D-06) and not part of the verdict")
                  .arg(countOf("cmd_equal")));

  const bool pass = layoutId.contains("German") && requiredOk && crossFiresRequired == 0 && outputTabsGroup != "none"
                    && thumbnailsGroup != "none";
  summary.add("layout_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

// Section identity ----------------------------------------------------------------------------------------------

// Repeats the order of src/app/main.cpp (application object, then the two names, then the INI format and a
// default-constructed QSettings, no path redirect) with HOME redirected below the output directory.
int runIdentitySection(const QString& out, int& argc, char** argv) {
  qunsetenv("XDG_CONFIG_HOME");
  const QString expectedHome = QDir::cleanPath(out + "/home");
  const QString fixedHome = QDir::cleanPath(QString::fromLocal8Bit(qgetenv("CFFIXED_USER_HOME")));
  if (!QDir(expectedHome).exists() || QDir::homePath() != expectedHome || fixedHome != expectedHome) {
    std::fprintf(stderr, "mac_check: identity needs HOME and CFFIXED_USER_HOME set to %s/home: home not redirected\n",
                 out.toLocal8Bit().constData());
    return 2;
  }

  Application app(argc, argv);
  if (QGuiApplication::platformName() != "cocoa") {
    std::fprintf(stderr, "mac_check: platform is %s, not cocoa\n",
                 QGuiApplication::platformName().toLocal8Bit().constData());
    return 3;
  }

  Summary summary;
  summary.add("platform", QGuiApplication::platformName());
  summary.add("qt_runtime", QString::fromLatin1(qVersion()));
  summary.add("section", "identity");
  summary.add("default_application_name", orNone(QCoreApplication::applicationName()));
  summary.add("default_organization_name", orNone(QCoreApplication::organizationName()));

  // The calls of src/app/main.cpp lines 25 to 28, in that order, with no path redirect.
  Application::setApplicationName(APPLICATION_NAME);
  Application::setOrganizationName(ORGANIZATION_NAME);
  QSettings::setDefaultFormat(QSettings::IniFormat);

  const QString settingsPath = QSettings().fileName();
  const QString profilesPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles";
  summary.add("application_name", orNone(QCoreApplication::applicationName()));
  summary.add("organization_name", orNone(QCoreApplication::organizationName()));
  summary.add("organization_domain", orNone(QCoreApplication::organizationDomain()));
  summary.add("app_settings_path", shown(settingsPath, out));
  summary.add("app_profiles_path", shown(profilesPath, out));
  summary.add("bundle_name", orNone(cocoaprobe::bundleName()));
  summary.add("home_directory", shown(cocoaprobe::homeDirectory(), out));
  const QString root = QDir::cleanPath(QString::fromUtf8(MAC_CHECK_ROOT));
  const bool redirected = isBelow(cocoaprobe::homeDirectory(), root) && isBelow(QDir::homePath(), root)
                          && isBelow(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), root);
  summary.add("home_redirected", yesNo(redirected));

  const bool settingsBelowHome = isBelow(settingsPath, expectedHome) && settingsPath != expectedHome;
  if (!settingsBelowHome) {
    summary.add("identity_reason", "settings path not redirected");
  }
  const bool pass = QGuiApplication::platformName() == "cocoa"
                    && QCoreApplication::applicationName() == QString(APPLICATION_NAME)
                    && QCoreApplication::organizationName() == QString(ORGANIZATION_NAME) && settingsBelowHome
                    && settingsPath.endsWith("/scantailor-advanced/scantailor-advanced.ini");
  summary.add("identity_verdict", pass ? "PASS" : "FAIL");
  summary.add("verdict", pass ? "PASS" : "FAIL");
  if (!summary.save(out + "/summary.txt")) {
    std::fprintf(stderr, "mac_check: cannot write %s/summary.txt\n", out.toLocal8Bit().constData());
    return 2;
  }
  return pass ? 0 : 1;
}

}  // namespace

int main(int argc, char* argv[]) {
  QString outArg;
  QString sectionArg = "menubar";
  bool badArguments = false;
  int positional = 0;
  for (int i = 1; i < argc; ++i) {
    const QString arg = QString::fromLocal8Bit(argv[i]);
    if (arg.startsWith("-Apple")) {
      ++i;  // AppKit's argument domain, e.g. -ApplePersistenceIgnoreState YES
    } else if (arg == "--section" && i + 1 < argc) {
      sectionArg = QString::fromLocal8Bit(argv[++i]);
    } else if (positional == 0 && !arg.startsWith("-")) {
      outArg = arg;
      ++positional;
    } else {
      badArguments = true;
    }
  }

  // The list of section names: known names only, "all" expands to every name except identity, identity runs alone.
  const QStringList knownSections = {"menubar", "layout", "identity", "all"};
  const QStringList requested = sectionArg.split(',');
  QStringList sections;
  for (const QString& name : requested) {
    if (!knownSections.contains(name)) {
      badArguments = true;
    } else if (name == "all") {
      sections << "menubar" << "layout";
    } else {
      sections << name;
    }
  }
  const bool identityOnly = requested == QStringList{"identity"};
  if (requested.contains("identity") && !identityOnly) {
    badArguments = true;
  }
  if (badArguments || positional != 1) {
    std::fprintf(stderr, "%s", kUsage);
    return 2;
  }

  const QString root = QDir::cleanPath(QString::fromUtf8(MAC_CHECK_ROOT));
  const QString out = QDir::cleanPath(QDir::current().absoluteFilePath(outArg));
  if (out != root && !out.startsWith(root + "/")) {
    std::fprintf(stderr, "mac_check: %s is not under %s\n", out.toLocal8Bit().constData(),
                 root.toLocal8Bit().constData());
    return 2;
  }
  if (!QDir().mkpath(out + "/tmp") || !QDir().mkpath(out + "/settings")) {
    std::fprintf(stderr, "mac_check: cannot create %s\n", out.toLocal8Bit().constData());
    return 2;
  }

  // Everything below is set before the application object exists, so no setting is read from the user's account and
  // the measurement sees bare Cocoa behaviour (D-00c).
  qunsetenv("QT_QPA_PLATFORM");
  qunsetenv("QT_SCALE_FACTOR");
  qunsetenv("QT_SCREEN_SCALE_FACTORS");
  qunsetenv("QT_ENABLE_HIGHDPI_SCALING");
  qunsetenv("QT_SCALE_FACTOR_ROUNDING_POLICY");
  qunsetenv("QT_USE_PHYSICAL_DPI");
  qputenv("LC_ALL", "en_US.UTF-8");
  qputenv("TMPDIR", (out + "/tmp").toUtf8());

  if (identityOnly) {
    return runIdentitySection(out, argc, argv);
  }

  QCoreApplication::setOrganizationName("scantailor-tests");
  QCoreApplication::setApplicationName("mac_check");
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, out + "/settings");

  Application app(argc, argv);

  const QString settingsFile = QSettings().fileName();
  if (!settingsFile.startsWith(out + "/settings/")) {
    std::fprintf(stderr, "mac_check: settings file %s is not under %s/settings\n",
                 settingsFile.toLocal8Bit().constData(), out.toLocal8Bit().constData());
    return 2;
  }
  if (QGuiApplication::platformName() != "cocoa") {
    std::fprintf(stderr, "mac_check: platform is %s, not cocoa\n",
                 QGuiApplication::platformName().toLocal8Bit().constData());
    return 3;
  }

  {
    std::unique_ptr<ColorScheme> scheme
        = ColorSchemeFactory().create(ApplicationSettings::getInstance().getColorScheme());
    ColorSchemeManager::instance().setColorScheme(*scheme);
  }
  IconProvider::getInstance().setIconPack(StyledIconPack::createDefault());

  Summary summary;
  summary.add("platform", QGuiApplication::platformName());
  summary.add("qt_runtime", QString::fromLatin1(qVersion()));
  summary.add("section", sectionArg);
  summary.add("keyboard_layout", orNone(cocoaprobe::keyboardLayoutId()));
  summary.add("home_directory", shown(cocoaprobe::homeDirectory(), out));
  const bool redirected = isBelow(cocoaprobe::homeDirectory(), root) && isBelow(QDir::homePath(), root)
                          && isBelow(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), root);
  summary.add("home_redirected", yesNo(redirected));
  summary.add("frontmost_app", orNone(cocoaprobe::frontmostAppName()));

  bool pass = true;
  if (sections.contains("menubar")) {
    pass = runMenubarSection(summary) && pass;
  }
  if (sections.contains("layout")) {
    pass = runLayoutSection(summary) && pass;
  }
  summary.add("verdict", pass ? "PASS" : "FAIL");
  if (!summary.save(out + "/summary.txt")) {
    std::fprintf(stderr, "mac_check: cannot write %s/summary.txt\n", out.toLocal8Bit().constData());
    return 2;
  }
  return pass ? 0 : 1;
}
