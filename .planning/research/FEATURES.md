# Feature Research

**Domain:** Making an existing Qt 6 desktop image-processing app (ScanTailor Advanced 1.2.1, personal fork) feel right and work reliably on Apple Silicon macOS, plus a regression test suite for its image pipeline
**Researched:** 2026-10-18
**Confidence:** MEDIUM overall (HIGH for what the repo does today and for Qt's documented menu-role / modifier / icon-engine behaviour; MEDIUM for test-suite conventions; LOW for two items flagged "verify empirically" below)

Scope reminder (from PROJECT.md): Apple Silicon only, run from the build directory (no `.app` bundle), Qt 6 only, no dark-mode following, no trackpad gestures, no Linux/Windows, synthetic test images only, existing output must not change unintentionally.

## How to read this file

- Feature IDs (`BLD-`, `HDP-`, `MAC-`, `TST-`) are meant to be lifted straight into REQUIREMENTS.md.
- "Repo evidence" lines are what I found by grepping `src/`; they are the reason an item is on the list, not a guess about macOS in general.
- "VERIFY" marks claims I could not confirm from documentation and that need a 5-minute empirical check on the actual Mac before they drive design.

---

## Area 1: Builds and runs on Apple Silicon from the build directory

### Table Stakes

| ID | Feature | Why Expected | Complexity | Notes |
|----|---------|--------------|------------|-------|
| BLD-01 | One documented configure/build/run command sequence using Homebrew Qt 6 (`CMAKE_PREFIX_PATH` from `brew --prefix qt`), producing an arm64 `scantailor-advanced` binary that starts and shows the main window | "Builds" is the Core Value; the previous macOS attempt failed | MEDIUM | Check `file build/.../scantailor-advanced` says arm64. Repo `CMakeLists.txt` already tries `find_package(Qt6 ... CONFIG)` first, so Qt 6 discovery is plausible; the unknowns are Boost/JPEG/PNG/TIFF/ZLIB discovery and compile errors, which only a real build will show. |
| BLD-02 | Remove Qt 5 fallback, `QT_VERSION` branches (~19 files) and the Qt-5-only HiDPI block in `src/app/main.cpp` (lines 17-33) | Single code path; Qt 6 is always high-DPI, the `AA_EnableHighDpiScaling` / `AA_UseHighDpiPixmaps` / rounding-policy calls are Qt 5 relics | LOW-MEDIUM | Do this before the HiDPI work so there is one behaviour to reason about. |
| BLD-03 | Test executables build and `ctest` is green locally (`BUILD_TESTS=ON`) | Core Value says the suite "makes later changes safe" | LOW-MEDIUM | `BOOST_TEST_DYN_LINK` is already added for dynamic Boost (root `CMakeLists.txt` line ~214). Watch for newer CMake/Boost: `FindBoost` module removal (policy CMP0167) and Boost config-mode differences. Only `unit_test_framework` is requested, which limits exposure. Confidence MEDIUM (from general CMake/Boost knowledge, not checked against installed versions). |
| BLD-04 | Batch-finished notification actually works on macOS | Today the `Q_OS_UNIX` branch (`src/app/MainWindow.cpp` ~1327) defaults to `play /usr/share/sounds/freedesktop/stereo/bell.oga`, which does not exist on macOS and is run through `std::system`. macOS also defines `Q_OS_UNIX`, so it takes this path and silently does nothing | LOW | Default to `QApplication::beep()` (empty command) on macOS. Replace `std::system` with `QProcess` argument-list execution (also closes the CONCERNS.md shell-execution item). |
| BLD-05 | Remove or neutralise the Wayland/xcb fallback and `_WIN32` library-path code in `main.cpp` | Dead code on macOS; part of the platform cleanup requirement | LOW | Mechanical. |
| BLD-06 | Native file dialogs everywhere | `RelinkingDialog.cpp` lines 69 and 74 pass `QFileDialog::DontUseNativeDialog`; main-window open/save dialogs are already native | LOW | Drop the flag unless a concrete reason is found (the Qt dialog was likely chosen for Linux). Half-native dialogs are one of the first things a Mac user notices. |

### Differentiators

| ID | Feature | Value Proposition | Complexity | Notes |
|----|---------|-------------------|------------|-------|
| BLD-10 | Embed an `Info.plist` into the bare executable at link time (`-sectcreate __TEXT __info_plist`) with `CFBundleName`, `NSHighResolutionCapable`, `NSPrincipalClass` | Gives the app menu and About item the right app name instead of the binary name, and removes any doubt about Retina capability without building a `.app` | LOW-MEDIUM | **VERIFY first** (see HDP-00). Not bundling/signing, so it stays inside the stated scope. Confidence LOW that it is needed; MEDIUM that it works if needed. |
| BLD-11 | `scripts/build-macos.sh` wrapper (configure, build, test) | Repeatable, replaces the Linux/Windows `build-deb.sh` / `build-windows.sh` scripts the cleanup deletes | LOW | |
| BLD-12 | Set a Dock/application icon from `src/resources/appicon.svg` via `QApplication::setWindowIcon` | Bare binaries otherwise show a generic exec icon in the Dock | LOW | VERIFY that Qt sets the Dock icon from this on an unbundled run. Cosmetic. |

### Anti-Features

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| `.app` bundle, `macdeployqt`, DMG, signing/notarisation | "Proper Mac app" | Explicitly out of scope; large time sink; the user's workflow is run-from-build-dir | BLD-10 gives most of the bundle's visible benefits for ~10 lines of CMake |
| Universal / x86_64 build | Habit | Out of scope, doubles test matrix (and FMA differences, see TST pitfalls) | arm64 only; set `CMAKE_OSX_ARCHITECTURES=arm64` |
| Vendoring or building Qt/Boost from source | Avoids Homebrew drift | Slow, brittle, contradicts the Constraints section | Homebrew, pin nothing, record versions in the build script output |
| Keeping a Qt 5 compatibility shim "just in case" | Cheap-looking | Untested second path, which CONCERNS.md already lists as a risk | Delete it |

---

## Area 2: Sharp Retina / HiDPI rendering of the custom views

### What "table stakes" means here

Qt 6 enables high-DPI scaling unconditionally, so widgets, text and QPainter vector drawing are sharp with no work. What stays blurry is anything that holds **raster pixels sized in logical units**: a `QPixmap(w, h)` or `QImage(w, h)` created at logical size and then drawn on a 2x screen is upscaled by the compositor. Qt's own high-DPI documentation states that pixmaps/images "hold raw pixels" and that a 400x400 image with `devicePixelRatio` 2.0 occupies 200x200 logical points. The fix for each raster path is: allocate at `logical size * devicePixelRatioF()`, call `setDevicePixelRatio()`, and use the **widget's** ratio (`devicePixelRatioF()`), never a hard-coded 2 or an integer.

### Repo evidence (where the app creates logical-size raster content)

| Location | What it does | Retina effect |
|----------|--------------|---------------|
| `src/core/ImageViewBase.cpp` `initiateBuildingHqVersion` / `HqTransformTask` / `hqVersionBuilt` (~lines 954-977, 1030-1050) | Builds the sharp "HQ" version of the page: transform `m_imageToVirtual * m_virtualToWidget`, target size `viewport()->size()`, result stored via `QPixmap::fromImage` and drawn 1:1 with `drawPixmap(m_hqPixmapPos, m_hqPixmap)` (comment: "maps one to one to screen pixels") | **The main blurry path.** On a 2x screen the HQ pixmap has half the needed resolution and is upscaled. This is the page view in every filter (all views derive from `ImageViewBase`). |
| `src/core/ImageViewBase.cpp` `paintEvent` (~410-455) | Fast path draws the downscaled `m_pixmap` through a world transform | Fine for instant feedback; resolution is limited by the downscale target, so check `createDownscaledImage` size vs viewport physical size |
| `src/core/ThumbnailBase.cpp` (~95-135, 170) | `QPixmap(w, h)` temp pixmap sized in display (logical) pixels, cached in `QPixmapCache`, drawn with `drawPixmap(displayRect.topLeft(), tempPixmap)` | Thumbnails in the page list are rendered at 1x and upscaled |
| `src/core/ThumbnailPixmapCache.cpp` `makeThumbnail` (~635-647) | Thumbnail image scaled to `maxThumbSize` and persisted on disk | Source thumbnails are generated at logical size. Also a **cache-key trap**: existing on-disk thumbnails are low-res |
| `src/core/FontIconPack.cpp` / `StyledIconPack.cpp` `pixmap()` | Custom `QIconEngine`s allocate `QImage(size, ...)` and call `paint()` | Both are vector (icon font / SVG via `QSvgRenderer`) so sharp at any size. Qt's default `scaledPixmap` hook calls `pixmap()` with `size * scale` and fixes up the ratio afterwards (read from Qt source). Low risk; spot-check only |
| `src/app/ThumbnailSequence.cpp` ~1422 | `pageThumb.pixmap(QSize{23,17})` for left/right-page badges | Uses the no-DPR `QIcon::pixmap` overload; the result may be 1x. Move to the overload taking a device pixel ratio (documented as available since Qt 6.0) |
| `src/core/StageListView.cpp` ~270 | Batch-processing bubble animation pixmaps `QPixmap(squareSide, squareSide)` | Blurry animation on Retina |
| `src/core/filters/page_split/Thumbnail.cpp` (`m_trashPixmap`), `UnremoveButton.cpp`, `src/core/filters/deskew/ImageView.cpp` (`m_handlePixmap`) | Small pre-rendered pixmaps drawn with `drawPixmap` | Blurry handles/buttons on Retina |
| `src/core/zones/ZoneContextMenuInteraction.cpp` ~84 | Small coloured swatch `QPixmap`s for menu entries | Minor |
| QPainter overlays (zones, splines, deskew grid, guides) | Vector drawing in widget coordinates | Already sharp; just verify pen widths look right |

### Table Stakes

| ID | Feature | Why Expected | Complexity | Notes |
|----|---------|--------------|------------|-------|
| HDP-00 | **Measure first:** log `QWindow::devicePixelRatio()` / `QScreen::devicePixelRatio()` from the unbundled binary on the actual Mac, and screenshot a zoomed page view | Establishes the baseline and settles the unbundled-binary question (below) before any code changes | LOW | See "Open question" under this table. |
| HDP-01 | Page image view (`ImageViewBase` HQ pixmap path) renders at physical resolution: target size and transform include `devicePixelRatioF()`, resulting pixmap has `setDevicePixelRatio()` set, draw position and clip stay in logical units | This is the thing the user stares at all day; blurry zoomed-in page text is the headline Retina failure | MEDIUM | Touches `initiateBuildingHqVersion`, `HqTransformTask`, `hqVersionBuilt`, `validateHqPixmap`, `paintEvent`. Memory: HQ pixmap is viewport-sized (about 20 MB at 2880x1800), acceptable. Work stays on the background executor as today. |
| HDP-02 | Invalidate and rebuild the HQ pixmap when the device pixel ratio changes (window dragged between screens, display scaling changed) | Without it, one stale low-res pixmap persists after moving to an external monitor | LOW-MEDIUM | Handle `QEvent::DevicePixelRatioChange` / screen change in `ImageViewBase`. Depends on HDP-01. |
| HDP-03 | Thumbnails sharp: `ThumbnailBase` temp pixmap and thumbnail source images generated at `size * DPR` | Thumbnail strip is always visible | MEDIUM | Depends on a cache decision (HDP-04). Keep `QPixmapCache` key including DPR. |
| HDP-04 | On-disk/in-memory thumbnail cache keyed or versioned so old low-res thumbnails are not reused | Otherwise HDP-03 appears to "not work" until the cache is cleared | LOW | Check where `ThumbnailPixmapCache` stores its files and whether size is part of the key (`loadSaveThumbnail`). |
| HDP-05 | Small raster glyphs (page badges, bubble animation, handles, trash/unremove, menu swatches) created DPR-aware, or redrawn as vectors | Visible blur on UI chrome | LOW each, MEDIUM total | Prefer drawing with `QPainter` or reusing the existing SVG icon engine over adding `@2x` PNG files. |
| HDP-06 | Use `devicePixelRatioF()` (double), never an integer ratio | macOS "scaled" display modes and external monitors give non-integer effective factors | LOW | Review rule, not a task. |
| HDP-07 | Delete Qt-5 HiDPI attributes (see BLD-02) | They are no-ops/deprecated in Qt 6 and misleading | LOW | |

**Open question (VERIFY, HDP-00):** Whether an *unbundled* Qt executable gets the full Retina backing scale on macOS. Historically macOS ran non-bundled binaries and bundles without `NSHighResolutionCapable` in low-resolution compatibility mode. My search found that Qt's own build tooling adds `NSHighResolutionCapable` for bundled apps and found no authoritative statement for bare executables. If `devicePixelRatio()` reports 1.0 on the Retina display, the fix is BLD-10 (embedded `Info.plist`), not code. This could invalidate all of HDP-01..05 as "needed", so it is the first check in the phase.

### Differentiators

| ID | Feature | Value Proposition | Complexity | Notes |
|----|---------|-------------------|------------|-------|
| HDP-10 | Automated DPR test: render a view/thumbnail into a `QImage` with `devicePixelRatio = 2` (or run the Qt tests with `QT_SCALE_FACTOR=2` under the offscreen platform) and assert the HQ pixmap's ratio and pixel size | Catches HiDPI regressions without a Retina screen | MEDIUM | Offscreen platform ratio handling is MEDIUM-LOW confidence; render-to-`QImage` is the safer route. |
| HDP-11 | Hairline-correct overlays: align 1-px cosmetic lines and handle centres to device pixels | Crisper alignment handles, guides, deskew grid | LOW-MEDIUM | Only after HDP-01. Visual polish, not correctness. |
| HDP-12 | Defer HQ rebuild while zooming/panning; keep fast path | Already largely present (`scheduleHqVersionRebuild` + timer) | LOW | Keep, do not regress. |

### Anti-Features

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| Setting `QT_SCALE_FACTOR` / `QT_ENABLE_HIGHDPI_SCALING` or the old Qt-5 attributes to "force" Retina | Quick fix for blur | Qt 6 documents the scaling env var as a testing aid; forcing it hides the real cause (logical-size rasters) | Fix the rasters |
| Always render the page at 2x supersampled | Simple | Memory/CPU blow-up on 600 dpi scans; the HQ path is already viewport-sized | Viewport x DPR only |
| Hiding blur by turning on `SmoothPixmapTransform` for the upscale | One-liner | Soft text instead of sharp text; the app deliberately disables smoothing for the 1:1 HQ path | Real physical-resolution pixmap |
| `@2x` PNG sets for every raster icon | Qt supports `@2x` auto-loading | Asset duplication and drift for an app whose icons are already vector (SVG engine, icon fonts) | SVG / `QPainter` |
| Per-screen DPR caches or live multi-monitor DPR tracking beyond invalidate-and-rebuild | Completeness | One user, usually one screen; large complexity | HDP-02 only |
| Introducing an OpenGL rendering path | "Faster HiDPI" | OpenGL module is linked but not the view path; adds a second renderer to maintain | Keep raster painting |

---

## Area 3: Mac-conventional shortcuts and menus

### Facts that shape the design (verified against Qt 6 docs unless marked)

- Qt's `Ctrl` / `Qt::ControlModifier` / `Qt::CTRL` **is the Command key** on Apple platforms; `Meta` is the physical Control key. Existing `Ctrl+S`, `Ctrl+N`, `Ctrl+O`, `Ctrl+W`, `Ctrl+Q`, `Ctrl+G`, `Ctrl++`, `Ctrl+-` in `MainWindow.ui` and `Ctrl+1..5` in `output/TabbedImageView.cpp` therefore **already become Cmd shortcuts with no code change**. (HIGH)
- Physical **Control+click is delivered as a right-click** (documented: Control-click maps to `QContextMenuEvent` on a one-button mouse). The app handles context menus through `contextMenuEvent` (`ImageViewBase.cpp` 545, `output/ImageView.cpp` 45) and has no `Qt::RightButton` handling, so right-click, Control-click and two-finger secondary click all arrive the same way. (HIGH, from docs plus repo grep)
- A `QMenuBar` on the main window becomes the native menu bar automatically. A window without its own bar (e.g. a QDialog or a parentless window) uses the **default** menu bar, which is **empty** unless the app creates a **parentless `QMenuBar`** (the first one created becomes the default). (HIGH, Qt macOS docs)
- Menu roles: `TextHeuristicRole` is the default and matches by text: "about…" -> About, "config/options/setup/settings/preferences" -> Preferences, "quit/exit" -> Quit. The docs say the role can only be set before the menu bar is shown and only applies to **direct children** of top-level menus. The docs do not say whether the heuristic runs on translated text, so set roles explicitly. (HIGH for the rules; translation behaviour unconfirmed)
- `QKeySequence::Preferences` and `QKeySequence::SaveAs` exist as standard keys; macOS bindings are `Cmd+,` and `Cmd+Shift+S`. (Docs table was ambiguous for Preferences; MEDIUM. `Cmd+,` is the macOS convention in any case.)
- `QKeySequence::ZoomIn` is `Ctrl+Plus`; the docs list no `Ctrl+=` alternate. Mac users press the unshifted `=` key. (HIGH for docs, MEDIUM for the user-impact inference)
- Mac laptop keyboards have no Insert key and the key labelled "Delete" is Backspace; forum reports from the Qt 5 era describe Backspace and forward-Delete both arriving as `Qt::Key_Backspace` (QTBUG-33015). Not re-verified on Qt 6. (MEDIUM-LOW)

### Repo evidence

- Menu bar (File / Tools [object name `menuDebug`] / Help) is a plain `QMenuBar` in `MainWindow.ui`. No `menuRole`, no `QKeySequence::StandardKey`, no `setNativeMenuBar`, no `AA_DontUseNativeMenuBar` anywhere in `src/`.
- Actions: `actionSettings` text "Settings ..." (no shortcut), `actionAbout` "About", `actionQuit` "Quit" (`Ctrl+Q`, connected to `close()` so `closeEvent` runs, which is right), `actionSaveProjectAs` "Save Project As ..." (no shortcut), `actionDefaults` "Default parameters ...", `actionCloseProject` (`Ctrl+W`).
- Zone editor: `setShortcut(QKeySequence("Ins"))` and `"Delete"` in `select_content/ImageView.cpp` 152-153; status tip "Press Del to delete this zone" in `ZoneDefaultInteraction.cpp` 24. Deskew rotate uses `,` and `.`.
- Single-letter page/stage shortcuts (`Q`, `W`, `O`, `E`, `N`, `V`, `K`, `P`, `Home`, `End`, `PgUp`, `PgDown`, `Shift+...`) live in `MainWindow.ui`.
- Many user-visible strings say "Ctrl": status tips in `deskew/ImageView.cpp` 32, `output/TabbedImageView.cpp` 17, `ZoneDefaultInteraction.cpp` 24/30; tooltips in `MainWindow.ui` (425, 482, 549, 934); the help text in `page_layout/OptionsWidget.ui` (~905-907).
- Mouse-modifier interactions: `Qt::ControlModifier` / `ShiftModifier` / `AltModifier` combinations in `select_content`, `page_layout`, `deskew` (Ctrl+wheel to rotate), zones (`Ctrl+Alt+click`), thumbnails (`Ctrl+click` toggles selection, `Alt+wheel` scales).
- The only Mac code is the 200 ms `Q_OS_MAC` workaround in `ZoneContextMenuInteraction` (QAction `triggered` arrives about 150 ms after `aboutToHide` on macOS).
- A global Fusion style plus QSS (`ColorSchemeManager`, `*ColorScheme::getStyle()` returns `QStyleFactory::create("Fusion")`; QSS also styles `QMenuBar`) is applied on every platform.
- `RecentProjects.cpp` exists (the start panel lists recent projects) but there is no File > Open Recent menu.

### Table Stakes

| ID | Feature | Why Expected | Complexity | Notes |
|----|---------|--------------|------------|-------|
| MAC-01 | Native menu bar at the top of the screen with the standard application menu (About, Preferences, Hide, Quit) | The first thing a Mac user looks at | LOW | Mostly automatic from the existing `QMenuBar`. Task is to verify it and set roles explicitly. |
| MAC-02 | Explicit `menuRole`: `actionAbout` = `AboutRole`, `actionSettings` = `PreferencesRole`, `actionQuit` = `QuitRole`; set before the window is shown | Do not depend on heuristics that may not run on translated text; the app ships translations | LOW | Keep `actionQuit` wired to `close()` so the unsaved-changes prompt in `closeEvent` still runs; check that Dock "Quit" and Cmd+Q both end up there. |
| MAC-03 | `Cmd+,` opens Settings (`QKeySequence::Preferences`) | Universal Mac convention | LOW | The action has no shortcut today. Depends on MAC-02. |
| MAC-04 | `Cmd+Shift+S` for Save As (`QKeySequence::SaveAs`) | Standard; File menu has Save and Save As without it | LOW | |
| MAC-05 | Keep Cmd (not physical Control) as the primary modifier: use `QKeySequence::StandardKey` or plain `Ctrl+` in Qt notation; do **not** set `Qt::AA_MacDontSwapCtrlAndMeta` | Existing code becomes Mac-correct for free; flipping the attribute would turn every `Ctrl` shortcut into physical Control, which the OS reserves for right-click emulation | LOW | Review rule. |
| MAC-06 | Replace literal "Ctrl" in all user-visible strings with a platform-aware form (generate from `QKeySequence(...).toString(QKeySequence::NativeText)` or write "Cmd") and fix `.ui` tooltips/help text | A Mac user following "Ctrl+click" presses physical Control, gets a context menu instead, and concludes the feature is broken | MEDIUM (many scattered strings incl. `.ui` HTML and translations) | Inventory is in the Repo evidence list. Translated strings (`.ts`) will fall out of date; acceptable for a personal fork. |
| MAC-07 | Zone delete works with the Mac Delete key (Backspace) as well as forward-Delete; zone create has a binding that exists on a laptop keyboard (no Insert key) | Zone editing (picture/fill/content zones) is core to the mixed-page use case; today it is unreachable by key on a MacBook | LOW | Use `setShortcuts({Qt::Key_Backspace, Qt::Key_Delete})` and an extra create key; update status tips. **VERIFY** that Backspace actually reaches the action on Qt 6 (QTBUG-33015 history). Do not use `setShortcut` twice: it replaces. |
| MAC-08 | Page navigation keys remain reachable without dedicated keys: `Home`/`End`/`PgUp`/`PgDn` arrive via `fn`+arrows; the existing `Q`/`W` alternates stay | Laptop keyboards | LOW (verify only) | No new work expected; just do not remove `Q`/`W`. |
| MAC-09 | Control-click, two-finger click (secondary click) and right-click all open the zone context menu, and picking an item works the first time | Context menus are how picture/fill zones are managed | LOW-MEDIUM | Code already uses `contextMenuEvent`. Test the existing 200 ms `Q_OS_MAC` workaround against current macOS and Qt 6; it was written for old OS X and may now be unnecessary or insufficient. |
| MAC-10 | Modifier-based drags/wheel still work with **Cmd** (the Qt `Ctrl` modifier) and are documented that way; physical Control must not be required anywhere | Control+click is hijacked by the OS | LOW | Audit list: `deskew/ImageView` wheel, `select_content` and `page_layout` drag modifiers, zone interactions, thumbnail multi-select. Cmd+click toggling and Shift+click range select match Finder conventions. |
| MAC-11 | `Cmd+=` also zooms in (add alongside `Ctrl++`) | Users press the unshifted key | LOW | `setShortcuts({QKeySequence::ZoomIn, QKeySequence("Ctrl+=")})` on the thumbnail magnify action. |
| MAC-12 | A menu bar that is still populated when a dialog (Settings, Fix DPI, Relinking, Default Parameters) is frontmost | Otherwise the menu bar looks empty or wrong while a dialog is active | LOW | Create one parentless `QMenuBar` (Qt docs: the first one becomes the default) containing at least File/Quit and Help/About, or confirm modal dialogs inherit correctly. VERIFY behaviour on the actual Mac. |
| MAC-13 | Menu text without Windows idioms: no `&` mnemonics, "Settings" lives in the app menu rather than under "Tools", no "Exit" wording | Half-native menus are the classic giveaway | LOW | Keep the "Tools" menu for the rest; roles move Settings automatically. |
| MAC-14 | Native file dialogs (see BLD-06) | Same reason | LOW | |

### Differentiators

| ID | Feature | Value Proposition | Complexity | Notes |
|----|---------|-------------------|------------|-------|
| MAC-20 | File > Open Recent submenu backed by the existing `RecentProjects` | Standard Mac File menu item; data already exists | LOW-MEDIUM | |
| MAC-21 | `setWindowFilePath(projectPath)` and `setWindowModified` with `[*]` in the title: proxy icon, Cmd-click path popup, and the dot in the close button for unsaved changes | Very visible Mac-native feel at almost no cost | LOW | Depends on knowing when the project is dirty; `MainWindow` already tracks this for the save prompt. |
| MAC-22 | Dock attention on batch completion (`QApplication::alert`) in addition to the beep | Replaces the shell-command alarm with something native | LOW | |
| MAC-23 | `Ctrl+Cmd+F` full screen via `QKeySequence::FullScreen` plus a View menu item | Convention for image-heavy apps | LOW | |
| MAC-24 | A Window menu (Minimize, Zoom) | Convention, but low value for a one-window app | LOW | VERIFY what Cocoa/Qt adds automatically before building it by hand. |
| MAC-25 | Strip `QMenu` / `QMenuBar` rules from the QSS on macOS, so popup context menus are native-looking | A stylesheet'd `QMenu` is drawn by Qt, not by the system | LOW | Confidence LOW that this changes anything visible; try it and look. |
| MAC-26 | A shortcut-cheat-sheet help entry listing the Mac equivalents | The app is keyboard-heavy | LOW | Optional. |

### Anti-Features

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| Writing Objective-C++/`NSMenu` code to build the menu bar | "Truly native" | Qt's roles and native menu bar already deliver it; adds a language and a platform file | `QAction::menuRole` |
| `Qt::AA_DontUseNativeMenuBar` or `setNativeMenuBar(false)` | Easier to style | In-window menu bar on a Mac is the textbook half-native look | Leave native |
| `Qt::AA_MacDontSwapCtrlAndMeta` | "Keep Ctrl as Ctrl" | Breaks all existing shortcuts and collides with Control-click right-click emulation | Accept Cmd |
| Maintaining parallel Ctrl and Cmd shortcut sets | Support both | On a Mac they are the same Qt modifier, so it is meaningless | One set |
| Using `Meta` as the primary modifier because Control-click is "taken" | Avoid conflict | Maps to physical Control, which is context-click territory | Cmd, document it |
| Per-user shortcut customisation UI | Power users | Large UI and persistence surface for one user | Edit the code |
| Reworking the whole UI to the native `macos` Qt style | "Looks native" | The app depends on its own palette/QSS schemes and Fusion; dark-mode following is out of scope. Switching style would break the colour-scheme system | Keep Fusion + schemes, make only menus, shortcuts, dialogs native; be aware Fusion controls will not look native and accept it |
| Following system dark mode or adding gesture support | Mac expectations | Explicitly out of scope for this milestone | Defer |
| Re-translating all `.ts` files for the new key names | Completeness | Personal fork, translations are not a goal | Fix source strings only |
| Registering file associations / opening `.ScanTailor` by double-click | Mac convention | Requires a bundle (out of scope) | Open from within the app or the command line |

---

## Area 4: Regression test suite for the image pipeline

### What table-stakes coverage looks like

For an image-processing pipeline, the conventional strategy is a layered one:

1. **Ground-truth-by-construction tests** (primary). Because the inputs are synthetic, the correct answer is known (skew angle, spine position, text block rectangle, speckle count). Assert measured properties with tolerances. These tests survive refactors and library upgrades.
2. **Golden-image (snapshot) tests** (secondary). Record the current output once, compare later runs with a tolerance, fail with a saved diff image. Needed here because of the Behaviour constraint ("output for existing settings should not change unintentionally"): the user wants characterization of *current* behaviour, not only "correct" behaviour. Search results (WebKit, Flutter, Hugo discussions) consistently recommend a small per-pixel threshold plus a bounded fraction of differing pixels instead of byte equality; exact equality only where output is integer-deterministic. A large tolerance makes a golden test pass regardless of bugs, so keep it tight.
3. **Metamorphic / property sweeps**. Rotate, shift, rescale or repeat the input and assert the relation (detected skew = injected angle; processing an already clean page changes nothing). Boost.Test's `BOOST_DATA_TEST_CASE` covers sweeps; no property-testing library is needed.
4. **Round-trip tests** for persistence: write -> read -> write again must be a fixed point.

An arm64-specific issue: fused multiply-add changes floating-point rounding between architectures (the Hugo project needed tolerances on arm64/s390x/ppc64 for goldens generated on amd64). Goldens are generated and consumed on the same arm64 Mac, but a macOS, Qt or libtiff upgrade can still shift pixels, so tolerances are mandatory for any stage that scales, rotates, or dewarps.

### Table Stakes

| ID | Feature | Why Expected | Complexity | Notes |
|----|---------|--------------|------------|-------|
| TST-01 | **Synthetic image generator** (shared test utility): seeded, font-free, deterministic. Primitives: paper background with optional noise and gradient (uneven lighting), "text lines" drawn as rows of small solid glyph-like blocks, rotation by a known angle, picture region (gradient or halftone pattern), coloured element (red stamp, tinted paper), speckles, dark scanner border, two-page spread with spine gap and shadow, cylindrical warp of text lines using a known model | Everything else depends on it; fixtures are code, not files, so the repo stays small and no personal scans are committed | MEDIUM-HIGH | Do **not** use `QPainter::drawText`: font rendering differs across macOS/Qt versions and would make goldens unstable. Set DPI explicitly (e.g. 300) because output size and despeckle depend on it. The existing `src/imageproc/tests/Utils.{h,cpp}` has random/make helpers; extend rather than start over, and consider hosting the generator where both `imageproc_tests` and `core_tests` can link it. |
| TST-02 | **Image comparison helper** with per-channel tolerance, max differing-pixel fraction, mean absolute error, and on failure write expected/actual/diff PNGs to `${CMAKE_BINARY_DIR}/test-artifacts` | Makes failures diagnosable and keeps artifacts out of the repo | LOW-MEDIUM | Binary (1-bit) outputs: compare XOR pixel count, allow a tiny budget. |
| TST-03 | **Golden update workflow**: goldens regenerated only by an explicit command or environment variable (for example `UPDATE_GOLDEN=1`), never during a normal `ctest` run; goldens small (a few hundred pixels per side, 1-bit or indexed PNG), stored under each module's `tests/golden/` | Without this, goldens either rot or silently auto-update | LOW | The user reviews the diff of a golden before committing it. |
| TST-04 | **Flat-document pipeline tests**: skew detection recovers injected angle (sweep, for example -5..+5 degrees in 0.5-degree steps, tolerance about 0.1-0.2 degrees); deskew output is straight; content box on a known text block (IoU at least 0.95); margins/page layout produce the expected output size; binarization against known text mask (pixel accuracy) for Otsu, Sauvola and Wolf under uneven illumination | The user's most common job; maps to `SkewFinder`, `ContentBoxFinder`, `Binarize` | MEDIUM | Resurrect `src/imageproc/tests/TestBinarize.cpp` (currently `#if 0`) as real tests (requirement from CONCERNS.md). |
| TST-05 | **Book-page pipeline tests**: page split finds the spine within a stated tolerance on a synthetic two-page spread; both halves come out; dewarping reduces curvature of synthetic text lines (for example RMS deviation of detected baselines from a straight line drops below a fixed bound) | Second use case; dewarping has no tests at all | HIGH | Dewarping (`DistortionModelBuilder`, `TextLineTracer`, `TopBottomEdgeTracer`) is the hardest to make deterministic and to hit with a synthetic input. Start by testing `CylindricalSurfaceDewarper` and `RasterDewarper` with a known model (exact geometry), then add one end-to-end warped page with a loose tolerance. |
| TST-06 | **Mixed-page output tests** driven through `OutputGenerator::process`: picture zone keeps greyscale/colour inside the zone while the outside is pure black/white; colour mode preserves a red element's hue within tolerance; despeckle removes injected speckles (expected component count) but keeps text components; one golden image per mode | Third use case and the highest-risk, completely untested file (`OutputGenerator.cpp`, 2814 lines) | HIGH | Characterize current behaviour first (record, then assert), because the Behaviour constraint is "do not change output unintentionally". Setup needs `ImageTransformation`, content rect, `Settings`/params and a `TaskStatus`; wrap in one test helper. |
| TST-07 | **Project-file round-trip**: build a project (pages, sub-pages after split, non-default parameters in every filter) -> `ProjectWriter::write` -> parse with `QDomDocument` -> `ProjectReader` + `readFilterSettings` -> write again -> second file equals first (compare canonical XML). Include special characters in file and directory names (`&`, `<`, `>`, `"`, `'`, umlauts, spaces, non-ASCII) | Closes the CONCERNS.md XML FIXME (`XmlUnmarshaller::string` "needs unescaping") with evidence either way | MEDIUM | `ProjectWriter`/`ProjectReader` live in the `core` library that `core_tests` already links. Non-default values matter: default values may be omitted on write and would hide bugs. |
| TST-08 | **Default-parameters XML round-trip** (`DefaultParams` profiles) and **backward-compatibility load** of a hand-written project file in the 1.2.1 format | The deskew defaults change (`d710384`) is called out as fragile; old files must still load | LOW-MEDIUM | Hand-write the compat fixture as synthetic XML pointing at generated images. |
| TST-09 | **Test plumbing**: all targets run under `ctest` with a total budget well under a minute; `QT_QPA_PLATFORM=offscreen` set through the test environment so no windows appear; temp files in a per-test directory; the `|| true` problem does not arise because CI is removed | Tests that open windows or leave files behind get skipped in practice | LOW | Add `set_tests_properties(... ENVIRONMENT ...)` to each `add_test`. |
| TST-10 | Run the existing suite green first (BLD-03) and treat any currently failing test as a finding, not something to skip | Baseline for later changes | LOW-MEDIUM | |

### Differentiators

| ID | Feature | Value Proposition | Complexity | Notes |
|----|---------|-------------------|------------|-------|
| TST-20 | **Metamorphic tests**: rotating the input by 90 degree multiples gives the matching orientation result; translating content does not change the content-box size; same page at 300 and 600 dpi yields the same physical output size; running deskew on its own output gives about 0 degrees; larger despeckle level never leaves more components | Catches bugs that tolerances on a single image miss, no goldens needed | MEDIUM | Use `BOOST_DATA_TEST_CASE` with fixed seeds. |
| TST-21 | **Sanitizer build option** (ASan + UBSan via `-fsanitize=address,undefined`) for the test targets | CONCERNS.md flags ~114 raw `new` and ~148 `.get()/.release()` ownership sites and unbounded TIFF allocation; sanitizers find these during tests | LOW-MEDIUM | Apple clang supports ASan/UBSan/TSan. Run only on demand, not in the default test run. |
| TST-22 | **Coverage via LLVM source-based coverage** (`-fprofile-instr-generate -fcoverage-mapping`, `llvm-cov`) | Shows which pipeline files the new tests really reach (especially `OutputGenerator.cpp`) | MEDIUM | `TESTING.md` documents gcov/lcov, which is awkward on Apple clang. Informational only. |
| TST-23 | **Malformed-input tests** for the hardening items: oversized TIFF dimensions rejected before allocation; project file with out-of-range DPI/size values clamped or rejected | Locks in the security fixes listed in CONCERNS.md | MEDIUM | Depends on the fixes existing; write the test first (expected failing), then the fix. |
| TST-24 | **Thread/cancel tests** for `BackgroundExecutor` / `ProcessingTaskQueue` | Flagged fragile area | MEDIUM-HIGH | Flaky by nature; keep deterministic with explicit synchronisation, or skip. |
| TST-25 | HiDPI view test (see HDP-10) | Locks in the Retina work | MEDIUM | Lives in `qt_tests`. |
| TST-26 | "Test the tests": a one-off check that deliberately perturbing a threshold or angle makes the new suite fail | Confirms tolerances are tight enough to catch regressions | LOW | Manual checklist item per phase, not automation. |

### Anti-Features

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| Real personal scans as fixtures | More realistic | Out of scope; privacy; large binaries | Synthetic images only; the user may run ad-hoc manual comparisons outside the repo |
| Pixel-exact goldens for scaled, rotated or dewarped output | Simplest assertion | Breaks on any OS/Qt/libtiff update and on FMA differences; creates noise that trains you to ignore failures | Tolerance plus measured properties |
| Text rendered with `drawText` in generated images | Realistic look | Font and hinting changes across macOS/Qt versions shift goldens | Block-glyph text lines drawn with `fillRect` / integer geometry |
| GUI/screenshot-based tests of widgets or the main window (Squish, pixel-comparing windows) | "End-to-end" | Flaky, needs a display, slow; `TESTING.md` already marks it as a distant option | Test algorithm classes and `Task`-level functions; keep `qt_tests` for small widget logic |
| Full-pipeline tests through the GUI event loop and `WorkerThreadPool` | Realism | Timing-dependent | Call the same functions the `Task`s call, synchronously |
| Property-testing framework (RapidCheck etc.) or fuzzing infrastructure | "Property-based inputs" | New dependency for modest gain; Apple's clang does not bundle libFuzzer | Seeded sweeps with `BOOST_DATA_TEST_CASE` plus a handful of explicit malformed-input cases |
| Coverage percentage gate | Looks rigorous | Encourages shallow tests | Use TST-22 as a map, not a gate |
| Auto-updating goldens during normal runs, or `ctest ... \|\| true` | Convenience | Hides regressions | TST-03 |
| Large golden image sets | Thoroughness | Repo bloat, slow review | A few small images per mode |
| Mocking framework | Isolation | Codebase has none; `TESTING.md` advises real value types | Hand-written stubs only where I/O is expensive |

---

## Feature Dependencies

```
BLD-01 (builds + runs)
    +--requires--> BLD-02 (Qt 6 only cleanup)   [one code path before touching HiDPI]
    +--requires--> BLD-03 (tests build + run)
    +--enables---> HDP-00 (measure DPR on the real binary)
                       +--if DPR is 1.0--> BLD-10 (embedded Info.plist)
                       +--if DPR is 2.0--> HDP-01 (HQ page pixmap)
                                               +--requires--> HDP-02 (DPR-change rebuild)
                                               +--enables---> HDP-10 / TST-25 (HiDPI test)
                       HDP-04 (thumbnail cache key) --required-by--> HDP-03 (sharp thumbnails)
                       HDP-05 (small glyph pixmaps)  [independent]

MAC-02 (explicit menu roles)
    +--requires--> MAC-13 (menu text cleanup)    [roles read the final text]
    +--enables---> MAC-03 (Cmd+, Preferences)
MAC-12 (default parentless menu bar)  --enhances--> MAC-01
MAC-06 (Ctrl -> Cmd strings) --depends-on--> MAC-10 (modifier audit)  [text must match what works]
MAC-07 (Backspace/zone keys) --enhances--> zone workflows tested by TST-06
MAC-09 (context menu) --depends-on--> BLD-01 (needs a running app)
MAC-21 (window file path / modified) --depends-on--> project dirty tracking in MainWindow (exists)

TST-01 (synthetic generator)
    +--required-by--> TST-04, TST-05, TST-06, TST-20
TST-02 (compare helper) + TST-03 (golden workflow)
    +--required-by--> TST-06 (golden per output mode), TST-05 (dewarp golden)
TST-09 (ctest plumbing) --required-by--> all TST-*
TST-07 (project round-trip) --independent-of--> image generator (needs only tiny placeholder images)
TST-23 (malformed-input tests) --precedes--> TIFF/XML hardening fixes
TST-04/05/06 (characterisation) --should-precede--> any change to OutputGenerator.cpp / ContentBoxFinder.cpp split-ups ("known-debt cleanup")

Conflicts:
AA_MacDontSwapCtrlAndMeta  --conflicts--> MAC-05/MAC-10 (Cmd as primary modifier)
Pixel-exact goldens        --conflicts--> OS/Qt upgrades, FMA differences
Forcing QT_SCALE_FACTOR    --conflicts--> HDP-01 (hides real fix)
```

### Dependency Notes

- **Tests before refactors:** the "known-debt cleanup" requirement (splitting `OutputGenerator.cpp`, `MainWindow.cpp`) is only safe after TST-04..06 give a characterization net. Order the roadmap accordingly.
- **HDP-00 gates the HiDPI phase:** if the bare binary runs at 1x, the work shifts from many code edits to one linker setting plus the same raster fixes.
- **The generator (TST-01) is the biggest single investment** and serves every pipeline test; schedule it first, and build the flat-document fixture before the dewarp fixture.
- **MAC-06 and MAC-10 should be done together:** text and behaviour must agree on Cmd.

## MVP Definition

### Launch With (this milestone)

- [ ] BLD-01..06 - builds, runs, tests green, Qt 6 only, working beep, native dialogs
- [ ] HDP-00, HDP-01, HDP-02, HDP-03/04, HDP-05 (plus BLD-10 if HDP-00 says so) - sharp page view, thumbnails and chrome on Retina
- [ ] MAC-01..14 - native menu bar with explicit roles, `Cmd+,`, `Cmd+Shift+S`, Cmd-correct strings, Mac-reachable zone keys, working context menus
- [ ] TST-01..03, TST-04, TST-06, TST-07, TST-09, TST-10 - generator, comparator, golden workflow, flat-document and mixed-page output tests, project round-trip, ctest plumbing
- [ ] TST-05 at the geometry level (dewarper with a known model) and one loose end-to-end warped page

### Add After Validation (same milestone if time, else next)

- [ ] TST-08 default-params and backward-compat load - trigger: any change to `DefaultParams`
- [ ] TST-20 metamorphic sweeps - trigger: first false pass of a tolerance-based test
- [ ] TST-21 sanitizer option - trigger: before touching ownership code during cleanup
- [ ] MAC-20, MAC-21, MAC-22 - cheap Mac-feel upgrades
- [ ] BLD-11, BLD-12, HDP-10, TST-25

### Future Consideration

- [ ] TST-22 coverage reporting, TST-23 malformed-input tests (as soon as the hardening fixes are scheduled), TST-24 thread tests
- [ ] MAC-23..26, HDP-11
- [ ] Anything under "Anti-Features"

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| BLD-01 build and run | HIGH | MEDIUM | P1 |
| BLD-02 Qt 6 only | MEDIUM | LOW-MEDIUM | P1 |
| BLD-04 working beep | LOW-MEDIUM | LOW | P1 |
| HDP-00 measure DPR | HIGH | LOW | P1 |
| HDP-01 sharp page view | HIGH | MEDIUM | P1 |
| HDP-03/04 sharp thumbnails | MEDIUM-HIGH | MEDIUM | P1 |
| HDP-05 small glyphs | MEDIUM | MEDIUM | P2 |
| MAC-01/02/03/04 menu roles and standard keys | HIGH | LOW | P1 |
| MAC-06/10 Cmd strings and modifier audit | HIGH | MEDIUM | P1 |
| MAC-07 zone keys on Mac keyboard | HIGH | LOW | P1 |
| MAC-09 context menu verification | HIGH | LOW-MEDIUM | P1 |
| MAC-12 default menu bar for dialogs | MEDIUM | LOW | P2 |
| MAC-20/21/22 recent menu, proxy icon, Dock alert | MEDIUM | LOW | P2 |
| TST-01 generator | HIGH | MEDIUM-HIGH | P1 |
| TST-02/03 comparator + golden workflow | HIGH | LOW-MEDIUM | P1 |
| TST-04 flat-document tests | HIGH | MEDIUM | P1 |
| TST-06 mixed-page output tests | HIGH | HIGH | P1 |
| TST-05 book/dewarp tests | MEDIUM-HIGH | HIGH | P1 (geometry) / P2 (end-to-end) |
| TST-07 project round-trip | HIGH | MEDIUM | P1 |
| TST-20 metamorphic | MEDIUM | MEDIUM | P2 |
| TST-21 sanitizers | MEDIUM | LOW-MEDIUM | P2 |
| TST-22 coverage | LOW-MEDIUM | MEDIUM | P3 |

**Priority key:** P1 must have this milestone; P2 should have; P3 later.

## Reference-Application Analysis

| Concern | Typical native Mac document/image apps (Preview, Pixelmator class) | Typical Qt apps that feel "half native" | Our approach |
|---------|--------------------------------------------------------------------|------------------------------------------|--------------|
| Menu bar | Native bar, Preferences under app menu with `Cmd+,`, Window and Help menus | In-window or empty bar when a dialog is up; "Exit" and "Options" under Tools | Roles + parentless default bar |
| Modifier keys | Cmd for commands, Option for alternate actions, Control rarely | Documentation says "Ctrl" | Strings generated from native key text |
| Delete | Backspace key deletes selection | Binds only forward-Delete or Insert | Bind both, add laptop-reachable create key |
| Retina | Crisp at all zoom levels | Upscaled pixmaps, blurry thumbnails | Physical-resolution HQ pixmap, DPR-aware thumbnails |
| Title bar | Proxy icon and unsaved dot | Plain title | MAC-21 |
| Tests (image tools such as unpaper, ImageMagick-style CLIs) | Reference-image comparisons with tolerance; unpaper's maintainer has blogged that small pixel differences should not fail tests | Byte-exact comparison that breaks on architecture change | Tolerance + measured properties |

## Sources

- Qt 6 `QAction` documentation, `MenuRole` values: https://doc.qt.io/qt-6/qaction.html (HIGH)
- Qt 6 `QMenuBar` documentation, macOS text heuristic and native menu bar: https://doc.qt.io/qt-6/qmenubar.html (HIGH)
- Qt for macOS issues (default menu bar, parentless `QMenuBar`, Ctrl/Meta swap, Control-click right-click): https://doc.qt.io/qt-6/macos-issues.html (HIGH)
- Qt 6 `QKeySequence` standard keys table: https://doc.qt.io/qt-6/qkeysequence.html (HIGH for the rows read; the Preferences row was ambiguous)
- Qt 6 High DPI overview: https://doc.qt.io/qt-6/highdpi.html (HIGH; "painting into a pixmap" is deferred to QPainter docs)
- Qt 6 `QIcon` (DPR overload since 6.0, `@2x` loading, SVG engine): https://doc.qt.io/qt-6/qicon.html (HIGH)
- Qt source `QIconEngine::scaledPixmap` / default hook calls `pixmap(size * scale)`: https://raw.githubusercontent.com/qt/qtbase/dev/src/gui/image/qiconengine.cpp (HIGH for logic, read via summary)
- Qt forum thread on Delete/Backspace on macOS (QTBUG-33015, Qt 5 era): https://forum.qt.io/post/207697 (LOW-MEDIUM, not re-verified on Qt 6)
- qBittorrent PR using Cmd+Backspace on macOS: https://github.com/qbittorrent/qBittorrent/pull/20668 (MEDIUM)
- Unbundled-binary Retina behaviour: fruitbasket notes https://git.trevorbentley.com/fruitbasket/commit/e7e5ae4731cfa413cd670de45a5458bc58b9111b.html and Qt Retina blog https://qt.io/blog/2013/04/25/retina-display-support-for-mac-os-ios-and-x11 (LOW; no authoritative statement found for bare Qt 6 executables)
- Golden-image tolerance practice: https://flameeyes.blog/2014/11/15/small-differences-don-t-matter-to-unpaper, https://bugs.webkit.org/show_bug.cgi?id=149828, https://src.stg.fedoraproject.org/rpms/hugo/blob/e64dfc80fd81760e26f48a6de9902dad92a81090/f/6396.patch (FMA on arm64), https://itnext.io/easy-flutter-golden-tests-with-tolerance-f4706e133239 (MEDIUM; practitioner posts, no standard numbers)
- Repo files read or grepped: `.planning/PROJECT.md`, `.planning/codebase/{ARCHITECTURE,TESTING,CONCERNS}.md`, `src/app/main.cpp`, `src/app/MainWindow.{ui,cpp}`, `src/core/ImageViewBase.cpp`, `src/core/ThumbnailBase.cpp`, `src/core/ThumbnailPixmapCache.cpp`, `src/core/{FontIconPack,StyledIconPack,StageListView,ColorSchemeManager,LightColorScheme}.cpp`, `src/core/zones/ZoneContextMenuInteraction.cpp`, `src/core/filters/{deskew,select_content,page_layout,output}/*`, `src/imageproc/tests/*`, test `CMakeLists.txt` files, root `CMakeLists.txt` (HIGH, direct inspection)

---
*Feature research for: macOS port and regression-test hardening of a Qt 6 image-processing desktop app*
*Researched: 2026-10-18*
