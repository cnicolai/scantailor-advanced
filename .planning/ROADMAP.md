# Roadmap: ScanTailor Advanced (personal macOS fork)

## Overview

This roadmap ports and hardens ScanTailor Advanced 1.2.1 for one Apple Silicon Mac. Each phase protects the next one. First the unmodified code is made to build and run natively, changing only what the build needs. Next a synthetic-image test suite records what that unchanged code produces. With those baselines in place, the Qt 5, Windows, Linux, packaging, CI and translation code is removed, and the baselines show that output did not change. After the strip, the view layer is made Mac-native: Retina rendering first, then menus and shortcuts. The last phase fixes known safety defects, removes the OpenGL viewer and splits the oversized files. Through all of this the six-stage pipeline keeps producing the same output.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Deletion-Free macOS Build** - Unmodified 1.2.1 builds as arm64 against Homebrew and runs a project end to end; existing test results and FP settings recorded (completed 2026-10-23)
- [x] **Phase 2: Golden Baselines** - Synthetic-image test suite and golden baselines recorded from the unmodified code before anything is removed (completed 2026-10-24)
- [x] **Phase 3: Qt 6 and macOS Only** - Qt 5, Windows, Linux, packaging, CI and translations removed, with baselines unchanged; README documents the Mac build (completed 2026-10-24)
- [x] **Phase 4: Retina Rendering** - Page view, thumbnails, overlays and colour picker are sharp and correct on Retina displays (completed 2026-10-25)
- [x] **Phase 5: Mac Menus and Shortcuts** - Standard application menu, Cmd shortcuts, Mac key names, native dialogs and Mac-friendly input (completed 2026-11-15)
- [x] **Phase 6: Hardening and Cleanup** - Safety defects fixed, OpenGL viewer removed, oversized files split and dead code removed, with baselines unchanged (completed 2026-11-17)
- [x] **Phase 06.1: Address tech debt: post-audit cleanup** (completed 2026-11-18)

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

**Plans:** 7/7 plans complete

Plans:
**Wave 1**
- [x] 01-01-PLAN.md — Tracer: Homebrew toolchain to a passing foundation_tests via CMakePresets.json; Clang warnings, frozen -ffp-contract=off, SANITIZE option, debug/asan presets

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 01-02-PLAN.md — First full release/debug build, error classes recorded, formatting decision, one-commit-per-class compile fixes
- [x] 01-05-PLAN.md — Scratchpad DPR/settings-path probe, synthetic smoke pages, D-02 permission with corrected INI paths

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 01-03-PLAN.md — Warning log and counts, arm64 and one-copy library checks, ctest records for Release and Debug, qt_tests offscreen decision
- [x] 01-04-PLAN.md — ASan/UBSan build, existing tests under sanitizers, findings recorded not fixed

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 01-06-PLAN.md — User-driven smoke walkthrough (one step per message), output and project checks, SMOKE-FINDINGS.md

**Wave 5** *(blocked on Wave 4 completion)*
- [x] 01-07-PLAN.md — Closing check of all five success criteria from fresh trees, criterion-5 diff audit, Phase 2 baseline commit

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

**Plans:** 14/14 plans complete

Plans:
**Wave 1**
- [x] 02-01-PLAN.md — Access question, then tracer: generated 300 dpi flat page through the six headless stage Tasks to a passing pipeline_tests (offscreen, isolated settings)

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 02-02-PLAN.md — Per-kind image comparator, text goldens, toolchain manifest, GoldenStore with UPDATE_GOLDENS, expected-failure trip-wire
- [x] 02-03-PLAN.md — Spread, mixed and curved fixtures with ground truth and determinism suite, truth metrics, multi-page/dewarp/MIXED pipeline smoke

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 02-04-PLAN.md — Revived TestBinarize (nine binarizers), TestScale settled in test code (D-17), new dewarping_tests
- [x] 02-05-PLAN.md — Flat-document truth checks and deskew sweep at 300/600 dpi, B/W and colour goldens
- [x] 02-06-PLAN.md — Book-spread split and AUTO-dewarp truth checks, spread and dewarp goldens
- [x] 02-07-PLAN.md — Mixed-page despeckle, colour and picture-zone truth checks, mixed goldens, AUTO-layout mis-cut marked
- [x] 02-08-PLAN.md — Project round trip with special-character names and non-default parameters in every stage, SAFE-02 disposition

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 02-09-PLAN.md — Measured and frozen comparator limits, permanent comparator self-test, isolated qt_tests, full ctest in Release and Debug

**Wave 5** *(blocked on Wave 4 completion)*
- [x] 02-10-PLAN.md — One ASan/UBSan run recorded, findings and backlog items, TEST-04 proof and baseline commit, summary for the user

**Gap closure G-02-3 (code review CR-01, WR-01 to WR-05)**

**Wave 1**
- [x] 02-11-PLAN.md — Access question for the review-fix builds, answer recorded next to the Phase 2 approval

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 02-12-PLAN.md — CR-01: generator refuses dpi values whose geometry does not fit, guarded raster writes, dpi sweep red then green under ASan, 75/600 dpi pixels pinned

**Wave 3** *(blocked on Wave 2 completion: same executable, build tree and branch as 02-12)*
- [x] 02-13-PLAN.md — WR-02 manifest marked handled only after it is written, WR-05 text goldens must parse back, WR-01 UPDATE_GOLDENS restored by an RAII guard

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 02-14-PLAN.md — WR-03 page-split layout type asserted in the round trip, WR-04 map-based spread cache, closing ctest 7/7 in Release and Debug, disposition updated

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

**Plans:** 9/9 plans complete

Plans:
**Wave 1**
- [x] 03-01-PLAN.md — Tracer: gate tooling (D-11 gate, build-command diff), offscreen ui_snapshot harness, before screenshots and counts, harness commit c00 gated

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 03-02-PLAN.md — Qt 5 removal in the Pitfall 7 order: C++ conditionals by unifdef plus D-15 manifest define (c01), Qt 6 only CMake with a 6.5 floor (c02), main.cpp and ImageViewBase blocks (c03)

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 03-03-PLAN.md — GitHub automation removed (c04), packaging and every install rule removed (c05)

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 03-04-PLAN.md — Windows code paths by unifdef (c06) with backlog item 999.8 for RelinkablePath, Windows build options, helpers and files (c07)

**Wave 5** *(blocked on Wave 4 completion)*
- [x] 03-05-PLAN.md — Linux code paths, GCC flag block and Linux resources in one commit (c08), portable mode (c09)

**Wave 6** *(blocked on Wave 5 completion)*
- [x] 03-06-PLAN.md — Translations removed atomically (c10), D-12 after checks: assertion run, static counts, screenshots and verdict

**Wave 7** *(blocked on Wave 6 completion)*
- [x] 03-07-PLAN.md — README top and English TESTING.md (c11), fresh-clone proof with README commands only (D-13)

**Wave 8** *(blocked on Wave 7 completion)*
- [x] 03-08-PLAN.md — One macos-asan run compared with Phase 2 (D-14), final leftover sweeps and gate-log completeness

**Wave 9** *(blocked on Wave 8 completion)*
- [x] 03-09-PLAN.md — Codebase maps and CLAUDE.md brought up to date (D-10), closing check with the summary for the user

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

**Plans:** 8/8 plans complete

Plans:
**Wave 1**
- [x] 04-01-PLAN.md — Tracer: HiDpi helpers, page-view HQ pixmap at device resolution, two-screen offscreen qt_tests (TestHiDpiQt), rebuild on display change

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 04-02-PLAN.md — Live Cocoa harness retina_check: HIDPI-01 ratio measured on the Studio Display, page view at fit/100%/200% against a device reference (Info.plist only if 1.0)

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 04-03-PLAN.md — Thumbnail cache keyed by ratio (Quality x DPR, q500 files, flush and stale-load guard), Quality default 250

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 04-04-PLAN.md — Thumbnails drawn at device resolution, ThumbnailDprTracker and MainWindow wiring for display changes

**Wave 5** *(blocked on Wave 4 completion)*
- [x] 04-05-PLAN.md — Picture-zone mask and stage-list animation at device resolution, rebuilt on ratio change

**Wave 6** *(blocked on Wave 5 completion)*
- [x] 04-06-PLAN.md — Icon pixmaps placed by logical size, swatch at view ratio, colour picker median over logical pixels with tests

**Wave 7** *(blocked on Wave 6 completion)*
- [x] 04-07-PLAN.md — Live checks of thumbnails, mask, badges, handles, stage animation, list decorations and colour picker on the Studio Display

**Wave 8** *(blocked on Wave 7 completion)*
- [x] 04-08-PLAN.md — TESTING.md, TESTX-03 traceability, static and sanitizer checks, closing record

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

**Plans:** 11/11 plans complete

Plans:
**Wave 1**
- [x] 05-01-PLAN.md — Tracer: app_gui library so qt_tests builds MainWindow, Settings… ⌘, in the application menu; explicit roles, Mac File and Tools menus, ⇧⌘S (TestMacQt)

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 05-02-PLAN.md — Live Cocoa harness mac_check (Objective-C++ probe): native application menu, app name (Info.plist if needed), empty Help menu, German-layout chord groups

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 05-03-PLAN.md — View menu: ⌘1–⌘6 for stages with Output tabs on the measured ⌥⌘ group in the same commit, ⌘+/⌘=/⌘- page zoom, thumbnail keys moved

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 05-04-PLAN.md — ⌫ and ⌦ remove zones and the content box, B creates it, fields keep their keys, right-click menus offscreen

**Wave 5** *(blocked on Wave 4 completion)*
- [x] 05-05-PLAN.md — Live window keys and zone keys on Cocoa with the German layout (answers the Backspace question)

**Wave 6** *(blocked on Wave 5 completion)*
- [x] 05-06-PLAN.md — Live context-menu triggers (right-click, Control-click; two-finger click recorded as a residual) and the 200 ms deferral decided by measurement

**Wave 7** *(blocked on Wave 6 completion)*
- [x] 05-07-PLAN.md — Mac key names in the multi-page hint of the nine apply dialogs

**Wave 8** *(blocked on Wave 7 completion)*
- [x] 05-08-PLAN.md — Mac key names in the Margins help, the main window and the C++ tips, with a permanent scan for PC key names

**Wave 9** *(blocked on Wave 8 completion)*
- [x] 05-09-PLAN.md — System beep instead of the shell command, native relinking panels, both pinned by source checks

**Wave 10** *(blocked on Wave 9 completion)*
- [x] 05-10-PLAN.md — Live menu bar during dialogs (fallback bar only if measured reduced) and native file panels, with a redirected home

**Wave 11** *(blocked on Wave 10 completion)*
- [x] 05-11-PLAN.md — TESTING.md live Mac check, closing runs, sanitizer and static checks, closing record

**Cross-cutting constraints:** <!-- gsd:generated -->
- Every commit of this plan passes gate.sh with Release and Debug 100% tests passed out of 7 and goldens unchanged (D-00a)

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

**Plans**: 16/16 plans complete

Plans:
**Wave 1**
- [x] 06-01-PLAN.md — Conventions, gate log and output-path allowlist; dead #if 0 blocks removed, proven by preprocessor equivalence (CLEAN-03)

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 06-02-PLAN.md — Unused Boost foreach includes removed from foundation, math and core (CLEAN-04, first half)

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 06-03-PLAN.md — Unused Boost foreach includes removed from imageproc, dewarping and app (CLEAN-04, second half)

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 06-04-PLAN.md — ImageSizeLimit (fewer than 2^29 px so RGB32 byte counts fit in int, at most 65535 px per side): forged TIFF refused before allocation, "too large" in the page view, exclusive TIFF boundaries incl. an RGB header at exactly 2^29 px, multi-page rule and no-allocation proof (SAFE-01)

**Wave 5** *(blocked on Wave 4 completion)*
- [x] 06-05-PLAN.md — Same limit for PNG and JPEG, Qt reader limit raised once, JPEG metadata stops before decompression, PNG metadata reports every claimed size (SAFE-01)

**Wave 6** *(blocked on Wave 5 completion)*
- [x] 06-06-PLAN.md — Oversized files marked in the project file dialog and the insert flow, thumbnails refuse, sanitizer run, int-arithmetic audit, SAFE-01 record with the ceiling choice against D-01's 1 GP example (SAFE-01)

**Wave 7** *(blocked on Wave 6 completion)*
- [x] 06-07-PLAN.md — Control characters in names escaped only when needed, unusual-name matrix, backslash per D-10, FIXME resolved (SAFE-02)

**Wave 8** *(blocked on Wave 7 completion)*
- [x] 06-08-PLAN.md — Worker threads capped by RAM and the largest page through one cap function (batchMemoryCap) at batch start; cap arithmetic incl. the D-08 page pinned (SAFE-03)

**Wave 9** *(blocked on Wave 8 completion)*
- [x] 06-09-PLAN.md — Slider tooltip with the memory limit, shared setting reader, opt-in peak and batch memory probes on the real batch path (SAFE-03)

**Wave 10** *(blocked on Wave 9 completion)*
- [x] 06-10-PLAN.md — OpenGL viewer option removed from Settings, settings API, viewer and colour picker; picker tests unchanged (STRIP-07)

**Wave 11** *(blocked on Wave 10 completion)*
- [x] 06-11-PLAN.md — OpenGLSupport and the Qt OpenGL modules out of the build; scan test, otool proof, STRIP-07 record (STRIP-07)

**Wave 12** *(blocked on Wave 11 completion)*
- [x] 06-12-PLAN.md — One shared Save Project As flow for the main window and the out-of-memory dialog (CLEAN-05)

**Wave 13** *(blocked on Wave 12 completion)*
- [x] 06-13-PLAN.md — OutputGenerator split, first step: digest probe and move check, Processor class, shared helpers and no-dewarping path moved, proven by chunk multiset and output digests (CLEAN-01)

**Wave 14** *(blocked on Wave 13 completion)*
- [x] 06-14-PLAN.md — OutputGenerator split, second step: dewarping and binarization paths moved, whole split proven by chunk multiset and output digests, CLEAN-01 record (CLEAN-01)

**Wave 15** *(blocked on Wave 14 completion)*
- [x] 06-15-PLAN.md — MainWindow.cpp split into same-class translation units (batch, actions, project, pages), beep scan follows the move (CLEAN-02)

**Wave 16** *(blocked on Wave 15 completion)*
- [x] 06-16-PLAN.md — TESTING.md probes, automated closing checks and sanitizers, then on the user's go the D-08 batch through the worker pool with the real cap and the live mac_check, closing record

**Cross-cutting constraints:** <!-- gsd:generated -->
- The commit passes gate.sh: Release and Debug report 100% tests passed out of 7, goldens unchanged against a796c2ae; the build-command diff is empty (include changes alter dependency files, not commands) or the gate-log row explains every line (D-00a)
- Every commit of this plan passes gate.sh with Release and Debug 100% tests passed out of 7 and goldens unchanged (D-00a)
- A stored settings/enable_opengl value is neither read, deleted nor rewritten (D-00c)

**UI hint**: no

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4 → 5 → 6

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Deletion-Free macOS Build | 7/7 | Complete    | 2026-10-23 |
| 2. Golden Baselines | 14/14 | Complete    | 2026-10-24 |
| 3. Qt 6 and macOS Only | 9/9 | Complete    | 2026-10-24 |
| 4. Retina Rendering | 8/8 | Complete    | 2026-10-25 |
| 5. Mac Menus and Shortcuts | 11/11 | Complete    | 2026-11-15 |
| 6. Hardening and Cleanup | 16/16 | Complete    | 2026-11-17 |

---
*Roadmap created: 2026-10-18*

## Backlog

### Phase 06.1: Address tech debt: post-audit cleanup (INSERTED)

**Goal:** Every open v1.0 milestone-audit tech-debt item and every review finding of Phases 1 to 6 that can be closed without changing output is closed: no finding in any `*-REVIEW-DISPOSITION.md` stays `open` (each is `fixed`, or `skipped`/`deferred` with a one-line reason), comments under `src/` carry no bare planning IDs (each explains its reason in plain words), and every commit passes the Phase 3 gate with goldens unchanged.
**Requirements**: TBD
**Depends on:** Phase 6
**Plans:** 12/12 plans complete

Plans:
**Wave 1**
- [x] 06.1-01-PLAN.md — Gate reference, gate log, conventions and finding ledger; OpenGL lines out of the project instructions, pinned by the scan test (D-15); token comparison script

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 06.1-02-PLAN.md — Bookkeeping (D-16), moot findings closed with evidence, won't-fix findings with the CONTEXT reasons (D-02, D-09), TESTING.md preset differences (Phase 1 IN-03); disposition checker

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 06.1-03-PLAN.md — Phase 3 IN-01/02/03/07: unused includes, dead LanguageChange handler, Windows comments, unreachable UNC branch pinned by TestRelinkablePath (D-03), dead remove_definitions and ignore rules

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 06.1-04-PLAN.md — Phase 2 IN-01/02/03/04/06: cache test that fails for a reallocating container (D-04), refused dpi values, rounding helper, unused include

**Wave 5** *(blocked on Wave 4 completion)*
- [x] 06.1-05-PLAN.md — Phase 4 IN-01/02/03 and Phase 5 IN-01/06: qt_tests environment, diagnostics wording, tooltips, polled zone-menu test, named scan floors

**Wave 6** *(blocked on Wave 5 completion)*
- [x] 06.1-06-PLAN.md — Own-file read ceiling (D-11), unused unlimited loader deleted and own-file loader renamed (D-12), forged cached output regenerated in a pipeline test, Phase 6 IN-02/03 docs

**Wave 7** *(blocked on Wave 6 completion)*
- [x] 06.1-07-PLAN.md — Canonical harness output guard and ui_snapshot argument parser (D-08), empty batch stops cleanly (D-14)

**Wave 8** *(blocked on Wave 7 completion)*
- [x] 06.1-08-PLAN.md — Include lists of the split OutputGenerator and MainWindow files trimmed, output object code proven unchanged (Phase 6 IN-01)

**Wave 9** *(blocked on Wave 8 completion)*
- [x] 06.1-09-PLAN.md — D-05 comment rewrite, build files and app/harness sources; comment-only proof script; closes Phase 4 IN-04

**Wave 10** *(blocked on Wave 9 completion)*
- [x] 06.1-10-PLAN.md — D-05 comment rewrite, remaining test-side files (CONTEXT regex only); closes Phase 2 IN-05

**Wave 11** *(blocked on Wave 10 completion)*
- [x] 06.1-11-PLAN.md — UBSan fixes, strict macos-asan and new macos-asan-collect presets (D-10), SANITIZE validation (D-06), small CMake findings (P1 IN-02/IN-04, P3 WR-02); full strict ASan run

**Wave 12** *(blocked on Wave 11 completion)*
- [x] 06.1-12-PLAN.md — fp-contract measurement (D-13), Boost config targets deferred (D-07, user decision 2026-11-17), closing checks and last stale notes

**Cross-cutting constraints:** <!-- gsd:generated -->
- Both commits pass the gate with empty command diffs and unchanged goldens (D-00a)
- Each of the three commits passes the gate with an empty command diff and unchanged goldens (D-00a)
- Both commits pass the gate with diff_lines=0 and unchanged goldens (D-00a)

### Phase 999.1: Global binarizers reject Format_Grayscale8 input (BACKLOG)

**Goal:** `BinaryImage::BinaryImage(const QImage&, BinaryThreshold)` (`src/imageproc/BinaryImage.cpp:142`), used by `binarizeOtsu` and `binarizeMokji`, throws `std::runtime_error: Unsupported QImage format` for `Format_Grayscale8` input; the check requires no exception (measured: throws, bound: no exception, FLAT page, 300 dpi, seed 7). No production caller passes this format, so no requirement exists. When fixed, remove the `XFAIL_CHECK` tagged `global thresholds reject Format_Grayscale8 (std::runtime_error): no requirement` in `src/imageproc/tests/TestBinarize.cpp`. Finding F-04-1 in `.planning/phases/02-golden-baselines/02-FINDINGS.md`.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.2: Distortion model can leave out the first or last text line (BACKLOG)

**Goal:** `DistortionModelBuilder::tryBuildModel` builds a model whose top curve follows the second text line of the generated curved page (seed 1, 300 dpi) although `TextLineTracer` traced the first: the top curve lies 64.40 px from the first line against a bound of 55 px (one line pitch), and 5.73 px from the second. The production dewarping stage adds the page edges afterwards and falls back to a trivial model, which this unit test does not run. No requirement exists. When fixed, remove the `XFAIL_CHECK` tagged `model top curve skips the first text line of the curved page (seed 1): no requirement` in `src/dewarping/tests/TestTextLineTracer.cpp`. Finding F-04-2 in `.planning/phases/02-golden-baselines/02-FINDINGS.md`.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.3: Output of the 600 dpi flat page keeps about 0.1 degrees of residual skew (BACKLOG)

**Goal:** On the generated flat page (skew 1.5 degrees, black-and-white output) the 600 dpi input leaves a residual rotation of about 0.08 to 0.12 degrees that the stored deskew angle (exactly -1.5 degrees) does not show: binarization mismatch against the truth mask 0.0829 against a bound of 0.005 (0 at 300 dpi). The cause inside the output or deskew transformation was not analysed. No requirement exists. When fixed, remove the `XFAIL_CHECK` tagged `output of the 600 dpi flat page keeps about 0.1 deg residual skew, binarization mismatch 8.3% against 0.5% (F-05-1): no requirement` in `src/pipeline_tests/TestFlatDocument.cpp` and re-record the golden `flat_600_bw` (it records this behaviour). Finding F-05-1 in `.planning/phases/02-golden-baselines/02-FINDINGS.md`.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.4: page_split puts the cutter 28 to 30 px from the gutter centre (BACKLOG)

**Goal:** On the generated book spread (seed 1, skew 1.0 degree, AUTO layout) the page_split cutter lies 30.376 px (300 dpi) and 55.736 px (600 dpi, 27.87 px in 300 dpi pixels) from the gutter centre at mid-height against a bound of 2 px; it stays inside the gutter at every sample (margin 80.5 px at 300 dpi), so no content is cut. The cause was not analysed. No requirement exists. When fixed, remove the four `XFAIL_CHECK` tags `page_split cutter <30.4|55.7> px from the gutter centre at mid-height on the <300|600> dpi spread, bound 2 px (F-06-1): no requirement` in `src/pipeline_tests/TestBookSpread.cpp` and re-record the spread goldens. Finding F-06-1 in `.planning/phases/02-golden-baselines/02-FINDINGS.md`.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.5: Test support: straightnessResidual cannot measure the curved book page (BACKLOG)

**Goal:** Limitation of `src/test_support`, not of the application: `straightnessResidual` finds no valid strip (0 of 8) on the output of the curved page without dewarping and has a 4 to 9 px noise floor from glyph content (dots of `i` and `j`, ascenders, descenders) on the AUTO output, so `src/pipeline_tests/TestBookSpread.cpp` measures line offsets against the truth mask (noise floor 0.583 px on a straight page) and reports the plan's measure as reference only. Goal: give `src/test_support` a glyph-independent straightness measure. No requirement exists and no expected-failure mark exists to remove. Finding F-06-2 in `.planning/phases/02-golden-baselines/02-FINDINGS.md`.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.6: page_split AUTO cuts the mixed page and keeps only a fragment of its picture (BACKLOG)

**Goal:** On the generated mixed page (300 dpi, seed 1, skew 0) page_split AUTO chooses layout type 1 (SINGLE_PAGE_CUT) with two cutters at x = 588..603 and 893..903, and the output keeps an ink box of 386 x 542 px against 1600 x 1610 px for text and picture (0.241 and 0.337 of the box, bound 0.9), dropping the text column. The coverage scenarios therefore force SINGLE_PAGE_UNCUT. The cause was not analysed. No requirement exists. When fixed, remove the `XFAIL_CHECK` tagged `page_split AUTO cuts the mixed page and keeps only a fragment of its picture, dropping the text: no requirement` in `src/pipeline_tests/TestMixedPage.cpp` and consider the forced layout in the mixed scenarios. Finding F-07-1 in `.planning/phases/02-golden-baselines/02-FINDINGS.md`.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.7: A layout type from the default profile is not written to the project file until page_split has run (BACKLOG)

**Goal:** After `loadDefaultSettings` alone (profile with `SINGLE_PAGE_UNCUT`) the written element is `<page-split defaultLayoutType="auto-detect"/>` with no `<image>` child, identical to the default-profile project: `page_split::Filter::writeImageSettings` writes an `<image>` element only when the page has params, and `defaultLayoutType` is a `Settings`-level value that `DefaultParams` never sets. The application loads defaults at processing time, so a project saved before page_split has run carries no layout type for its pages. Possibly by design; the decision is open. No requirement exists and no expected-failure mark exists (the test runs the stage first). Finding F-08-1 in `.planning/phases/02-golden-baselines/02-FINDINGS.md`.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)

### Phase 999.8: RelinkablePath::normalize turns a backslash in a macOS file name into a path separator (BACKLOG)

**Goal:** `RelinkablePath::normalize` (`src/core/RelinkablePath.cpp`) replaces every backslash by a slash before splitting the path and, when resolving `..`, refuses a previous component that ends in `:` (a Windows drive letter). On macOS a backslash is a legal file-name character, so by reading the code `normalize("/Users/me/scan\\01.tif")` returns `/Users/me/scan/01.tif` (not executed). Phase 3 keeps this logic because it runs on the Mac and removing it would change behaviour (D-03). Fix it with a test in a later phase; decide first whether project files written by Windows builds must still be read. No requirement exists. Source: Phase 3 CONTEXT D-03.
**Requirements:** TBD
**Plans:** 0 plans

Plans:
- [ ] TBD (promote with /gsd-review-backlog when ready)
