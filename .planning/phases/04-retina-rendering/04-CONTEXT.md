# Phase 4: Retina Rendering - Context

**Gathered:** 2026-10-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Everything the app draws on screen is sharp on the Retina display, and output files stay unchanged. In scope are the main page view at every zoom level, the thumbnails and their cache, the overlay pixmaps (zone masks, page badges, handles, trash and unremove buttons, the stage-list animation, the zone context-menu swatches), re-rendering when the device pixel ratio (DPR) changes, and a colour picker that samples correctly on Retina. Requirements HIDPI-01 to HIDPI-06, plus TESTX-03, which D-04 brings forward from v2.

Not in scope: the OpenGL viewport path (off by default, removed in Phase 6), any change to `imageproc/`, `dewarping/` or any `Task::process`, menus and shortcuts (Phase 5), and bundling.

</domain>

<decisions>
## Implementation Decisions

### Carried forward (not re-asked)
- **D-00a:** Phase 1 measured DPR 2.0 on the Studio Display from a bare, unbundled Qt 6.12.0 executable (`.planning/phases/01-deletion-free-macos-build/records/probe-output.txt`; screen and window both 2). The measurement is provisional because it came from a probe, not the app, and no `qt.highdpi` line was captured. Phase 4 confirms the ratio from the real app binary. An embedded `Info.plist` (`-sectcreate __TEXT __info_plist`, `NSHighResolutionCapable`) is added **only** if that check reports 1.0.
- **D-00b:** Every commit passes the Phase 3 gate (`.planning/phases/03-qt-6-and-macos-only/gate.sh`): it builds, `ctest --preset macos-release` and `ctest --preset macos-debug` pass 7/7, and no golden or manifest file changes. `UPDATE_GOLDENS` is never set. Retina work stays in the view layer.
- **D-00c:** `QT_SCALE_FACTOR`, `QT_ENABLE_HIGHDPI_SCALING` and the Qt 5 HiDPI attributes are never used as the fix (REQUIREMENTS Out of Scope). Fix the raster buffers instead. Inside tests, a scale factor or an offscreen DPR setting is allowed as a test aid only.
- **D-00d:** The OpenGL viewer path is ignored, including `ImageViewBase::sampleImageAtWidgetRect` in the colour picker. It stays off by default, and Phase 6 (STRIP-07) removes it, relying on this phase's colour-picker test.

### Verification on the Mac
- **D-01:** Claude verifies everything. There is no GUI walkthrough and no human-verify checkpoint, as in Phases 2 and 3. Proof comes from `ctest`, from Claude's own checks of the real app on the Studio Display, and from committed records. The end-of-phase report is a short written summary.
- **D-02:** Success criteria 2 and 3 (sharp page view at fit, 100% and 200%; sharp thumbnails, zone masks, badges, handles and the stage-list animation) are checked by Claude on the real Cocoa display:
  - Claude launches the build-dir app or a harness on the Studio Display.
  - It grabs the widgets at 2× (`QWidget::grab()` returns the backing-store pixels at the window's DPR, so no screen-recording permission is needed).
  - It judges sharpness by measurement. Examples: buffers are logical size × DPR and carry DPR 2; a 1-px edge in a synthetic page covers one device pixel, not two; the grab matches a reference resampling of the same synthetic page at device resolution.

  "As sharp as Preview" is satisfied by that reference comparison, not by opening Preview. Only synthetic pages are used, and verdicts and any screenshots go into `.planning/phases/04-retina-rendering/records/`. The Phase 1 probe showed that a process Claude starts from its own shell reaches the window server at DPR 2.
- **D-03:** The Mac has only the Studio Display (DPR 2) and no non-Retina screen. Success criterion 4 (moving between a Retina and a non-Retina display re-renders without a restart) is proven by simulation: an automated test gives the view a 1× and a 2× screen, moves the page view and the thumbnail view between them, and asserts that the HQ page pixmap, thumbnails and overlay pixmaps are rebuilt at the new ratio. The records state that no physical 1× display was available. The preferred mechanism is Qt's offscreen platform with two configured screens of different DPR, if Qt 6.12's offscreen plugin supports per-screen DPR (research to confirm). The fallback is to deliver `QEvent::DevicePixelRatioChange` or a screen change to the widgets directly. — **Reversibility:** reversible — the criterion can later be re-checked live if a 1× display is attached.
- **D-04:** All 2× checks that don't need the physical display become permanent tests in `qt_tests` and run on every `ctest`, offscreen: buffer sizes and DPR for the HQ pixmap, thumbnails and overlays, the D-03 display-move test, and the colour-picker sampling test that criterion 5 requires. This brings **TESTX-03** ("Automated HiDPI render test checks view buffers are built at 2×", listed under v2 in `.planning/REQUIREMENTS.md`) into Phase 4, so its traceability row should move to Phase 4 when the requirements are next updated. The live Studio Display checks from D-02 run once and are recorded, because `ctest` runs offscreen.

### Thumbnails and their cache
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

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase scope and requirements
- `.planning/ROADMAP.md` § "Phase 4: Retina Rendering" — goal and the five success criteria
- `.planning/REQUIREMENTS.md` § Retina (HIDPI-01 to HIDPI-06), § Tests (TESTX-03, brought forward by D-04), § Out of Scope (`QT_SCALE_FACTOR` as a Retina fix is an anti-pattern)
- `.planning/STATE.md` § Blockers/Concerns — the Phase 4 research flag (unbundled DPR, `widthMM()`, the smoothing heuristic) and the gate script for later phases

### Prior measurements and decisions
- `.planning/phases/01-deletion-free-macos-build/records/probe-output.txt` — measured DPR 2 (screen and window), Studio Display geometry 2560×1440 logical, physical DPI 109.75
- `.planning/phases/01-deletion-free-macos-build/records/SMOKE-FINDINGS.md` § "Device pixel ratio" — why HIDPI-01 is provisional, and the user's "looks sharp" report from the smoke run
- `.planning/phases/03-qt-6-and-macos-only/gate.sh` — per-commit gate (Release and Debug `ctest`, goldens, build-command diff)
- `.planning/phases/03-qt-6-and-macos-only/03-CONTEXT.md` — D-11 and D-12 (automated verification, offscreen grabs judged by Claude)
- `.planning/phases/02-golden-baselines/records/access-approval.md` — standing approval for build-tool reads under `/opt/homebrew` and the Xcode/CLT installation in Phases 3 to 6

### Research
- `.planning/research/FEATURES.md` § "Area 2: Sharp Retina / HiDPI rendering of the custom views" — HDP-00 to HDP-10, the raster-site table, anti-features
- `.planning/research/PITFALLS.md` § "Pitfall 8" — the fix order, the `m_pixmap`/`PixmapRenderer` trap, the `grab()` audit, DPR-change invalidation
- `.planning/research/ARCHITECTURE.md` — Retina row of the change table (pure geometry function such as `hqTargetRect(...)` testable in `qt_tests`), and the note that the offscreen platform has DPR 1 unless configured
- `.planning/research/STACK.md` — unbundled-binary Retina notes and the `Info.plist` fallback
- `TESTING.md` — how the seven suites run, formatting with `xcrun clang-format`

</canonical_refs>

<code_context>
## Existing Code Insights

### Raster sites to fix (all in logical pixels today; `devicePixelRatio` is used nowhere in `src/`)
- `src/core/ImageViewBase.cpp` — `HqTransformTask` (line 36, run at 1021) builds the HQ image with `viewport()->size()` as target (947); `hqVersionBuilt` does `QPixmap::fromImage` at DPR 1 (964); `validateHqPixmap` (902) compares transforms only, so a DPR change does not invalidate it; `paintEvent` holds the 0.5 mm smoothing switch (405–424).
- `src/core/ThumbnailBase.cpp:100-125` — `tempPixmap = QPixmap(w, h)` in logical pixels, shared `QPixmapCache` key without DPR.
- `src/core/ThumbnailPixmapCache.cpp` — `getThumbFilePath` (612) names files `<base>_<page>_<md5>_q<width>.png`; `makeThumbnail` (635) scales to `maxThumbSize`. Cache directory `<output>/cache/thumbs` from `src/core/Utils.cpp:39`.
- `src/app/MainWindow.cpp:1534-1551` — applies Quality and Size from Settings; 2161–2165 resize the thumbnails interactively (Size 100–1000), so the file size must not follow Size (that would rebuild files on every resize).
- `src/core/filters/output/PictureZoneEditor.cpp:315` — zone mask built with `transformToGray` at logical size, kept in `m_screenPictureMask`.
- `src/core/StageListView.cpp:272` — batch animation frames `QPixmap(squareSide, squareSide)`.
- `src/app/ThumbnailSequence.cpp:1422-1423` — page badges via `QIcon::pixmap(size)`.
- `src/core/filters/deskew/ImageView.cpp:26` (`m_handlePixmap`, 16×16), `src/core/filters/page_split/Thumbnail.cpp:24` (`m_trashPixmap`, 64×64), `src/core/filters/page_split/UnremoveButton.cpp:15-16` (128×128), `src/core/zones/ZoneContextMenuInteraction.cpp:94` (16×16 swatch).
- `src/core/filters/output/ColorPickupInteraction.cpp:67-100` — `takeColor()`: `grab(rect)` then the median of a circle; only `grab()` caller in the view code.

### Reusable assets
- `IconProvider` with `StyledIconPack` / `FontIconPack` (`src/core/`): SVG and font engines that render at the requested size, probably correct on Retina already; check them rather than assume.
- `src/qt_tests/` (`tst_core_qt.cpp`, `tst_ux_qt.cpp`, `main.cpp` forcing `QT_QPA_PLATFORM=offscreen`): home of the permanent 2× tests (D-04).
- `src/test_support/` page generator: synthetic pages for the live checks (D-02) and the tests.
- `src/app/ui_snapshot/main.cpp`: offscreen grab tool (forces offscreen at line 127); a possible starting point for a live Cocoa harness.

### Integration points
- `QEvent::DevicePixelRatioChange` / `QWindow::screenChanged` in `ImageViewBase` and the thumbnail view drive the rebuild (HIDPI-05).
- `ApplicationSettings` holds the Quality default (D-06); `SettingsDialog.{ui,cpp}` shows it.

</code_context>

<specifics>
## Specific Ideas

- The user wants no manual verification in this phase either ("Claude only"), even though Retina is visual. Claude's own grabs on the real display and the permanent `ctest` checks take that role.
- The machine has one display: the Studio Display at DPR 2 (5K panel, 2560×1440 logical).
- The thumbnail default is meant to be sharp out of the box at the default size: Quality 250 × DPR 2 = 500 px for a 250-wide thumbnail.

</specifics>

<deferred>
## Deferred Ideas

- Zoomed-in look and colour-picker semantics were offered as discussion areas and not chosen. Their defaults are recorded under Claude's Discretion. Switching the picker to sample the image's own pixels instead of the drawn screen would be a separate decision for a later phase.
- Deleting stale thumbnail files of other sizes: rejected for now (D-08). A cache-cleanup command could be a later feature.

</deferred>

---

*Phase: 04-retina-rendering*
*Context gathered: 2026-10-25*
