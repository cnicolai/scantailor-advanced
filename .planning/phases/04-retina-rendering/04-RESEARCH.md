# Phase 4: Retina Rendering - Research

**Researched:** 2026-10-25
**Domain:** Qt 6.12 HiDPI raster rendering in a QWidget / QGraphicsView desktop app on macOS (Apple Silicon, Cocoa)
**Confidence:** HIGH for the rendering technique and the offscreen test mechanism (both run in this session); MEDIUM for Cocoa-only behaviour that this Mac cannot exercise (display-move event delivery, fractional ratios)

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Carried forward (not re-asked)**
- **D-00a:** Phase 1 measured DPR 2.0 on the Studio Display from a bare, unbundled Qt 6.12.0 executable (`.planning/phases/01-deletion-free-macos-build/records/probe-output.txt`; screen and window both 2). The measurement is provisional because it came from a probe, not the app, and no `qt.highdpi` line was captured. Phase 4 confirms the ratio from the real app binary. An embedded `Info.plist` (`-sectcreate __TEXT __info_plist`, `NSHighResolutionCapable`) is added **only** if that check reports 1.0.
- **D-00b:** Every commit passes the Phase 3 gate (`.planning/phases/03-qt-6-and-macos-only/gate.sh`): it builds, `ctest --preset macos-release` and `ctest --preset macos-debug` pass 7/7, and no golden or manifest file changes. `UPDATE_GOLDENS` is never set. Retina work stays in the view layer.
- **D-00c:** `QT_SCALE_FACTOR`, `QT_ENABLE_HIGHDPI_SCALING` and the Qt 5 HiDPI attributes are never used as the fix (REQUIREMENTS Out of Scope). Fix the raster buffers instead. Inside tests, a scale factor or an offscreen DPR setting is allowed as a test aid only.
- **D-00d:** The OpenGL viewer path is ignored, including `ImageViewBase::sampleImageAtWidgetRect` in the colour picker. It stays off by default, and Phase 6 (STRIP-07) removes it, relying on this phase's colour-picker test.

**Verification on the Mac**
- **D-01:** Claude verifies everything. There is no GUI walkthrough and no human-verify checkpoint, as in Phases 2 and 3. Proof comes from `ctest`, from Claude's own checks of the real app on the Studio Display, and from committed records. The end-of-phase report is a short written summary.
- **D-02:** Success criteria 2 and 3 (sharp page view at fit, 100% and 200%; sharp thumbnails, zone masks, badges, handles and the stage-list animation) are checked by Claude on the real Cocoa display:
  - Claude launches the build-dir app or a harness on the Studio Display.
  - It grabs the widgets at 2× (`QWidget::grab()` returns the backing-store pixels at the window's DPR, so no screen-recording permission is needed).
  - It judges sharpness by measurement. Examples: buffers are logical size × DPR and carry DPR 2; a 1-px edge in a synthetic page covers one device pixel, not two; the grab matches a reference resampling of the same synthetic page at device resolution.

  "As sharp as Preview" is satisfied by that reference comparison, not by opening Preview. Only synthetic pages are used, and verdicts and any screenshots go into `.planning/phases/04-retina-rendering/records/`. The Phase 1 probe showed that a process Claude starts from its own shell reaches the window server at DPR 2.
- **D-03:** The Mac has only the Studio Display (DPR 2) and no non-Retina screen. Success criterion 4 (moving between a Retina and a non-Retina display re-renders without a restart) is proven by simulation: an automated test gives the view a 1× and a 2× screen, moves the page view and the thumbnail view between them, and asserts that the HQ page pixmap, thumbnails and overlay pixmaps are rebuilt at the new ratio. The records state that no physical 1× display was available. The preferred mechanism is Qt's offscreen platform with two configured screens of different DPR, if Qt 6.12's offscreen plugin supports per-screen DPR (research to confirm). The fallback is to deliver `QEvent::DevicePixelRatioChange` or a screen change to the widgets directly. — **Reversibility:** reversible — the criterion can later be re-checked live if a 1× display is attached.
- **D-04:** All 2× checks that don't need the physical display become permanent tests in `qt_tests` and run on every `ctest`, offscreen: buffer sizes and DPR for the HQ pixmap, thumbnails and overlays, the D-03 display-move test, and the colour-picker sampling test that criterion 5 requires. This brings **TESTX-03** ("Automated HiDPI render test checks view buffers are built at 2×", listed under v2 in `.planning/REQUIREMENTS.md`) into Phase 4, so its traceability row should move to Phase 4 when the requirements are next updated. The live Studio Display checks from D-02 run once and are recorded, because `ctest` runs offscreen.

**Thumbnails and their cache**
- **D-05:** The Settings "Quality" value (`ApplicationSettings::getThumbnailQuality`, `src/core/ApplicationSettings.cpp:183`) becomes a screen-independent size. The thumbnail file is built at Quality × the DPR of the screen showing the thumbnails (`devicePixelRatioF()`, never a hard-coded 2). The setting is still stored as before, and a value the user set is kept and multiplied, not rewritten. — **Reversibility:** reversible — one multiplication; old and new files sit side by side under different names.
- **D-06:** The Quality default rises from 200 to 250, to match the default display Size of 250 (`DEFAULT_MAX_LOGICAL_THUMBNAIL_SIZE`, 250×160). On Retina, thumbnail files are then 500 px (`…_q500.png`), exactly sharp at the default Size. `DEFAULT_THUMBNAIL_QUALITY` in `src/core/ApplicationSettings.cpp:25` and the Quality tooltip in `src/app/SettingsDialog.ui` (which says "The default value is 200") change with it. The tooltip should say the value is multiplied on Retina.
- **D-07:** The thumbnail file name keeps encoding the effective pixel size (`q<width>` in `ThumbnailPixmapCache::Impl::getThumbFilePath`). That makes the on-disk cache keyed by DPR (HIDPI-03), and a DPR change picks the matching file or builds it. The in-memory caches also need DPR in their keys: the `QPixmapCache` key `"ThumbnailBase::tempPixmap"` in `src/core/ThumbnailBase.cpp:102` and any pixmap cache in `ThumbnailPixmapCache`.
- **D-08:** Old thumbnail files (`…_q200.png`) in existing projects' `<output>/cache/thumbs/` are **left in place**. The app never deletes anything in the user's output folders because of this change. The user can delete `cache/thumbs` by hand.
- **D-09:** On the first open of an existing project, missing thumbnails at the new size show the usual placeholder until the background loader has built them, exactly as today for any missing thumbnail. There is no fallback to stretching the old q200 file.

### Claude's Discretion
- **Zoomed-in look (the user did not choose to discuss it; default recorded).** Keep today's behaviour: smooth sampling while zoomed out, hard pixel edges once a source pixel is wider than 0.5 mm on screen (`ImageViewBase::paintEvent`, `src/core/ImageViewBase.cpp:417`, `widthMM() * xscale / width()`). Recalibrate it so the switch happens at the same physical size on Retina. Research must check what `widthMM()` and `width()` return for a widget on the Studio Display under Qt 6 (STATE research flag). The comment "HQ pixmap maps one to one to screen pixels" becomes true only once the HQ pixmap is built at device pixels.
- **Colour picker (the user did not choose to discuss it; default recorded).** Keep the median colour of the circle as drawn on screen (`ColorPickupInteraction::takeColor`, `src/core/filters/output/ColorPickupInteraction.cpp:67`, `grab()` path). Make it correct for a 2× grab, and keep the circle the same size on screen in logical pixels. Picked colours in existing projects are untouched. The automated test (D-04) puts a known colour under the cursor at DPR 2 and asserts the returned colour.
- **Rule for every screen buffer:** view-space coordinates stay logical. Every pixel buffer built for the screen is logical size × `devicePixelRatioF()` and gets `setDevicePixelRatio()`. Positions and clip paths stay logical. Write the rule into the code once.
- **The downscaled preview pixmap `m_pixmap`** (`ImageViewBase`, `m_pixmapToImage` at `src/core/ImageViewBase.cpp:169`, drawn through `PixmapRenderer`) stays at DPR 1 unless `m_pixmapToImage` and `PixmapRenderer::drawPixmap` change in the same commit (PITFALLS Pitfall 8).
- **Small icon pixmaps** (deskew handle, trash and unremove buttons, page badges, context-menu swatches): use the `QIcon::pixmap` overload that takes a device pixel ratio, or draw vector shapes with `QPainter`. Don't add `@2x` PNG files.
- **Memory:** the HQ pixmap costs viewport × DPR² (about 59 MB ARGB32 for a full-screen window on the 5K Studio Display). That is acceptable, so there is no tiling unless a measurement shows a problem.
- **Order, plan split and commit split.** Suggested order from PITFALLS Pitfall 8: `HqTransformTask` and the HQ pixmap, the `PictureZoneEditor` mask, the `ThumbnailBase` temp pixmap, `ThumbnailPixmapCache`, small pixmaps and the stage-list animation, DPR-change handling, then the colour picker.
- **How the HIDPI-01 re-check from the real app is done** (for example, `qt.highdpi` logging captured from a Claude-launched run, or a check inside the live harness).
- **Where the live-check harness lives:** a one-off scratchpad program, or a committed tool. If it extends `src/app/ui_snapshot`, note open review finding WR-01: `ui_snapshot` already compiles the GUI sources a second time.
- **The Quality spin box range** (100–1500 today, which allows up to 3000-px files at 2×) and the exact tooltip wording.

### Deferred Ideas (OUT OF SCOPE)
- Zoomed-in look and colour-picker semantics were offered as discussion areas and not chosen. Their defaults are recorded under Claude's Discretion. Switching the picker to sample the image's own pixels instead of the drawn screen would be a separate decision for a later phase.
- Deleting stale thumbnail files of other sizes: rejected for now (D-08). A cache-cleanup command could be a later feature.

Also out of scope per the CONTEXT domain section: the OpenGL viewport path (off by default, removed in Phase 6), any change to `imageproc/`, `dewarping/` or any `Task::process`, menus and shortcuts (Phase 5), and bundling.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| HIDPI-01 | The device pixel ratio of the bare build-dir binary is measured on a Retina display; if it reports 1.0, an embedded `Info.plist` makes it 2.0 | The app binary has the same Mach-O state as the Phase 1 probe (no `__info_plist`, same `minos 26.0` / `sdk 27.0`); measure in-process with a committed or scratch harness, not with `qt.highdpi` logging; linker flags for the fallback were built and checked (see "HIDPI-01") |
| HIDPI-02 | The main page view renders the page sharply at 2× at every zoom level | HQ path inventory and exact change (device-resolution transform, `setDevicePixelRatio`, logical `QPointF` position, DPR in `validateHqPixmap`); pixel-exactness of a DPR-2 pixmap in a `grab()` verified (2048/2048 pixels); transform cost measured |
| HIDPI-03 | Thumbnails render sharply, with the thumbnail cache keyed by device pixel ratio | `ThumbnailPixmapCache` gets a DPR; file name `q<effective width>` already keys the disk cache; `ThumbnailBase` temp pixmap at DPR with DPR in the `QPixmapCache` key; Quality default 250; memory clamp recommended |
| HIDPI-04 | Overlay pixmaps (zone masks, page badges, handles, stage-list animation) render sharply | Zone-mask and stage-list buffers must be rebuilt at DPR; handles, trash and unremove icons are already sharp (QIcon returns DPR 2) but their **geometry is wrong today** because it is read from `pixmap.rect()`; badges verified to need no change |
| HIDPI-05 | Moving a window between a Retina and a non-Retina display re-renders at the right resolution | `QEvent::DevicePixelRatioChange` (value 222, Qt 6.6) reaches widgets and children; reproduced offscreen with two screens; paint-time DPR validation as a safety net |
| HIDPI-06 | The colour picker samples the correct pixel on Retina | `grab(rect)` at DPR 2 returns 30×30 px with DPR 2 (verified); the legacy algorithm is nearly DPR-invariant; recommended logical-block sampling keeps DPR-1 results identical; extract a pure function for the test |
| TESTX-03 (brought forward by D-04) | Automated HiDPI render test checks view buffers are built at 2× | Two-screen offscreen mechanism that works in Qt 6.12 (JSON `configfile` + `QT_SCREEN_SCALE_FACTORS`); the JSON `dpr` key alone does NOT change window DPR |
</phase_requirements>

## Summary

The page view and thumbnails are blurry on Retina for one reason: every screen buffer is allocated in logical pixels and carries DPR 1 (`devicePixelRatio` appears nowhere in `src/`; verified with grep this session). The fix is the rule already recorded in CONTEXT: view-space coordinates stay logical, each screen buffer is `logical size × devicePixelRatioF()` with `setDevicePixelRatio()`. Two facts verified in this session change the shape of the work. First, `QIcon::pixmap(w, h)` already returns a DPR-2 pixmap of twice the pixel size whenever any screen has ratio 2 (`qApp->devicePixelRatio()` is the maximum over all screens), so the handle, trash and unremove icons are already sharp, but the code that centres and hit-tests them uses `pixmap.rect()` / `pixmap.width()`, which are device pixels. On the Studio Display today the deskew and page-split handles are drawn 8 logical pixels up-left of their anchor, the trash icon 32, and the unremove button 64 (with a 256-px hit rectangle). That is a geometry fix (`deviceIndependentSize()`), not a sharpness fix. Second, the 0.5 mm smoothing heuristic needs no recalibration: `widthMM()` is the screen's physical width scaled by logical widget width over logical screen width, so `widthMM() * xscale / width()` is millimetres per source pixel regardless of DPR, and it only governs the instant preview (the HQ path uses an area-averaging resampler that gives hard source-pixel edges when magnified by itself).

The test mechanism that CONTEXT D-03 asked about works, with one correction. Qt 6.12's offscreen plugin accepts `QT_QPA_PLATFORM=offscreen:configfile=<json>` with several named screens, but the `dpr` key in that JSON only sets the screen's native ratio: a window moved onto such a screen still reports DPR 1. What does change the window ratio, and delivers `QEvent::DevicePixelRatioChange` to the widget and its children when the window changes screen, is `QT_SCREEN_SCALE_FACTORS="OneX=1;TwoX=2"` on a two-screen JSON that has no `dpr` key. Both are set with `qputenv` in `qt_tests/main.cpp` before the `QApplication` exists, and are test aids only (D-00c). On that setup `QWidget::grab(QRect(15×15))` returns a 30×30 pixmap with DPR 2, a DPR-2 pixmap drawn at a logical position lands pixel-for-pixel on device pixels even with antialiasing and a polygon clip, and the HQ transform costs about 120 ms for a half-width 5K window (about 45 ms at DPR 1). Nothing in the work touches `imageproc/`, any `Task::process`, or output generation, so `pipeline_tests` and its goldens are the proof that output files stay unchanged.

**Primary recommendation:** Add one small `core` helper that states the buffer rule once, convert `ImageViewBase`'s HQ path first (device transform and target, DPR on the image, logical `QPointF` origin, DPR in `validateHqPixmap`), then the zone mask, the thumbnail cache plus temp pixmap, the stage-list frames and the icon-geometry fixes, then the colour-picker extraction; build a two-screen offscreen `TestHiDpiQt` in `qt_tests` first so each step is test-driven; do the live Cocoa measurement with a small harness that links `core` only (not `ui_snapshot`).

## Project Constraints (from CLAUDE.md)

Source: `/Users/christoph/Developer/scantailor-advanced/.claude/CLAUDE.md` (read this session) plus the user's global `~/.claude/CLAUDE.md`.

- C++17 only; do not introduce C++20. Qt 6, CMake, Ninja; keep the six-stage pipeline.
- **Behaviour:** output for existing settings must not change unintentionally; synthetic test images only; no personal scans committed.
- Formatting is a manual gate: `xcrun clang-format -i <file>` on every touched C++ file; check with `xcrun clang-format --dry-run -Werror <file>`. `src/dewarping/TextLineTracer.cpp` keeps two older violations.
- Header guards `SCANTAILOR_<MODULE>_<NAME>_H_`; no `#pragma once`; new files start with the project copyright header; `m_` prefix for members; `tr()` / `QCoreApplication::translate` for user-visible strings.
- Do not swallow exceptions silently; keep diagnostics out of normal paths (about 19 debug calls in `src/`; do not add more).
- Module headers are included by bare name (each module directory is an include root).
- Work goes through GSD commands; do not edit the repo outside a GSD workflow.
- User global rules: one manual step at a time (not relevant here, D-01 has no human steps); never touch OmniFocus; never read or write outside the project directory without asking; scratch files go under `build/scratch/` (not `/tmp`).
- Memory notes (MEMORY.md): use `xcrun clang-format`; run `ctest` yourself in execute; no `gsd_run` in subagents.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Page-view HQ pixmap at device resolution | View layer (`ImageViewBase`, `core`) | Background executor (`HqTransformTask`) | The widget knows its DPR; the heavy resample stays on the existing background thread and is passed `dpr` by value |
| Zone-mask pixmap at device resolution | View layer (`PictureZoneEditor`, an interaction handler on the view) | Background executor (`MaskTransformTask`) | Same rule; DPR comes from the owning `ImageViewBase` |
| Thumbnail pixel size and on-disk cache key | Cache (`ThumbnailPixmapCache`) | `MainWindow` (supplies DPR) | File name `q<width>` is the key; the cache must receive the DPR at creation and on change, because `LoadFileTask` and the output `Task` write thumbnails from worker threads before any paint |
| Thumbnail drawing (temp pixmap) | Graphics item (`ThumbnailBase::paint`) | — | Reads the DPR from the painter's device, so it also works when rendering into a `QImage` in tests |
| Small overlay icons (handles, trash, unremove, badges) | Icon engines (QIcon, already DPR aware) | Callers (geometry) | Sharpness is already correct; callers must use `deviceIndependentSize()` for layout and hit testing |
| Stage-list animation frames | `StageListView` | `BubbleAnimation` | Frames are pre-rendered pixmaps; they must be rebuilt on DPR change and painted with logical size |
| DPR-change detection | `QEvent::DevicePixelRatioChange` handlers + paint-time check | `MainWindow` event filter (thumbnails) | Event for immediate invalidation; paint-time comparison covers a missed event |
| Colour picking | `ColorPickupInteraction` (grab path) | `ImageViewBase::grab` | Samples what is drawn; sampling must be a pure function of the grabbed image and its DPR |
| Output files (TIFF/PNG) | Output stage (`Task::process`, `OutputGenerator`) — untouched | — | No Retina work may reach it; proof is `pipeline_tests` goldens |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt 6 (Core, Gui, Widgets, Svg, Test) | 6.12.0 [VERIFIED: `.planning/phases/01-deletion-free-macos-build/records/probe-output.txt` line `QLibraryInfo::build(): Qt 6.12.0 (arm64-little_endian-lp64 shared (dynamic) release build; by Apple LLVM 21.0.0 (clang-2100.3.34.2))`; `Qt6_DIR:PATH=/opt/homebrew/lib/cmake/Qt6` in `build/macos-release/CMakeCache.txt`] | All rendering and the Qt Test framework for the new test class | Already the project's stack; Qt 6 turns high-DPI scaling on unconditionally, so the work is only to size buffers correctly |
| Qt Test (`Qt6::Test`) | 6.12.0 | New `TestHiDpiQt` class in `qt_tests` | `qt_tests` already links `Qt6::Test`; `QTRY_VERIFY`, `QSignalSpy`, `QTest::qWaitForWindowExposed` fit the async HQ build |

### Supporting
No new libraries. Tools used by the work: CMake 4.4.4 and Ninja 1.13.2 [VERIFIED: `cmake --version`, `ninja --version` this session]; Apple `clang-format` 21.0.0 via `xcrun` [VERIFIED: `xcrun clang-format --version`].

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Per-widget DPR in each buffer site | One `hidpi` helper in `core` | Helper states the rule once (CONTEXT: "Write the rule into the code once") and gives a pure function to unit test |
| `QT_SCALE_FACTOR=2` for the whole `qt_tests` run | Two named screens plus `QT_SCREEN_SCALE_FACTORS` | The single factor cannot move a window between a 1× and a 2× screen (D-03); the per-screen setup can |
| JSON `dpr` key in the offscreen config | `QT_SCREEN_SCALE_FACTORS` | The `dpr` key leaves the window ratio at 1 (verified); do not use it |
| New `@2x` PNGs for icons | Existing SVG icon engines | Icons are already vector (`:/icons/*.svg`); QIcon returns DPR-2 pixmaps |

**Installation:** none. Nothing is installed in this phase.

**Version verification:** no package versions to verify beyond Qt, taken from the probe record and the build cache above.

## Package Legitimacy Audit

No external packages are installed or added in this phase (Qt, Boost and the image libraries are already provided by Homebrew and unchanged). The audit table is therefore empty: **Packages removed due to [SLOP] verdict:** none. **Packages flagged as suspicious [SUS]:** none.

## Architecture Patterns

### System Architecture Diagram

```
                       screen / window DPR
 Cocoa window --(backingScaleFactor)--> QWindow::devicePixelRatio  ----> QWidget::devicePixelRatioF()
      |                                   |                                    |
      | moved to another display          | QEvent::DevicePixelRatioChange     |
      +-----------------------------------+--> (to widget AND children)        |
                                                                                v
 PAGE VIEW (ImageViewBase)                                          dpr = viewport()->devicePixelRatioF()
   paintEvent -> validateHqPixmap()? --no (xform, source id, or dpr differs)--> scheduleHqVersionRebuild
        | yes                                                         (150 ms timer) -> initiateBuildingHqVersion
        v                                                                      |
   painter.drawPixmap(QPointF logicalPos, m_hqPixmap [dpr])                    v   BackgroundExecutor thread
        ^                                                          HqTransformTask(image, xform, logicalViewportSize, dpr)
        |   hqVersionBuilt(deviceOrigin, image[dpr]) <-- GUI-thread event --   devXform = xform * scale(dpr)
        |   pos = deviceOrigin / dpr                                           target = devXform(image) ∩ (viewport*dpr)
        |                                                                      imageproc::transform(...)  [unchanged code]
 ZONE MASK (PictureZoneEditor)  same pattern with transformToGray, mask pixmap [dpr], logical origin

 THUMBNAILS
   MainWindow --(dpr at project open, and on DevicePixelRatioChange)--> ThumbnailPixmapCache::setDevicePixelRatio
        |                                                                 effective size = Quality * dpr
        |   LoadFileTask / output Task (worker threads) --ensure/recreateThumbnail--> <out>/cache/thumbs/<base>_<page>_<md5>_q<effective>.png
        v
   ThumbnailBase::paint(painter): dpr = painter->device()->devicePixelRatioF()
        loadRequest -> pixmap (thumbnail file, DPR 1, size = effective px)
        tempPixmap = QPixmap(logical*dpr) + setDevicePixelRatio  (QPixmapCache key includes dpr)
        draw pixmap -> tempPixmap through world transform (logical coordinates), then tempPixmap -> view at logical rect

 COLOUR PICKER   grab(logical 15x15) -> 30x30 [dpr 2] -> pure function: median of logical-pixel circle

 OUTPUT FILES    Task::process / OutputGenerator  (not touched; guarded by pipeline_tests goldens)
```

### Recommended Project Structure
```
src/core/
├── HiDpi.h / HiDpi.cpp        # NEW: the one-place rule + pure geometry helpers (add to common_sources in src/core/CMakeLists.txt)
├── ImageViewBase.{h,cpp}      # HQ path, event(), test accessors
├── ThumbnailPixmapCache.{h,cpp}  # setDevicePixelRatio, effective size
├── ThumbnailBase.cpp          # temp pixmap at DPR
├── StageListView.{h,cpp}      # frames at DPR, rebuild on DPR change
├── BubbleAnimation.cpp        # unchanged (callers pass an explicit rect)
└── filters/output/{PictureZoneEditor,ColorPickupInteraction}.{h,cpp}
src/qt_tests/
├── main.cpp                   # writes the two-screen JSON, sets env before QApplication
├── tst_hidpi_qt.{h,cpp}       # NEW: TestHiDpiQt
src/app/MainWindow.cpp         # DPR into the cache; DevicePixelRatioChange on thumbView
```

### Pattern 1: The buffer rule (state once, in `HiDpi.h`)
**What:** View-space coordinates are logical. A buffer built for the screen is `logical size * dpr` pixels, carries `setDevicePixelRatio(dpr)`, is positioned in logical coordinates, and is clipped in logical coordinates. `dpr` always comes from `devicePixelRatioF()` of the widget (or `painter->device()->devicePixelRatioF()` when no widget is at hand), never from a constant.
**When to use:** every pixmap or image that the widget paints onto the screen.
**Example (suggested helper shape; every value shown is a Qt API or a name from the repo):**
```cpp
// src/core/HiDpi.h  (guard: SCANTAILOR_CORE_HIDPI_H_)
namespace hidpi {
// Device-pixel size of a logical size. Rounds each side; never returns an empty size for a non-empty input.
QSize deviceSize(const QSize& logical, double dpr);
// Logical-to-device transform for the HQ / mask transforms: xform * scale(dpr, dpr).
QTransform toDevice(const QTransform& logicalXform, double dpr);
// Target rect of a transformed image inside a device-sized viewport (the exact computation both
// HqTransformTask and MaskTransformTask do today, with dpr applied).
QRect targetRect(const QRectF& imageRect, const QTransform& logicalXform, const QSize& logicalViewport, double dpr);
// Logical position of a device-pixel origin.
QPointF toLogical(const QPoint& deviceOrigin, double dpr);
// Pixmap of logical size * dpr with the ratio set (use for overlays and animation frames).
QPixmap makePixmap(const QSize& logicalSize, double dpr);
}  // namespace hidpi
```
At `dpr == 1.0` every helper must return exactly what the old code computed (`xform * QTransform::fromScale(1, 1)` is exact), so DPR-1 behaviour is unchanged.

### Pattern 2: HQ page pixmap (HIDPI-02)
**What:** the change to `ImageViewBase`, in the order the data flows.
**Where (all `src/core/ImageViewBase.cpp` unless noted):**
- `initiateBuildingHqVersion` (934-954): `const double dpr = viewport()->devicePixelRatioF();` pass it to `HqTransformTask` (constructor at 1015-1019) next to `viewport()->size()`, and store `m_hqDpr = dpr;` beside `m_hqXform` / `m_hqSourceId`. Keep `m_hqXform` logical.
- `HqTransformTask::operator()` (1021-1040): replace the target and transform with the device versions:
```cpp
const QTransform devXform(hidpi::toDevice(m_xform, m_dpr));
const QRect targetRect(hidpi::targetRect(QRectF(m_image.rect()), m_xform, m_targetSize, m_dpr));
QImage hqImage(transform(m_image, devXform, targetRect, OutsidePixels::assumeWeakColor(Qt::white), QSizeF(0.0, 0.0)));
hqImage = hqImage.convertToFormat(hqImage.hasAlphaChannel() ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32);
hqImage.setDevicePixelRatio(m_dpr);               // after convertToFormat
m_result->setData(targetRect.topLeft(), hqImage);   // device-pixel origin
```
- `hqVersionBuilt` (959-968): `m_hqPixmap = QPixmap::fromImage(image);` keeps the image's ratio [VERIFIED: probe2, pixmap built by `QPixmap::fromImage` from a DPR-2 image drew pixel-exactly]; `m_hqPixmapPos = QPointF(origin) / image.devicePixelRatio();`
- `ImageViewBase.h:341`: `QPoint m_hqPixmapPos;` becomes `QPointF`; add `double m_hqDpr` (initialise to 1.0).
- `paintEvent` (433): `painter.drawPixmap(m_hqPixmapPos, m_hqPixmap);` (the `QPointF` overload). Update the comment at line 426 ("HQ pixmap maps one to one to screen pixels") to say it is now true in device pixels.
- `validateHqPixmap` (902-919): add `if (!qFuzzyCompare(m_hqDpr, viewport()->devicePixelRatioF())) return false;`
- `scheduleHqVersionRebuild` (921-932): no change needed; `initiateBuildingHqVersion` cancels the old task when `validateHqPixmap()` is false.
- Keep `m_pixmap` / `m_pixmapToImage` / `PixmapRenderer::drawPixmap` (preview) at DPR 1 (CONTEXT).
**Why it is exact:** [VERIFIED: probe2] on a widget at DPR 2, a checkerboard DPR-2 pixmap drawn with `drawPixmap(QPointF(10,10) | QPointF(10.5,10.5))`, antialiasing on and off, under a polygon clip path with fractional edges, reproduced all 2048 sampled device pixels exactly in `widget.grab()`.
**Cost:** [VERIFIED: bench in `build/scratch/dpr-research/bench.cpp`] `imageproc::transform` of a 4960×7016 source into a 1280×1440-logical viewport (A4 at 600 dpi, half-width 5K window): about 45–60 ms at DPR 1 and 90–125 ms at DPR 2 (Release, Apple Silicon, synthetic white page; real content may differ). It runs on the background thread, behind the existing 150 ms timer. No tiling needed.

### Pattern 3: Zone mask (HIDPI-04)
Same change in `PictureZoneEditor.cpp`: `MaskTransformTask` (301-324) gets `dpr`, target and transform from `hidpi::*`, `mask.setDevicePixelRatio(dpr)` after `setAlphaChannel`. `screenPictureMaskBuilt` (199-206): origin is stored as `QPointF` (`origin / dpr`), header `PictureZoneEditor.h:83` `QPoint m_screenPictureMaskOrigin` becomes `QPointF`, add `double m_screenPictureMaskDpr` next to `m_screenPictureMaskXform` (line 84). `validateScreenPictureMask` (162-164) also compares the ratio. In `onPaint` (137-148) the per-frame animation copy must keep the ratio:
```cpp
QPixmap mask(m_screenPictureMask.size());                       // device pixels
mask.setDevicePixelRatio(m_screenPictureMask.devicePixelRatio());
mask.fill(Qt::transparent);
```
`maskPainter.translate(-m_screenPictureMaskOrigin)` and `painter.drawPixmap(m_screenPictureMaskOrigin, mask)` then take the logical `QPointF` unchanged. This animation redraws every 120 ms (`m_pictureMaskAnimateTimer`), so the per-frame pixmap is 4× larger at DPR 2; acceptable, measure in the live check.

### Pattern 4: Thumbnails (HIDPI-03)
**`ThumbnailPixmapCache` (`src/core/ThumbnailPixmapCache.{h,cpp}`):** add `void setDevicePixelRatio(double)`; in `Impl` add `double m_dpr` (default 1.0) and `QSize effectiveThumbSize() const` = `m_maxThumbSize` times `m_dpr`, each side `qRound`ed. `setDevicePixelRatio` locks, returns if unchanged, otherwise calls `removeLoadedItemsLocked()` exactly as `setMaxThumbSize` does (865-870) and stores the ratio. `getMaxThumbSize()` keeps returning the Quality value, so `MainWindow::onSettingsChanged` (1534-1537) still compares Quality to Quality. Replace every use of the member size for file work with the effective size copied under the mutex: the `loadNow` branch (375), `ensureThumbnailExists` (445, 448), `recreateThumbnail` (473, 476) and `backgroundProcessing` (577). Note `ensureThumbnailExists` (448) and `recreateThumbnail` (476) currently pass `m_maxThumbSize` (the member, unlocked) to `getThumbFilePath` instead of their local copy; that is an existing data race and must use the local copy in this edit.
**Key (D-07):** `getThumbFilePath` (610-633) builds `QChar('q') + QString::number(maxThumbSize.width())` (line 619) and the full name `<completeBaseName>_<zeroBasedPage>_<md5 of path>_q<width>.png`. With the effective size passed in, Retina files are `…_q500.png` at Quality 250, and `…_q250.png` at DPR 1; old `…_q200.png` files are left alone (D-08). In-memory items are keyed by `ImageId` only; flushing LOADED items on a ratio change makes the in-memory cache effectively keyed by DPR (only one ratio is live at a time).
**Creation:** `Utils::createThumbnailCache(const QString& outputDir)` (`src/core/Utils.cpp:42-46`) reads `ApplicationSettings::getInstance().getThumbnailQuality()` and builds the cache. Add a `double dpr` parameter and call `setDevicePixelRatio` before the cache is returned; `MainWindow.cpp:432` passes `devicePixelRatioF()`. If the ratio were pushed only after the first paint, `LoadFileTask` (`ensureThumbnailExists`, `LoadFileTask.cpp:69`) would first write DPR-1-size files for every page and then rebuild them from the full-size image.
**Change:** `MainWindow` already installs an event filter on `thumbView` (`MainWindow.cpp:516`, handler at 520). Handle `QEvent::DevicePixelRatioChange` there (and on the window): `m_thumbnailCache->setDevicePixelRatio(devicePixelRatioF()); thumbView->viewport()->update();`. The next `ThumbnailBase::paint` issues a fresh `loadRequest` because the flush removed the LOADED items.
**`ThumbnailBase::paint` (`src/core/ThumbnailBase.cpp:58-171`):**
```cpp
const double dpr = painter->device()->devicePixelRatioF();
const QString cacheKey = QStringLiteral("ThumbnailBase::tempPixmap@%1").arg(dpr, 0, 'f', 2);   // was fromLatin1("ThumbnailBase::tempPixmap")
// needed size in device pixels:
const int needW = qCeil(displayRect.width() * dpr);
const int needH = qCeil(displayRect.height() * dpr);
// compare tempPixmap.width()/height() (device px) with needW/needH, allocate +10 % as today,
// then:  tempPixmap = QPixmap(w, h);  tempPixmap.setDevicePixelRatio(dpr);  fill(transparent) as today
```
Everything the temp painter does is already in logical coordinates (`tempAdjustment` translates by the logical `displayRect`), and `painter->drawPixmap(displayRect.topLeft(), tempPixmap)` (line 170) draws at the logical position, so only the allocation and the key change. The loaded thumbnail `pixmap` stays DPR 1: `ThumbnailBase` maps it through `preScaleXform` built from `pixmap.width()` (line 87-89), which is correct for a device-pixel pixmap. `PixmapRenderer::drawPixmap(tempPainter, pixmap)` (line 132) reads `painter.viewport()`, which for a painter on a DPR-2 pixmap is in device pixels [VERIFIED: probe, `QPixmap 100x100 dpr2: devW=100 dpr=2 viewport=100x100 … deviceTransform.m11=2`]; the source rectangle it computes is then 2× too large, which is harmless because it is intersected with `pixmap.rect()` and the painter clip.
**Quality default (D-06):** `src/core/ApplicationSettings.cpp:25` reads `const QSize ApplicationSettings::DEFAULT_THUMBNAIL_QUALITY = QSize(200, 200);` and becomes `QSize(250, 250)`; the key is `THUMBNAIL_QUALITY_KEY = "thumbnail_quality"` (line 47) and is unchanged. `src/app/SettingsDialog.ui:110` reads `<string>The pixel size of the thumbnail image. The default value is 200.</string>`. Note that `SettingsDialog::commitChanges` (`SettingsDialog.cpp:102-103`) writes the spin-box value back with `settings.setThumbnailQuality(QSize(quality, quality))` on every OK, so any user who has pressed OK once has 200 stored explicitly and keeps getting `q400` on Retina (D-05 says keep and multiply; this is expected).
**Memory clamp (recommendation, security-relevant):** the spin box allows 100–1500 (`SettingsDialog.ui:112-117`). With ×2 that is 3000-px thumbnails; `ThumbnailPixmapCache` holds up to 40 pixmaps (`Utils.cpp:45`: `std::make_shared<ThumbnailPixmapCache>(thumbsCachePath, maxPixmapSize, 40, 5)`), so the worst case grows from about 360 MB (1500² × 4 B × 40) to about 1.4 GB. Clamp the effective width inside the cache (for example to 2000 px) and/or lower the spin-box maximum to 1000; state the multiplication in the tooltip.

### Pattern 5: Stage-list animation (HIDPI-04)
`StageListView::createBatchAnimationSequence(squareSide)` (`StageListView.cpp:259-277`) allocates `QPixmap(squareSide, squareSide)` (line 272) and calls `animation.nextFrame(headColor, tailColor, &pixmap)`; `BubbleAnimation::nextFrame(…, QPaintDevice* pd, QRectF rect)` substitutes `QRectF(0.0, 0.0, pd->width(), pd->height())` when `rect` is null, and `pd->width()` is the device width (probe: `devW=100` for a 100-px DPR-2 pixmap while the logical size is 50). So the frames must be created with `hidpi::makePixmap(QSize(squareSide, squareSide), devicePixelRatioF())` and rendered with an explicit rect `QRectF(QPointF(0, 0), pixmap.deviceIndependentSize())`. The comparison `pixmap.width() != squareSide` (line 271) must compare device sizes. `RightColDelegate::paint` (370-373) centres with `QRect r(pixmap.rect())`; use `QRect(QPoint(0, 0), pixmap.deviceIndependentSize().toSize())`. Rebuild the frames on `QEvent::DevicePixelRatioChange` in a new `StageListView::event` override (remember `squareSide` in a member; `createBatchAnimationSequence` is called only from `setStages`, lines 124-130).

### Pattern 6: Icon pixmaps: geometry, not sharpness (HIDPI-04)
[VERIFIED: probe, with the maximum screen ratio 2] `icon.pixmap(16, 16)` returns `w=32 dpr=2 logical=16`; with a single 1× screen it returns `w=16 dpr=1`. The icon engines `SvgStyledIconEngine` (`StyledIconPack.cpp:53, 80-88`) and `FontIconEngine` implement only `pixmap(size, …)`; Qt's default `QIconEngine::scaledPixmap` asks for `size × dpr` and sets the ratio, so the icons come out sharp with no change. The defects are in the callers. Replace device-pixel reads with logical ones:

| Site | Today | Change |
|------|-------|--------|
| `core/filters/deskew/ImageView.cpp:36` | `0.5 * m_handlePixmap.width()` | `0.5 * m_handlePixmap.deviceIndependentSize().width()` |
| `core/filters/deskew/ImageView.cpp:145` | `QRectF rect(m_handlePixmap.rect());` | `QRectF rect(QPointF(0, 0), m_handlePixmap.deviceIndependentSize());` |
| `core/filters/deskew/ImageView.cpp:259, 264` | `0.5 * m_handlePixmap.width()` / `.height()` | logical size |
| `core/filters/page_split/ImageView.cpp:61, 179, 192` | `0.5 * m_handlePixmap.width()` | logical size |
| `core/filters/page_split/ImageView.cpp:143` | `QRectF rect(m_handlePixmap.rect());` | logical rect |
| `core/filters/page_split/Thumbnail.cpp:60` | `QRectF rect(m_trashPixmap.rect());` (64×64 icon) | logical rect |
| `core/filters/page_split/UnremoveButton.cpp:25, 33` | `QRectF rect(pixmap.rect());` / `m_defaultPixmap.rect()` (128×128 icons; line 33 is the hit rectangle) | logical rect |
| `core/zones/ZoneContextMenuInteraction.cpp:94` | `pixmap = QPixmap(16, 16);` solid swatch | `hidpi::makePixmap(QSize(16, 16), context.imageView().devicePixelRatioF())` (a solid fill shows no blur; this is for consistency only) |

Verified to need no change: page badges `ThumbnailSequence.cpp:1422-1423` (`QGraphicsPixmapItem::boundingRect()` for a 46×34 DPR-2 pixmap is 23×17 [VERIFIED: probe4]) and `QLabel` icons (`ErrorWidget.cpp:16`, `StatusBarPanel.cpp:146-152`, `SplitModeDialog.cpp:41,58,64`, `DefaultParamsDialog.cpp:439`, `fix_orientation/OptionsWidget.cpp:224`, `MainWindow.cpp:1941`; `QLabel::sizeHint()` for the same pixmap is 23×17 [VERIFIED: probe4]). `RelinkingModel.cpp:91-92` returns `QPixmap` icons as model data; item-view handling of DPR pixmaps was not exercised, so include it in the live check.

### Pattern 7: Display move / DPR change (HIDPI-05)
- Delivery [VERIFIED: probe with `QT_SCREEN_SCALE_FACTORS`]: after `window->windowHandle()->setScreen(other)` or `window->move(...)` into the other screen's rectangle, the top-level widget received `QEvent::DevicePixelRatioChange` twice and a child widget once, each with the new `devicePixelRatioF()`, followed by `ScreenChangeInternal` and a repaint. Handlers must be idempotent. [CITED: doc.qt.io/qt-6/qevent.html] the enum entry is `DevicePixelRatioChange`, value 222, since Qt 6.6, "The devicePixelRatio has changed for this widget's or window's underlying backing store."
- `ImageViewBase`: override `bool event(QEvent*)`; on `DevicePixelRatioChange` cancel and reset `m_hqTransformTask`, `m_hqPixmap = QPixmap()`, `update()`. Independently, `validateHqPixmap()` compares the stored ratio at every paint (above), so a missed event still rebuilds on the repaint that follows a display move.
- Subclasses: `PictureZoneEditor` (compare in `validateScreenPictureMask`), `StageListView::event`, `MainWindow` filter for the thumbnail cache. Vector drawing needs nothing.
- `QIcon`-derived pixmaps use `qApp->devicePixelRatio()`, the maximum over all screens [CITED: Qt source `qicon.cpp`, `pixmap(const QSize&, qreal devicePixelRatio, …)` replaces -1 with `qApp->devicePixelRatio()`; VERIFIED by probe: `app.devicePixelRatio=2` with screens at 1 and 2], so they do not change when a window moves and are never softer than the best screen.
- Cocoa delivery of this event on a real display drag cannot be observed on this Mac (one display). [ASSUMED] Qt's Cocoa plugin raises it from the window's backing-scale change. The paint-time comparison is the mitigation; record it in the criterion-4 record.

### Pattern 8: Colour picker (HIDPI-06)
See "Colour-picker sampling path" below.

### Anti-Patterns to Avoid
- **Forcing the ratio:** `QT_SCALE_FACTOR`, `QT_ENABLE_HIGHDPI_SCALING`, Qt 5 attributes (REQUIREMENTS Out of Scope; D-00c). Tests may set `QT_SCREEN_SCALE_FACTORS`.
- **Reading logical geometry from `QPixmap::width()/rect()`:** these are device pixels for any DPR pixmap. Use `deviceIndependentSize()`.
- **Setting a DPR on `m_pixmap` or on the loaded thumbnail pixmap:** `m_pixmapToImage` and `ThumbnailBase`'s `preScaleXform` are computed from `pixmap.width()` in device pixels and depend on it (CONTEXT, PITFALLS Pitfall 8).
- **Hard-coding 2 or an integer ratio:** use `devicePixelRatioF()` everywhere.
- **Editing `imageproc/Transform.cpp` or any shared helper to be DPR aware:** the HQ code calls `transform()` with a device-scaled matrix; the function itself is also used by output generation and must stay byte-identical.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Pixel-exact drawing of a device-resolution buffer | Own blitting or pixel copying | `QPixmap` with `setDevicePixelRatio` + `QPainter::drawPixmap(QPointF, pixmap)` | Verified exact (2048/2048 pixels), including AA and clip path |
| Icon rendering at the right resolution | `@2x` assets or own SVG rasteriser | `QIcon::pixmap(size)` (DPR from the app) / `QIcon::pixmap(size, dpr)` | Already returns DPR-2 pixmaps with the project's SVG engines |
| Detecting a display change | Polling `screen()` or timers | `QEvent::DevicePixelRatioChange` + compare the stored ratio in `paintEvent` | Event verified offscreen; the paint comparison is free |
| Offscreen multi-DPR test platform | A test-only `devicePixelRatioF()` hook in production code | Offscreen plugin `configfile` + `QT_SCREEN_SCALE_FACTORS` | Exercises the real Qt scaling path and the real events |
| Thumbnail cache key scheme | A new on-disk layout or database | Existing `q<effective width>` file name | Already keyed by pixel size; old files simply stay (D-08) |
| Median colour | A second implementation | The existing bit-mixing median in `ColorPickupInteraction` (extract, do not rewrite) | Keeps DPR-1 results byte-identical |

**Key insight:** every defect here comes from one confusion, logical pixels versus device pixels. Centralising the conversion in one small helper, and testing it, is cheaper than reviewing each site.

## Raster Drawing Site Inventory (by requirement)

Verified by `grep -rn` over `src/` for `QPixmap`, `fromImage`, `.pixmap(`, `drawPixmap`, `drawImage`, `grab(`, `devicePixelRatio` this session (zero `devicePixelRatio` hits before the work).

| Req | Site (file:line) | Today | DPR source | Where sized | `setDevicePixelRatio` |
|-----|------------------|-------|------------|-------------|-----------------------|
| HIDPI-02 | `core/ImageViewBase.cpp:947` task creation, `1021-1040` task body, `964` pixmap, `433` draw, `902-919` validate, `ImageViewBase.h:341` `QPoint m_hqPixmapPos` | Viewport logical size as target, DPR-1 pixmap, int origin | `viewport()->devicePixelRatioF()` read in `initiateBuildingHqVersion` | `HqTransformTask` (device xform and device target) | On the `QImage` in the task, after `convertToFormat` |
| HIDPI-02 | `core/ImageViewBase.cpp:161-169` `m_pixmap`, `444` `PixmapRenderer::drawPixmap` | Downscaled preview at DPR 1 | — | — | Not set (CONTEXT) |
| HIDPI-04 | `core/filters/output/PictureZoneEditor.cpp:312-322` task, `137` per-frame mask copy, `148` draw, `200` pixmap, `162-164` validate; `.h:82-84` | Logical-size mask, int origin | `imageView.devicePixelRatioF()` (the editor is a `ZoneEditorBase`, an `ImageViewBase`) | `MaskTransformTask` | On the mask `QImage`; per-frame copy copies the ratio |
| HIDPI-03 | `core/ThumbnailBase.cpp:101-119` temp pixmap, `102` key, `170` draw | `QPixmap(w, h)` logical, one global key | `painter->device()->devicePixelRatioF()` | `paint` | On `tempPixmap` after construction |
| HIDPI-03 | `core/ThumbnailPixmapCache.cpp:375, 445-448, 473-476, 577, 610-633, 865-870`; `core/Utils.cpp:42-46`; `app/MainWindow.cpp:432, 1534-1537` | File named by Quality width, no DPR | `MainWindow::devicePixelRatioF()` pushed into the cache | `getThumbFilePath` / `makeThumbnail` receive the effective size | Not set (thumbnail pixmap stays DPR 1) |
| HIDPI-03 | `core/ApplicationSettings.cpp:25`, `app/SettingsDialog.ui:110` | Default 200 | — | — | — |
| HIDPI-04 | `core/StageListView.cpp:272, 275, 370-373` | Logical frames, `pd->width()` device-size trap | `StageListView::devicePixelRatioF()` | `createBatchAnimationSequence` | On each frame |
| HIDPI-04 | `core/filters/deskew/ImageView.cpp:26, 36, 145-160, 259, 264`; `core/filters/page_split/ImageView.cpp:34, 61, 143-149, 179, 192` | `pixmap(16,16)` is already DPR 2; geometry read in device px | `qApp` via QIcon (nothing to do) | — | Already set by QIcon |
| HIDPI-04 | `core/filters/page_split/Thumbnail.cpp:24, 60-62` (64×64); `core/filters/page_split/UnremoveButton.cpp:15-16, 25, 33` (128×128) | same | same | — | same |
| HIDPI-04 | `core/zones/ZoneContextMenuInteraction.cpp:86-96` | `QPixmap(16, 16)` | `context.imageView().devicePixelRatioF()` | here | after construction |
| HIDPI-04 (verify only) | `app/ThumbnailSequence.cpp:1422-1423` badges `pageThumb.pixmap(size)` / `QIcon::Selected` | DPR 2 pixmap, `QGraphicsPixmapItem` handles it | — | — | — |
| HIDPI-04 (verify only) | `app/RelinkingModel.cpp:91-92`, QLabel icon sites listed above | `pixmap(16,16)` etc. | — | — | — |
| HIDPI-06 | `core/filters/output/ColorPickupInteraction.cpp:70-126` | `grab(rect)` then median of a circle | widget ratio via the grabbed pixmap | — | — |
| not code | `core/FontIconPack.cpp:87-95`, `core/StyledIconPack.cpp:80-88` | engines `pixmap(size)` | Qt calls them with `size × dpr` | — | Qt sets it |
| out of scope | `ImageViewBase.cpp:152-156` OpenGL viewport, `273-290` `sampleImageAtWidgetRect` | — | — | — | Ignored (D-00d) |

No other raster site paints to the screen: `drawImage` hits in `select_content/ContentBoxFinder.cpp`, `imageproc/HoughLineDetector.cpp` and `dewarping/*` paint into debug images, not widgets.

## Colour-picker sampling path (HIDPI-06)

**Where the cursor maps to pixels:** `ColorPickupInteraction::targetBoundingRect()` (lines 128-133) takes `mapFromGlobal(QCursor::pos())` (logical), builds `QRect(0, 0, 15, 15)` and `moveCenter(mousePos)` ("Odd width and height are needed for symmetry"). `takeColor()` (67-126) hides the red circle with `ScopedIncDec<int> guard(m_dontDrawCircle)`, calls `m_context.imageView().grab(rect)` (line 76), converts with `pixmap.toImage().convertToFormat(QImage::Format_RGB32)` (80), then, with `xCenter = width / 2`, `yCenter = height / 2` and `sqdistThreshold = xCenter * yCenter`, takes the median of the pixels within that circle using the bit-mixing look-up tables.

**What happens at DPR 2 [VERIFIED: probe2]:** `grab(QRect(60, 30, 15, 15))` returns `30x30 dpr=2 logical=15`. So `image.width()` is 30, `xCenter` 15, threshold 225: the circle has the same size on screen (radius 7.5 logical) but four times the samples, and its centre is on pixel index 15 where the true centre of the logical pixel under the cursor is the continuous coordinate 15.0 (a half-device-pixel, 0.25 logical pixel, offset). For a flat colour under the cursor the result is the same colour as at DPR 1. The legacy code is therefore nearly DPR-invariant: the requirement is to make that explicit, keep DPR-1 results identical, and cover it with a test, not to fix a large error. A "fix" that misreads the 30×30 image as 15×15 (for example copying the first 15 columns) would sample the wrong quadrant, which is the regression the test must catch.

**Recommended implementation:** extract `static QColor ColorPickupInteraction::medianColor(const QImage& grabbed)` (pure; the table-driven median moves in unchanged). Compute the dpr from `grabbed.devicePixelRatio()` and include a device pixel in the circle when its logical-pixel index lies in the old circle: `lx = int(x / dpr)`, `ly = int(y / dpr)`, `(lx - lxCenter)² + (ly - lyCenter)² <= lxCenter * lyCenter` with `lxCenter = logicalWidth / 2` (logical width = `qRound(width / dpr)`). At DPR 1 this is the old predicate exactly; at an integer DPR every logical pixel contributes its d×d block, and the median element is the same as at DPR 1 when the content is the upscaled DPR-1 content (the median index `size / 2` maps to the same source element for even d²). `takeColor()` then reduces to `grab` → `medianColor` → the existing property updates (lines 119-125). The OpenGL branch (72-74) stays untouched (Phase 6 removes it).

**Test (qt_tests, offscreen, 2× screen):**
1. Pure function: build `QImage` 15×15 (dpr 1) and the same content at 30×30 (dpr 2) with a known pattern, assert both return the same `QColor`; assert a pattern that differs only in the top-left quadrant does not change the answer.
2. Real grab: show a `BasicImageView` (or any `ImageViewBase`) on the 2× screen with a synthetic image whose centre has a 12×12-logical patch of a known colour on a different background, `QCursor::setPos(view.mapToGlobal(centre))` [VERIFIED: offscreen `QCursor::setPos` / `pos()` / `mapFromGlobal` round-trip works], grab the same rectangle `targetBoundingRect()` would use, assert `grab.size() == QSize(30, 30)`, `devicePixelRatio() == 2`, and `medianColor(...) == patchColor`. A patch smaller than about 9×9 logical is out-voted by the background (the median is of the circle, not the pixel), so size the patch at 12×12 or larger.
3. If cheap: drive the whole interaction (`ZoneInteractionContext(view, zones)`, `setDefaultInteractionCreator` returning a new `InteractionHandler`, `startInteraction`, a `QMouseEvent` with `Qt::LeftButton` through `mousePressEvent`) and assert the `FillColorProperty` colour. `EditableZoneSet::Zone` has a private constructor, so obtain the zone by `zones.addZone(spline)` and iterating `zones`.

## Common Pitfalls

### Pitfall 1: `QPixmap::width()/rect()` are device pixels
**What goes wrong:** a DPR-2 icon of logical 16 px reports 32; code that centres or hit-tests with `pixmap.rect()` is wrong by a factor of two. On the Studio Display today: deskew and page-split handles drawn 8 logical px up-left of the anchor, the page-split trash icon 32, the unremove button 64, and its hit rectangle is 256×256 logical instead of 128×128.
**Why:** `QIcon::pixmap(w, h)` returns the app's maximum DPR without being asked.
**Avoid:** `deviceIndependentSize()` at every site in the table above; add a test that asserts the logical size of each icon pixmap.
**Warning signs:** handles visibly offset from their arcs; unremove button reacts far from where it is drawn.

### Pitfall 2: `BubbleAnimation::nextFrame(QPaintDevice*)` and `PixmapRenderer` read device sizes
**What goes wrong:** the default rect is `pd->width() × pd->height()` (device pixels), so a DPR-2 frame is painted at twice the intended size and clipped; `PixmapRenderer::drawPixmap` reads `painter.viewport()`, device-sized for a painter on a DPR pixmap.
**Avoid:** pass an explicit logical rect to `nextFrame`; leave `PixmapRenderer` alone (the over-large source rect is harmless, intersected with the pixmap and the clip).

### Pitfall 3: Offscreen `dpr` config key does not change the window ratio
**What goes wrong:** a test that sets `"dpr": 2.0` in the offscreen JSON sees `QScreen::devicePixelRatio() == 2` but `QWindow::devicePixelRatio() == 1` and 1× rendering [VERIFIED: probe run 1].
**Avoid:** use `QT_SCREEN_SCALE_FACTORS="OneX=1;TwoX=2"` with no `dpr` key. Combining both multiplies (screen ratio 4 in the probe).

### Pitfall 4: A window moved to a scaled offscreen screen changes its logical size
**What goes wrong:** the offscreen window keeps its native size, so `setScreen(TwoX)` halves the widget's logical size (200→100 in the probe).
**Avoid:** `resize()` after the move and before asserting sizes. Cocoa keeps the logical size; this is a test-platform quirk.

### Pitfall 5: `DevicePixelRatioChange` arrives more than once
**What goes wrong:** non-idempotent handlers rebuild twice, or cancel a task that was just started.
**Avoid:** compare the new ratio with the stored one before acting; keep the paint-time check as the source of truth.

### Pitfall 6: `QPixmapCache` limit
**What goes wrong:** the default limit is 10240 KB [CITED: doc.qt.io/qt-6/qpixmapcache.html]. The `ThumbnailBase` temp pixmap at the largest allowed thumbnail Size (1000×640 logical, `MainWindow.cpp` `scaleThumbnails` clamps to 1000 and 640) is about 2200×1400×4 B = 12 MB at DPR 2, over the limit. [ASSUMED] An insert of an oversized pixmap fails, so the temp pixmap would be re-allocated on every paint (slow, not wrong).
**Avoid:** keep the pixmap as a member of the item or raise the limit; or accept it, because the default Size is 250.

### Pitfall 7: Thumbnail load results racing a ratio change
**What goes wrong:** an item that is IN_PROGRESS at the moment of `setDevicePixelRatio` (or `setMaxThumbSize`, an existing behaviour) completes afterwards and is stored as LOADED with the old-size pixmap; it renders correctly (scaled through `pixmap.width()`) but at the old resolution until the next flush.
**Avoid:** optional generation counter in `Item` to discard stale results; otherwise document as accepted (rare, self-healing).

### Pitfall 8: Thumbnail memory at high Quality
See the clamp recommendation under Pattern 4.

### Pitfall 9: Do not let DPR leak into output generation
`HqTransformTask`, `MaskTransformTask`, `ThumbnailPixmapCache::makeThumbnail` and `ColorPickupInteraction` are view-side. `imageproc::transform` / `transformToGray` are shared with output generation and are called with a device-scaled matrix, never modified. `ImageViewBase::createDownscaledImage` (static, used by tasks incl. `output/Task.cpp:506-514`) must stay unchanged. The output `Task` calls `recreateThumbnail` (`output/Task.cpp:425`) for the output TIFF; with the new ratio it writes the thumbnail at the effective size, so the only change visible in `cache/thumbs` is the file-name width, and the output TIFFs themselves are not touched.

### Pitfall 10: Fractional ratios
Qt 6 rounding policy default is PassThrough [CITED: doc.qt.io/qt-6/highdpi.html, `QT_SCALE_FACTOR_ROUNDING_POLICY`]. macOS scaled modes normally report 2 or 1. Use `qRound` for device sizes, compare ratios with `qFuzzyCompare`, and let the painter round the logical position (the helper returns `deviceOrigin / dpr`, exactly representable for dpr 1, 2, 4).

### Pitfall 11: Logging rule used in Phase 1
The Phase 1 run used `QT_LOGGING_RULES="qt.highdpi.*=true"` and captured no line. [CITED: doc.qt.io/qt-6/qloggingcategory.html] the optional rule suffix must be `debug`, `info`, `warning` or `critical` and lines that do not fit are ignored; [CITED: Qt source qhighdpiscaling.cpp via code.qt.io search result] the category is `qt.highdpi`. The rule was therefore probably ignored; `qt.highdpi=true` would have been the matching rule. [ASSUMED] Do not rely on logging for HIDPI-01; measure in-process.

## Code Examples

### Two-screen offscreen setup in `qt_tests/main.cpp` (before `QApplication`)
```cpp
// Source: verified in build/scratch/dpr-research (screens2.json, probe.cpp); Qt 6.12 offscreen plugin
const QString screensFile = tmpRoot + "/offscreen-screens.json";
{
  QFile f(screensFile);
  f.open(QIODevice::WriteOnly | QIODevice::Truncate);
  f.write(R"({"screens":[
    {"name":"OneX","x":0,"y":0,"width":800,"height":600},
    {"name":"TwoX","x":800,"y":0,"width":1600,"height":1200}]})");
}
qputenv("QT_QPA_PLATFORM", ("offscreen:configfile=" + screensFile).toUtf8());   // replaces the current "offscreen"
qputenv("QT_SCREEN_SCALE_FACTORS", "OneX=1;TwoX=2");                           // test aid only (D-00c)
```
Observed with this configuration: `app.devicePixelRatio()` is 2; `OneX` ratio 1, `TwoX` ratio 2, both 800×600 logical; new windows open on `OneX`. `pipeline_tests` has its own `main` and keeps its environment (it must stay at DPR 1). The default single-screen offscreen platform has ratio 1.

### Moving a view between screens in a test
```cpp
// Source: verified in probe.cpp (events 222 and 216, devicePixelRatioF 1 -> 2 -> 1)
QScreen* two = screenNamed("TwoX");
view.show();
QVERIFY(QTest::qWaitForWindowExposed(&view));
view.window()->windowHandle()->setScreen(two);        // or: view.window()->move(two->geometry().topLeft() + QPoint(20, 20));
QTRY_COMPARE(view.devicePixelRatioF(), 2.0);
view.window()->resize(800, 600);                       // offscreen halves the logical size on a scaled screen
QTRY_VERIFY(view.hqPixmapIsCurrent());
QCOMPARE(view.hqPixmap().devicePixelRatio(), 2.0);
```
`setScreen()` before `show()` does not stick: `W::setScreen(TwoX)` reported `dpr=2` before show and `screen=OneX dpr=1` after it [VERIFIED: probe run 3]. Show first, then move.

### Thumbnail cache test (no widget needed)
```cpp
// Source: derived from ThumbnailPixmapCache API read this session (ctor, ensureThumbnailExists, loadNow)
ThumbnailPixmapCache cache(thumbDir, QSize(250, 250), 40, 5);
cache.setDevicePixelRatio(2.0);
cache.ensureThumbnailExists(ImageId(origPath), syntheticImage);           // image larger than 500 px
QVERIFY(QFile::exists(thumbDir + "/" + expectedName + "_q500.png"));
QPixmap pm; QCOMPARE(cache.loadNow(ImageId(origPath), pm), ThumbnailPixmapCache::LOADED);
QCOMPARE(pm.width(), 500);
cache.setDevicePixelRatio(1.0);                                           // flushes LOADED items
cache.ensureThumbnailExists(ImageId(origPath), syntheticImage);
QVERIFY(QFile::exists(thumbDir + "/" + expectedName + "_q250.png"));      // 500 px file is left in place (D-08)
QVERIFY(QFile::exists(thumbDir + "/" + expectedName + "_q500.png"));
```
`ensureThumbnailExists` takes the in-memory image, so no original file is needed for the test.

### Thumbnail paint into a DPR-2 image
```cpp
QImage target(QSize(2 * w, 2 * h), QImage::Format_ARGB32_Premultiplied);
target.setDevicePixelRatio(2.0);
QPainter p(&target);
ThumbnailBase item(cache, QSizeF(250, 160), imageId, xform);   // concrete; hooks have empty defaults
QStyleOptionGraphicsItem opt;
item.paint(&p, &opt, nullptr);
```
Warm the in-memory cache with `loadNow` first so `loadRequest` returns LOADED at once.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Qt 5 `AA_UseHighDpiPixmaps` / `AA_EnableHighDpiScaling` | Always on in Qt 6; pixmaps and images carry `devicePixelRatio` | Qt 6.0 | Nothing to enable; do not add attributes |
| `QIcon::pixmap(QSize)` at DPR 1 | Uses `qApp->devicePixelRatio()` when the ratio is not given [CITED: Qt source qicon.cpp] | Qt 6 | Icon pixmaps are DPR-2 on this Mac; callers must use logical size |
| `QEvent::ScreenChangeInternal` only | `QEvent::DevicePixelRatioChange` (222) delivered to widgets | Qt 6.6 [CITED: doc.qt.io/qt-6/qevent.html] | Public hook for HIDPI-05 |
| Offscreen plugin with one screen | `offscreen:configfile=<json>` with named screens | present in 6.12 [CITED: Qt source qoffscreenintegration.cpp, v6.12.0] | Enables the D-03 simulation |

**Deprecated/outdated:** `QIcon::pixmap(QWindow*, …)` is deprecated since Qt 6.0; use the overload with an explicit ratio when a specific window's ratio is wanted.

## Runtime State Inventory

Not a rename/refactor phase, but it changes where cached files are written, so the cache category is stated explicitly.

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | `<output>/cache/thumbs/*_q200.png` (and any other `q<n>`) in existing projects stay on disk; new Retina files `…_q500.png` (Quality 250 × 2) appear beside them. User settings key `thumbnail_quality` may hold 200 (written by every OK in the Settings dialog) and is kept (D-05) | Code edit only (new names); no migration, no deletion (D-08) |
| Live service config | None — verified: the app has no external services | None |
| OS-registered state | None — verified: no launchd/Task Scheduler entries in the project | None |
| Secrets/env vars | None. Tests set `QT_SCREEN_SCALE_FACTORS` and a `configfile` platform argument inside the test process only | None |
| Build artifacts | `build/macos-*` hold stale `.qm`/`.desktop`/CPack files (STATE.md Phase 3 note); not related to this phase | `cmake --fresh --preset <name>` optional |

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | build | ✓ | 4.4.4 | — |
| Ninja | build | ✓ | 1.13.2 | — |
| Apple clang-format | formatting gate | ✓ | 21.0.0 (`xcrun clang-format`) | — |
| Qt 6.12.0 (Homebrew) incl. Widgets, Svg, Test, offscreen plugin | all | ✓ | 6.12.0 | — |
| Studio Display (DPR 2) | live checks D-02 | ✓ (per Phase 1 probe) | 2560×1440 logical, 109.75 physical DPI | — |
| A 1× display | physical criterion 4 | ✗ | — | Offscreen two-screen simulation (D-03); record that no 1× display was available |
| Screen-recording permission | — | not needed | — | `QWidget::grab()` (D-02) |

**Missing dependencies with no fallback:** none.
**Missing dependencies with fallback:** a 1× display (simulated).

## HIDPI-01: measurement from the real app binary

- [VERIFIED: `otool -l build/macos-release/scantailor-advanced`] the app has `LC_BUILD_VERSION platform 1 minos 26.0 sdk 27.0` and no `__info_plist` section. [VERIFIED: same command on the Phase 4 scratch probe] the probe built from the same Homebrew toolchain has the identical `minos 26.0` / `sdk 27.0` and no `__info_plist`. The Phase 1 probe (`probe-output.txt`) reported `screen[0] devicePixelRatio: 2` and `window devicePixelRatio: 2` for that same kind of binary. AppKit therefore treats the app and the probe alike, so the measurement carries over; it is still provisional until measured from code that runs inside the project's own link.
- **Recommended re-check:** a tiny harness target (suggested `src/app/retina_check/main.cpp`, built under `BUILD_TESTS`, linking `core` only so it does not recompile the GUI sources a second time, which is the open `ui_snapshot` finding WR-01) that runs on the default Cocoa platform and prints `QGuiApplication::platformName()`, `screen->devicePixelRatio()`, `window->devicePixelRatio()`, `widget.devicePixelRatioF()` for a shown `BasicImageView`, then performs the D-02 measurements (buffer sizes, 1-px edge width, reference comparison) and writes PNGs and a text verdict into `records/`. Keep it out of ctest. A one-off scratchpad program is equally acceptable.
- **Best-effort supplement:** start the real `build/macos-release/scantailor-advanced` with `QT_LOGGING_RULES="qt.highdpi=true"` and capture stderr for a few seconds. It may print nothing in Qt 6.12; an empty log proves nothing either way.
- **Fallback only if a ratio of 1.0 is ever measured:** [VERIFIED: built in `build/scratch/dpr-research`, target `probe3`] `target_link_options(<target> PRIVATE "LINKER:-sectcreate,__TEXT,__info_plist,${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")` produced an executable with a `(__TEXT,__info_plist)` section of `size 0x138` (the plist). The plist needs `NSHighResolutionCapable` set to `<true/>` (and a `CFBundleIdentifier`). Whether AppKit honours the section for an unbundled executable was not tested (no 1.0 case exists), so this stays unproven: [ASSUMED]. The target would be `${target_name}` in `src/app/CMakeLists.txt`.

## `widthMM()` and the smoothing heuristic

- `ImageViewBase::paintEvent` computes `pixelWidth = widthMM() * xscale / width()` (line 417) and `setRenderHint(SmoothPixmapTransform, pixelWidth < 0.5)` (line 423).
- [VERIFIED: probe, offscreen] for a 200-px-wide widget on a screen with `physSize=203.2x152.4` and 800 logical px, `widthMM()` was 50; on the scaled screen with `physSize=406.4` and 800 logical px, a 100-px widget gave 50. So `widthMM() = screenPhysicalWidthMM × widgetLogicalWidth / screenLogicalWidth`, and `widthMM() / width()` is millimetres per logical pixel, independent of DPR.
- [VERIFIED: probe-output.txt] the Studio Display reports `physicalDotsPerInch: 109.75` for 2560×1440 logical, i.e. 0.2314 mm per logical pixel; the 0.5 mm threshold is crossed at `xscale ≈ 2.16` logical pixels per source pixel. **No recalibration is needed.**
- The flag only affects the instant preview `m_pixmap` drawn before the HQ pixmap exists. The HQ image comes from `imageproc::transform`, an area-averaging resampler (`calcSrcUnitSize`, `transformGeneric` in `imageproc/Transform.cpp`) that produces hard source-pixel edges when magnified and averages when reduced, independent of the flag. At DPR 2 it simply does so at device resolution.
- `widthMM()` returns `int`, so very small widgets round; irrelevant for the full-size page view.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Qt Test (Qt 6.12.0) inside the existing `qt_tests` executable; Boost.Test for the other six suites |
| Config file | `src/qt_tests/CMakeLists.txt` (sources list; `add_test(NAME qt_tests …)` with `ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LC_ALL=en_US.UTF-8;TMPDIR=…"`), `src/qt_tests/main.cpp` (sets env, then `QTest::qExec` per test class) |
| Quick run command | `ctest --preset macos-release -R qt_tests` (1.7 s measured today) or `build/macos-release/qt_tests` |
| Full suite command | `ctest --preset macos-release` and `ctest --preset macos-debug` (expected `100% tests passed out of 7`) |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| HIDPI-02 | HQ pixmap is device resolution at 2×: `devicePixelRatio()==2`, pixmap size equals `hidpi::targetRect` device size, logical position is `origin/2`; at 1× unchanged | Qt Test, offscreen 2× screen | `ctest --preset macos-release -R qt_tests` | ❌ Wave 0 (`tst_hidpi_qt.cpp`) |
| HIDPI-02 | 1-device-pixel edge of a synthetic page stays within one device pixel in `grab()` (edge transition ≤ 1 px), vs ≥ 2 px for an upscaled DPR-1 buffer | Qt Test | same | ❌ Wave 0 |
| HIDPI-02 | Pure helper `hidpi::targetRect/toDevice/deviceSize` at dpr 1, 1.5, 2, equal to the legacy computation at 1 | Qt Test (pure) | same | ❌ Wave 0 |
| HIDPI-03 | `ensureThumbnailExists` writes `…_q500.png` at DPR 2 / Quality 250 and `…_q250.png` at DPR 1; `loadNow` pixmap width 500; flush on `setDevicePixelRatio` | Qt Test | same | ❌ Wave 0 |
| HIDPI-03 | `ThumbnailBase::paint` into a DPR-2 `QImage` builds a DPR-2 temp pixmap, keys differ by ratio, 1-px feature is not doubled | Qt Test | same | ❌ Wave 0 |
| HIDPI-03 | `DEFAULT_THUMBNAIL_QUALITY` is 250 (isolated settings) | Qt Test | same | ❌ Wave 0 |
| HIDPI-04 | Zone mask pixmap carries the view ratio and device size; per-frame copy keeps it | Qt Test (needs `PictureZoneEditor` + default `output::Settings`; add an accessor) | same | ❌ Wave 0 |
| HIDPI-04 | Stage-list frames: size = `squareSide × dpr`, ratio set, bubbles inside the frame; delegate centring uses logical size | Qt Test (needs `StageListView` + `StageSequence`, or a static frame renderer) | same | ❌ Wave 0 |
| HIDPI-04 | Icon pixmaps: logical size 16/64/128 via `deviceIndependentSize()`; handle rect centred on the anchor | Qt Test using a `StyledIconPack` built from a scratch `icons.xml` + SVG in `test-tmp` (resources are in the app target, not in `qt_tests`) | same | ❌ Wave 0 |
| HIDPI-05 | Moving the view 1×→2×→1× rebuilds the HQ pixmap at 2 then 1; thumbnails cache flushed; stage frames rebuilt | Qt Test, two-screen offscreen | same | ❌ Wave 0 |
| HIDPI-06 | `medianColor` same at 15×15/dpr 1 and 30×30/dpr 2; real 2× `grab` returns 30×30 and the patch colour | Qt Test | same | ❌ Wave 0 |
| "Output files stay unchanged" | no golden, manifest or output byte changes | Boost.Test goldens | `ctest --preset macos-release` and `macos-debug`; per commit `bash .planning/phases/03-qt-6-and-macos-only/gate.sh check <label>` | ✅ (`src/pipeline_tests/golden/{flat,book,mixed}/`) |
| HIDPI-01 | App binary DPR is 2.0 on the Studio Display | Manual-on-the-Mac, run once by Claude (D-02), recorded in `records/` | harness run | ❌ (harness) |
| HIDPI-02/03/04 live sharpness | Buffers and edges at fit/100%/200% on the real display; thumbnails, masks, badges, handles, stage animation | Manual-on-the-Mac (Claude-run harness + `grab()`), recorded | harness run | ❌ (harness) |

### Sampling Rate
- **Per task commit:** `ctest --preset macos-release -R qt_tests` plus `xcrun clang-format --dry-run -Werror` on touched files.
- **Per wave merge / per commit that the gate script requires:** `bash .planning/phases/03-qt-6-and-macos-only/gate.sh check <label>` on a committed tree (it configures, builds, runs Release and Debug ctest, compares goldens with `a796c2ae`, and diffs build commands; it refuses to run with `UPDATE_GOLDENS` set or with uncommitted changes in `src`, `CMakeLists.txt`, `CMakePresets.json`, `cmake`, `config.h.in`, `version.h.in`).
- **Phase gate:** full suite green in Release and Debug, goldens untouched, then the live Studio Display run recorded.

### Wave 0 Gaps
- [ ] `src/qt_tests/tst_hidpi_qt.{h,cpp}` — all HIDPI tests above; register in `main.cpp` (`QTest::qExec`) and in `src/qt_tests/CMakeLists.txt`.
- [ ] `src/qt_tests/main.cpp` — write the two-screen JSON, replace `qputenv("QT_QPA_PLATFORM", "offscreen")` with the `configfile` form, add `QT_SCREEN_SCALE_FACTORS`.
- [ ] `src/core/HiDpi.{h,cpp}` and its entry in `common_sources` of `src/core/CMakeLists.txt`.
- [ ] Test accessors: `ImageViewBase::hqPixmap()` and `hqPixmapIsCurrent()`, `PictureZoneEditor::screenPictureMask()`; keep them documented as diagnostic.
- [ ] Optional: a static frame renderer in `StageListView` or `BubbleAnimation` so the animation test needs no `StageSequence`.
- [ ] `records/` folder for the live verdicts (`.planning/phases/04-retina-rendering/records/`).
- [ ] No framework install needed (Qt Test is already linked).

## Security Domain

`security_enforcement` is enabled (`.planning/config.json`, `security_asvs_level: 1`).

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | — |
| V3 Session Management | no | — |
| V4 Access Control | no | — |
| V5 Input Validation | yes (small) | The Quality spin box range (100–1500) times DPR sets an allocation size; clamp the effective width in `ThumbnailPixmapCache` (for example ≤ 2000 px) and validate in the cache, not only in the dialog (project convention: validate in the dialog) |
| V6 Cryptography | no | The MD5 in the thumbnail file name is a name hash, not a security control; unchanged |
| V12 Files | yes (unchanged) | Thumbnail files are written through `AtomicFileOverwriter` into `<output>/cache/thumbs`; the name is `completeBaseName` + page + MD5 of the path + `q<width>`; no new input reaches the path |

### Known Threat Patterns for Qt widget raster code

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Memory exhaustion from an enlarged buffer (Quality × DPR, HQ pixmap viewport × DPR²) | Denial of service | Clamp the effective thumbnail width; HQ pixmap is bounded by viewport size × DPR² (about 59 MB at full-screen 5K, accepted in CONTEXT) |
| Stale or foreign file picked up through a shared file name | Tampering | Name encodes the pixel width; atomic writes; old files never deleted or reused across sizes |
| Integer overflow in `qRound(w * dpr)` for absurd ratios | Denial of service | `deviceSize` helper clamps to ≥ 1 and relies on `QPixmap` allocation failure handling already in `OutOfMemoryHandler` (existing `std::bad_alloc` handling in `backgroundProcessing`) |

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Qt's Cocoa plugin delivers `QEvent::DevicePixelRatioChange` when a window is dragged between displays of different backing scale | Pattern 7 | Event handler never fires on the real Mac; mitigated by the paint-time ratio comparison and by the simulated test |
| A2 | AppKit honours an embedded `__TEXT,__info_plist` `NSHighResolutionCapable` for an unbundled executable | HIDPI-01 fallback | Only matters if a ratio of 1.0 were measured; then the fallback would need a different fix (bundle) |
| A3 | `QPixmapCache::insert` of a pixmap larger than the cache limit fails | Pitfall 6 | If it evicts everything instead, thumbnails at the largest Size repaint slower; performance only |
| A4 | The Phase 1 `qt.highdpi.*=true` rule did not match the `qt.highdpi` category, which is why no line was logged | Pitfall 11 | Logging cannot be trusted as a measurement either way; the plan measures in-process |
| A5 | `QStyledItemDelegate` lays out `RelinkingModel` pixmap decorations by device-independent size | Pattern 6 | 16-px file/folder icons in the relinking dialog could be drawn at the wrong size on Retina; caught by the live check |
| A6 | Cosmetic pens (thumbnail overlays use `pen.setCosmetic(true)`, width 1) keep their visual weight at DPR 2 | Pattern 4 | Hairlines in thumbnails could look thinner than before; visual, caught by the live check |
| A7 | `QT_SCREEN_SCALE_FACTORS` remains honoured by the offscreen platform in later Qt 6 versions | Code Examples | Test setup would need a different mechanism after a Qt upgrade; Qt docs call the variable "not recommended" but do not remove it |
| A8 | Real page content makes `transform()` take roughly the measured time (the bench used a blank page) | Pattern 2 | HQ latency might double; still behind the 150 ms timer on a background thread |

## Open Questions

1. **Does the real Cocoa path deliver `DevicePixelRatioChange` on a display drag?**
   - What we know: delivery works for `QWindow::setScreen` and `move()` on the offscreen platform; the event exists since Qt 6.6.
   - What's unclear: the Cocoa behaviour; only one display is attached.
   - Recommendation: implement both the event handler and the paint-time check; state in the criterion-4 record that the claim rests on simulation (D-03).

2. **Thumbnail Quality ceiling.**
   - What we know: 1500 × DPR 2 = 3000-px files, up to about 1.4 GB in the cache.
   - What's unclear: the user's wanted ceiling (CONTEXT leaves range and wording to discretion).
   - Recommendation: clamp the effective width in the cache and set the spin-box maximum to 1000; keep stored values.

3. **Where should the live harness live?**
   - Recommendation: a small committed `retina_check` target linking `core` only, not in ctest, so the records can be regenerated. A scratch program works too (CONTEXT discretion).

4. **`RelinkingModel` and other model-data pixmaps.**
   - What we know: Qt widgets handle DPR pixmaps; one case (`RelinkingModel.cpp:91-92`) returns `QPixmap` through a model.
   - Recommendation: include a grab of the relinking dialog in the live check; fix only if it looks wrong.

## Sources

### Primary (HIGH confidence)
- In-repo files read with `Read` this session: `src/core/ImageViewBase.{cpp,h}`, `PixmapRenderer.cpp`, `ThumbnailBase.{cpp,h}`, `ThumbnailPixmapCache.{cpp,h}`, `filters/output/ColorPickupInteraction.cpp`, `filters/output/PictureZoneEditor.{cpp,h}`, `StageListView.cpp`, `ApplicationSettings.cpp` (lines 22-29, 45-48), `src/app/SettingsDialog.ui` (107-122), `src/qt_tests/{CMakeLists.txt,main.cpp,tst_ux_qt.cpp}`, `.planning/phases/04-retina-rendering/04-CONTEXT.md`, `REQUIREMENTS.md`, `STATE.md`, `config.json`.
- Executed probes in `build/scratch/dpr-research/` (`probe.cpp`, `probe2.cpp`, `probe4.cpp`, `bench.cpp`, `screens.json`, `screens2.json`, `Info.plist`): offscreen two-screen behaviour, event delivery, `grab()` size and ratio, painter viewport on DPR devices, icon pixmap ratios, pixel-exact drawing, `QGraphicsPixmapItem` / `QLabel` logical sizes, `-sectcreate` section, `imageproc::transform` timing. Reproduce with `QT_SCREEN_SCALE_FACTORS="OneX=1;TwoX=2" QT_QPA_PLATFORM="offscreen:configfile=build/scratch/dpr-research/screens2.json" build/scratch/dpr-research/build/probe`.
- `otool -l` on `build/macos-release/scantailor-advanced` and the probe.
- `.planning/phases/01-deletion-free-macos-build/records/probe-output.txt` and `SMOKE-FINDINGS.md` § "Device pixel ratio".
- `.planning/phases/03-qt-6-and-macos-only/gate.sh`.

### Secondary (MEDIUM confidence)
- [doc.qt.io/qt-6/qevent.html](https://doc.qt.io/qt-6/qevent.html) — `DevicePixelRatioChange` = 222, introduced in Qt 6.6.
- [doc.qt.io/qt-6/highdpi.html](https://doc.qt.io/qt-6/highdpi.html) — `QT_SCALE_FACTOR` for testing, `QT_SCREEN_SCALE_FACTORS` not recommended, `PassThrough` default, 400×400 image at ratio 2 is 200×200 logical.
- [doc.qt.io/qt-6/qpixmapcache.html](https://doc.qt.io/qt-6/qpixmapcache.html) — default limit 10240 KB.
- [doc.qt.io/qt-6/qloggingcategory.html](https://doc.qt.io/qt-6/qloggingcategory.html) — rule syntax and wildcards.
- Qt source via raw GitHub, tag v6.12.0: `src/plugins/platforms/offscreen/qoffscreenintegration.cpp` (config file keys `name`, `x/y/width/height`, `logicalDpi`, `logicalBaseDpi`, `dpr`; `configfile=` argument) and `src/gui/image/qicon.cpp` (`pixmap` overload chain and `qApp->devicePixelRatio()`), read through a summarising fetch; the observable behaviour was then confirmed by the probes.

### Tertiary (LOW confidence)
- Web search results for embedding `Info.plist` with `-sectcreate __TEXT __info_plist` (Swift forums reply from an Apple engineer; `ld` man page), and for the `qt.highdpi` category name; used only for the fallback and the logging note.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — no new dependencies; Qt version from the build cache and the probe record.
- Architecture (what to change and where): HIGH — every site read; the key Qt behaviours were reproduced with executed probes.
- Test mechanism: HIGH — two-screen offscreen with `QT_SCREEN_SCALE_FACTORS` run end to end; limitation of the `dpr` key found and documented.
- Pitfalls: HIGH for the geometry and offscreen items (reproduced); MEDIUM for Cocoa-only delivery and for `QPixmapCache` oversize behaviour.

**Research date:** 2026-10-25
**Valid until:** 2026-11-24 (Qt 6.12 stable; re-check the offscreen mechanism if Qt is upgraded)
