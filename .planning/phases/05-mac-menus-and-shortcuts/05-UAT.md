---
status: partial
phase: 05-mac-menus-and-shortcuts
source: [05-VERIFICATION.md]
started: 2026-11-15T13:30:00Z
updated: 2026-11-15T18:26:57.102Z
---

## Current Test

[testing paused — 2 items outstanding (tests 1 and 2 need the user at the trackpad and listening; this run covered only what Claude can check)]

## Tests

### 1. Two-finger trackpad click on a zone (see behavior_unverified_items)
expected: Zone context menu opens
result: [pending]

### 2. Batch run with 'Beep when finished' ticked
expected: System beep audible
result: [pending]

### 3. Look at the nine apply dialogs (multi-page hint) and the Margins help in the running app
expected: The shortened hints with the Mac key glyphs and the 7 pt bold glyphs of the Margins help are readable and not clipped
result: pass
source: automated
checked_by: Claude, offscreen render (2026-11-15, HEAD 7ef43942)
evidence: |
  Throwaway renderer build/scratch/uat05/uat05_render (not in the repository) linked against the
  macos-release libraries built the nine real dialog classes and the page_layout OptionsWidget, with
  the app's Fusion style and stylesheet, the macOS system font (.AppleSystemUIFont 13 pt), scale 2,
  light and dark scheme, and one or two selected pages (hint greyed / enabled). Images: build/scratch/uat05/out/.
  - All nine selectedPagesHint labels show "Use ⌘-click / ⇧-click to select multiple pages." at 7 pt:
    label width 248-336 pt against 220 pt needed, inside the dialog, 0 missing glyphs (glyph-run check).
  - Guides Help (Margins): the 7 pt bold ⌥⌘-drag, ⇧/⌘-drag, ⇧, ⌘, ⇧⌘ spans render; the document is
    184 pt tall in the 220 pt box (scroll bars off), so nothing is cut; 0 missing glyphs.
  - Looked at the images: the greyed hint stays legible in both schemes.
  Limits: offscreen platform rather than Cocoa (same CoreText fonts, same Fusion style); no window frame drawn.
note: |
  Not a Phase 5 defect: at the Filters dock's minimum width (274 pt) the whole Margins options panel
  (minimum 316 pt, set by its margin and alignment controls) is cut on the right, the Guides Help with it.
  Widening the dock shows everything. Phase 5 changed only the help strings in OptionsWidget.ui and not the dock.

## Summary

total: 3
passed: 1
issues: 0
pending: 2
skipped: 0
blocked: 0

## Gaps
