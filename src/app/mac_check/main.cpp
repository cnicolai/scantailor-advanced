// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
//
// Live Mac check: shows the real MainWindow on the default Cocoa platform and reads the native
// menu bar (application menu, Help menu, application name), sends synthesised key events to a probe window to find
// which chords fire on the German keyboard layout, and measures the app's storage identity (names and paths) the way
// src/app/main.cpp sets it up. The sections keys and zone press the window keys of the real MainWindow and the zone and
// content-box keys, and read the native View menu. The section context right-clicks and Control-clicks a zone, clicks
// the first menu item, dismisses the menu with Escape and measures the order of triggered() and aboutToHide.
// usage: mac_check <output-dir> [--section menubar|layout|keys|zone|context|dialogs|panels|all, comma-separated |
// --section identity]
// The output directory must lie below the checkout's build/ directory. QSettings and temporary files are kept inside
// it, so a run never touches the user's real settings. The section identity runs alone; it and the sections dialogs and
// panels (native file panels, profile listing) need HOME and CFFIXED_USER_HOME set to <output-dir>/home and refuse
// otherwise. The sections dialogs and panels also measure the menu bar during dialogs and prove the file panels native.
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

#include <BasicImageView.h>
#include <BinaryImage.h>
#include <ContentMask.h>
#include <Dpi.h>
#include <ImageId.h>
#include <ImagePixmapUnion.h>
#include <ImageTransformation.h>
#include <InteractionState.h>
#include <PageId.h>
#include <SerializableSpline.h>
#include <filters/output/ImageViewTab.h>
#include <filters/output/PictureZoneEditor.h>
#include <filters/output/Settings.h>
#include <filters/output/TabbedImageView.h>
#include <filters/select_content/ImageView.h>

#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QCursor>
#include <QDialog>
#include <QDir>
#include <QElapsedTimer>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGuiApplication>
#include <QImage>
#include <QKeySequence>
#include <QLayoutItem>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPointer>
#include <QPolygonF>
#include <QSettings>
#include <QSpinBox>
#include <QStackedLayout>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <functional>
#include <memory>
#include <vector>

#include "CocoaProbe.h"
#include "FixDpiDialog.h"
#include "HarnessGuards.h"
#include "MainWindow.h"
#include "ProjectFilesDialog.h"
#include "RelinkingDialog.h"

namespace {

const char* const kUsage
    = "usage: mac_check <output-dir> [--section menubar|layout|keys|zone|context|dialogs|panels|all, comma-separated | "
      "--section identity]\n"
      "identity, dialogs and panels need HOME and CFFIXED_USER_HOME set to <output-dir>/home\n";

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

  // The path flush() writes to; a section that might end the process (Cmd-Q) saves what it has first.
  void setPath(const QString& path) { m_path = path; }

  void flush() const {
    if (!m_path.isEmpty()) {
      save(m_path);
    }
  }

 private:
  QStringList m_lines;
  QString m_path;
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

// The symlink-resolved form of a path, to compare with the output directory that the harness guard returns already
// resolved. A part that does not exist yet keeps its cleaned spelling below its deepest existing ancestor. An empty
// or relative path gives an empty result, so an unset variable or a path that depends on the working directory never
// matches anything.
QString resolvedPath(const QString& path) {
  const QString cleaned = QDir::cleanPath(path);
  if (!QDir::isAbsolutePath(cleaned)) {
    return QString();
  }
  QString head = cleaned;
  QString rest;
  while (!head.isEmpty()) {
    const QString canonical = QFileInfo(head).canonicalFilePath();
    if (!canonical.isEmpty()) {
      return rest.isEmpty() ? canonical : QDir::cleanPath(canonical + "/" + rest);
    }
    const int slash = head.lastIndexOf('/');
    if (slash < 0) {
      break;
    }
    rest = rest.isEmpty() ? head.mid(slash + 1) : head.mid(slash + 1) + "/" + rest;
    head = slash == 0 ? QString("/") : head.left(slash);
  }
  return cleaned;
}

bool samePath(const QString& a, const QString& b) {
  const QString resolved = resolvedPath(a);
  return !resolved.isEmpty() && resolved == resolvedPath(b);
}

bool isBelow(const QString& path, const QString& root) {
  const QString resolved = resolvedPath(path);
  const QString resolvedRoot = resolvedPath(root);
  return !resolved.isEmpty() && !resolvedRoot.isEmpty()
         && (resolved == resolvedRoot || resolved.startsWith(resolvedRoot + "/"));
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
  bool aboutEnabled = false;
  bool quitEnabled = false;
  if (!menus.empty()) {
    for (const cocoaprobe::MenuItemInfo& item : menus[0].items) {
      if (item.separator || item.hidden) {
        continue;
      }
      appMenuItems << item.title;
      if (item.title.startsWith("About")) {
        aboutTitles << item.title;
        aboutEnabled = aboutEnabled || item.enabled;
      } else if (item.title.startsWith("Settings") || item.title.startsWith("Preferences")) {
        settingsTitles << item.title;
        if (settingsKey.isEmpty()) {
          settingsKey = keyText(item);
          settingsEnabled = item.enabled;
        }
      } else if (item.title.startsWith("Quit")) {
        quitTitles << item.title;
        quitEnabled = quitEnabled || item.enabled;
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
  summary.add("about_item_enabled", yesNo(aboutEnabled));
  summary.add("quit_item_enabled", yesNo(quitEnabled));
  for (const char* const name : {"actionAbout", "actionSettings", "actionQuit"}) {
    const QAction* const action = window.findChild<QAction*>(name);
    summary.add(QString("%1_enabled").arg(name),
                action != nullptr ? yesNo(action->isEnabled()) + (action->isVisible() ? "" : " (hidden)") : "missing");
  }
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
      if (!item.enabled) {
        entry += " {disabled}";
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
                      "and not part of the verdict")
                  .arg(countOf("cmd_equal")));

  const bool pass = layoutId.contains("German") && requiredOk && crossFiresRequired == 0 && outputTabsGroup != "none"
                    && thumbnailsGroup != "none";
  summary.add("layout_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

// Sections keys and zone ----------------------------------------------------------------------------------------

// Option-Command is the group the layout section measured for the Output tabs and the thumbnail keys (see
// .planning/phases/05-mac-menus-and-shortcuts/records/layout.md: output_tabs_group and thumbnails_group are
// option-command).
const unsigned kGroupModifiers = cocoaprobe::Command | cocoaprobe::Option;

// Counts the triggers of a set of actions by name. The counters live in a deque so that their addresses stay valid.
class ActionCounter {
 public:
  void watch(const QString& name, QAction* const action, QObject* const context) {
    m_names << name;
    m_counts.push_back(0);
    int* const counter = &m_counts.back();
    QObject::connect(action, &QAction::triggered, context, [counter] { ++*counter; });
  }

  void reset() { std::fill(m_counts.begin(), m_counts.end(), 0); }

  int count(const QString& name) const {
    const int index = m_names.indexOf(name);
    return index >= 0 ? m_counts[static_cast<size_t>(index)] : 0;
  }

  int others(const QString& name) const {
    int sum = 0;
    for (int i = 0; i < m_names.size(); ++i) {
      sum += m_names[i] == name ? 0 : m_counts[static_cast<size_t>(i)];
    }
    return sum;
  }

  QString describeOthers(const QString& name) const {
    QStringList parts;
    for (int i = 0; i < m_names.size(); ++i) {
      if (m_names[i] != name && m_counts[static_cast<size_t>(i)] != 0) {
        parts << QString("%1=%2").arg(m_names[i]).arg(m_counts[static_cast<size_t>(i)]);
      }
    }
    return parts.join(", ");
  }

  int total(const QStringList& names) const {
    int sum = 0;
    for (const QString& name : names) {
      sum += count(name);
    }
    return sum;
  }

 private:
  QStringList m_names;
  std::deque<int> m_counts;
};

// Counts the close events of the window it is installed on and swallows them, so that Cmd-Q can be observed without
// ending the run.
class CloseSwallower : public QObject {
 public:
  bool eventFilter(QObject* const watched, QEvent* const event) override {
    if (event->type() == QEvent::Close) {
      ++m_count;
      event->ignore();
      return true;
    }
    return QObject::eventFilter(watched, event);
  }

  int count() const { return m_count; }

 private:
  int m_count = 0;
};

// Looks the character up in the current layout (with Shift when the layout needs it, as for = on the German layout),
// sends the chord to the window and processes events for 300 ms. False when the layout has no such key or the window
// has no NSWindow.
bool pressCharacter(QWindow* const window, const QChar character, unsigned modifiers) {
  int code = 0;
  if (!cocoaprobe::keyCodeFor(character, false, &code)) {
    if (!cocoaprobe::keyCodeFor(character, true, &code)) {
      return false;
    }
    modifiers |= cocoaprobe::Shift;
  }
  const bool sent = cocoaprobe::sendKey(window, code, modifiers);
  pump(300);
  return sent;
}

bool pressSpecial(QWindow* const window, const cocoaprobe::SpecialKey key) {
  const bool sent = cocoaprobe::sendKey(window, cocoaprobe::specialKeyCode(key), 0);
  pump(300);
  return sent;
}

// Shows a widget as a top-level window, activates the application and waits for the window to have the keyboard focus.
bool showAndActivate(QWidget& widget) {
  widget.show();
  widget.activateWindow();
  widget.raise();
  cocoaprobe::activate(widget.windowHandle());
  const bool focused = waitFor([&widget] { return QGuiApplication::focusWindow() == widget.windowHandle(); }, 5000);
  pump(500);
  return cocoaprobe::activate(widget.windowHandle()) && focused;
}

QImage greyImage(const int width, const int height) {
  QImage image(width, height, QImage::Format_RGB32);
  image.fill(QColor(128, 128, 128));
  return image;
}

QStackedLayout* imageFrameLayout(MainWindow& window) {
  QFrame* const frame = window.findChild<QFrame*>("imageViewFrame");
  return frame != nullptr ? qobject_cast<QStackedLayout*>(frame->layout()) : nullptr;
}

// Removes every widget from the main area (they stay children of the frame, hidden) so that the section can put its own
// view in. Deletes the layout items, not the widgets.
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

QString ratioText(const double value) {
  return QString::number(value, 'f', 6);
}

bool nearly(const double value, const double expected) {
  return std::abs(value - expected) <= 0.000002;
}

bool runKeysSection(Summary& summary) {
  MainWindow window;
  window.resize(1280, 800);
  CloseSwallower closeSwallower;
  window.installEventFilter(&closeSwallower);
  const bool appActive = showAndActivate(window);
  summary.add("keys_app_active", yesNo(appActive));
  const bool german = cocoaprobe::keyboardLayoutId().contains("German");
  summary.add("keys_layout_german", yesNo(german));
  QWindow* const handle = window.windowHandle();

  // Production slots that would open dialogs or panels, close the window or change the stage are disconnected; the
  // zoom actions stay connected because their effect is measured.
  struct Watched {
    const char* name;
    const char* objectName;
    bool disconnectProduction;
  };
  const Watched watched[] = {
      {"stage_1", "actionSwitchFilter1", true},
      {"stage_2", "actionSwitchFilter2", true},
      {"stage_3", "actionSwitchFilter3", true},
      {"stage_4", "actionSwitchFilter4", true},
      {"stage_5", "actionSwitchFilter5", true},
      {"stage_6", "actionSwitchFilter6", true},
      {"new", "actionNewProject", true},
      {"open", "actionOpenProject", true},
      {"save", "actionSaveProject", true},
      {"save_as", "actionSaveProjectAs", true},
      {"close", "actionCloseProject", true},
      {"quit", "actionQuit", true},
      {"settings", "actionSettings", true},
      {"zoom_in", "actionZoomIn", false},
      {"zoom_out", "actionZoomOut", false},
      {"thumbs_larger", "actionMagnifyThumbnails", true},
      {"thumbs_smaller", "actionDiminishThumbnails", true},
  };
  ActionCounter counts;
  bool actionsFound = true;
  bool slotsDisconnected = true;
  for (const Watched& entry : watched) {
    QAction* const action = window.findChild<QAction*>(entry.objectName);
    if (action == nullptr) {
      actionsFound = false;
      summary.add(QString("keys_missing_action_%1").arg(entry.objectName), "yes");
      continue;
    }
    if (entry.disconnectProduction) {
      QObject::disconnect(action, &QAction::triggered, &window, nullptr);
      QObject::disconnect(action, &QAction::triggered, action, nullptr);
      // Qt's menus listen to the triggered signal of their actions to close themselves; they react to nothing here.
      for (QMenu* const menu : window.findChildren<QMenu*>()) {
        QObject::disconnect(action, &QAction::triggered, menu, nullptr);
      }
      // Fail closed: the wildcard disconnect returns true only if another receiver was left. Such a receiver could
      // open a native panel on the real home folder, so no key is sent. The check precedes the counter's own
      // connection.
      if (QObject::disconnect(action, &QAction::triggered, nullptr, nullptr)) {
        slotsDisconnected = false;
        summary.add(QString("keys_slot_still_connected_%1").arg(entry.objectName), "yes");
        continue;
      }
      action->setEnabled(true);
    }
    counts.watch(entry.name, action, &window);
  }
  if (!slotsDisconnected) {
    summary.add("keys_reason", "a production slot is still connected; no key was sent");
    summary.add("keys_verdict", "FAIL");
    return false;
  }
  const QStringList stageNames = {"stage_1", "stage_2", "stage_3", "stage_4", "stage_5", "stage_6"};

  bool allKeysOk = actionsFound;
  // One chord: counts the own action and every other watched action, records the own count and, if there is one, the
  // cross fires. True when the own action fired exactly once and no other did.
  auto press = [&](const QString& key, const QString& own, const QChar character, const unsigned modifiers) {
    counts.reset();
    const bool sent = pressCharacter(handle, character, modifiers);
    const int ownCount = counts.count(own);
    const int others = counts.others(own);
    summary.add(key, QString::number(ownCount));
    if (others != 0) {
      summary.add(key + "_cross", counts.describeOthers(own));
    }
    if (!sent) {
      summary.add(key + "_note", "event not sent or no key code in this layout");
    }
    const bool ok = sent && ownCount == 1 && others == 0;
    allKeysOk = allKeysOk && ok;
    return ok;
  };

  // Stage keys: the Command digits and the letters.
  for (int n = 1; n <= 6; ++n) {
    press(QString("keys_stage_cmd_%1").arg(n), QString("stage_%1").arg(n), QChar('0' + n), cocoaprobe::Command);
  }
  const char letters[6] = {'o', 'e', 'n', 'v', 'k', 'p'};
  for (int n = 1; n <= 6; ++n) {
    const QChar letter(letters[n - 1]);
    press(QString("keys_stage_letter_%1").arg(letter.toUpper()), QString("stage_%1").arg(n), letter, 0);
  }

  // File and Settings keys.
  press("keys_new", "new", 'n', cocoaprobe::Command);
  press("keys_open", "open", 'o', cocoaprobe::Command);
  press("keys_save", "save", 's', cocoaprobe::Command);
  press("keys_save_as", "save_as", 's', cocoaprobe::Command | cocoaprobe::Shift);
  press("keys_close", "close", 'w', cocoaprobe::Command);
  press("keys_settings", "settings", ',', cocoaprobe::Command);
  summary.add("keys_close_events_before_quit", QString::number(closeSwallower.count()));

  // Output tabs: a TabbedImageView with five tabs in the main area. The current tab starts at the last one so that
  // every key has to change it.
  emptyImageFrame(window);
  QStackedLayout* const layout = imageFrameLayout(window);
  auto* const tabs = new output::TabbedImageView;
  const output::ImageViewTab tabIds[5] = {output::TAB_OUTPUT, output::TAB_PICTURE_ZONES, output::TAB_FILL_ZONES,
                                          output::TAB_DEWARPING, output::TAB_DESPECKLING};
  const char* const tabNames[5] = {"Output", "Picture Zones", "Fill Zones", "Dewarping", "Despeckling"};
  for (int i = 0; i < 5; ++i) {
    tabs->addTab(new BasicImageView(greyImage(200, 150)), tabNames[i], tabIds[i]);
  }
  bool tabsOk = layout != nullptr;
  if (layout != nullptr) {
    layout->addWidget(tabs);
    layout->setCurrentWidget(tabs);
  }
  tabsOk = waitFor([tabs] { return tabs->isVisible(); }, 3000) && tabsOk;
  tabs->setCurrentIndex(4);
  tabs->currentWidget()->setFocus();
  pump(300);
  counts.reset();
  for (int n = 1; n <= 5; ++n) {
    pressCharacter(handle, QChar('0' + n), kGroupModifiers);
    summary.add(QString("keys_output_tab_%1").arg(n), QString::number(tabs->currentIndex()));
    tabsOk = tabsOk && tabs->currentIndex() == n - 1;
    tabs->setCurrentIndex(4);  // back to the last tab, so the next key has to change it again
    tabs->currentWidget()->setFocus();
    pump(100);
  }
  const int tabStageFires = counts.total(stageNames);
  summary.add("keys_output_tab_stage_fires", QString::number(tabStageFires));
  tabsOk = tabsOk && tabStageFires == 0;

  // Command-1 in the Output stage: the stage key fires once and changes no tab.
  tabs->setCurrentIndex(2);
  tabs->currentWidget()->setFocus();
  pump(300);
  counts.reset();
  pressCharacter(handle, '1', cocoaprobe::Command);
  const int cmd1Stage = counts.count("stage_1");
  const bool cmd1TabChanged = tabs->currentIndex() != 2;
  summary.add("keys_cmd1_in_output_stage", QString::number(cmd1Stage));
  summary.add("keys_cmd1_in_output_tab_changed", yesNo(cmd1TabChanged));
  tabsOk = tabsOk && cmd1Stage == 1 && !cmd1TabChanged;

  // Zoom keys: a page view in the main area.
  emptyImageFrame(window);
  auto* const view = new BasicImageView(greyImage(1200, 900));
  if (layout != nullptr) {
    layout->addWidget(view);
    layout->setCurrentWidget(view);
  }
  QAction* const zoomIn = window.findChild<QAction*>("actionZoomIn");
  bool zoomOk = layout != nullptr && zoomIn != nullptr;
  zoomOk = waitFor([zoomIn] { return zoomIn != nullptr && zoomIn->isEnabled(); }, 3000) && zoomOk;
  view->setFocus();
  pump(300);
  const double zoom0 = view->zoomLevel();
  pressCharacter(handle, '+', cocoaprobe::Command);
  const double zoom1 = view->zoomLevel();
  pressCharacter(handle, '-', cocoaprobe::Command);
  const double zoom2 = view->zoomLevel();
  pressCharacter(handle, '=', cocoaprobe::Command);
  const double zoom3 = view->zoomLevel();
  const double plusRatio = zoom1 / zoom0;
  const double minusRatio = zoom2 / zoom1;
  summary.add("keys_zoom_plus_ratio", ratioText(plusRatio));
  summary.add("keys_zoom_minus_ratio", ratioText(minusRatio));
  summary.add("keys_zoom_equal_ratio", ratioText(zoom3 / zoom2));
  zoomOk = zoomOk && nearly(plusRatio, std::pow(2.0, 1.0 / 6)) && nearly(minusRatio, std::pow(2.0, -1.0 / 6));

  // Thumbnail keys: the secondary group; the page zoom must not move.
  counts.reset();
  const double zoomBeforeThumbs = view->zoomLevel();
  pressCharacter(handle, '+', kGroupModifiers);
  const int larger = counts.count("thumbs_larger");
  pressCharacter(handle, '-', kGroupModifiers);
  const int smaller = counts.count("thumbs_smaller");
  counts.reset();
  pressCharacter(handle, '=', kGroupModifiers);
  const int largerEqual = counts.count("thumbs_larger");
  summary.add("keys_thumbs_larger", QString::number(larger));
  summary.add("keys_thumbs_smaller", QString::number(smaller));
  summary.add("keys_thumbs_larger_equal", QString::number(largerEqual));
  summary.add("keys_thumbs_zoom_moved", yesNo(view->zoomLevel() != zoomBeforeThumbs));
  const bool thumbsOk = larger == 1 && smaller == 1 && view->zoomLevel() == zoomBeforeThumbs;

  // The native View menu with the key equivalents Qt gave its items.
  QStringList actualView;
  bool viewFound = false;
  for (const cocoaprobe::MenuInfo& menu : cocoaprobe::mainMenu()) {
    if (menu.title != "View") {
      continue;
    }
    viewFound = true;
    for (const cocoaprobe::MenuItemInfo& item : menu.items) {
      if (item.separator || item.hidden) {
        continue;
      }
      actualView << (item.keyEquivalent.isEmpty() ? item.title : item.title + " " + keyText(item));
    }
  }
  summary.add("keys_view_menu", viewFound ? actualView.join(" | ") : "no View menu");
  const QString cmd = modifierGlyphs(cocoaprobe::Command);
  const QString optCmd = modifierGlyphs(kGroupModifiers);
  QStringList expectedView;
  for (int n = 1; n <= 6; ++n) {
    const QAction* const stage = window.findChild<QAction*>(QString("actionSwitchFilter%1").arg(n));
    expectedView << (stage != nullptr ? stripAmpersand(stage->text()) : QString("?")) + " " + cmd + QString::number(n);
  }
  expectedView << "Zoom In " + cmd + "+" << "Zoom Out " + cmd + "-" << "Larger Thumbnails " + optCmd + "+"
               << "Smaller Thumbnails " + optCmd + "-";
  const bool viewOk = viewFound && actualView == expectedView;
  if (!viewOk) {
    summary.add("keys_view_menu_expected", expectedView.join(" | "));
  }

  // Cmd-Q last: through actionQuit or through the application's termination request, both end in the window's close
  // path, which the swallower counts. The summary is saved first in case the run ends here.
  summary.flush();
  counts.reset();
  const bool quitSent = pressCharacter(handle, 'q', cocoaprobe::Command);
  const int quitTriggered = counts.count("quit");
  const int quitCloseEvents = closeSwallower.count();
  summary.add("keys_quit_triggered", QString::number(quitTriggered));
  summary.add("keys_quit_close_events", QString::number(quitCloseEvents));
  const bool quitOk = quitSent && (quitTriggered >= 1 || quitCloseEvents >= 1);

  const bool pass = appActive && german && allKeysOk && tabsOk && zoomOk && thumbsOk && viewOk && quitOk;
  summary.add("keys_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

// Zone editor of an empty picture mask over a white page, as in tst_mac_qt.cpp.
std::unique_ptr<output::PictureZoneEditor> makeZoneEditor(const QImage& page) {
  QImage maskImage(page.size(), QImage::Format_RGB32);
  maskImage.fill(Qt::black);
  return std::make_unique<output::PictureZoneEditor>(
      page, ImagePixmapUnion(), imageproc::BinaryImage(maskImage), QTransform(), QPolygonF(QRectF(page.rect())),
      PageId(ImageId(QStringLiteral("page.png"))), std::make_shared<output::Settings>());
}

QImage zonePage() {
  QImage page(1200, 900, QImage::Format_RGB32);
  page.fill(Qt::white);
  page.setDotsPerMeterX(11811);
  page.setDotsPerMeterY(11811);
  return page;
}

// Adds a rectangular zone (image pixels) the way the creation interaction does, without committing.
void addRectZone(output::PictureZoneEditor& editor, const QRectF& imageRect) {
  QPolygonF polygon;
  polygon << imageRect.topLeft() << imageRect.topRight() << imageRect.bottomRight() << imageRect.bottomLeft();
  editor.zones().addZone(std::make_shared<EditableSpline>(SerializableSpline(polygon)));
}

int zoneCount(const output::PictureZoneEditor& editor) {
  int count = 0;
  for (const EditableZoneSet::Zone& zone : editor.zones()) {
    Q_UNUSED(zone);
    ++count;
  }
  return count;
}

// A top-level window with a zone editor, a line edit with the text abc and a spin box. The window is declared first so
// that it is destroyed last.
struct ZoneWindow {
  ZoneWindow() {
    editor = makeZoneEditor(zonePage());
    auto* const layout = new QVBoxLayout(&window);
    layout->addWidget(editor.get(), 1);
    spinBox = new QSpinBox(&window);
    spinBox->setValue(5);
    lineEdit = new QLineEdit(QStringLiteral("abc"), &window);
    layout->addWidget(spinBox);
    layout->addWidget(lineEdit);
    window.resize(700, 760);
  }

  QWidget window;
  std::unique_ptr<output::PictureZoneEditor> editor;
  QSpinBox* spinBox = nullptr;
  QLineEdit* lineEdit = nullptr;
};

// A top-level window with a Select Content view that has no box. The window is declared first so that it is destroyed
// last.
struct ContentWindow {
  ContentWindow() {
    QImage page(800, 600, QImage::Format_RGB32);
    page.fill(Qt::white);
    view = std::make_unique<select_content::ImageView>(
        page, page, ContentMask(), ImageTransformation(QRectF(page.rect()), Dpi(300, 300)), QRectF(), QRectF(), false);
    auto* const layout = new QVBoxLayout(&window);
    layout->addWidget(view.get(), 1);
    window.resize(700, 600);
  }

  QWidget window;
  std::unique_ptr<select_content::ImageView> view;
};

bool runZoneSection(Summary& summary) {
  const QRectF zoneRect(400, 300, 400, 300);  // centre of the 1200 by 900 page, image pixels
  const QString tip = QString::fromUtf8("Press \xE2\x8C\xAB");

  bool zonesOk = true;
  bool appActive = false;
  {
    ZoneWindow zw;
    appActive = showAndActivate(zw.window);
    summary.add("zone_app_active", yesNo(appActive));
    QWindow* const handle = zw.window.windowHandle();
    output::PictureZoneEditor& editor = *zw.editor;
    addRectZone(editor, zoneRect);

    // The pointer is placed with cocoaprobe::warpPointer (QCursor::setPos left it where it was, see the record) and
    // one synthesised mouse-move event; a nudge of a few pixels on every call makes sure the interaction sees a move
    // even when the zone is new.
    bool pointerPlaced = false;
    int nudge = 0;
    auto pointAtZone = [&]() {
      const QPoint local = editor.imageToWidget().map(zoneRect.center()).toPoint() + QPoint(nudge, nudge);
      nudge = (nudge + 3) % 9;
      pointerPlaced = cocoaprobe::warpPointer(editor.viewport()->mapToGlobal(local));
      cocoaprobe::sendMouse(handle, QPointF(editor.viewport()->mapTo(&zw.window, local)), cocoaprobe::MouseAction::Move,
                            0);
      const bool tracked
          = waitFor([&editor, &tip] { return editor.interactionState().statusTip().contains(tip); }, 2000);
      pump(100);
      return tracked;
    };
    auto focusEditor = [&]() {
      editor.viewport()->setFocus();
      waitFor(
          [&] {
            const QWidget* const focus = QApplication::focusWidget();
            return focus != nullptr && (focus == &editor || editor.isAncestorOf(focus));
          },
          2000);
    };

    focusEditor();
    const bool tracked = pointAtZone();
    summary.add("zone_pointer_tracked", yesNo(tracked));
    const QPoint expectedGlobal
        = editor.viewport()->mapToGlobal(editor.imageToWidget().map(zoneRect.center()).toPoint());
    const QPoint actualGlobal = QCursor::pos();
    const bool placed = pointerPlaced && (actualGlobal - expectedGlobal).manhattanLength() <= 12;
    summary.add("zone_pointer_placed", yesNo(placed));
    int trackedLater = 0;

    // Backspace over the zone.
    pressSpecial(handle, cocoaprobe::SpecialKey::Backspace);
    const bool backspaceRemoved = zoneCount(editor) == 0;
    summary.add("zone_backspace_removed", yesNo(backspaceRemoved));

    // Forward delete over a new zone.
    addRectZone(editor, zoneRect);
    trackedLater += pointAtZone() ? 1 : 0;
    pressSpecial(handle, cocoaprobe::SpecialKey::ForwardDelete);
    const bool forwardRemoved = zoneCount(editor) == 0;
    summary.add("zone_forward_delete_removed", yesNo(forwardRemoved));

    // Backspace in the focused line edit: the field edits its text and the zone stays.
    addRectZone(editor, zoneRect);
    trackedLater += pointAtZone() ? 1 : 0;
    zw.lineEdit->setFocus();
    zw.lineEdit->end(false);
    waitFor([&zw] { return QApplication::focusWidget() == zw.lineEdit; }, 2000);
    pressSpecial(handle, cocoaprobe::SpecialKey::Backspace);
    const bool lineEditRemoved = zoneCount(editor) == 0;
    summary.add("zone_backspace_in_line_edit_removed", yesNo(lineEditRemoved));
    summary.add("zone_line_edit_text", zw.lineEdit->text());

    // Backspace in the focused spin box.
    zw.spinBox->setFocus();
    waitFor(
        [&zw] {
          const QWidget* const focus = QApplication::focusWidget();
          return focus != nullptr && (focus == zw.spinBox || zw.spinBox->isAncestorOf(focus));
        },
        2000);
    pressSpecial(handle, cocoaprobe::SpecialKey::Backspace);
    const bool spinBoxRemoved = zoneCount(editor) == 0;
    summary.add("zone_backspace_in_spin_box_removed", yesNo(spinBoxRemoved));
    summary.add("zone_pointer_tracked_later", QString("%1 of 2").arg(trackedLater));

    zonesOk = appActive && placed && tracked && trackedLater == 2 && backspaceRemoved && forwardRemoved
              && !lineEditRemoved && !spinBoxRemoved && zw.lineEdit->text() == "ab";
  }

  // Select Content: B creates the box, Backspace and forward delete remove it.
  bool contentOk = false;
  {
    ContentWindow cw;
    const bool contentActive = showAndActivate(cw.window);
    summary.add("content_app_active", yesNo(contentActive));
    QWindow* const handle = cw.window.windowHandle();
    int events = 0;
    QRectF last;
    QObject::connect(cw.view.get(), &select_content::ImageView::manualContentRectSet, &cw.window,
                     [&events, &last](const QRectF& rect) {
                       ++events;
                       last = rect;
                     });
    cw.view->viewport()->setFocus();
    waitFor(
        [&cw] {
          const QWidget* const focus = QApplication::focusWidget();
          return focus != nullptr && (focus == cw.view.get() || cw.view->isAncestorOf(focus));
        },
        2000);

    pressCharacter(handle, 'b', 0);
    const bool created = events == 1 && !last.isEmpty();
    summary.add("content_b_created", yesNo(created));
    pressSpecial(handle, cocoaprobe::SpecialKey::Backspace);
    const bool backspaceRemoved = events == 2 && last.isEmpty();
    summary.add("content_backspace_removed", yesNo(backspaceRemoved));
    pressCharacter(handle, 'b', 0);
    const bool createdAgain = events == 3 && !last.isEmpty();
    pressSpecial(handle, cocoaprobe::SpecialKey::ForwardDelete);
    const bool forwardRemoved = events == 4 && last.isEmpty();
    summary.add("content_forward_delete_removed", yesNo(forwardRemoved));
    summary.add("content_events", QString::number(events));
    contentOk = contentActive && created && backspaceRemoved && createdAgain && forwardRemoved;
  }

  const bool pass = zonesOk && contentOk;
  summary.add("zone_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

// Section context -----------------------------------------------------------------------------------------------

// Counts the QEvent::ContextMenu events delivered to one object; installed on the application so that nothing that
// reaches the object is missed.
class ContextEventCounter : public QObject {
 public:
  explicit ContextEventCounter(QObject* const target) : m_target(target) {}

  bool eventFilter(QObject* const watched, QEvent* const event) override {
    if (watched == m_target && event->type() == QEvent::ContextMenu) {
      ++m_count;
    }
    return false;
  }

  int count() const { return m_count; }

  void reset() { m_count = 0; }

 private:
  QObject* const m_target;
  int m_count = 0;
};

// The zone menu is a QMenu child of the editor, created per interaction.
QMenu* visibleZoneMenu(const QWidget& editor) {
  for (QMenu* const menu : editor.findChildren<QMenu*>()) {
    if (menu->isVisible()) {
      return menu;
    }
  }
  return nullptr;
}

bool anyZoneMenuObject(const QWidget& editor) {
  return !editor.findChildren<QMenu*>().isEmpty();
}

// What happened after one right-click or Control-click on a zone, and after the first click on Delete.
struct TriggerResult {
  bool tracked = false;
  int events = 0;
  bool menuShown = false;
  bool qtMenu = false;
  bool nativeMenuObject = false;
  bool deleteFound = false;
  bool clickSent = false;
  bool deleted = false;
  bool hideRan = false;
  bool triggeredBeforeHide = false;
  bool confirmed = false;  // a menu showed on an attempt that started with the pointer confirmed on the target
  int retries = 0;
};

// Probe filled by the Delete action's triggered signal and by a queued slot of the menu's aboutToHide, the order the
// interaction relies on.
struct HideProbe {
  bool triggered = false;
  bool hideRan = false;
  bool triggeredBeforeHide = false;
};

const int kEscapeKeyCode = 53;  // kVK_Escape

bool runContextSection(Summary& summary) {
  const QRectF zoneRect(400, 300, 400, 300);  // centre of the 1200 by 900 page, image pixels
  const QString tip = QString::fromUtf8("Press \xE2\x8C\xAB");
  const int orderRuns = 20;

  ZoneWindow zw;
  const bool appActive = showAndActivate(zw.window);
  summary.add("context_app_active", yesNo(appActive));
  QWindow* const handle = zw.window.windowHandle();
  output::PictureZoneEditor& editor = *zw.editor;
  ContextEventCounter counter(editor.viewport());
  QCoreApplication::instance()->installEventFilter(&counter);

  bool pointerPlaced = true;
  int nudge = 0;
  auto zoneWindowPos = [&]() {
    const QPoint local = editor.imageToWidget().map(zoneRect.center()).toPoint();
    return QPointF(editor.viewport()->mapTo(&zw.window, local));
  };
  // The zone menu is looked up through QCursor::pos(), which can lag behind a pointer warp: after the warp the target
  // is waited for (re-warping every 100 ms) before the move event is sent, and again right before each trigger.
  int cursorLagged = 0;       // placements where QCursor::pos() was not at the target at the first look
  int cursorUnconfirmed = 0;  // placements where it never got there within the bound
  QPoint targetGlobal;
  auto confirmCursor = [&]() {
    const auto atTarget = [&targetGlobal] { return (QCursor::pos() - targetGlobal).manhattanLength() <= 1; };
    if (atTarget()) {
      return true;
    }
    ++cursorLagged;
    for (int attempt = 0; attempt < 10; ++attempt) {
      pointerPlaced = cocoaprobe::warpPointer(targetGlobal) && pointerPlaced;
      if (waitFor(atTarget, 100)) {
        return true;
      }
    }
    ++cursorUnconfirmed;
    return false;
  };
  auto pointAtZone = [&]() {
    const QPoint local = editor.imageToWidget().map(zoneRect.center()).toPoint() + QPoint(nudge, nudge);
    nudge = (nudge + 3) % 9;
    targetGlobal = editor.viewport()->mapToGlobal(local);
    pointerPlaced = cocoaprobe::warpPointer(targetGlobal) && pointerPlaced;
    confirmCursor();
    cocoaprobe::sendMouse(handle, QPointF(editor.viewport()->mapTo(&zw.window, local)), cocoaprobe::MouseAction::Move,
                          0);
    const bool tracked = waitFor([&editor, &tip] { return editor.interactionState().statusTip().contains(tip); }, 2000);
    pump(100);
    return tracked;
  };
  auto focusEditor = [&]() {
    editor.viewport()->setFocus();
    waitFor(
        [&] {
          const QWidget* const focus = QApplication::focusWidget();
          return focus != nullptr && (focus == &editor || editor.isAncestorOf(focus));
        },
        2000);
  };
  // One zone under the pointer with the default interaction active.
  auto prepare = [&]() {
    if (zoneCount(editor) == 0) {
      addRectZone(editor, zoneRect);
    }
    focusEditor();
    return pointAtZone();
  };

  int retries = 0;
  int interference = 0;    // attempts where the pointer was off the target (moved by someone else) when it mattered
  int failedCursorOn = 0;  // attempts without a menu although the pointer was confirmed on the target
  const auto nearTarget = [&targetGlobal] { return (QCursor::pos() - targetGlobal).manhattanLength() <= 3; };
  // Triggers on the zone until a menu shows on a confirmed attempt, at most three times. An attempt is only sent
  // with QCursor::pos() on the target (re-warped when it is not); a miss with the pointer off the target is counted
  // as interference, a miss with the pointer on the target as a failure. Nothing changes when no menu shows (the
  // default interaction stays), so the trigger can be repeated.
  auto openMenu = [&](const cocoaprobe::MouseAction action, const unsigned modifiers, TriggerResult& result) {
    result.tracked = prepare();
    for (int attempt = 0; attempt < 3 && !result.confirmed; ++attempt) {
      if (attempt > 0) {
        ++retries;
        ++result.retries;
        result.tracked = prepare() && result.tracked;
      }
      confirmCursor();
      if (!nearTarget()) {
        ++interference;
        continue;
      }
      counter.reset();
      cocoaprobe::sendMouse(handle, zoneWindowPos(), action, modifiers);
      result.menuShown = waitFor([&editor] { return visibleZoneMenu(editor) != nullptr; }, 2000);
      result.events = counter.count();
      if (result.menuShown) {
        result.confirmed = true;
      } else if (nearTarget()) {
        ++failedCursorOn;
      } else {
        ++interference;
      }
    }
  };
  // One trigger on the zone; with clickDelete the first click on the Delete item follows.
  auto runTrigger = [&](const cocoaprobe::MouseAction action, const unsigned modifiers, const bool clickDelete) {
    TriggerResult result;
    openMenu(action, modifiers, result);
    const int before = zoneCount(editor);
    QMenu* const menu = visibleZoneMenu(editor);
    result.qtMenu = menu != nullptr;
    result.nativeMenuObject = menu == nullptr && anyZoneMenuObject(editor);
    if (menu == nullptr || !clickDelete) {
      return result;
    }
    QAction* deleteAction = nullptr;
    for (QAction* const candidate : menu->actions()) {
      if (candidate->text() == "Delete") {
        deleteAction = candidate;
      }
    }
    result.deleteFound = deleteAction != nullptr;
    if (deleteAction == nullptr || menu->windowHandle() == nullptr) {
      return result;
    }
    const auto probe = std::make_shared<HideProbe>();
    QObject::connect(deleteAction, &QAction::triggered, &zw.window, [probe] { probe->triggered = true; });
    QObject::connect(
        menu, &QMenu::aboutToHide, &zw.window,
        [probe] {
          probe->hideRan = true;
          probe->triggeredBeforeHide = probe->triggered;
        },
        Qt::QueuedConnection);
    result.clickSent = cocoaprobe::sendMouse(menu->windowHandle(), QPointF(menu->actionGeometry(deleteAction).center()),
                                             cocoaprobe::MouseAction::LeftClick, 0);
    pump(400);  // longer than the 200 ms deferral of the interaction
    result.deleted = zoneCount(editor) == before - 1;
    result.hideRan = probe->hideRan;
    result.triggeredBeforeHide = probe->hideRan && probe->triggeredBeforeHide;
    waitFor([&editor] { return visibleZoneMenu(editor) == nullptr; }, 2000);
    return result;
  };
  auto report = [&summary](const QString& name, const TriggerResult& result) {
    const QString prefix = "context_" + name + "_";
    summary.add(prefix + "tracked", yesNo(result.tracked));
    summary.add(prefix + "events", QString::number(result.events));
    summary.add(prefix + "menu_shown", yesNo(result.menuShown));
    summary.add(prefix + "pointer_confirmed", yesNo(result.confirmed));
    summary.add(prefix + "retries", QString::number(result.retries));
    summary.add(prefix + "first_click_deleted", result.qtMenu ? yesNo(result.deleted) : "not measurable in-process");
    summary.add(prefix + "triggered_before_queued_hide",
                result.qtMenu ? (result.hideRan ? yesNo(result.triggeredBeforeHide) : "hide slot did not run")
                              : "not measurable in-process");
  };

  const TriggerResult right = runTrigger(cocoaprobe::MouseAction::RightClick, 0, true);
  report("right_click", right);
  const TriggerResult control = runTrigger(cocoaprobe::MouseAction::LeftClick, cocoaprobe::Control, true);
  report("control_click", control);
  const bool qtMenu = right.qtMenu && control.qtMenu;
  const bool nativeMenu = right.nativeMenuObject || control.nativeMenuObject;
  summary.add("context_menu_kind", qtMenu ? "qt" : nativeMenu ? "native" : "none");

  // Dismissal: Escape on the open menu changes nothing and leaves the default interaction in place.
  bool dismissHidden = false;
  bool dismissUnchanged = false;
  bool dismissRestored = false;
  bool dismissConfirmed = false;
  QString dismissVia = "none";
  {
    TriggerResult opened;
    openMenu(cocoaprobe::MouseAction::RightClick, 0, opened);
    const int before = zoneCount(editor);
    summary.add("context_dismiss_menu_shown", yesNo(opened.tracked && opened.menuShown));
    summary.add("context_dismiss_pointer_confirmed", yesNo(opened.confirmed));
    summary.add("context_dismiss_retries", QString::number(opened.retries));
    dismissConfirmed = opened.confirmed;
    QMenu* const menu = visibleZoneMenu(editor);
    if (menu != nullptr) {
      if (menu->windowHandle() != nullptr) {
        cocoaprobe::sendKey(menu->windowHandle(), kEscapeKeyCode, 0);
        dismissVia = "menu window";
      }
      dismissHidden = waitFor([&editor] { return visibleZoneMenu(editor) == nullptr; }, 1000);
      if (!dismissHidden) {
        cocoaprobe::sendKey(handle, kEscapeKeyCode, 0);
        dismissVia = "main window";
        dismissHidden = waitFor([&editor] { return visibleZoneMenu(editor) == nullptr; }, 1000);
      }
      if (!dismissHidden) {
        menu->hide();  // never leave a menu open under the real session
      }
    }
    pump(500);  // the interaction's deferred restore of the default interaction (200 ms) and a margin
    dismissUnchanged = zoneCount(editor) == before;
    dismissRestored = pointAtZone();
  }
  summary.add("context_dismiss_via", dismissVia);
  summary.add("context_dismiss_menu_hidden", yesNo(dismissHidden));
  summary.add("context_dismiss_zones_unchanged", yesNo(dismissUnchanged));
  summary.add("context_dismiss_default_restored", yesNo(dismissRestored));

  // Ordering: in how many of 20 right-click plus Delete runs had triggered() fired before the queued aboutToHide slot
  // ran.
  // Only runs whose attempt had the pointer confirmed on the target count; unconfirmed runs are repeated, with a bound
  // on how many runs may fail.
  int before = 0;
  int ran = 0;
  int deleted = 0;
  int orderConfirmed = 0;
  int orderAttempted = 0;
  while (orderConfirmed < orderRuns && orderAttempted - orderConfirmed < 5) {
    ++orderAttempted;
    const TriggerResult result = runTrigger(cocoaprobe::MouseAction::RightClick, 0, true);
    if (!result.confirmed) {
      continue;
    }
    ++orderConfirmed;
    before += result.triggeredBeforeHide ? 1 : 0;
    ran += result.hideRan ? 1 : 0;
    deleted += result.deleted ? 1 : 0;
  }
  summary.add("context_order_runs", QString::number(orderConfirmed));
  summary.add("context_order_attempted", QString::number(orderAttempted));
  summary.add("context_order_hide_ran", QString::number(ran));
  summary.add("context_order_deleted", QString::number(deleted));
  summary.add("context_triggered_before_queued_hide_count", QString::number(before));
  summary.add("context_two_finger_click",
              "same rightMouseDown event as right_click; not distinguishable in-process; unverified residual");
  summary.add("context_pointer_placed", yesNo(pointerPlaced));
  summary.add("context_cursor_lagged", QString::number(cursorLagged));
  summary.add("context_cursor_unconfirmed", QString::number(cursorUnconfirmed));
  summary.add("context_retries", QString::number(retries));
  summary.add("context_pointer_interference", QString::number(interference));
  summary.add("context_failed_attempts_pointer_confirmed", QString::number(failedCursorOn));

  const bool removeDeferral = qtMenu && orderConfirmed == orderRuns && before == orderRuns;
  summary.add("context_delay_decision", removeDeferral ? "remove" : "keep");

  QCoreApplication::instance()->removeEventFilter(&counter);
  const bool pass = appActive && pointerPlaced && right.tracked && control.tracked && right.events == 1
                    && control.events == 1 && right.confirmed && control.confirmed && dismissConfirmed && right.deleted
                    && control.deleted && dismissHidden && dismissUnchanged && dismissRestored;
  summary.add("context_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

// Sections dialogs and panels -----------------------------------------------------------------------------------

// True only when HOME and CFFIXED_USER_HOME both name <out>/home and the home directory as Cocoa, Qt and the standard
// paths see it lies below the checkout's build/ directory. The two sections open native file panels and list the
// profile folder, which must never reach the user's folders (a live check never touches them).
bool homeRedirected(const QString& out) {
  const QString expectedHome = QDir::cleanPath(out + "/home");
  const QString root = QDir::cleanPath(QString::fromUtf8(MAC_CHECK_ROOT));
  const QString fixedHome = QString::fromLocal8Bit(qgetenv("CFFIXED_USER_HOME"));
  const QString envHome = QString::fromLocal8Bit(qgetenv("HOME"));
  return QDir(expectedHome).exists() && samePath(envHome, expectedHome) && samePath(fixedHome, expectedHome)
         && isBelow(cocoaprobe::homeDirectory(), root) && isBelow(QDir::homePath(), root)
         && isBelow(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), root);
}

// The folders the two sections use, below the output directory, and the QSettings keys the panels start from.
void prepareDialogFolders(const QString& out) {
  QDir().mkpath(out + "/panels/input");
  QDir().mkpath(out + "/panels/output");
  QDir().mkpath(out + "/relink/old");
  QSettings settings;
  settings.setValue("project/lastDir", out + "/panels");
  settings.setValue("lastInputDir", out + "/panels/input");
  settings.sync();
}

// What the native menu bar shows at one moment.
struct MenuBarReading {
  QString topLevel;  // titles after the application menu, joined by " | "
  bool appMenu = false;
  bool newItemFound = false;
  bool newItemEnabled = false;
  bool populated = false;
};

MenuBarReading readMenuBar() {
  const std::vector<cocoaprobe::MenuInfo> menus = cocoaprobe::mainMenu();
  MenuBarReading reading;
  QStringList titles;
  for (size_t i = 1; i < menus.size(); ++i) {
    titles << menus[i].title;
    for (const cocoaprobe::MenuItemInfo& item : menus[i].items) {
      if (item.title.startsWith("New Project")) {
        reading.newItemFound = true;
        reading.newItemEnabled = item.enabled;
      }
    }
  }
  reading.topLevel = titles.join(" | ");
  reading.appMenu = !menus.empty() && !menus[0].items.empty();
  reading.populated = reading.appMenu && titles.contains("File") && titles.contains("View") && titles.contains("Tools");
  return reading;
}

void addMenuBarLines(Summary& summary, const QString& row, const MenuBarReading& reading) {
  const QString prefix = "dialogs_" + row;
  summary.add(prefix + "_top_level", orNone(reading.topLevel));
  summary.add(prefix + "_app_menu", yesNo(reading.appMenu));
  summary.add(prefix + "_new_item_enabled", reading.newItemFound ? yesNo(reading.newItemEnabled) : QString("none"));
  summary.add(prefix + "_populated", yesNo(reading.populated));
}

// Shared by the open_panel row: the block scheduled before the native panel opens fills it in.
struct OpenPanelProbe {
  bool done = false;  // the call that opened the panel has returned
  bool sawPanel = false;
  bool populated = false;
  int cmdNFired = 0;
};

bool runDialogsSection(Summary& summary, const QString& out) {
  if (!homeRedirected(out)) {
    summary.add("dialogs_reason", "home not redirected");
    summary.add("dialogs_verdict", "FAIL");
    return false;
  }
  prepareDialogFolders(out);

  MainWindow window;
  window.resize(1280, 800);
  const bool appActive = showAndActivate(window);
  summary.add("dialogs_app_active", yesNo(appActive));

  // New Project would open the project-creation dialog: its production slot is cut off and only counted.
  QAction* const newAction = window.findChild<QAction*>("actionNewProject");
  int newCount = 0;
  const bool newSlotCut
      = newAction != nullptr && QObject::disconnect(newAction, SIGNAL(triggered(bool)), &window, SLOT(newProject()));
  if (newAction != nullptr) {
    QObject::connect(newAction, &QAction::triggered, [&newCount](bool) { ++newCount; });
  }
  summary.add("dialogs_new_project_slot_cut", yesNo(newSlotCut));
  int nKeyCode = 0;
  const bool haveN = cocoaprobe::keyCodeFor(QChar('n'), false, &nKeyCode);
  summary.add("dialogs_cmd_n_key_code", haveN ? QString::number(nKeyCode) : QString("none"));

  bool allShown = true;
  bool allPopulated = true;

  // One dialog row: the dialog is already requested; wait until it is visible, read the bar, send Cmd-N, close it.
  auto measureRow = [&](const QString& row, const QPointer<QDialog>& dialog) {
    const bool shown
        = !dialog.isNull()
          && waitFor([&dialog] { return !dialog.isNull() && dialog->isVisible() && dialog->windowHandle(); }, 5000);
    pump(500);
    summary.add("dialogs_" + row + "_shown", yesNo(shown));
    allShown = shown && allShown;
    if (!shown) {
      allPopulated = false;
      return;
    }
    const MenuBarReading reading = readMenuBar();
    addMenuBarLines(summary, row, reading);
    allPopulated = reading.populated && allPopulated;
    const int before = newCount;
    cocoaprobe::sendKey(dialog->windowHandle(), nKeyCode, cocoaprobe::Command);
    pump(300);
    summary.add("dialogs_" + row + "_cmd_n_fired", QString::number(newCount - before));
    dialog->close();
    pump(300);
  };

  // A dialog the main window creates on its own: the visible direct child dialog after the action fired.
  auto triggerRow = [&](const QString& row, const QString& actionName) {
    QAction* const action = window.findChild<QAction*>(actionName);
    if (action == nullptr) {
      summary.add("dialogs_" + row + "_shown", "no");
      allShown = false;
      allPopulated = false;
      return;
    }
    action->trigger();
    QPointer<QDialog> found;
    waitFor(
        [&] {
          for (QDialog* const child : window.findChildren<QDialog*>(Qt::FindDirectChildrenOnly)) {
            if (child->isVisible()) {
              found = child;
              return true;
            }
          }
          return false;
        },
        5000);
    measureRow(row, found);
  };

  triggerRow("settings", "actionSettings");
  triggerRow("about", "actionAbout");

  {
    FixDpiDialog dialog(std::vector<ImageFileInfo>(), &window);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.show();
    measureRow("fix_dpi", QPointer<QDialog>(&dialog));
  }
  {
    RelinkingDialog dialog(out + "/relink/project.ScanTailor", &window);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.show();
    measureRow("relinking", QPointer<QDialog>(&dialog));
  }

  triggerRow("default_params", "actionDefaults");

  {
    QMessageBox box(QMessageBox::Information, "Menu bar check", "Application-modal message box of the live Mac check.",
                    QMessageBox::Ok, &window);
    box.setWindowModality(Qt::ApplicationModal);
    box.show();
    measureRow("message_box", QPointer<QDialog>(&box));
  }

  // The native Open Project panel: a block scheduled before the call runs inside the panel's modal session, reads the
  // bar, sends Cmd-N to the panel and cancels it. A second block rescues the call if no native panel ever appeared.
  {
    const auto probe = std::make_shared<OpenPanelProbe>();
    cocoaprobe::runLater(800, [probe, &summary, &newCount, nKeyCode]() {
      if (probe->done) {
        return;
      }
      probe->sawPanel = cocoaprobe::modalWindowIsFilePanel();
      const MenuBarReading reading = readMenuBar();
      addMenuBarLines(summary, "open_panel", reading);
      probe->populated = reading.populated;
      const int before = newCount;
      cocoaprobe::sendKeyToModalWindow(nKeyCode, cocoaprobe::Command);
      probe->cmdNFired = newCount - before;
      cocoaprobe::cancelModalPanel();
    });
    cocoaprobe::runLater(4000, [probe]() {
      if (probe->done) {
        return;
      }
      cocoaprobe::cancelModalPanel();
      if (QWidget* const modal = QApplication::activeModalWidget()) {
        modal->close();
      }
    });
    QMetaObject::invokeMethod(&window, "openProject", Qt::DirectConnection);
    probe->done = true;
    pump(300);
    summary.add("dialogs_open_panel_shown", yesNo(probe->sawPanel));
    summary.add("dialogs_open_panel_cmd_n_fired", QString::number(probe->cmdNFired));
    allShown = probe->sawPanel && allShown;
    allPopulated = probe->populated && allPopulated;
  }

  const bool pass = appActive && allShown && allPopulated;
  summary.add("dialogs_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

// What one native panel row observed. The shared state outlives the call so the block that runs inside the panel's
// modal session and the code after the call see the same object.
struct PanelObservation {
  bool done = false;  // the call has returned
  bool probed = false;
  bool filePanel = false;
  QString className;
  QString directory;
  QString title;
  bool underOut = false;
  bool dialogFound = false;
  bool finishedSeen = false;
  int finishedResult = -1;
  bool selectedSeen = false;
  QString selectedPath;
};

// The three measured clauses of one panel row.
struct PanelResult {
  bool native = false;
  bool underOut = false;
  bool cancelledEmpty = false;
};

// Opens one panel through `call`, inspects it from a block that runs inside the modal session, attaches to the
// QFileDialog object the static function created, cancels the panel and records the row. `owner` is the widget passed
// as parent to the static function. A panel that never appears within 3000 ms is closed so the call returns.
PanelResult runPanelRow(Summary& summary,
                        const QString& out,
                        const QString& row,
                        QWidget& owner,
                        const std::function<void()>& call) {
  const auto obs = std::make_shared<PanelObservation>();
  QPointer<QWidget> ownerPointer(&owner);
  const auto step = std::make_shared<std::function<void(int)>>();
  *step = [obs, step, ownerPointer, out](int elapsed) {
    if (obs->done || obs->probed) {
      return;
    }
    if (cocoaprobe::modalWindowIsFilePanel()) {
      obs->probed = true;
      obs->filePanel = true;
      obs->className = cocoaprobe::modalWindowClass();
      obs->directory = cocoaprobe::modalPanelDirectory();
      obs->title = cocoaprobe::modalPanelTitle();
      obs->underOut = isBelow(obs->directory, out);
      QFileDialog* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
      if (dialog == nullptr && !ownerPointer.isNull()) {
        dialog = ownerPointer->findChild<QFileDialog*>();
      }
      if (dialog != nullptr) {
        obs->dialogFound = true;
        QObject::connect(dialog, &QFileDialog::finished, [obs](int result) {
          obs->finishedSeen = true;
          obs->finishedResult = result;
        });
        QObject::connect(dialog, &QFileDialog::filesSelected, [obs](const QStringList& files) {
          obs->selectedSeen = true;
          obs->selectedPath = files.isEmpty() ? QString() : files.first();
        });
      }
      cocoaprobe::cancelModalPanel();
      return;
    }
    if (elapsed >= 3000) {
      obs->probed = true;
      obs->className = cocoaprobe::modalWindowClass();
      if (QWidget* const modal = QApplication::activeModalWidget()) {
        modal->close();
      }
      return;
    }
    cocoaprobe::runLater(200, [step, elapsed]() { (*step)(elapsed + 200); });
  };
  cocoaprobe::runLater(800, [step]() { (*step)(800); });

  call();
  obs->done = true;
  pump(300);

  const QString prefix = "panels_" + row;
  summary.add(prefix + "_class", orNone(obs->className));
  summary.add(prefix + "_native", yesNo(obs->filePanel));
  summary.add(prefix + "_directory", orNone(shown(obs->directory, out)));
  summary.add(prefix + "_directory_under_out", yesNo(obs->underOut));
  summary.add(prefix + "_title", orNone(shown(obs->title, out)));
  QString cancelResult = "unobserved";
  if (obs->dialogFound && obs->selectedSeen) {
    cancelResult = "selected " + shown(obs->selectedPath, out);
  } else if (obs->dialogFound && obs->finishedSeen && obs->finishedResult == QDialog::Rejected) {
    cancelResult = "empty";
  }
  summary.add(prefix + "_cancel_result", cancelResult);
  PanelResult result;
  result.native = obs->filePanel;
  result.underOut = obs->underOut;
  result.cancelledEmpty = cancelResult == "empty";
  return result;
}

bool runPanelsSection(Summary& summary, const QString& out) {
  if (!homeRedirected(out)) {
    summary.add("panels_reason", "home not redirected");
    summary.add("panels_verdict", "FAIL");
    return false;
  }
  prepareDialogFolders(out);

  MainWindow window;
  window.resize(1280, 800);
  const bool appActive = showAndActivate(window);
  summary.add("panels_app_active", yesNo(appActive));

  bool allNative = true;
  bool allUnderOut = true;
  bool allEmpty = true;
  bool allUnchanged = true;
  auto collect = [&](const PanelResult& result) {
    allNative = result.native && allNative;
    allUnderOut = result.underOut && allUnderOut;
    allEmpty = result.cancelledEmpty && allEmpty;
  };

  // The relinking dialog is shared by the two relinking rows; commitChanges (the OK button's slot) commits the
  // model's uncommitted paths into the relinker and only hides the dialog, so it can be called after each row.
  const RelinkablePath orig(out + "/relink/old/page.png", RelinkablePath::File);
  RelinkingDialog relinking(out + "/relink/project.ScanTailor", &window);
  relinking.pathCollector()(orig);

  auto relinkRow = [&](const QString& row, const QString& prefixPath, const int type) {
    collect(runPanelRow(summary, out, row, relinking, [&relinking, &prefixPath, type]() {
      QMetaObject::invokeMethod(&relinking, "pathButtonClicked", Qt::DirectConnection, Q_ARG(QString, prefixPath),
                                Q_ARG(QString, QString()), Q_ARG(int, type));
    }));
    QMetaObject::invokeMethod(&relinking, "commitChanges", Qt::DirectConnection);
    pump(200);
    const bool unchanged = relinking.relinker()->substitutionPathFor(orig) == orig.normalizedPath();
    summary.add("panels_" + row + "_substitution_unchanged", yesNo(unchanged));
    allUnchanged = unchanged && allUnchanged;
  };
  relinkRow("relink_file", out + "/relink/old/page.png", RelinkablePath::File);
  relinkRow("relink_folder", out + "/relink/old", RelinkablePath::Dir);

  collect(runPanelRow(summary, out, "open_project", window,
                      [&window]() { QMetaObject::invokeMethod(&window, "openProject", Qt::DirectConnection); }));

  {
    ProjectFilesDialog dialog(&window);
    collect(runPanelRow(summary, out, "input_folder", dialog,
                        [&dialog]() { QMetaObject::invokeMethod(&dialog, "inpDirBrowse", Qt::DirectConnection); }));
    if (QLineEdit* const outLine = dialog.findChild<QLineEdit*>("outDirLine")) {
      outLine->setText(out + "/panels/output");
    }
    collect(runPanelRow(summary, out, "output_folder", dialog,
                        [&dialog]() { QMetaObject::invokeMethod(&dialog, "outDirBrowse", Qt::DirectConnection); }));
  }

  summary.add("panels_not_opened",
              "save_project_as, out_of_memory_save, files_to_insert (need a loaded project; plain QFileDialog calls "
              "without options, checked by testNoDontUseNativeDialog)");

  const bool pass = appActive && allNative && allUnderOut && allEmpty && allUnchanged;
  summary.add("panels_verdict", pass ? "PASS" : "FAIL");
  return pass;
}

// Section identity ----------------------------------------------------------------------------------------------

// Repeats the order of src/app/main.cpp (application object, then the two names, then the INI format and a
// default-constructed QSettings, no path redirect) with HOME redirected below the output directory.
int runIdentitySection(const QString& out, int& argc, char** argv) {
  qunsetenv("XDG_CONFIG_HOME");
  const QString expectedHome = QDir::cleanPath(out + "/home");
  const QString fixedHome = QString::fromLocal8Bit(qgetenv("CFFIXED_USER_HOME"));
  if (!QDir(expectedHome).exists() || !samePath(QDir::homePath(), expectedHome) || !samePath(fixedHome, expectedHome)) {
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
  const QStringList knownSections
      = {"menubar", "layout", "keys", "zone", "context", "dialogs", "panels", "identity", "all"};
  const QStringList requested = sectionArg.split(',');
  QStringList sections;
  for (const QString& name : requested) {
    if (!knownSections.contains(name)) {
      badArguments = true;
    } else if (name == "all") {
      sections << "menubar" << "layout" << "zone" << "context" << "dialogs" << "panels" << "keys";
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

  QString out;
  QString guardError;
  if (!harness::outputDirIsStrictlyInside(outArg, QString::fromUtf8(MAC_CHECK_ROOT), &out, &guardError)) {
    std::fprintf(stderr, "mac_check: %s\n", guardError.toLocal8Bit().constData());
    return 2;
  }
  if (!QDir().mkpath(out + "/tmp") || !QDir().mkpath(out + "/settings")) {
    std::fprintf(stderr, "mac_check: cannot create %s\n", out.toLocal8Bit().constData());
    return 2;
  }

  // Everything below is set before the application object exists, so no setting is read from the user's account and
  // the measurement sees bare Cocoa behaviour.
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
  summary.setPath(out + "/summary.txt");
  summary.add("platform", QGuiApplication::platformName());
  summary.add("qt_runtime", QString::fromLatin1(qVersion()));
  summary.add("section", sectionArg);
  summary.add("keyboard_layout", orNone(cocoaprobe::keyboardLayoutId()));
  summary.add("home_directory", shown(cocoaprobe::homeDirectory(), out));
  summary.add("home_redirected", yesNo(homeRedirected(out)));
  summary.add("frontmost_app", orNone(cocoaprobe::frontmostAppName()));

  bool pass = true;
  if (sections.contains("menubar")) {
    pass = runMenubarSection(summary) && pass;
  }
  if (sections.contains("layout")) {
    pass = runLayoutSection(summary) && pass;
  }
  // The zone section runs before keys: keys ends with Cmd-Q, which might end the process.
  if (sections.contains("zone")) {
    pass = runZoneSection(summary) && pass;
  }
  if (sections.contains("context")) {
    pass = runContextSection(summary) && pass;
  }
  if (sections.contains("dialogs")) {
    pass = runDialogsSection(summary, out) && pass;
  }
  if (sections.contains("panels")) {
    pass = runPanelsSection(summary, out) && pass;
  }
  if (sections.contains("keys")) {
    pass = runKeysSection(summary) && pass;
  }
  summary.add("verdict", pass ? "PASS" : "FAIL");
  if (!summary.save(out + "/summary.txt")) {
    std::fprintf(stderr, "mac_check: cannot write %s/summary.txt\n", out.toLocal8Bit().constData());
    return 2;
  }
  return pass ? 0 : 1;
}
