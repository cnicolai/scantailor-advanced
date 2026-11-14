// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
//
// Objective-C++ half of mac_check (Phase 5, D-12). See CocoaProbe.h for the contract of every function.

#include "CocoaProbe.h"

#include <QWindow>

#import <AppKit/AppKit.h>
#include <Carbon/Carbon.h>

#include <utility>

// Runs a std::function after a delay, from the run loop.
@interface MacCheckLaterTask : NSObject
- (instancetype)initWithFunction:(std::function<void()>)function;
- (void)run;
@end

@implementation MacCheckLaterTask {
  std::function<void()> m_function;
}

- (instancetype)initWithFunction:(std::function<void()>)function {
  self = [super init];
  if (self != nil) {
    m_function = std::move(function);
  }
  return self;
}

- (void)run {
  if (m_function) {
    m_function();
  }
}
@end

namespace cocoaprobe {

namespace {

QString toQString(NSString* text) {
  return text != nil ? QString::fromUtf8([text UTF8String]) : QString();
}

NSView* viewOf(QWindow* window) {
  if (window == nullptr) {
    return nil;
  }
  return (__bridge NSView*) reinterpret_cast<void*>(window->winId());
}

NSWindow* nsWindowOf(QWindow* window) {
  return [viewOf(window) window];
}

// The Carbon modifier state UCKeyTranslate expects: the shiftKey, optionKey and controlKey bits shifted right by 8.
UInt32 carbonModifierState(unsigned modifiers) {
  UInt32 state = 0;
  if ((modifiers & Shift) != 0) {
    state |= shiftKey;
  }
  if ((modifiers & Option) != 0) {
    state |= optionKey;
  }
  if ((modifiers & Control) != 0) {
    state |= controlKey;
  }
  return (state >> 8) & 0xFF;
}

NSEventModifierFlags eventFlags(unsigned modifiers) {
  NSEventModifierFlags flags = 0;
  if ((modifiers & Command) != 0) {
    flags |= NSEventModifierFlagCommand;
  }
  if ((modifiers & Option) != 0) {
    flags |= NSEventModifierFlagOption;
  }
  if ((modifiers & Shift) != 0) {
    flags |= NSEventModifierFlagShift;
  }
  if ((modifiers & Control) != 0) {
    flags |= NSEventModifierFlagControl;
  }
  return flags;
}

unsigned modifierBits(NSEventModifierFlags flags) {
  unsigned bits = 0;
  if ((flags & NSEventModifierFlagCommand) != 0) {
    bits |= Command;
  }
  if ((flags & NSEventModifierFlagOption) != 0) {
    bits |= Option;
  }
  if ((flags & NSEventModifierFlagShift) != 0) {
    bits |= Shift;
  }
  if ((flags & NSEventModifierFlagControl) != 0) {
    bits |= Control;
  }
  return bits;
}

NSTimeInterval now() {
  return [[NSProcessInfo processInfo] systemUptime];
}

}  // namespace

QString keyboardLayoutId() {
  TISInputSourceRef source = TISCopyCurrentKeyboardLayoutInputSource();
  if (source == nullptr) {
    return QString();
  }
  const CFStringRef identifier = static_cast<CFStringRef>(TISGetInputSourceProperty(source, kTISPropertyInputSourceID));
  const QString result = toQString((__bridge NSString*) identifier);
  CFRelease(source);
  return result;
}

QString charactersFor(int keyCode, unsigned modifiers) {
  TISInputSourceRef source = TISCopyCurrentKeyboardLayoutInputSource();
  if (source == nullptr) {
    return QString();
  }
  QString result;
  const CFDataRef layoutData
      = static_cast<CFDataRef>(TISGetInputSourceProperty(source, kTISPropertyUnicodeKeyLayoutData));
  if (layoutData != nullptr) {
    const UCKeyboardLayout* layout = reinterpret_cast<const UCKeyboardLayout*>(CFDataGetBytePtr(layoutData));
    UInt32 deadKeyState = 0;
    UniCharCount length = 0;
    UniChar characters[8] = {};
    const OSStatus status
        = UCKeyTranslate(layout, static_cast<UInt16>(keyCode), kUCKeyActionDown, carbonModifierState(modifiers),
                         LMGetKbdType(), kUCKeyTranslateNoDeadKeysBit, &deadKeyState, 8, &length, characters);
    if (status == noErr) {
      result = QString(reinterpret_cast<const QChar*>(characters), static_cast<qsizetype>(length));
    }
  }
  CFRelease(source);
  return result;
}

bool keyCodeFor(QChar character, bool withShift, int* keyCode) {
  for (int code = 0; code < 128; ++code) {
    if (code >= kVK_ANSI_KeypadDecimal && code <= kVK_ANSI_Keypad9) {
      continue;  // the main keyboard only: the keypad block would answer for = before the real key
    }
    const QString text = charactersFor(code, withShift ? static_cast<unsigned>(Shift) : 0u);
    if (text.size() == 1 && text.at(0) == character) {
      if (keyCode != nullptr) {
        *keyCode = code;
      }
      return true;
    }
  }
  return false;
}

int specialKeyCode(SpecialKey key) {
  return key == SpecialKey::Backspace ? kVK_Delete : kVK_ForwardDelete;
}

bool activate(QWindow* window) {
  NSWindow* nsWindow = nsWindowOf(window);
  if (nsWindow == nil) {
    return false;
  }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  [NSApp activateIgnoringOtherApps:YES];
#pragma clang diagnostic pop
  if (@available(macOS 14.0, *)) {
    [NSApp activate];
  }
  [nsWindow makeKeyAndOrderFront:nil];
  return [NSApp isActive] && [nsWindow isKeyWindow];
}

bool sendKey(QWindow* window, int keyCode, unsigned modifiers) {
  NSWindow* nsWindow = nsWindowOf(window);
  if (nsWindow == nil) {
    return false;
  }
  QString characters = charactersFor(keyCode, modifiers);
  QString plainCharacters = charactersFor(keyCode, modifiers & Shift);
  if (keyCode == kVK_Delete) {
    characters = plainCharacters = QString(QChar(0x007F));
  } else if (keyCode == kVK_ForwardDelete) {
    characters = plainCharacters = QString(QChar(static_cast<ushort>(NSDeleteFunctionKey)));
  }
  NSString* const nsCharacters = characters.toNSString();
  NSString* const nsPlainCharacters = plainCharacters.toNSString();

  const NSEventType types[2] = {NSEventTypeKeyDown, NSEventTypeKeyUp};
  for (const NSEventType type : types) {
    NSEvent* const event = [NSEvent keyEventWithType:type
                                            location:NSZeroPoint
                                       modifierFlags:eventFlags(modifiers)
                                           timestamp:now()
                                        windowNumber:[nsWindow windowNumber]
                                             context:nil
                                          characters:nsCharacters
                         charactersIgnoringModifiers:nsPlainCharacters
                                           isARepeat:NO
                                             keyCode:static_cast<unsigned short>(keyCode)];
    if (event == nil) {
      return false;
    }
    [NSApp sendEvent:event];
  }
  return true;
}

bool sendMouse(QWindow* window, const QPointF& logicalPos, MouseAction action, unsigned modifiers) {
  NSView* const view = viewOf(window);
  NSWindow* const nsWindow = [view window];
  if (view == nil || nsWindow == nil) {
    return false;
  }
  const CGFloat y = [view isFlipped] ? logicalPos.y() : [view bounds].size.height - logicalPos.y();
  const NSPoint location = [view convertPoint:NSMakePoint(logicalPos.x(), y) toView:nil];

  NSEventType types[2] = {NSEventTypeMouseMoved, NSEventTypeMouseMoved};
  int count = 1;
  switch (action) {
    case MouseAction::Move:
      break;
    case MouseAction::LeftClick:
      types[0] = NSEventTypeLeftMouseDown;
      types[1] = NSEventTypeLeftMouseUp;
      count = 2;
      break;
    case MouseAction::RightClick:
      types[0] = NSEventTypeRightMouseDown;
      types[1] = NSEventTypeRightMouseUp;
      count = 2;
      break;
  }
  for (int i = 0; i < count; ++i) {
    const bool isDown = count == 2 && i == 0;
    NSEvent* const event = [NSEvent mouseEventWithType:types[i]
                                              location:location
                                         modifierFlags:eventFlags(modifiers)
                                             timestamp:now()
                                          windowNumber:[nsWindow windowNumber]
                                               context:nil
                                           eventNumber:0
                                            clickCount:count == 2 ? 1 : 0
                                              pressure:isDown ? 1.0f : 0.0f];
    if (event == nil) {
      return false;
    }
    [NSApp sendEvent:event];
  }
  return true;
}

std::vector<MenuInfo> mainMenu() {
  std::vector<MenuInfo> menus;
  NSMenu* const bar = [NSApp mainMenu];
  if (bar == nil) {
    return menus;
  }
  for (NSMenuItem* const topItem in [bar itemArray]) {
    MenuInfo info;
    NSMenu* const submenu = [topItem submenu];
    info.title = toQString(submenu != nil ? [submenu title] : [topItem title]);
    if (submenu != nil) {
      for (NSMenuItem* const item in [submenu itemArray]) {
        MenuItemInfo entry;
        entry.title = toQString([item title]);
        entry.keyEquivalent = toQString([item keyEquivalent]);
        entry.modifiers = modifierBits([item keyEquivalentModifierMask]);
        entry.enabled = [item isEnabled];
        entry.hidden = [item isHidden];
        entry.separator = [item isSeparatorItem];
        entry.hasSubmenu = [item hasSubmenu];
        info.items.push_back(entry);
      }
    }
    menus.push_back(info);
  }
  return menus;
}

QString localizedAppName() {
  return toQString([[NSRunningApplication currentApplication] localizedName]);
}

QString bundleName() {
  const CFTypeRef value = CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), CFSTR("CFBundleName"));
  if (value == nullptr || CFGetTypeID(value) != CFStringGetTypeID()) {
    return QString();
  }
  return toQString((__bridge NSString*) value);
}

QString frontmostAppName() {
  return toQString([[[NSWorkspace sharedWorkspace] frontmostApplication] localizedName]);
}

QString homeDirectory() {
  return toQString(NSHomeDirectory());
}

void runLater(int ms, std::function<void()> fn) {
  MacCheckLaterTask* const task = [[MacCheckLaterTask alloc] initWithFunction:std::move(fn)];
  [task performSelector:@selector(run)
             withObject:nil
             afterDelay:ms / 1000.0
                inModes:@[ NSDefaultRunLoopMode, NSModalPanelRunLoopMode, NSEventTrackingRunLoopMode ]];
}

}  // namespace cocoaprobe
