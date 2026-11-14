// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
//
// Test-only helper of mac_check (Phase 5, D-12). The functions are implemented in Objective-C++ (CocoaProbe.mm), which
// exists nowhere else in the repository (D-00b). Every event goes to this process's own windows through
// [NSApp sendEvent:]; nothing is posted system-wide and no event tap is installed, so no Accessibility, Input
// Monitoring or screen-recording permission is needed and nothing is physically pressed.

#ifndef MAC_CHECK_COCOA_PROBE_H
#define MAC_CHECK_COCOA_PROBE_H

#include <QChar>
#include <QPointF>
#include <QString>
#include <functional>
#include <vector>

class QWindow;

namespace cocoaprobe {

/// Modifier keys of a synthesised event.
enum Modifier : unsigned { NoModifier = 0, Command = 1, Option = 2, Shift = 4, Control = 8 };

enum class MouseAction { Move, LeftClick, RightClick };

/// Keys without a printable character, named so that main.cpp needs no Carbon header.
enum class SpecialKey { Backspace, ForwardDelete };

/// One item of a native menu. `modifiers` are Modifier bits of the key equivalent.
struct MenuItemInfo {
  QString title;
  QString keyEquivalent;
  unsigned modifiers = 0;
  bool enabled = false;
  bool hidden = false;
  bool separator = false;
  bool hasSubmenu = false;
};

/// One top-level native menu with its items in order.
struct MenuInfo {
  QString title;
  std::vector<MenuItemInfo> items;
};

/// kTISPropertyInputSourceID of the current keyboard layout, e.g. com.apple.keylayout.German.
QString keyboardLayoutId();

/// Scans the virtual key codes 0 to 127 with UCKeyTranslate on the current layout and returns the first code whose
/// output (with or without Shift) equals character.
bool keyCodeFor(QChar character, bool withShift, int* keyCode);

/// UCKeyTranslate output for the Shift, Option and Control part of modifiers (Command is ignored, as AppKit reports
/// characters).
QString charactersFor(int keyCode, unsigned modifiers);

/// The virtual key code of a key without a printable character.
int specialKeyCode(SpecialKey key);

/// Activates this application and makes the window's NSWindow key and front; true when the application is active and
/// the window is key.
bool activate(QWindow* window);

/// keyDown then keyUp for the window's NSWindow through [NSApp sendEvent:]; false if the window has no NSWindow.
bool sendKey(QWindow* window, int keyCode, unsigned modifiers);

/// Mouse events at the window-relative logical position through [NSApp sendEvent:]: Move is one mouse-moved event,
/// a click a down and up pair of the left or right button.
bool sendMouse(QWindow* window, const QPointF& logicalPos, MouseAction action, unsigned modifiers);

/// NSApp.mainMenu; element 0 is the application menu.
std::vector<MenuInfo> mainMenu();

/// NSRunningApplication.currentApplication.localizedName.
QString localizedAppName();

/// CFBundleName of the main bundle's info dictionary, empty when absent.
QString bundleName();

/// Localised name of the frontmost application (loginwindow while the session is locked).
QString frontmostAppName();

/// NSHomeDirectory().
QString homeDirectory();

/// Runs fn after ms milliseconds in the default, modal-panel and event-tracking run loop modes, so it also runs while a
/// modal panel or a menu is tracking.
void runLater(int ms, std::function<void()> fn);

}  // namespace cocoaprobe

#endif  // MAC_CHECK_COCOA_PROBE_H
