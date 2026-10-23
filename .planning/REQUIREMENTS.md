# Requirements: ScanTailor Advanced (personal macOS fork)

**Defined:** 2026-10-18
**Core Value:** The app builds from source and works reliably for daily scanning work on a current Apple Silicon Mac, with a test suite that makes later changes safe.

## v1 Requirements

Requirements for the macOS milestone. Each maps to roadmap phases.

### Build & Run

- [x] **BUILD-01**: One CMake preset configures and builds an arm64 binary against Homebrew Qt 6, Boost, jpeg-turbo, libpng and libtiff
- [x] **BUILD-02**: The built binary links exactly one copy each of libjpeg, libpng and libtiff (verified with `otool -L`)
- [x] **BUILD-03**: The app launches from the build directory, opens a project, processes it through all six stages and writes output
- [x] **BUILD-04**: The existing test suite runs under `ctest` on the Mac, and its initial pass/fail state is recorded before any change
- [x] **BUILD-05**: Apple Clang builds with warnings enabled (equivalents of `-Wall -Wextra`)
- [x] **BUILD-06**: A CMake option builds with AddressSanitizer and UndefinedBehaviorSanitizer
- [x] **BUILD-07**: Floating-point compiler settings (`-ffp-contract`) are fixed and recorded before any baseline images are captured

### Tests

- [x] **TEST-01**: A seeded generator produces synthetic flat-document, book-spread and mixed text-and-image pages with known ground truth, without font rendering or `rand()`
- [x] **TEST-02**: An image comparator applies per-kind tolerances (bi-level, grey/colour, geometry); a failure writes the actual and diff images
- [x] **TEST-03**: Baseline (golden) images change only through an explicit update flag, and a manifest records the toolchain versions they were made with
- [x] **TEST-04**: Baselines are recorded from the unmodified codebase before any Qt 5/platform removal or refactor
- [x] **TEST-05**: Flat-document tests check the deskew angle, the content box and binarization accuracy against ground truth
- [x] **TEST-06**: Book-page tests check the page-split position and that dewarping straightens synthetic curved text lines
- [x] **TEST-07**: Mixed-page tests cover picture zones, colour/greyscale output and despeckling
- [x] **TEST-08**: The disabled binarization test (`TestBinarize`) runs again, with synthetic inputs
- [x] **TEST-09**: Dewarping has its own unit tests (distortion model and dewarper)
- [x] **TEST-10**: A project-file round trip (write → read → write gives identical files) uses non-default parameters in every stage and special characters in names
- [x] **TEST-11**: The whole suite runs with one `ctest` command (offscreen Qt, isolated settings) and passes

### macOS-only Cleanup

- [ ] **STRIP-01**: Only Qt 6 is supported; Qt 5 CMake branches and `QT_VERSION` `#if` blocks are removed with baseline images unchanged
- [ ] **STRIP-02**: Windows-specific code paths and build options are removed
- [ ] **STRIP-03**: Linux-specific code (Wayland/xcb fallback, desktop and MIME files) is removed
- [ ] **STRIP-04**: Packaging is removed: Flatpak, DEB, NSIS/CPack, `build-*.sh` scripts and MXE notes
- [ ] **STRIP-05**: All GitHub Actions workflows and the Dependabot configuration are removed
- [ ] **STRIP-06**: Translations are removed; the UI is English only
- [ ] **STRIP-07**: The OpenGL viewer option and its code are removed, after a test covers the colour-picker path
- [ ] **STRIP-08**: The README documents the macOS build and test commands

### Retina

- [ ] **HIDPI-01**: The device pixel ratio of the bare build-dir binary is measured on a Retina display; if it reports 1.0, an embedded `Info.plist` makes it 2.0
- [ ] **HIDPI-02**: The main page view renders the page sharply at 2× at every zoom level
- [ ] **HIDPI-03**: Thumbnails render sharply, with the thumbnail cache keyed by device pixel ratio
- [ ] **HIDPI-04**: Overlay pixmaps (zone masks, page badges, handles, stage-list animation) render sharply
- [ ] **HIDPI-05**: Moving a window between a Retina and a non-Retina display re-renders at the right resolution
- [ ] **HIDPI-06**: The colour picker samples the correct pixel on Retina

### Mac Shortcuts & Menus

- [ ] **MAC-01**: About, Preferences and Quit appear in the application menu (menu roles); Settings moves out of the Debug menu
- [ ] **MAC-02**: Cmd+, opens Preferences
- [ ] **MAC-03**: Cmd+Shift+S saves the project under a new name
- [ ] **MAC-04**: Cmd+= and Cmd+- zoom the page view
- [ ] **MAC-05**: Existing shortcuts work with Cmd: New, Open, Save, Close, Quit, and Cmd+1… to switch stages
- [ ] **MAC-06**: Tooltips, status tips and help text show Mac key names (⌘, ⌥) instead of "Ctrl"/"Alt"
- [ ] **MAC-07**: In the zone editor, Backspace or Delete removes a zone, and creating a content box uses a key a MacBook has
- [ ] **MAC-08**: Context menus open with a right-click, a two-finger click and a Control-click
- [ ] **MAC-09**: The menu bar stays populated while dialogs are open
- [ ] **MAC-10**: The batch-finished alarm uses the system beep instead of a shell command
- [ ] **MAC-11**: Native macOS file dialogs are used everywhere, including the relinking dialog

### Safety

- [ ] **SAFE-01**: A TIFF file with absurd dimensions is rejected with an error instead of triggering a huge allocation
- [ ] **SAFE-02**: Names containing `&`, `<`, `"` or non-ASCII characters survive saving and reloading a project (resolves the `XmlUnmarshaller` FIXME)
- [ ] **SAFE-03**: The number of worker threads is capped by available RAM, so large scans don't push macOS into swap

### Code Cleanup

- [ ] **CLEAN-01**: `OutputGenerator.cpp` is split into topic files by pure moves, with baseline images unchanged
- [ ] **CLEAN-02**: `MainWindow.cpp` is split (menu/action wiring and batch processing move out) with no change in behaviour
- [ ] **CLEAN-03**: Dead `#if 0` blocks are removed
- [ ] **CLEAN-04**: Unused `boost/foreach.hpp` includes are removed
- [ ] **CLEAN-05**: The duplicated "Save project as" flow becomes one shared helper

## v2 Requirements

Deferred. Tracked but not in the current roadmap.

### Mac Polish

- **POLISH-01**: Dock shows an app icon instead of the generic one
- **POLISH-02**: File > Open Recent lists recent projects
- **POLISH-03**: Title bar shows the document proxy icon and the unsaved-changes dot
- **POLISH-04**: Dock icon bounces when a batch run finishes in the background
- **POLISH-05**: Self-contained `.app` bundle

### Tests

- **TESTX-01**: Metamorphic tests (transformed input gives a correspondingly transformed result)
- **TESTX-02**: Default-parameter profiles round-trip unchanged
- **TESTX-03**: Automated HiDPI render test checks view buffers are built at 2×

### Cleanup

- **CLEANX-01**: Boost.Lambda replaced with standard lambdas, file by file under tests
- **CLEANX-02**: Deprecated libtiff typedefs (`tsize_t`, `tdata_t`) replaced
- **CLEANX-03**: `ThumbnailSequence.cpp` and `ContentBoxFinder.cpp` split

## Out of Scope

| Feature | Reason |
|---------|--------|
| New features | None planned; revisit after the macOS milestone |
| Upstream contributions / compatibility | Personal fork |
| Linux and Windows support | macOS-only by choice |
| Intel / universal builds | Apple Silicon only |
| DMG, code signing, notarization | Running from the build directory is enough |
| CI of any kind | User wants local builds and tests only |
| Real personal scans as fixtures | Synthetic images only |
| Following macOS system dark mode | Not requested |
| Trackpad gestures | Not requested |
| `AA_MacDontSwapCtrlAndMeta`, `QT_SCALE_FACTOR` as a Retina fix, byte-exact goldens | Anti-patterns identified in research |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| BUILD-01 | Phase 1 | Complete |
| BUILD-02 | Phase 1 | Complete |
| BUILD-03 | Phase 1 | Complete |
| BUILD-04 | Phase 1 | Complete |
| BUILD-05 | Phase 1 | Complete |
| BUILD-06 | Phase 1 | Complete |
| BUILD-07 | Phase 1 | Complete |
| TEST-01 | Phase 2 | Complete |
| TEST-02 | Phase 2 | Complete |
| TEST-03 | Phase 2 | Complete |
| TEST-04 | Phase 2 | Complete |
| TEST-05 | Phase 2 | Complete |
| TEST-06 | Phase 2 | Complete |
| TEST-07 | Phase 2 | Complete |
| TEST-08 | Phase 2 | Complete |
| TEST-09 | Phase 2 | Complete |
| TEST-10 | Phase 2 | Complete |
| TEST-11 | Phase 2 | Complete |
| STRIP-01 | Phase 3 | Pending |
| STRIP-02 | Phase 3 | Pending |
| STRIP-03 | Phase 3 | Pending |
| STRIP-04 | Phase 3 | Pending |
| STRIP-05 | Phase 3 | Pending |
| STRIP-06 | Phase 3 | Pending |
| STRIP-07 | Phase 6 | Pending |
| STRIP-08 | Phase 3 | Pending |
| HIDPI-01 | Phase 4 | Pending |
| HIDPI-02 | Phase 4 | Pending |
| HIDPI-03 | Phase 4 | Pending |
| HIDPI-04 | Phase 4 | Pending |
| HIDPI-05 | Phase 4 | Pending |
| HIDPI-06 | Phase 4 | Pending |
| MAC-01 | Phase 5 | Pending |
| MAC-02 | Phase 5 | Pending |
| MAC-03 | Phase 5 | Pending |
| MAC-04 | Phase 5 | Pending |
| MAC-05 | Phase 5 | Pending |
| MAC-06 | Phase 5 | Pending |
| MAC-07 | Phase 5 | Pending |
| MAC-08 | Phase 5 | Pending |
| MAC-09 | Phase 5 | Pending |
| MAC-10 | Phase 5 | Pending |
| MAC-11 | Phase 5 | Pending |
| SAFE-01 | Phase 6 | Pending |
| SAFE-02 | Phase 6 | Pending |
| SAFE-03 | Phase 6 | Pending |
| CLEAN-01 | Phase 6 | Pending |
| CLEAN-02 | Phase 6 | Pending |
| CLEAN-03 | Phase 6 | Pending |
| CLEAN-04 | Phase 6 | Pending |
| CLEAN-05 | Phase 6 | Pending |

**Coverage:**
- v1 requirements: 51 total
- Mapped to phases: 51
- Unmapped: 0

---
*Requirements defined: 2026-10-18*
*Last updated: 2026-10-18 after roadmap creation*
