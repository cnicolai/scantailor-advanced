# Phase 5: Mac Menus and Shortcuts - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-11-14
**Phase:** 5-Mac Menus and Shortcuts
**Areas discussed:** Cmd+number keys, Zoom keys, Zone keys on a Mac keyboard, How Phase 5 is checked

---

## Cmd+number keys

**Question 1:** What should Cmd+1 to Cmd+6 do, given that Cmd+1–5 currently switch the tabs inside the Output stage?

| Option | Description | Selected |
|--------|-------------|----------|
| Stages; tabs → ⌥⌘1–5 (Recommended) | Cmd+1–6 switch the six stages everywhere (roadmap); the Output tabs move to Option+Cmd+1–5, the same pattern as Safari or Xcode | ✓ |
| Stages; tabs lose their keys | Cmd+1–6 switch stages; the Output tabs are click-only | |
| Keep tabs on Cmd+1–5 | Output tabs keep Cmd+1–5, stages get Option+Cmd+1–6; contradicts roadmap criterion 2 | |

**Question 2:** The stages also switch with O, E, N, V, K, P, and those actions sit in no menu. What happens to them?

| Option | Description | Selected |
|--------|-------------|----------|
| Keep letters + add a menu (Recommended) | O/E/N/V/K/P stay; a new View menu lists the six stages with ⌘1–6 | ✓ |
| Keep letters, no menu | Letters stay, Cmd+1–6 added, nothing in the menu bar | |
| Drop the letters | Only Cmd+1–6 switch stages | |

**User's choice:** Cmd+1–6 for stages, Output tabs on ⌥⌘1–5, letters kept, stages listed in a new View menu.

---

## Zoom keys

**Question 1:** Cmd+= and Cmd+- move to the page view. What should resize the thumbnails afterwards?

| Option | Description | Selected |
|--------|-------------|----------|
| ⌥⌘= / ⌥⌘- (Recommended) | Thumbnails move to Option+Cmd, the same pattern as the Output tabs; toolbar buttons and Option+wheel stay | ✓ |
| No keys for thumbnails | Toolbar buttons and Option+wheel only | |
| Thumbnails keep Cmd+/- | Page view gets other keys; contradicts roadmap criterion 2 | |

**Question 2:** Which keyboard layout does the user type on?

| Option | Description | Selected |
|--------|-------------|----------|
| German | Zoom in is the + key (⌘+); both ⌘+ and ⌘= bound, help text names ⌘+ | ✓ |
| US / English | Zoom in is the = key (⌘=); both bound, help text names ⌘= | |
| I switch between them | Both bindings, help text names the key by its role | |

**Question 3:** Should the new View menu also list the zoom commands with their keys?

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, all four (Recommended) | Zoom In ⌘+, Zoom Out ⌘-, Larger Thumbnails ⌥⌘+, Smaller Thumbnails ⌥⌘- | ✓ |
| Page zoom only | Thumbnail keys documented only in toolbar tooltips | |
| No zoom items | View menu holds only the six stages | |

**User's choice:** ⌘+/⌘=/⌘- for the page view, ⌥⌘ for thumbnails, German layout, all four zoom commands in the View menu.

---

## Zone keys on a Mac keyboard

**Question 1:** No Apple keyboard has an Insert key. Which key should create the content box?

| Option | Description | Selected |
|--------|-------------|----------|
| B, for box (Recommended) | A single letter, like Z, X, C and D in the zone editor; free everywhere; Insert stays for PC keyboards | ✓ |
| Return (↩) | Feels like "confirm", but collides with spin boxes in the options panel | |
| Context menu only | No key; contradicts roadmap criterion 3 | |

**Question 2:** Today Del removes the zone or content box under the mouse pointer, and there is no undo. Which Mac keys should remove it?

| Option | Description | Selected |
|--------|-------------|----------|
| ⌫ and ⌦ (Recommended) | The delete key and forward delete both remove what's under the pointer, as Del does today | ✓ |
| ⌘⌫ only | Finder-style, harder to hit by accident, two keys | |
| ⌫, ⌦ and ⌘⌫ | All three | |

**User's choice:** B creates the content box (Insert kept), ⌫ and ⌦ remove the zone or box under the pointer.

---

## How Phase 5 is checked

| Option | Description | Selected |
|--------|-------------|----------|
| Claude only (Recommended) | As in Phases 2–4: `ctest` for roles, shortcuts and strings; a Cocoa harness with settings in `build/` for the menu bar, dialogs and Cocoa-level key and click events; records say nothing was physically pressed | ✓ |
| Claude + 5-min check by you | The above plus one short walkthrough at the end, one step at a time | |
| Mostly by you | Claude runs `ctest`; the user tries every criterion in the real app | |

**User's choice:** Claude only.

---

## Claude's Discretion

Announced before area selection and accepted with "I'm ready for context":
- Apple-style key names in help text (⌘-click, ⇧⌘S), built from `QKeySequence::NativeText` where practical
- The app-menu item named as current macOS names it ("Settings…"); explicit menu roles; the Tools menu keeps its other entries
- Native relinking dialog (remove `DontUseNativeDialog`)
- System beep for the batch alarm; the stored `external_alarm_cmd` value is ignored and left in place

Recorded as defaults without discussion: the app-menu title without a bundle (minimal embedded Info.plist only if needed), the menu bar while a dialog is open, the 200 ms context-menu workaround, a fallback if ⌥⌘ combinations do not fire on the German layout, action wiring, harness location, plan split.

## Deferred Ideas

- v2 Mac polish (POLISH-01 to POLISH-05) stays deferred.
- Trackpad zoom feel and gestures stay out of scope.
- ⌘0 zoom-to-fit and a shortcut cheat-sheet in Help: not requested.
