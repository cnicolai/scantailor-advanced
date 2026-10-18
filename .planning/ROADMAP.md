# Roadmap: ScanTailor Advanced (personal macOS fork)

## Overview

This roadmap ports and hardens ScanTailor Advanced 1.2.1 for one Apple Silicon Mac. Each phase protects the next one. First the unmodified code is made to build and run natively, changing only what the build needs. Next a synthetic-image test suite records what that unchanged code produces. With those baselines in place, the Qt 5, Windows, Linux, packaging, CI and translation code is removed, and the baselines show that output did not change. After the strip, the view layer is made Mac-native: Retina rendering first, then menus and shortcuts. The last phase fixes known safety defects, removes the OpenGL viewer and splits the oversized files. Through all of this the six-stage pipeline keeps producing the same output.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [ ] **Phase 1: Deletion-Free macOS Build** - Unmodified 1.2.1 builds as arm64 against Homebrew and runs a project end to end; existing test results and FP settings recorded
- [ ] **Phase 2: Golden Baselines** - Synthetic-image test suite and golden baselines recorded from the unmodified code before anything is removed
- [ ] **Phase 3: Qt 6 and macOS Only** - Qt 5, Windows, Linux, packaging, CI and translations removed, with baselines unchanged; README documents the Mac build
- [ ] **Phase 4: Retina Rendering** - Page view, thumbnails, overlays and colour picker are sharp and correct on Retina displays
- [ ] **Phase 5: Mac Menus and Shortcuts** - Standard application menu, Cmd shortcuts, Mac key names, native dialogs and Mac-friendly input
- [ ] **Phase 6: Hardening and Cleanup** - Safety defects fixed, OpenGL viewer removed, oversized files split and dead code removed, with baselines unchanged

## Phase Details

### Phase 1: Deletion-Free macOS Build

**Goal**: The unmodified 1.2.1 code builds as a native arm64 binary on the Mac and processes a real project end to end. The only edits are those needed to build and launch it.
**Depends on**: Nothing (first phase)
**Requirements**: BUILD-01, BUILD-02, BUILD-03, BUILD-04, BUILD-05, BUILD-06, BUILD-07
**Success Criteria** (what must be TRUE):
  1. One CMake preset configures and builds the app with Apple Clang against Homebrew Qt 6, Boost, jpeg-turbo, libpng and libtiff. `file` reports the binary as arm64, the compile commands show warnings equivalent to `-Wall -Wextra`, and `otool -L` lists exactly one libjpeg, one libpng and one libtiff
  2. The app launches from the build directory, opens a project, takes it through all six stages and writes output files
  3. `ctest` runs the existing test suite on the Mac, and the pass/fail result of each test from this first build is committed to the repository
  4. The `-ffp-contract` setting is fixed in the build configuration and recorded. A CMake sanitizer option produces an ASan/UBSan build that runs the existing tests, and its findings are recorded, not fixed
  5. `git diff 5eaac18` (the 1.2.1 release) outside `.planning/` shows only build and launch fixes and added build settings: no deleted code paths and no change to processing behaviour

**Plans:** 7 plans

Plans:
**Wave 1**
- [ ] 01-01-PLAN.md — Tracer: Homebrew toolchain to a passing foundation_tests via CMakePresets.json; Clang warnings, frozen -ffp-contract=off, SANITIZE option, debug/asan presets

**Wave 2** *(blocked on Wave 1 completion)*
- [ ] 01-02-PLAN.md — First full release/debug build, error classes recorded, formatting decision, one-commit-per-class compile fixes
- [ ] 01-05-PLAN.md — Scratchpad DPR/settings-path probe, synthetic smoke pages, D-02 permission with corrected INI paths

**Wave 3** *(blocked on Wave 2 completion)*
- [ ] 01-03-PLAN.md — Warning log and counts, arm64 and one-copy library checks, ctest records for Release and Debug, qt_tests offscreen decision
- [ ] 01-04-PLAN.md — ASan/UBSan build, existing tests under sanitizers, findings recorded not fixed

**Wave 4** *(blocked on Wave 3 completion)*
- [ ] 01-06-PLAN.md — User-driven smoke walkthrough (one step per message), output and project checks, SMOKE-FINDINGS.md

**Wave 5** *(blocked on Wave 4 completion)*
- [ ] 01-07-PLAN.md — Closing check of all five success criteria from fresh trees, criterion-5 diff audit, Phase 2 baseline commit

**UI hint**: no

### Phase 2: Golden Baselines

**Goal**: A synthetic-image test suite records what the unmodified code produces for flat documents, book spreads and mixed pages, so every later change can be checked against it.
**Depends on**: Phase 1
**Requirements**: TEST-01, TEST-02, TEST-03, TEST-04, TEST-05, TEST-06, TEST-07, TEST-08, TEST-09, TEST-10, TEST-11
**Success Criteria** (what must be TRUE):
  1. The generator, run twice with the same seed, produces identical flat-document, book-spread and mixed text-and-image pages with known ground truth (skew angle, content box, split position, page curvature, picture zones). It uses no font rendering and no `rand()`
  2. Tests check deskew angle, content box and binarization on flat documents; split position and dewarped line straightness on book pages; and picture zones, colour/greyscale output and despeckling on mixed pages. The revived `TestBinarize` and new dewarping unit tests (distortion model and dewarper) run alongside them
  3. A deliberately altered output fails its comparison under the per-kind tolerance (bi-level, grey/colour, geometry) and leaves the actual and diff images on disk. Baselines change only when the update flag is passed, and a manifest next to them records the toolchain versions they were made with
  4. The baseline commit comes before every strip or cleanup commit in git history, and its source tree matches the Phase 1 code
  5. A project with non-default parameters in every stage and special characters in names is written, read back and written again with identical results. A single `ctest` command runs the whole suite offscreen with isolated settings and passes. Any known defect that the unmodified code cannot pass is marked as an expected failure that names the requirement that will fix it

**Plans**: TBD
**UI hint**: no

### Phase 3: Qt 6 and macOS Only

**Goal**: The codebase contains only what the Mac build needs: Qt 6, macOS code paths, an English UI, no packaging and no CI. The baselines prove that output is unchanged.
**Depends on**: Phase 2
**Requirements**: STRIP-01, STRIP-02, STRIP-03, STRIP-04, STRIP-05, STRIP-06, STRIP-08
**Success Criteria** (what must be TRUE):
  1. Searching CMake files and `src/` for `Qt5` and `QT_VERSION` finds nothing, and the full `ctest` suite passes with every baseline unchanged
  2. No Windows or Linux code paths, build options or files remain (Windows branches, Wayland/xcb fallback, desktop and MIME files), and no packaging remains (Flatpak, DEB, NSIS/CPack, `build-*.sh`, MXE notes). A clean build directory still configures, builds and passes `ctest`
  3. `.github/` contains no workflows and no Dependabot configuration, so pushing to the fork starts no CI runs
  4. The app starts with an English UI, no translation files are built or loaded, and no translation targets remain in CMake
  5. Someone following only the README can build the app on the Mac and run the test suite

**Plans**: TBD
**UI hint**: no

### Phase 4: Retina Rendering

**Goal**: Everything the app draws on screen is sharp on a Retina display, and output files stay unchanged.
**Depends on**: Phase 3
**Requirements**: HIDPI-01, HIDPI-02, HIDPI-03, HIDPI-04, HIDPI-05, HIDPI-06
**Success Criteria** (what must be TRUE):
  1. The device pixel ratio that the build-directory binary reports on the Retina display has been measured and recorded, and it now reports 2.0. If the bare binary reported 1.0, an embedded `Info.plist` brings it to 2.0
  2. At fit, 100% and 200% zoom the page in the main view is as sharp as the same image opened in Preview, and the thumbnails are sharp as well
  3. Zone masks, page badges, handles and the stage-list animation look sharp, not pixelated
  4. Dragging the window from the Retina display to a non-Retina display and back re-renders the page view and thumbnails at the right resolution without a restart
  5. On Retina the colour picker returns the colour of the pixel under the cursor, an automated test covers the sampling path, and the full `ctest` suite passes with baselines unchanged

**Plans**: TBD
**UI hint**: no

### Phase 5: Mac Menus and Shortcuts

**Goal**: The app behaves like a Mac app. It has a standard application menu, Cmd shortcuts, Mac key names and native dialogs, and it works with MacBook keyboards and trackpads.
**Depends on**: Phase 4
**Requirements**: MAC-01, MAC-02, MAC-03, MAC-04, MAC-05, MAC-06, MAC-07, MAC-08, MAC-09, MAC-10, MAC-11
**Success Criteria** (what must be TRUE):
  1. The application menu holds About, Preferences and Quit, Cmd+, opens Preferences, and Settings is no longer in the Debug menu. The menu bar stays populated while a dialog is open
  2. Cmd+N, Cmd+O, Cmd+S, Cmd+W, Cmd+Q, Cmd+Shift+S (save under a new name), Cmd+= and Cmd+- (zoom) and Cmd+1 onwards (switch stage) all work. Tooltips, status tips and help text show ⌘ and ⌥, not "Ctrl" and "Alt"
  3. In the zone editor, Backspace or Delete removes the selected zone, and a content box can be created with a key the MacBook keyboard has. A right-click, a two-finger click and a Control-click each open the context menu
  4. When a batch run finishes, the Mac plays the system beep and no shell command runs
  5. Every open, save and folder dialog, including the relinking dialog, is the native macOS panel

**Plans**: TBD
**UI hint**: no

### Phase 6: Hardening and Cleanup

**Goal**: Known safety defects are fixed, the OpenGL viewer is gone and the riskiest oversized files are split, so later feature work is safe. Output stays unchanged.
**Depends on**: Phase 5 (menu work precedes the `MainWindow.cpp` split); Phase 4 (colour-picker test precedes OpenGL removal)
**Requirements**: SAFE-01, SAFE-02, SAFE-03, STRIP-07, CLEAN-01, CLEAN-02, CLEAN-03, CLEAN-04, CLEAN-05
**Success Criteria** (what must be TRUE):
  1. Opening a TIFF whose header claims absurd dimensions shows an error instead of attempting a huge allocation or crashing, and a test covers this
  2. A project whose page and folder names contain `&`, `<`, `"` or non-ASCII characters saves and reloads with every name intact, and the project round-trip test passes without expected failures
  3. Batch-processing a project of large scans runs with a worker-thread count capped by the Mac's RAM, and Activity Monitor shows no growth in swap during the run
  4. The OpenGL option is gone from Settings, the build no longer links the Qt OpenGL modules, and the colour-picker test still passes
  5. `OutputGenerator.cpp` and `MainWindow.cpp` are split into smaller files, "Save project as" exists as one shared flow, and no dead `#if 0` blocks or unused `boost/foreach.hpp` includes remain. The full `ctest` suite passes with baselines unchanged, and the Phase 5 menus and shortcuts behave exactly as before

**Plans**: TBD
**UI hint**: no

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4 → 5 → 6

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Deletion-Free macOS Build | 0/7 | Planned | - |
| 2. Golden Baselines | 0/TBD | Not started | - |
| 3. Qt 6 and macOS Only | 0/TBD | Not started | - |
| 4. Retina Rendering | 0/TBD | Not started | - |
| 5. Mac Menus and Shortcuts | 0/TBD | Not started | - |
| 6. Hardening and Cleanup | 0/TBD | Not started | - |

---
*Roadmap created: 2026-10-18*
