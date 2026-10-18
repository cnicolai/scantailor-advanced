# Pitfalls Research

**Domain:** Porting a legacy C++17/Qt/Boost/libtiff desktop image-processing app (ScanTailor Advanced 1.2.1) to macOS Apple Silicon, dropping Qt 5, and adding regression tests before refactoring
**Researched:** 2026-10-18
**Confidence:** MEDIUM overall. Findings tagged `[code]` were confirmed by grepping `src/` and `CMakeLists.txt` (HIGH). Findings tagged `[web]` were confirmed by a web source (MEDIUM). Findings tagged `[exp]` come from general knowledge of Qt/macOS/libc++ ports and must be verified on the first build (MEDIUM to LOW). The research-plan/cache seam was not used; built-in WebSearch was queried directly.

## Suggested phase labels used in this file

The roadmap may name phases differently. These labels are used only so each pitfall can point at a place to be fixed.

| Label | Content |
|-------|---------|
| **P1 Bring-up** | First green macOS arm64 build and run from the build dir, with minimal edits |
| **P2 Strip** | Remove Qt 5, Windows/Linux code and packaging, CI |
| **P3 Baseline** | Test harness, synthetic fixtures, golden baselines, sanitizer run |
| **P4 HiDPI** | Retina rendering of views, thumbnails, icons |
| **P5 Mac UX** | Cmd shortcuts, native menu bar and roles, Mac key conventions |
| **P6 Debt** | `#if 0` cleanup, Boost modernisation, file splits, security fixes |

**Ordering advice that follows from the pitfalls below:** P1, then P3 (at least the baseline half), then P2, then P4/P5, then P6. Deleting code before a baseline exists is how "output changed unintentionally" happens. See Pitfalls 1 and 2.

---

## Critical Pitfalls

### Pitfall 1: Capturing golden images after the code has already changed

**What goes wrong:**
Goldens are generated from the working tree after the Qt 5 strip, the HiDPI work or a refactor. The tests then lock in whatever the changed code produces, including any accidental regression. The PROJECT.md constraint "output for existing settings should not change unintentionally" is then unenforceable.

**Why it happens:**
Test-writing is scheduled as a late phase. There is no pre-change oracle for `OutputGenerator.cpp` (2814 lines, no tests), so the first run is treated as truth.

**How to avoid:**
- Treat the first green macOS build of unmodified upstream code (commit `5eaac18` plus only the edits needed to compile) as the oracle. Run the pipeline on the synthetic fixtures and commit the goldens in a separate commit before any strip, refactor or HiDPI change.
- Record the toolchain in the same commit: `brew list --versions qt boost libtiff jpeg-turbo libpng`, `xcodebuild -version`, `cmake --version`. Goldens are only comparable under a known toolchain.
- Make every later phase's success criterion "baseline tests still pass unchanged". A golden update must be its own commit with a written reason.
- Keep P1 deletion-free. The CMake already prefers Qt 6 when found (`find_package(Qt6 ...)` runs first), so bring-up needs no Qt 5 removal. Mixing bring-up errors with deletion errors makes root causes hard to find.

**Warning signs:**
- A plan that has "write tests" after "remove Qt 5" or "split OutputGenerator".
- A golden-update commit that also touches `src/`.
- Goldens whose generation date is later than the first refactor commit.

**Phase to address:** P1 then P3 (baseline immediately after the first green build, before P2, P4, P5, P6).

---

### Pitfall 2: Golden-image comparison that is either byte-exact or blanket-tolerant

**What goes wrong:**
Two opposite failures. (a) Byte-exact compare of output files fails after any `brew upgrade` (Qt, libtiff, libjpeg-turbo), Xcode update or compiler flag change, so the suite becomes noise and is switched off. (b) One global tolerance (for example "max abs diff 5") hides real regressions in binarization or zone fills.

**Why it happens:**
Floating-point and library behaviour is not stable across the things that change on a Homebrew machine:
- arm64 Clang contracts `a*b+c` into fused multiply-add by default, x86 builds without FMA do not. The same C++ gives slightly different `float` results. Fixing this with `-ffp-contract=off` is possible but does not remove all differences (library maths, SIMD rounding). `[web]`
- `Binarize.cpp` computes Sauvola/Wolf thresholds in `double` and `float` (`std::vector<float> means/deviations`, line ~165) and then compares against pixel values. A last-bit change flips isolated pixels. `[code]`
- The output stage uses `QPainter` with antialiasing for fill and picture zones (`OutputGenerator.cpp` lines ~600-700) and `QImage::convertToFormat`. Qt rasteriser output differs between Qt minor versions. `[code]`
- Dewarping uses iterative optimisation (`src/math/adiff`, `spfit`). Small float differences can change the iteration path and the fitted curve. `[code]` (inferred from module layout)
- TIFF/PNG files contain encoder-specific bytes (tags, software string, compression details), so file bytes differ even when pixels are identical. `[exp]`

**How to avoid:**
Use a tolerance policy per kind of output, written down once in the test helper and not overridden per test without a comment:

| Output kind | Compare how |
|-------------|-------------|
| Pure integer paths (BinaryImage morphology, rotation by 90, seed fill, connected components) | Exact pixel equality |
| Binarized output | Hamming distance as a fraction of pixels (start at 0.01% of pixels, tighten once measured), plus exact image size and DPI |
| Gray/colour output | Mean absolute error and max error, for example MAE <= 0.5 and max <= 8 on 8-bit, measured first on two builds and set just above the observed spread |
| Geometry decisions (skew angle, page split line, content box, margins, output size) | Assert on the numbers with a tolerance in physical units (angle +-0.05 degrees, positions +-2 px), not on pixels |
| Files | Decode and compare pixels, DPI and compression tag. Never compare file bytes |

- Prefer asserting on stage decisions (the outputs of `SkewFinder`, `ContentBoxFinder`, `PageLayoutEstimator`) over asserting on final pixels wherever a decision exists. Decisions are far more stable than rendered pixels.
- Write a failing-diff helper that saves actual/expected/diff PNGs into the build dir so a failure is diagnosable in one step.
- Do not add `-ffast-math`, `-Ofast` or `-march=native`-style flags for Clang. They change results and make goldens toolchain-specific. `CMakeLists.txt` currently adds `-march=native` only for GCC (`CMAKE_COMPILER_IS_GNUCC`), so Apple Clang is unaffected; do not "fix" that asymmetry.

**Warning signs:**
- Golden failures right after `brew upgrade` with no source change.
- A tolerance constant that keeps growing in review.
- Tests comparing `QFile::readAll()` of two TIFFs.

**Phase to address:** P3.

---

### Pitfall 3: Synthetic fixtures that are not actually deterministic or not portable

**What goes wrong:**
Fixtures drawn with `QPainter::drawText` depend on installed fonts and macOS text rendering (CoreText), both of which change with macOS releases. Fixtures drawn with antialiased `QPainter` shapes depend on the Qt version. Fixtures from `rand()` depend on the C library.

**Why it happens:**
The existing helper `src/imageproc/tests/Utils.cpp` uses `rand()` (`randomBinaryImage`, `randomGrayImage`); `rand()` sequences differ between libc implementations, and `std::uniform_*_distribution` outputs differ between libc++ and libstdc++. `[code]` Developers copy this pattern for the new generator. A synthetic "book page" without text is also not a realistic trigger for the text-line based dewarping and skew code, so people reach for real fonts.

**How to avoid:**
- Generate fixtures with integer-only pixel operations (set pixels, filled rectangles, 1-px lines, a hand-written bitmap glyph set for "text-like" blocks) and your own tiny PRNG (for example xorshift32) seeded explicitly. No `QPainter` text, no `rand()`, no `<random>` distributions.
- For rotated or curved content, compute pixel coverage with your own formula, or run the project's own `imageproc::transform` once and commit the result.
- Commit the generated fixture PNGs plus the generator, and add a test that regenerates and compares to the committed PNG exactly. Then fixture drift is caught separately from pipeline drift.
- Use lossless formats (PNG, TIFF) for inputs. If a JPEG load test is needed, compare with a small tolerance because libjpeg-turbo upgrades can change decoding slightly. `[exp]`
- Include fixtures for each use case named in PROJECT.md: a two-page book spread with a gutter shadow, a flat single page with skew, and a mixed text-plus-picture page. Add at least one with a known skew (for example 1.5 degrees) so deskew tests can assert the angle.
- Keep sizes small (for example 600 x 800 at 300 DPI metadata) so the full-pipeline tests run in seconds. The DPI metadata matters: the pipeline's units come from `Dpi`, not from pixel size.

**Warning signs:**
- A fixture generator that includes `<QFont>` or `<QFontMetrics>`.
- Test expectations that change when a different macOS version is installed.
- Fixture PNGs committed without the script that made them.

**Phase to address:** P3.

---

### Pitfall 4: Threading nondeterminism and background execution in "end-to-end" tests

**What goes wrong:**
Tests run the pipeline through `WorkerThreadPool`/`BackgroundExecutor`, so results arrive as posted events in arbitrary order, with `QThread::idealThreadCount()` workers. Tests become flaky, or pass only because of timing.

**Why it happens:**
`WorkerThreadPool` sets max threads from `QThread::idealThreadCount()` (all performance and efficiency cores on Apple Silicon) and delivers results with `QCoreApplication::postEvent`. `BackgroundExecutor` is a `QThread` with an event loop and `wait()` in its destructor. `[code]` Output generation itself is a synchronous function once called; only the dispatch is asynchronous.

**How to avoid:**
- Test at the level of the synchronous task or generator call (for example `OutputGenerator::process`, the filter `Task::process`), not through the thread pool or `MainWindow`.
- If an integration test needs the pool, set `settings/batch_processing_threads` to 1 for the test and wait on a signal with `QSignalSpy::wait`, never `sleep`.
- Do not assert on completion order across pages.
- Check that results do not depend on iteration order of unordered or pointer-keyed containers (62 uses of `std::sort`/`unordered_*` in `src/**/*.cpp`). Sort before comparing anything produced from a set (connected components, zone lists). `[code]`
- Run the full suite several times in a loop (for example 20 times) once, and under ThreadSanitizer for the code paths that use `QWaitCondition` (`RelinkingModel.cpp`, `ThumbnailPixmapCache.cpp`, `BackgroundExecutor.cpp`) before touching them.

**Warning signs:**
- A test that passes alone and fails in `ctest -j`.
- `QTest::qWait` or `sleep` in test code.

**Phase to address:** P3 (harness) and P6 (before changing the hand-written wait code).

---

### Pitfall 5: Test runner pitfalls on macOS (GUI session, working directory, Boost.Test linking)

**What goes wrong:**
`qt_tests` creates a `QApplication` and widgets (`src/qt_tests/main.cpp`), so on macOS it uses the Cocoa plugin and needs a window server. It fails over SSH or in a headless shell, or flashes windows during `ctest`. `TestBinarize.cpp` is `#if 0` and reads `"test.png"` from the working directory, so simply re-enabling it fails. Boost.Test link mode mismatch gives `undefined symbol main` or duplicate-main errors.

**Why it happens:**
- `add_test(NAME qt_tests COMMAND qt_tests)` sets no environment. `[code]`
- Boost.Test is dynamic by default on Unix here (`Boost_USE_STATIC_LIBS` is only set for Windows) and `BOOST_TEST_DYN_LINK` is added when not static. Homebrew ships dynamic Boost libraries, so this works, but changing one side without the other breaks it. `[code]`
- The test `main.cpp` files use the old `#define BOOST_AUTO_TEST_MAIN` style. `[code]`

**How to avoid:**
- `set_tests_properties(qt_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")` so Qt widget tests need no display. Keep one manual UX check in the P4/P5 checklists since offscreen is not Cocoa.
- Locate fixtures through a compile definition such as `-DTEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"`, never a relative path.
- Replace the `#if 0` Binarize test with synthetic-image assertions (threshold output on a known gradient, Sauvola on a step edge). Do not enable it with only "does not crash" assertions.
- Configure once with `-DBUILD_TESTS=ON` and run `ctest --output-on-failure` as the P1 exit gate, before anything else changes, so existing tests that may already fail on macOS are known.
- Make the suite runnable with one command (`ctest --test-dir build --output-on-failure`).

**Warning signs:**
- `qt_tests` hangs or aborts with "could not connect to display"/"cocoa plugin" messages when run from a non-GUI shell.
- Tests that pass only when run from the build directory.

**Phase to address:** P1 (run existing tests), P3 (harness).

---

### Pitfall 6: Refactoring `OutputGenerator.cpp` / `MainWindow.cpp` with string-based Qt connections and no characterisation tests

**What goes wrong:**
Splitting a big file moves slots or signals between classes. Connections written with `SIGNAL()`/`SLOT()` strings fail only at runtime with a console warning ("No such slot"), and the feature silently stops working. The repository has 331 `SIGNAL(` and 318 `SLOT(` uses. `[code]` Output code is split without a before/after proof.

**Why it happens:**
Compile succeeds because the macros take strings. Fragile areas are listed in CONCERNS.md (output generation, deskew overlay geometry, threading), and nothing exercises them.

**How to avoid:**
- Do not split any file until the characterisation tests for that area exist and are green (Pitfall 1).
- Split by moving code verbatim into new translation units in a commit that changes nothing else (no renames, no signature tidying, no `boost::bind` replacement in the same commit). Review such a commit by diffing the moved text.
- When touching a class's slots, convert the affected `SIGNAL/SLOT` string connections to the pointer-to-member form first (a compile error then catches a missed move), in its own commit.
- Run the application with `QT_FATAL_WARNINGS=1` or grep the stderr for "QObject::connect:" during the manual smoke test after each split.
- Only split files that the active requirements touch. PROJECT.md says "oversized files where touched"; keep to that.
- `MainWindow.cpp` has no unit test seam. Extract pure logic (for example the duplicated "save project as" flow between `MainWindow.cpp` and `OutOfMemoryDialog.cpp`) before trying to test it.

**Warning signs:**
- "QObject::connect: No such slot" or "No such signal" on stderr.
- A refactor commit with both moved code and edited code.

**Phase to address:** P6 (gate: P3 baseline exists).

---

### Pitfall 7: Treating Qt 5 removal as a simplification when it is the activation of the untested path

**What goes wrong:**
The Qt 6 branches have never run in CI (CONCERNS.md: CI installs Qt 5 only). `[code]` Removing Qt 5 code does not remove risk; it makes the Qt 6 branches the only path, and their behaviour was never verified. Mechanical deletion of `#if` blocks also gets the polarity wrong.

**Why it happens:**
The conditionals come in several shapes in about 30 places: `#if QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR < 14` (keep `#else`), `#if QT_VERSION_MAJOR > 5 || QT_VERSION_MINOR > 9` (keep the true branch, delete the fallback), `#if QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR <= 9`, and plain `#if QT_VERSION_MAJOR == 5`. `[code]` Some Qt 6 branches change behaviour, not only spelling, for example `event->localPos()` versus `event->position()` and `enterEvent(QEvent*)` versus `enterEvent(QEnterEvent*)`.

**How to avoid:**
- Use `unifdef -DQT_VERSION_MAJOR=6` (or a regex script) to resolve conditionals, then read every diff. Do not delete by hand and by eye. After each batch, build and run tests.
- Delete in this order: C++ conditionals; CMake (`Qt5_add_translation`, `find_package(Qt5 ...)`, the `Qt_*_lib` indirection variables, `Qt5::` targets in `src/core/CMakeLists.txt` and `CMakeLists.txt`, `qt_min_version`); `main.cpp` Qt 5 block (xcb/Wayland fallback and `AA_EnableHighDpiScaling`, which Qt 6 does by default); the `QGLWidget` macro hack at the top of `ImageViewBase.cpp`; docs and `flatpak/`.
- Keep the Qt 6 `#else` code exactly as it is. Do not "improve" it in the same commit.
- Mouse-position code needs a deliberate review, see Pitfall 16.
- Pin to the Homebrew Qt 6 version in use and record it. Homebrew's `qt` formula follows the newest Qt 6 and removes deprecated APIs over time, so an unplanned `brew upgrade qt` can break the build. `[exp]`

**Warning signs:**
- A build that passes only because both `#if` arms compile.
- Leftover `Qt5`/`QT_VERSION_MAJOR` strings after the strip (`grep -rn "Qt5\|QT_VERSION" . --include=CMakeLists.txt --include='*.cpp' --include='*.h'` should be empty).

**Phase to address:** P2 (after the P3 baseline exists).

---

### Pitfall 8: Retina rendering fixed only where it is obvious (views) but not in the pixmap pipelines

**What goes wrong:**
The view looks sharp at "fit" zoom but blurry at 100% zoom, thumbnails are soft, and the colour picker and zone masks behave subtly differently. Or, after adding `setDevicePixelRatio` to some pixmaps, geometry in other places is off by a factor of 2.

**Why it happens:**
- Qt 6 turns high-DPI scaling on automatically, so the widgets get logical coordinates. Code that builds pixmaps at logical pixel size produces 1x pixmaps that are scaled up 2x on Retina. `[code]`
- `ImageViewBase::initiateBuildingHqVersion` builds the HQ pixmap with the logical transform and `viewport()->size()` as target (`HqTransformTask`), then `hqVersionBuilt` does `QPixmap::fromImage(image)` (DPR 1). The comment in `paintEvent` says "HQ pixmap maps one to one to screen pixels", which is false on Retina. `[code]`
- The same pattern is in `PictureZoneEditor` (`m_screenPictureMask`), `ThumbnailBase.cpp` (`QPixmap tempPixmap(w, h)` cached in `QPixmapCache` under one fixed key), and `ThumbnailPixmapCache` (thumbnails built at a pixel size, `QPixmap::fromImage`). `[code]`
- There is no use of `devicePixelRatio` anywhere in `src/` (grep: zero hits). `[code]`
- `QPixmap::width()/height()/rect()` return device pixels, not logical size. Existing math divides image size by `m_pixmap.width()` (`m_pixmapToImage`, `ThumbnailBase` pre-scale) and `PixmapRenderer::drawPixmap` intersects the painter's logical source rect with `pixmap.rect()`. Setting DPR on `m_pixmap` or thumbnails without touching these breaks scale and clipping. `[code]` (Qt behaviour: `[exp]`)
- `ColorPickupInteraction::takeColor` calls `imageView().grab(rect)` when OpenGL is off. On Retina `grab` returns a 2x pixmap, so `toImage()` has twice the pixels and the sampling circle covers a different number of samples. `[code]`
- Moving the window between a Retina and a 1x external display changes the DPR at run time and invalidates every cached pixmap. The cache key in `ThumbnailBase` has no DPR. `[code]`

**How to avoid:**
- Define one rule and write it in the code: view-space coordinates are logical, pixel buffers are physical, and every buffer built for the screen is `size_logical * dpr` with `setDevicePixelRatio(dpr)` set. Then fix, in this order: `HqTransformTask` (multiply the transform and target size by DPR, set DPR on the result, keep `m_hqPixmapPos` logical), `PictureZoneEditor` mask, `ThumbnailBase` temp pixmap (add DPR to the cache key), `ThumbnailPixmapCache` (generate thumbnails at `maxThumbSize * dpr`).
- Do not set DPR on `m_pixmap` (the downscaled whole-image pixmap) unless `m_pixmapToImage` and `PixmapRenderer` are updated in the same change. The simple route is to keep it DPR 1; it is only the fast-preview layer.
- Update the pixelWidth heuristic in `paintEvent` (`pixelWidth < 0.5` uses `widthMM()` and `m_virtualToWidget.m11()`); verify it still picks smooth versus nearest sampling sensibly on Retina.
- Audit `grab()` callers (only `ColorPickupInteraction`), and test the colour picker on Retina.
- Handle DPR change: react to `QEvent::DevicePixelRatioChange`/`QWindow::screenChanged` by invalidating `m_hqPixmap` and thumbnail caches.
- Custom icon engines (`SvgStyledIconEngine`, `FontIconEngine`) implement `pixmap(size, ...)` and generate at the requested size, which Qt 6 calls with the scaled size, so they are probably fine; verify by looking at toolbar icons on Retina rather than assuming. `[exp]`
- Memory: HQ pixmaps cost 4x at 2x DPR. Cap or tile if full-screen 5K displays are used.
- Verification needs eyes. Add a manual checklist: view at 100% and 200% zoom on built-in and external display, thumbnails, picture zone overlay, dewarping handles, colour picker.
- Check at least once with `QT_ENABLE_HIGHDPI_SCALING=0` to see what "before" looks like, so improvement is judged against a reference.

**Warning signs:**
- Text or fine lines in the main view look soft at 100% zoom, thumbnails are soft, but icons are sharp.
- Thumbnail or HQ pixmap appears at half size or shifted after a DPR change.
- Zero `devicePixelRatio` hits in `src/` after the HiDPI phase is declared done.

**Phase to address:** P4.

---

### Pitfall 9: Ctrl/Cmd confusion on macOS (changing code that is already right, missing text that is wrong)

**What goes wrong:**
Developers see `Qt::ControlModifier` throughout the interaction code and replace it with `MetaModifier` or set `AA_MacDontSwapCtrlAndMeta`, breaking things. Or they leave user-visible text saying "Ctrl" and "Alt".

**Why it happens:**
On macOS, Qt swaps the meanings by default: `Qt::ControlModifier` is the Command key and `Qt::MetaModifier` is the Control key; `Qt::AltModifier` is Option. `QKeySequence("Ctrl+S")` therefore becomes Cmd+S automatically. `[exp]` The Qt docs do not state this in the search results I found, so verify on the first run. The code uses `Qt::ControlModifier` in 11 places for gestures (deskew wheel rotate, zone vertex drag, thumbnail multi-select, page-layout drag) and `Ctrl+...` shortcuts in `MainWindow.ui` (S, N, O, W, Q, G, +, -) and `TabbedImageView` (`Qt::CTRL | Qt::Key_1..5`). `[code]` Cmd+click multi-select and Cmd+wheel are Mac-conventional, so the logic is correct as it stands. Control+click on macOS is a right-click and would be wrong for these gestures, which is another reason not to move to Meta.

**How to avoid:**
- Leave `Qt::ControlModifier` logic alone. Verify the behaviours once on the Mac: thumbnail Cmd+click, Cmd+wheel rotate in deskew, Cmd+Alt+click copy of a zone.
- Change only user-visible text: status tips in `ZoneDefaultInteraction.cpp`, `deskew/ImageView.cpp`, `TabbedImageView.cpp`; tooltips "(Ctrl++)" in `MainWindow.ui`; "Use Ctrl+Click / Shift+Click ..." in six `ApplyDialog.ui` files; rich text in `page_layout/OptionsWidget.ui`. Build them from `QKeySequence(...).toString(QKeySequence::NativeText)` or a small helper that returns the platform modifier names, instead of replacing "Ctrl" with "Cmd" in 30 translatable strings by hand.
- Mind that changing `tr()` source strings breaks existing translation matches. For a single-user English setup this only means untranslated fallbacks; accept it and note it.
- Fix keys that do not exist on a Mac keyboard: `QKeySequence("Ins")` for "Create Content Box" (`select_content/ImageView.cpp`) has no key; offer another binding. `Qt::Key_Delete` is Fn+Backspace on most Mac keyboards, so also bind `Qt::Key_Backspace` for zone and content-box removal (`ZoneDefaultInteraction.cpp`, `select_content/ImageView.cpp`). The `QShortcut` constructors use the default `Qt::WindowShortcut` context, so check Backspace does not interfere with text fields in the options panel when you add it. `[code]`
- Bare `Key_Plus`/`Key_Minus` zoom shortcuts depend on layout; test on the layout actually used.
- Avoid shortcuts that macOS reserves: Cmd+H, Cmd+M, Cmd+Space, Cmd+Tab, Cmd+Q/W/, behave specially. Cmd+W (Close Project) and Cmd+Q (Quit) match Mac conventions, so keep them.

**Warning signs:**
- A PR that touches `ControlModifier` in `src/core/interaction/` or `zones/`.
- Help text still shows "Ctrl" on the Mac.
- Zone cannot be deleted from the keyboard.

**Phase to address:** P5.

---

### Pitfall 10: Native menu bar roles hoisting the wrong items (and the app-menu name coming from a missing Info.plist)

**What goes wrong:**
The menu bar looks wrong: an item vanishes from its menu, or the application menu shows the bare executable name; "About" says "About scantailor-advanced" or a blank name.

**Why it happens:**
- `MainWindow.ui` has a real `QMenuBar`. Qt on macOS shows it in the global menu bar and moves items to the application menu by `QAction::menuRole`; the default `TextHeuristicRole` looks at the text of the action, and roles apply only to actions directly in top-level menus, not in submenus. `[web]` Here `actionSettings` ("Settings ...") sits in the menu `menuDebug`, `actionAbout` in Help, `actionQuit` in File. `[code]`
- The application name for "About <name>" comes from the bundle's Info.plist. `[web]` The project runs from the build dir without a bundle (`add_executable(... WIN32 ...)` has no `MACOSX_BUNDLE`), so there is no Info.plist and the name falls back to the process name. `[code]`
- Without a bundle, the Dock shows a generic icon and macOS may not treat the process as a regular foreground app (focus quirks when launched from a terminal). `[exp]`
- An empty menu after hoisting (for example `menuDebug` if its remaining entries are hidden) disappears. `[exp]`

**How to avoid:**
- Set roles explicitly: `QuitRole` on `actionQuit`, `PreferencesRole` on `actionSettings`, `AboutRole` on `actionAbout`. Set `NoRole` on anything whose text could trigger the heuristic by accident. In `MainWindow.ui` the only candidates are the "Settings", "About" and "Quit" actions; the "Default parameters ..." action should not be hoisted.
- Move "Settings" out of the "Debug" menu into the app menu via role; do not leave the Debug menu with a few orphan items. Decide what the Debug menu should contain on a release build (`DEVELOPER_VERSION` controls this).
- Set `QApplication::setApplicationDisplayName` and `setApplicationName`. Accept that the "About" name and Dock name stay imperfect unbundled, since PROJECT.md excludes bundling. If it bothers: a minimal bundle is a 15-line Info.plist plus a folder layout, which is a separate decision, not part of this scope.
- The dark/light scheme uses Qt stylesheets that style `QMenuBar` (`stylesheet.qss`). The native menu bar ignores them; do not spend time fixing the QSS menu bar colours on macOS.
- Native dialogs: `QFileDialog` is the native panel on macOS except where `DontUseNativeDialog` is passed (relinking dialogs). Decide whether that is wanted; the non-native dialog looks out of place. `[code]`

**Warning signs:**
- Settings or About missing from the app menu, or present twice.
- The menu bar title shows `scantailor-advanced`.

**Phase to address:** P5.

---

### Pitfall 11: Homebrew dependency discovery picking the wrong libjpeg/libpng/libtiff/Boost, or a stale CMake configuration

**What goes wrong:**
The build links against a different copy of libjpeg/libtiff/libpng than the headers it compiled with, or finds them in a framework directory on the machine (Mono, Anaconda, a Perl distribution). The result is link errors or a runtime crash in image I/O. Or CMake configures but warns forever about Boost.

**Why it happens:**
- Homebrew keeps some formulae out of the default prefix (keg-only), for example `jpeg-turbo` and `zlib`, and Qt links its own set. `[web]` partially; the search confirmed explicit `JPEG_LIBRARY`/`JPEG_INCLUDE_DIR` is the usual fix; the keg-only status itself is `[exp]`.
- The top-level `CMakeLists.txt` only has special search logic for Windows. On Unix it uses plain `find_package(JPEG/ZLIB/PNG/TIFF)`. `[code]`
- `cmake_minimum_required(VERSION 3.9)` still configures on CMake 4.x (CMake 4.0 removed compatibility only for versions below 3.5), but it prints deprecation warnings, and policy `CMP0167` stays OLD, so the old `FindBoost` module is used. The module warns about unknown newer Boost versions (the line `#set(Boost_NO_WARN_NEW_VERSIONS ON)` is already commented in the file). If you raise the minimum to 3.30 or later, `find_package(Boost ...)` switches to `BoostConfig.cmake`, which can fail to be found unless `Boost_DIR` or the prefix path is set. `[web]`
- Only `unit_test_framework` is requested as a Boost component, so the Boost 1.89+ removal of stub libraries such as `system` does not hit this build. Do not add `COMPONENTS system`. `[web]`
- `CMAKE_CXX_FLAGS` and `DEFAULT_COMPILER_FLAGS_IS_SET` are written into the cache with `FORCE`, so changing compiler flags in the file may not take effect in an existing build dir. `[code]`

**How to avoid:**
- Capture the exact configure command in the repository as a preset or a short script, for example `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix)" ...` plus explicit `-DJPEG_INCLUDE_DIR`/`-DJPEG_LIBRARY` if the first configure shows a non-Homebrew path.
- After configuring, read `build/CMakeCache.txt` for `JPEG_LIBRARY`, `PNG_LIBRARY`, `TIFF_LIBRARY`, `ZLIB_LIBRARY`, `Boost_INCLUDE_DIR`, `Qt6_DIR` and confirm each points into `/opt/homebrew` (or the SDK for zlib). Then run `otool -L build/scantailor-advanced`: exactly one libjpeg, one libpng, one libtiff.
- Deliberately raise `cmake_minimum_required` to one tested value (3.21 is a good level for presets and modern Qt 6 support) and test `find_package(Boost CONFIG)` in the same commit. Do not leave it at 3.9 for a "later" cleanup; every configure prints the warning.
- Use a fresh build dir whenever flags or toolchains change.
- Do not install anything from `$(brew --prefix)` into `/usr/local`, and do not let a conda/Mono `PATH` entry come first when configuring.

**Warning signs:**
- `otool -L` shows two libjpeg versions, or a path outside `/opt/homebrew`.
- Crashes in `JpegMetadataLoader`/`TiffReader` that do not occur in unit tests.
- CMake deprecation or `CMP0167` warnings that nobody has decided on.

**Phase to address:** P1.

---

### Pitfall 12: Compile-error classes that appear on the first Apple Clang + libc++ build

**What goes wrong:**
P1 takes much longer than planned because the first build produces hundreds of diagnostics in a few repeated categories, and fixes get mixed with behaviour changes.

**Why it happens:**
The code has only been built with GCC, MSVC and (for a few lines) Clang on Linux. The macOS toolchain differs in these ways `[exp]`:
- **Missing transitive includes.** libc++ includes fewer headers indirectly than libstdc++. Expect missing `<cstdint>`, `<algorithm>`, `<memory>`, `<limits>`, `<cstring>`, `<cmath>`.
- **`size_t` / `uint64_t` mismatch.** On macOS `uint64_t` is `unsigned long long` while `size_t` is `unsigned long`, so `std::max(size_t, uint64_t)` and some overloads fail.
- **Qt 6 API removals/deprecations** in code that only compiled under Qt 5 in practice (`QRegExp`, `QDesktopWidget`, `QString::SkipEmptyParts`, `QMouseEvent::pos()` and similar). The Qt 6 branches already exist but were never run in CI. `[code]`
- **Boost header notices.** `boost/bind.hpp` pulls global placeholders and prints a deprecation message. The code already migrated to `boost/bind/bind.hpp` with `boost::placeholders::_1` in several files (for example `ProjectReader.cpp`, `OutputGenerator.cpp`), so only stragglers remain. `[code]`
- **OpenGL deprecation warnings** from Apple's headers when `QOpenGLFunctions`/`GL_RENDERER` are used (`OpenGLSupport.cpp`). `[code]`
- No warning flags are applied at all for Clang because the flag block is GCC-only. `[code]` You will see a different warning set than the one the authors saw.

**How to avoid:**
- Do not add `-Werror`. Triage warnings once, after the build works.
- Fix compile errors with the smallest diff in the order they appear. Add the missing include; do not restructure. One commit per error class helps review.
- Define `GL_SILENCE_DEPRECATION` only if OpenGL stays (see Pitfall 15).
- Keep a build log from the first successful build so later "new warning" claims can be diffed.

**Warning signs:**
- A P1 commit that also reformats code or renames things.
- A fix that adds `static_cast` noise over a real type mismatch.

**Phase to address:** P1.

---

### Pitfall 13: "Modernising" Boost.Lambda / Boost.Function / Boost.Bind / `boost/foreach` all at once

**What goes wrong:**
A sweeping replacement of Boost functional helpers with `std::` lambdas introduces behaviour changes in image kernels and callbacks (integer promotion, copy versus reference capture, overload resolution on `QTransform::map`), with no tests to notice.

**Why it happens:**
The numbers look large, so replacement looks urgent, but the facts are mixed. `[code]`
- `boost::lambda` is used in about 20 files (33 lines), including imageproc kernels (`GaussBlur.cpp`), dewarping (`TopBottomEdgeTracer.cpp`, `TextLineTracer.cpp`, `TextLineRefiner.cpp`, `DetectVertContentBounds.cpp`, `TowardsLineTracer.cpp`) and `ThumbnailSequence.cpp` (file-scope `using namespace ::boost::lambda;`). Boost.Lambda is header-only and I found no evidence that it was removed in recent Boost releases; the one search on Boost 1.90 changes returned nothing definitive, so check the release notes of the installed Boost. `[web]` (inconclusive)
- `boost::bind` is used in about 17 files and already uses the modern include and `boost::placeholders` spelling in the places checked.
- `boost/foreach.hpp` is included in 26 files, but there are zero uses of `BOOST_FOREACH` or `BOOST_REVERSE_FOREACH`. Those includes are dead. Deleting them is safe and removes noise.
- `boost::function` appears 76 times in 28 files; it works with `std::function` for most uses but converting changes empty-function and comparison semantics.
- Multi-index (`boost::multi_index` in `ProjectPages`, `ThumbnailSequence`, `FileNameDisambiguator`, page-layout `Settings`) has no standard replacement. Keep Boost as a dependency for it regardless.

**How to avoid:**
- First action: delete the 26 unused `#include <boost/foreach.hpp>` lines (verify with a compile).
- Leave Boost.Lambda and Boost.Function in place unless the compiler rejects them. Replace only within a file that has a characterisation test, one file per commit, and keep the lambda body arithmetic types explicit (for example `static_cast<int>(...)`).
- Never mix `using namespace boost::lambda;` with `std::placeholders` or `boost::placeholders` in one scope: `_1` becomes ambiguous.
- Do not spend P6 time on "remove Boost" as a goal. The requirement is "safe to change later", not "no Boost".

**Warning signs:**
- A PR titled "replace boost::lambda" that touches imageproc and dewarping in one go.
- New `auto` lambdas capturing by reference into callbacks that outlive the scope.

**Phase to address:** P6 (only with tests); dead-include deletion can go in P1/P2.

---

### Pitfall 14: Deleting Linux/Windows pieces breaks the build in ways unrelated to those platforms

**What goes wrong:**
After the strip, configure fails or tests are missing because something shared depended on a platform artefact.

**Why it happens:**
- `CMakeLists.txt` runs `configure_file("${SCANTAILOR_RESOURCES_DIR}/unix/${target_name}.desktop.in" ...)` unconditionally. Deleting `src/resources/unix/` breaks configure on every platform. `[code]`
- `PORTABLE_VERSION` defaults to ON and `config.h.in` sets `PORTABLE_CONFIG_DIR = "config"`: `Application` then makes a `config` directory next to the executable, and `main.cpp` points `QSettings` there. In the build dir that means settings live in `build/config` and are lost with the build dir, while default parameter profiles use `QStandardPaths::AppDataLocation` (under `~/Library/Application Support`). Two different stores. `[code]`
- `TRANSLATION_DIRS` and `PLUGIN_DIRS` are set differently for WIN32 and non-WIN32; the non-Windows value includes `../share/...`, which is right for an install tree, and `.` / `translations` which is right for the build dir. Translations are compiled by `compile_translations` into the build root. `[code]`
- `ByteOrder.h` has a `_WIN32` branch for `winsock2.h`; the else-branch (`netinet/in.h`) is the right one on macOS. `Utils::overwritingRename` and `MainWindow.cpp` alarm code have platform branches with a Unix default that is wrong on macOS (Pitfall 18).
- Disabling CI by deleting `.github/workflows/` is fine, but the lint workflow was the only enforcement of `.clang-format`.

**How to avoid:**
- Strip in this order and rebuild after each step: CI workflows, packaging (`flatpak/`, `build-deb.sh`, `build-windows.sh`, CPack blocks), Windows CMake blocks and `Q_OS_WIN`/`_WIN32` code, then resources only after confirming nothing references them (`grep -rn "unix/\|win32/" CMakeLists.txt src`).
- Decide the portable question explicitly: set `PORTABLE_VERSION` OFF (settings in `~/Library/Preferences`) or keep it ON and document that `build/config` holds the settings. Keep the `QSettings` format consistent with whichever is chosen.
- Keep `.clang-format` and decide whether to enforce it locally (a `format` target or pre-commit hook), since CI no longer will.

**Warning signs:**
- "Cannot find source file ... desktop.in".
- Settings reset after `rm -rf build`.

**Phase to address:** P2.

---

### Pitfall 15: Spending effort on the OpenGL viewer path (or breaking it) on macOS

**What goes wrong:**
Time is spent making `QOpenGLWidget` look right on Retina, or the build is left carrying OpenGL link dependencies for a feature that does not help on Apple Silicon.

**Why it happens:**
- Apple deprecated OpenGL in 2018 and has not announced a removal date. `[web]` Qt Widgets painting (`QGraphicsView`-style paint engines) does not use Metal; only QRhi-based widgets do (`QRhiWidget`, since Qt 6.7, where Metal is the default on macOS). `[web]` So there is no Metal path for this viewer. A `QOpenGLWidget` viewport on `QAbstractScrollArea` uses the GL paint engine, which on macOS is a translation layer over Metal. `[exp]`
- The feature is off by default (`DEFAULT_OPENGL_STATE = false`), lives in `ImageViewBase.cpp` (`setViewport(new QOpenGLWidget())`), `OpenGLSupport.cpp`, `SettingsDialog`, and one extra branch in `ColorPickupInteraction::takeColor` (`sampleImageAtWidgetRect` instead of `grab`). `[code]`
- `OpenGLSupport::supported()` creates a `QOpenGLContext` and is called in every `ImageViewBase` constructor when the setting is on, plus once in `SettingsDialog`. `deviceName()` creates a `QOffscreenSurface`. `[code]`
- The code comment records a known artefact: setting the viewport background fill on an OpenGL viewport produces "black hole" artefacts. `[code]`
- The Qt 6 CMake links `Qt::OpenGL` and `Qt::OpenGLWidgets` unconditionally. `[code]`

**How to avoid:**
- Decide once. Recommended: remove the OpenGL option in P6 (setting key, checkbox, `OpenGLSupport.*`, the `QOpenGLWidget` branch, the `Qt::OpenGL`/`Qt::OpenGLWidgets` link entries), after P3 has a test for the colour-picker sampling path, because `ColorPickupInteraction` has an OpenGL-specific sampling branch that must keep its non-OpenGL behaviour. For a one-user Apple Silicon fork, the raster paint engine is fast enough and there is nothing to gain.
- If it stays, treat it as unsupported on macOS: leave it default off, keep `GL_SILENCE_DEPRECATION`, and do not include it in HiDPI acceptance.
- Do not make the HiDPI work depend on it.

**Warning signs:**
- HiDPI tasks that mention `QOpenGLWidget`.
- A black or empty view after enabling the setting.

**Phase to address:** P6 (decision at roadmap time; never block P1/P4 on it).

---

## Moderate Pitfalls

### Pitfall 16: Fractional mouse positions from Retina/Qt 6 combined with hard-coded `+0.5` pixel-centre math

**What goes wrong:**
Hit testing and zoom focus are off by half a pixel or jitter on Retina, handles feel slightly "sticky".

**Why it happens:**
The Qt 5 code path used integer `event->pos()` and added `QPointF(0.5, 0.5)` to get the pixel centre. `[code]` `ImageViewBase::mouseMoveEvent` still does `QPointF(0.5, 0.5) + event->pos()` for proximity but uses `event->position()` (fractional) for `updateCursorPos`; `ZoomHandler::onWheelEvent` uses `event->position() + QPointF(0.5, 0.5)` in the Qt 6 branch, adding half a pixel to an already fractional position. On Retina, logical positions come in 0.5 steps.

**How to avoid:**
During P2/P4, make one explicit decision per use: either everything uses `position()` without `+0.5`, or everything uses `pos()` with it. Grep `QPointF(0.5, 0.5)` and fix all sites together. Test handle dragging at high zoom (page_split cutter handles, deskew handles, zone vertices) on Retina.

**Phase to address:** P4.

---

### Pitfall 17: Trackpad and magic-mouse wheel semantics

**What goes wrong:**
Zooming becomes violently fast or too slow, "scroll past the edge" does nothing, momentum scrolling keeps zooming after the fingers lift.

**Why it happens:**
`ZoomHandler` zooms by `2^(angleDelta/8/60)` per event and, when already fully zoomed out, scrolls by `angleDelta().y() / 120` (integer division, so trackpad deltas below 120 give 0). `[code]` macOS trackpads send many small deltas with inertia phases. PROJECT.md puts trackpad gestures out of scope, so this is about not making things worse.

**How to avoid:**
Accept mouse-wheel behaviour as is. If zoom feels wrong, clamp per-event zoom, use `event->pixelDelta()` when non-null, and ignore events in `Qt::ScrollMomentum` phase for zoom. Keep it a small, separate change.

**Phase to address:** P5 (only if the feel is unacceptable).

---

### Pitfall 18: The batch-finished alarm is wrong on macOS and runs a shell string

**What goes wrong:**
The "beep when finished" option silently does nothing on the Mac, and the code path is a shell injection surface.

**Why it happens:**
`MainWindow.cpp` (~1327) uses `#if defined(Q_OS_UNIX)` for the default command `play /usr/share/sounds/freedesktop/stereo/bell.oga`. `Q_OS_UNIX` is defined on macOS, so the Linux default applies; `play` (SoX) is usually not installed and the freedesktop sound file does not exist. The result of `std::system` is discarded with `Q_UNUSED`, so the failure is invisible. `[code]`

**How to avoid:**
Default to `QApplication::beep()` (or `afplay /System/Library/Sounds/<name>.aiff`), and run the command through `QProcess::startDetached(program, arguments)` with no shell. Migrate or ignore an existing stored `main_window/external_alarm_cmd` value. This fixes the CONCERNS.md security note and the macOS behaviour in one change.

**Phase to address:** P6 (security) with a P5 smoke test.

---

### Pitfall 19: Memory behaviour: bad_alloc-based out-of-memory handling on a system that swaps instead

**What goes wrong:**
Batch processing makes the machine crawl, with memory pressure in red, instead of showing the app's "out of memory" dialog.

**Why it happens:**
`OutOfMemoryHandler` and the dialog are triggered by `std::bad_alloc` (`WorkerThreadPool`, `BackgroundExecutor`). `[code]` On macOS, allocations rarely fail; the system compresses memory and swaps. `WorkerThreadPool::updateNumberOfThreads` defaults to `QThread::idealThreadCount()`, which counts every core; each worker holds full-resolution `QImage`, `GrayImage` and `BinaryImage` buffers plus integral images in Binarize (`Binarize.cpp` ~194 TODO). A 600-DPI A4 colour scan is roughly 35 megapixels (about 140 MB per 32-bit image), and the output stage keeps several. `[code]` + `[exp]` (arithmetic)

**How to avoid:**
Cap the default thread count (for example min(performance cores, RAM / 2 GB)) and expose it as the existing `settings/batch_processing_threads`. Do not rely on `bad_alloc`. Add the TIFF dimension bound check from CONCERNS.md, since it makes failure explicit before allocation. Do not try to redesign memory use in this scope.

**Phase to address:** P6.

---

### Pitfall 20: File-name Unicode normalisation and macOS path behaviour in project files

**What goes wrong:**
A project with file names containing umlauts (likely for a German-speaking user) re-opens with "file not found" entries, duplicates, or wrong ordering when the same name comes from the file dialog (composed form) and from a directory listing or another tool (decomposed form).

**Why it happens:**
Paths are stored as strings in the project XML and used as keys (`ImageId`, `RelinkablePath`, `FileNameDisambiguator`, `SmartFilenameOrdering`). macOS file systems can present the same name in different Unicode normal forms depending on how the file was created. `[exp]` The existing test gap "project-file XML round-trip with special characters" (CONCERNS.md) is the same area. `[code]`

**How to avoid:**
Add a P3 test that creates a temp directory with names like `Bücher/Seite_ä_1.png` in both NFC and NFD, writes a project, reads it back and checks that all pages resolve and keep their order. Fix only what the test shows. At the same time resolve the `XmlUnmarshaller::string` FIXME with a round-trip test containing `&`, `<`, `>` and quotes, as CONCERNS.md already proposes.

**Phase to address:** P3 (test), P6 (fix if needed).

---

### Pitfall 21: System appearance mixing with the app's own stylesheet

**What goes wrong:**
With macOS in dark mode, the app's light scheme is partly dark (native dialogs, scroll bars, title bar) or vice versa.

**Why it happens:**
The app styles widgets with Qt stylesheets from `src/resources/{light,dark}_scheme`, which overrides the native style for those widgets only. Native pieces (file dialogs, window frame, some controls) follow the system appearance in Qt 6 on macOS. `[exp]` PROJECT.md excludes "following macOS system dark mode", but not mixing.

**How to avoid:**
At startup pin the Qt colour scheme to match the app's chosen scheme (Qt 6.8 added `QStyleHints::setColorScheme`; check the installed Qt version), or accept the mismatch and note it. Do not build dark-mode following in this scope.

**Phase to address:** P5 (cheap fix or documented acceptance).

---

## Minor Pitfalls

### Pitfall 22: Unsanitised builds hide the bugs the refactoring is meant to avoid

**What goes wrong:**
The project defaults to Release (`st_set_default_build_type(Release)`), so every `assert` is compiled out, including the TIFF `bytesPerLine() % 4` check (CONCERNS.md). Raw-pointer image code (`(uint32_t*) image.bits()` in many files; about 114 `new` and 148 `.get()/.release()` uses) is never run under checks. `[code]`

**Prevention:** Add a CMake option (for example `SANITIZE=address,undefined`) and run the whole test suite once in Debug with ASan and UBSan before any refactor, and TSan once for the threading tests. Apple Clang on arm64 supports these. Fix or consciously baseline the findings. Also run the suite in Debug (assertions on) at least once.

**Phase to address:** P3.

---

### Pitfall 23: Comparing TIFF/PNG/JPEG files by bytes, or assuming libtiff behaviour is the Linux one

**What goes wrong:** Output TIFFs differ in tags and compression details between libtiff versions; Homebrew libtiff builds with more codecs than the Debian one the authors used. `TIFFReadRGBAImageOriented` results and orientation handling in `TiffReader.cpp` can differ between libtiff releases.

**Prevention:** Decode before compare (see Pitfall 2). Add reader tests for the input variants the user's scanner produces (1-bit, 8-bit gray, 24-bit RGB, multi-page, with and without DPI tags, each orientation value) using small synthetic TIFFs written by the test itself.

**Phase to address:** P3.

---

### Pitfall 24: Qt test macro and Boost.Test mixing

**What goes wrong:** `qt_tests` is one executable that runs two `QTest::qExec` objects under a hand-written `main`; adding more files to it grows a slow GUI-flavoured binary while Boost.Test is used elsewhere.

**Prevention:** Put new pipeline and golden tests in a new Boost.Test executable (for example `pipeline_tests`) linked against `core`, registered through `add_test`. Keep `qt_tests` for widget tests only.

**Phase to address:** P3.

---

### Pitfall 25: Translations after the strip

**What goes wrong:** `update_translations_target` and `finalize_translations` reference `Qt6::lupdate`; removing the Qt 5 branch can leave the translation CMake half-edited. `Qt_add_translation`/`Qt5_add_translation` is selected by `Qt6_FOUND`. `[code]`

**Prevention:** After the strip, build the `compile_translations` target and check `.qm` files appear in the build dir. Decide whether translations are kept at all for an English-speaking single user; if dropped, remove the CMake and the `.ts` files together.

**Phase to address:** P2.

---

### Pitfall 26: Leaving `#if 0` blocks "for later" while the tests need their behaviour

**What goes wrong:** Some of the nine disabled blocks (CONCERNS.md) sit in code that is under test or about to be (`PageLayoutEstimator.cpp`, `VertLineFinder.cpp`, `DistortionModelBuilder.cpp`). Deleting them without checking removes alternative code that a baseline may depend on, or leaves misleading code.

**Prevention:** Delete only after the baseline passes, one file per commit. For `DistortionModelBuilder.cpp`, keep the TODO as an issue note, then delete.

**Phase to address:** P6.

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Regenerating goldens whenever a test fails | Green suite fast | Baseline loses meaning (Pitfall 1) | Only in a dedicated commit with reasoning and a visual diff |
| One global image tolerance | Few flaky tests | Hides binarization regressions | Never; use the per-kind policy in Pitfall 2 |
| Skipping the `otool -L` check | Saves a minute | Mixed libjpeg/libtiff crashes later | Never |
| Leaving Qt 5 `#if` blocks "since they compile" | No risk of wrong deletion | Dead branches rot, grep noise, double the review | Only until P2; then remove with `unifdef` |
| Replacing Boost.Lambda everywhere | Fewer old idioms | Silent numeric changes | Only per file with a test |
| Keeping `PORTABLE_VERSION=ON` unexamined | Nothing to decide | Settings tied to the build dir | Acceptable if documented |
| Hand-setting Retina scale in individual widgets | Quick visual win | Inconsistent, breaks on display change | Never; use the single DPR rule (Pitfall 8) |
| Keeping OpenGL option "just in case" | No code removal | Extra deps, an untestable path | Acceptable while default off and out of acceptance |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| Homebrew Qt 6 | Not setting `CMAKE_PREFIX_PATH`; letting `brew upgrade` move the target mid-project | Set it in a preset; record versions in the baseline commit |
| Homebrew Boost | Assuming static libs; adding `COMPONENTS system` | Dynamic Boost.Test with `BOOST_TEST_DYN_LINK` as is; only request `unit_test_framework` |
| libjpeg-turbo / libpng / libtiff / zlib | Trusting `find_package` to choose the Homebrew copy | Read the cache; set `JPEG_*` explicitly if needed; `otool -L` |
| libtiff | Treating Debian behaviour as the reference | Decode-based tests of the reader on synthetic TIFFs |
| CMake 4.x | Ignoring the minimum-version deprecation warning | Raise the minimum deliberately and test Boost config mode |
| Qt platform plugin in tests | Running `qt_tests` over SSH | `QT_QPA_PLATFORM=offscreen` in the test environment |
| macOS file dialogs | Relying on non-native dialogs for relinking | Decide on native versus non-native once |
| `QSettings` | Mixed stores (build-dir INI plus Application Support profiles) | Pick `PORTABLE_VERSION` on or off explicitly |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Workers = all cores with full-resolution buffers | Memory pressure, swapping, slow batch | Cap threads by RAM; expose setting | Colour scans above ~300 DPI A4 on 8-16 GB machines, batch of many pages |
| HQ pixmap at 2x DPR | Higher memory and rebuild delay while zooming | Build on demand only for the visible region (already limited to viewport size) | 5K external display, full-screen view |
| Thumbnails at 2x for every page | Slow project open | Generate lazily, cache on disk (existing thumbnail cache), include DPR in key | Projects with hundreds of pages |
| Golden tests on full-size scans | Suite takes minutes, so it is skipped | Small synthetic fixtures (about 600 x 800) | Any time a test takes longer than a few seconds |
| Debug build for everyday use | Unusably slow image processing | Keep Release for daily use; Debug + sanitizers only for test runs | Always for imageproc |

## Security Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| `std::system` with a settings string (`MainWindow.cpp` ~1337) | Arbitrary command execution if the settings file is writable by something else | `QProcess` with an argument list, default `QApplication::beep()` (Pitfall 18). Risk: low (local user) but trivial to fix |
| Allocating `QImage` from TIFF header width/height without bounds (`TiffReader.cpp`) | Huge allocation or integer overflow from a crafted or corrupt TIFF; the `assert` guarding stride is compiled out in Release | Bound `width * height * bytes` before allocation; turn the `assert` into a runtime error; add a test with a malicious header (synthetic) |
| Project XML values converted with `toInt()/toDouble()` defaulting to 0 | Out-of-range geometry reaches image routines | Clamp sizes and DPI on read; test with a malformed project file |
| Running the suite under `-fsanitize` only once | Missed memory errors in raw-pointer image code | Pitfall 22 |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| Help text and tooltips say "Ctrl" and "Alt" | Confusion on a Mac keyboard | Build strings from native key names (Pitfall 9) |
| No keyboard binding for "Create content box" (Insert key) and Delete needs Fn | Zone and content-box editing slower | Add alternative keys (Pitfall 9) |
| Settings item inside a "Debug" menu | Hard to find; macOS expects it in the app menu with Cmd+, | `PreferencesRole`, standard shortcut (Pitfall 10) |
| Soft images at 100% zoom on Retina | The core task (judging scan quality) is harder | Pitfall 8 |
| Beep option silently does nothing | User thinks batch is still running | Pitfall 18 |
| Window appears behind the terminal or without a Dock name | Awkward daily use | Accept for unbundled; revisit bundling later (out of scope now) |

## "Looks Done But Isn't" Checklist

- [ ] **Builds on macOS:** often missing a clean-build-dir test and an `otool -L` check. Verify with a fresh build dir and exactly one libjpeg/libpng/libtiff in the output.
- [ ] **Tests green:** often missing `qt_tests` under a non-GUI shell and under Debug. Verify with `QT_QPA_PLATFORM=offscreen ctest --output-on-failure` in both Release and Debug.
- [ ] **Baseline goldens:** often generated after changes. Verify the golden commit predates all `src/` behavioural commits and the toolchain versions are recorded.
- [ ] **Qt 5 removed:** often leaves `Qt5` in CMake and `QT_VERSION` in code. Verify with `grep -rn "Qt5\|QT_VERSION" CMakeLists.txt cmake src`, and that `compile_translations` still builds.
- [ ] **Retina sharp:** often fixed only in the main view. Verify HQ view at 100% zoom, thumbnails, zone mask overlay, colour picker, toolbar icons, and after dragging the window to a 1x display.
- [ ] **Shortcuts Mac-conventional:** often left with Ctrl text. Verify tooltips, status tips, apply-dialog hints, and that Delete/Backspace and content-box creation work from the keyboard.
- [ ] **Menu bar native:** often missing roles. Verify Settings, About, Quit sit in the application menu exactly once and the Debug menu is not left empty.
- [ ] **Platform code removed:** often leaves `Q_OS_UNIX` Linux defaults. Verify the alarm option does something on macOS.
- [ ] **CI disabled:** often only one workflow removed. Verify `.github/workflows/` has no active workflow and no dependabot noise.
- [ ] **Refactored file split:** often breaks string connections. Verify no "QObject::connect: No such slot" on stderr during a smoke run through all six stages.
- [ ] **Output unchanged:** often checked with one page. Verify all fixture classes (book spread, flat page, mixed) against baseline for every setting touched.

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Baseline captured after changes (1) | HIGH | `git worktree` of `5eaac18` plus compile fixes, regenerate goldens there, diff against current outputs, triage each difference as bug or intended |
| Toolchain drift breaks goldens (2) | LOW | Re-run with recorded versions (`brew` pin/`brew extract` or keep old bottle), decide whether drift is within policy, update tolerance consciously |
| Wrong Qt 5 deletion polarity (7) | MEDIUM | `git bisect` against baseline tests; restore the file and redo with `unifdef` |
| Mixed library copies (11) | LOW | Fresh build dir, explicit `JPEG_*`/`TIFF_*` paths, `otool -L` |
| Retina fixes broke geometry (8) | MEDIUM | Revert DPR on `m_pixmap`/thumbnails; keep DPR only on HQ and mask pixmaps; re-check `PixmapRenderer` assumptions |
| Missing slot after file split (6) | LOW | Grep stderr for `connect:`; restore connection; convert to pointer-to-member form |
| Boost.Lambda replacement changed numerics (13) | MEDIUM | Revert that file; add characterisation test; redo |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| 1 Baseline after change | P1 then P3 | Golden commit precedes behavioural commits; versions recorded |
| 2 Tolerance policy | P3 | Policy documented in the test helper; all tests use it |
| 3 Non-deterministic fixtures | P3 | Regenerate-and-compare test passes on two runs |
| 4 Threading in tests | P3, P6 | Suite passes 20 times in a loop; TSan clean on wait paths |
| 5 Test runner on macOS | P1, P3 | `ctest` passes from a non-GUI shell |
| 6 Refactor with string connections | P6 | No `connect:` warnings; moves are verbatim commits |
| 7 Qt 5 removal | P2 | No `Qt5`/`QT_VERSION` left; baseline unchanged |
| 8 Retina pixmap pipelines | P4 | Manual Retina checklist; `devicePixelRatio` handled in HQ, mask, thumbnail, cache key |
| 9 Ctrl/Cmd | P5 | Gestures verified with Cmd; text shows native names |
| 10 Menu roles | P5 | App menu has Settings/About/Quit once each |
| 11 Dependency discovery | P1 | `CMakeCache.txt` paths and `otool -L` clean |
| 12 First-build errors | P1 | Build log kept; commits per error class |
| 13 Boost modernisation | P6 (dead includes in P1/P2) | Only tested files changed; baseline unchanged |
| 14 Platform strip | P2 | Fresh configure works; settings location decided |
| 15 OpenGL | P6 (decision) | Option removed or documented as unsupported |
| 16 Fractional mouse positions | P4 | Handle dragging verified at high zoom on Retina |
| 17 Trackpad wheel | P5 (optional) | Zoom feels controllable |
| 18 Alarm command | P6 (+P5 smoke) | Beep works on macOS; no `std::system` |
| 19 Memory/threads | P6 | Default thread cap in place; TIFF bound check test |
| 20 Unicode paths | P3 test, P6 fix | NFC/NFD round-trip test green |
| 21 Appearance mixing | P5 | Colour scheme pinned or documented |
| 22 Sanitizers | P3 | ASan/UBSan run recorded |
| 23 Codec-dependent compare | P3 | No byte comparisons of image files |
| 24 Test executable layout | P3 | Pipeline tests in a Boost.Test target |
| 25 Translations | P2 | `compile_translations` builds or is removed cleanly |
| 26 `#if 0` deletion | P6 | One file per commit, baseline green |

## Phase-specific research flags (for the roadmap)

| Phase | Needs deeper research? | Why |
|-------|------------------------|-----|
| P1 Bring-up | Light, mostly empirical | Errors are discovered by building; the pitfalls above list the likely categories |
| P3 Baseline | Yes, a short spike | Pick the actual tolerance values by running the pipeline on two builds (e.g. with and without `-ffp-contract=off`) and measuring spread |
| P4 HiDPI | Yes | Needs a hands-on Retina experiment per pixmap pipeline; unclear how `widthMM()` and the smoothing heuristic behave under Qt 6 on macOS (`[exp]`) |
| P5 Mac UX | Small | Verify Ctrl/Cmd mapping and menu roles empirically on first run |
| P2, P6 | No | Standard, test-guarded mechanical work |

## Sources

- Local code and docs (HIGH): `.planning/PROJECT.md`, `.planning/codebase/CONCERNS.md`, `.planning/codebase/STACK.md`; `CMakeLists.txt`; `src/core/ImageViewBase.cpp`, `src/core/ThumbnailBase.cpp`, `src/core/PixmapRenderer.cpp`, `src/core/ThumbnailPixmapCache.cpp`, `src/core/OpenGLSupport.cpp`, `src/core/filters/output/ColorPickupInteraction.cpp`, `src/core/filters/output/OutputGenerator.cpp`, `src/core/interaction/ZoomHandler.cpp`, `src/core/zones/ZoneDefaultInteraction.cpp`, `src/core/filters/select_content/ImageView.cpp`, `src/core/WorkerThreadPool.cpp`, `src/core/BackgroundExecutor.cpp`, `src/app/MainWindow.ui`, `src/app/MainWindow.cpp`, `src/app/main.cpp`, `src/imageproc/tests/Utils.cpp`, `src/imageproc/tests/TestBinarize.cpp`, `src/qt_tests/*`.
- CMake policy CMP0167 and the Boost config module, Boost 1.89 stub-library removal (MEDIUM): [CMake CMP0167 documentation](https://cmake.org/cmake/help/v4.1/_sources/policy/CMP0167.rst.txt), [CMake Discourse: CMake with Boost and policy CMP0167](https://discourse.cmake.org/t/cmake-with-boost-and-policy-cmp0167/13613), [Orthanc commit removing the boost_system component](https://orthanc.uclouvain.be/hg/orthanc-gdcm/rev/9769daedca17).
- CMake 4.0 compatibility change (MEDIUM): [CMake 4.0.0 announcement](https://discourse.cmake.org/t/cmake-4-0-0-available-for-download/13846), [libp2p issue about brew CMake 4 and old minimum versions](https://github.com/libp2p/cpp-libp2p/issues/324).
- Boost.Bind global placeholders deprecation (MEDIUM): [boost/bind.hpp, Boost 1.82](https://live.boost.org/doc/libs/1_82_0/boost/bind.hpp).
- Qt menu roles and application name from Info.plist (MEDIUM): [QAction class, Qt 6](https://doc.qt.io/qt-6/qaction.html).
- OpenGL deprecation on macOS and QRhiWidget as the Metal-capable widget (MEDIUM): [Qt interest list thread on Qt 6.3 and OpenGL on macOS](https://lists.qt-project.org/pipermail/interest/2022-April/038178.html), [QRhiWidget, Qt 6](https://doc.qt.io/qt-6/ko/qrhiwidget.html).
- Floating-point differences between arm64 and x86 from FMA contraction (MEDIUM): [Arm Learning Path: floating-point behaviour across platforms](https://learn.arm.com/learning-paths/cross-platform/floating-point-behavior/how-to-3/), [DEV Community: CI on three platforms and a float-to-int rounding difference](https://dev.to/yusufihsangorgel/my-ci-ran-on-three-platforms-and-still-could-not-catch-this-l04).
- Unbundled executables are not subject to the Retina low-resolution mode, bundles without NSHighResolutionCapable are (LOW to MEDIUM, no Qt 6 specific source found): [fruitbasket commit notes](https://git.trevorbentley.com/fruitbasket/commit/e7e5ae4731cfa413cd670de45a5458bc58b9111b.html).
- Explicit `JPEG_LIBRARY`/`JPEG_INCLUDE_DIR` as the fix for wrong-library discovery (LOW to MEDIUM): [KDE bug 358192](https://bugs.kde.org/show_bug.cgi?id=358192).
- Items tagged `[exp]` are general Qt/macOS/libc++ porting knowledge and were not confirmed by a source in this session; confirm on the first build.

---
*Pitfalls research for: macOS Apple Silicon port, Qt 6-only cut-over and regression-test hardening of a legacy Qt/Boost image-processing app*
*Researched: 2026-10-18*
