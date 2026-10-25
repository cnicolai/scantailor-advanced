---
phase: 04-retina-rendering
verified: 2026-10-25T18:00:00Z
status: passed
score: 5/5 must-haves verified
covered_files:
  - ".planning/phases/04-retina-rendering/04-01-PLAN.md"
  - ".planning/phases/04-retina-rendering/04-01-SUMMARY.md"
  - ".planning/phases/04-retina-rendering/04-02-PLAN.md"
  - ".planning/phases/04-retina-rendering/04-02-SUMMARY.md"
  - ".planning/phases/04-retina-rendering/04-03-PLAN.md"
  - ".planning/phases/04-retina-rendering/04-03-SUMMARY.md"
  - ".planning/phases/04-retina-rendering/04-04-PLAN.md"
  - ".planning/phases/04-retina-rendering/04-04-SUMMARY.md"
  - ".planning/phases/04-retina-rendering/04-05-PLAN.md"
  - ".planning/phases/04-retina-rendering/04-05-SUMMARY.md"
  - ".planning/phases/04-retina-rendering/04-06-PLAN.md"
  - ".planning/phases/04-retina-rendering/04-06-SUMMARY.md"
  - ".planning/phases/04-retina-rendering/04-07-PLAN.md"
  - ".planning/phases/04-retina-rendering/04-07-SUMMARY.md"
  - ".planning/phases/04-retina-rendering/04-08-PLAN.md"
  - ".planning/phases/04-retina-rendering/04-08-SUMMARY.md"
  - "TESTING.md"
  - "src/app/CMakeLists.txt"
  - "src/app/MainWindow.cpp"
  - "src/app/MainWindow.h"
  - "src/app/SettingsDialog.ui"
  - "src/app/retina_check/main.cpp"
  - "src/core/ApplicationSettings.cpp"
  - "src/core/CMakeLists.txt"
  - "src/core/HiDpi.cpp"
  - "src/core/HiDpi.h"
  - "src/core/ImageViewBase.cpp"
  - "src/core/ImageViewBase.h"
  - "src/core/StageListView.cpp"
  - "src/core/StageListView.h"
  - "src/core/ThumbnailBase.cpp"
  - "src/core/ThumbnailDprTracker.cpp"
  - "src/core/ThumbnailDprTracker.h"
  - "src/core/ThumbnailPixmapCache.cpp"
  - "src/core/ThumbnailPixmapCache.h"
  - "src/core/Utils.cpp"
  - "src/core/Utils.h"
  - "src/core/filters/deskew/ImageView.cpp"
  - "src/core/filters/output/ColorPickupInteraction.cpp"
  - "src/core/filters/output/ColorPickupInteraction.h"
  - "src/core/filters/output/PictureZoneEditor.cpp"
  - "src/core/filters/output/PictureZoneEditor.h"
  - "src/core/filters/page_split/ImageView.cpp"
  - "src/core/filters/page_split/Thumbnail.cpp"
  - "src/core/filters/page_split/UnremoveButton.cpp"
  - "src/core/zones/ZoneContextMenuInteraction.cpp"
  - "src/qt_tests/CMakeLists.txt"
  - "src/qt_tests/main.cpp"
  - "src/qt_tests/tst_hidpi_qt.cpp"
  - "src/qt_tests/tst_hidpi_qt.h"
covered_digest: "v3:sha256:bf2c290d65fb827056ef848b7dd6bfac19ecdd41f9e4dcb100eeed4ceb20e34b"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 4: Retina Rendering Verification Report

**Phase Goal:** Everything the app draws on screen is sharp on a Retina display, and output files stay unchanged.
**Verified:** 2026-10-25
**Status:** passed (3 warnings carried, one accepted simulation limit, see below)
**Re-verification:** No, initial verification

## Goal Achievement

The verifier read the production diff against the phase base (`059f4800..HEAD`, 33 files under `src/`), ran a fresh `cmake --build --preset macos-release` (exit 0) and `ctest --preset macos-release` (`100% tests passed out of 7`, `TestHiDpiQt` 26 of 26 pass), and compared the summaries with the code. Records of the live Studio Display runs were read, and one crop was viewed.

### Observable Truths (ROADMAP success criteria)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | The ratio the build-directory binary reports on the Retina display is measured and recorded, and it reports 2.0 (Info.plist only if 1.0) | VERIFIED (proxy binary, see note) | `records/hidpi-01.md`: screen, window, page view and viewport ratio 2 on the Studio Display, Qt 6.12.0, Cocoa. `otool -l` run again by the verifier: neither `build/macos-release/scantailor-advanced` nor `retina_check` has a `__info_plist` section. `src/app/Info.plist` does not exist and `src/app/CMakeLists.txt` adds none, which matches the "only if 1.0" rule. The app binary itself was not launched (home-folder rule), `retina_check` with the same Mach-O platform, minos, sdk and Qt frameworks stands in; Phase 1 had measured 2 from a probe, and `src/app/main.cpp` sets no HiDpi attribute or scale variable. |
| 2 | At fit, 100% and 200% the page view is as sharp as the same image in Preview, and thumbnails are sharp | VERIFIED | Code: `HqTransformTask` builds the image with `hidpi::toDevice(xform, dpr)` and `hidpi::targetRect`, sets `setDevicePixelRatio`, `hqVersionBuilt` converts the origin to logical (`src/core/ImageViewBase.cpp:1056-1081, 995-998`), `paintEvent` draws it at the logical position (`:460`) and `validateHqPixmap` rejects a pixmap of another ratio (`:945`). Thumbnails: `ThumbnailPixmapCache` file size is Quality x ratio (`q500` at defaults), `ThumbnailBase::paint` uses a temp pixmap at the painter's ratio keyed `tempPixmap@<ratio>`. Evidence: `records/live-page-view.md` (grab equals the device-resolution reference, equal fraction 1.000000, max diff 0 at fit, 100%, 200%; a ratio-1 buffer fails the same comparison), `records/live-overlays.md` thumbnails (ratio 2, `q500`, detail 0.1824), `thumb-grab.png` viewed: crisp glyph edges. Tests `testHqGrabMatchesDeviceReference`, `testHqPixmapAtDeviceResolution`, `testThumbnailPaintAtRatio`, `testThumbnailFilesKeyedByRatio` pass in the verifier's run. "As sharp as Preview" is judged by the device-resolution reference, as fixed by D-02. |
| 3 | Zone masks, page badges, handles and the stage-list animation look sharp | VERIFIED | Code: `PictureZoneEditor::MaskTransformTask` at device ratio with logical origin and a ratio check at paint time; the animated per-frame copy keeps the ratio; `StageListView::createBatchAnimationFrames` at device resolution with an explicit logical rect and a `DevicePixelRatioChange` rebuild; handles, trash icon and unremove button placed by `hidpi::logicalSize/logicalRect`; context-menu swatch built with `hidpi::makePixmap`. Page badges need no change (the real `QIcon` engine returns a ratio-2 pixmap), pinned by `testPageBadgeAtDeviceResolution`. Live: `records/live-overlays.md` (mask, badge, handles, stage frame, decorations each with verdict PASS). Tests `testZoneMaskAtDeviceResolution`, `testZoneMaskOverlayConfined`, `testStageAnimationFrames`, `testIconPixmapLogicalGeometry`, `testPageBadgeAtDeviceResolution` pass. A sweep for `QPixmap(`, `QPixmap::fromImage` and `grab(` in `src/core` and `src/app` found no screen buffer left at ratio 1 outside the deliberate cases (downscaled preview `m_pixmap`, loaded thumbnail pixmap, both scaled through their own pixel size). |
| 4 | Moving the window between a Retina and a non-Retina display re-renders page view and thumbnails at the right resolution without a restart | VERIFIED under D-03 (simulation, see decision below) | Tests `testHqRebuildsOnScreenChange`, `testRepeatedRatioEventKeepsHqPixmap`, `testThumbnailsFollowScreenChange`, `testZoneMaskRebuildsOnScreenChange`, `testStageAnimationFollowsScreenChange` pass on the offscreen platform with screens `OneX` (ratio 1) and `TwoX` (ratio 2); the test `initTestCase` fails loudly if the two screens do not come up as 1 and 2. Production wiring read: `ImageViewBase::event` and `validateHqPixmap` (page view), `ThumbnailDprTracker` created in `MainWindow::setupThumbView` and handed each cache in `switchToNewProject`, `ThumbnailPixmapCache::setDevicePixelRatio` flushes and bumps the size generation, `StageListView::event`, `PictureZoneEditor::validateScreenPictureMask`. |
| 5 | On Retina the colour picker returns the colour under the cursor, an automated test covers the sampling path, and the full `ctest` suite passes with baselines unchanged | VERIFIED | `ColorPickupInteraction::takeColor` grabs the logical rect (device resolution) and calls the new pure `medianColor`, which counts the circle on logical pixel indices. `testMedianColorRatioInvariant` and `testColorPickerGrabAtRatio2` (a real `grab()` of a 15x15 logical rect is 30x30 at ratio 2 and returns the patch colour on `TwoX` and `OneX`) pass. Live `picker_color: #dc781e` equals the patch. `git diff --stat 059f4800 HEAD` over `src/pipeline_tests`, `src/imageproc`, `src/dewarping`, `src/foundation`, `src/math`, every `filters/*/Task.cpp`, every `CacheDrivenTask.cpp` and `OutputGenerator.cpp` is empty. Verifier's fresh Release ctest: 7 of 7 pass; `git status` shows no changed golden. Orchestrator's Release and Debug runs on `c3c389f1` also 7 of 7. |

**Score:** 5/5 truths verified (0 present, behavior-unverified)

### Decision on criterion 4 (physical drag)

Under D-03 (user-set verification method in `04-CONTEXT.md`, marked reversible), the offscreen two-screen simulation is the agreed proof, so criterion 4 is recorded as VERIFIED and is not a human-verification item. This is an honest limit and not a full equivalent: nothing in this phase observed the Cocoa plugin delivering `DevicePixelRatioChange` on a real drag between displays of different backing scale (assumption A1 in `records/closing-check.md`). Two things limit the risk. The ratio is also compared at paint time (`validateHqPixmap`, `validateScreenPictureMask`), and Qt repaints a window after a screen change, so a missed event is caught on the next paint. The thumbnail path has no paint-time check, it relies on the event filter. Recheck when a second display is attached, as the closing record describes (drag the app window, run `retina_check --section all` on each display, compare `window_dpr` and verdicts). The user may still choose to try this by hand.

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/core/HiDpi.{h,cpp}` | Buffer rule and eight helpers | VERIFIED | Read; exact old arithmetic at ratio 1 (`testHelpersMatchLegacyAtRatio1`). In `src/core/CMakeLists.txt`. |
| `src/core/ImageViewBase.{h,cpp}` | HQ pixmap at device ratio, DPR change handling | VERIFIED | Diff read, wired in `paintEvent`. |
| `src/core/ThumbnailPixmapCache.{h,cpp}`, `ThumbnailBase.cpp`, `ThumbnailDprTracker.{h,cpp}` | Ratio-keyed cache, device-resolution paint, display-change tracker | VERIFIED (with WR-01, WR-02) | Wired through `Utils::createThumbnailCache(outDir, dpr)` and `MainWindow`. |
| `PictureZoneEditor`, `StageListView`, icon geometry files, `ColorPickupInteraction` | Overlays at device resolution, logical geometry, logical-pixel median | VERIFIED | Diffs read. |
| `src/qt_tests/tst_hidpi_qt.{h,cpp}`, `main.cpp` | 26 permanent tests on two offscreen screens (TESTX-03) | VERIFIED | Registered in `qt_tests`, all 26 pass in the verifier's `ctest -R qt_tests -V`. |
| `src/app/retina_check/main.cpp` | Live Cocoa harness | VERIFIED | Built; not run by the verifier (opens windows). Records exist. |
| `records/hidpi-01.md`, `live-page-view.md`, `live-overlays.md`, `closing-check.md`, `gate-log.md`, `live/*.png` | Evidence | VERIFIED | Present, consistent with the code. |
| `src/app/Info.plist` | Only if ratio 1 | Correctly absent | Ratio measured 2. |

### Key Link Verification

| From | To | Via | Status |
|------|----|-----|--------|
| `ImageViewBase::initiateBuildingHqVersion` | `HqTransformTask` | passes `viewport()->devicePixelRatioF()` through `hidpi::sanitizedRatio` | WIRED |
| `HqTransformTask` result | `paintEvent` | `hqVersionBuilt` converts origin to logical; `drawPixmap(m_hqPixmapPos, m_hqPixmap)` | WIRED |
| `MainWindow::setupThumbView` / `switchToNewProject` | `ThumbnailDprTracker` / `ThumbnailPixmapCache` | tracker created on `thumbView`, `setCache(m_thumbnailCache)`, cache built with `thumbView->devicePixelRatioF()` | WIRED |
| `ThumbnailDprTracker` | `ThumbnailPixmapCache::setDevicePixelRatio` | event filter on `DevicePixelRatioChange`, then `viewport()->update()` | WIRED |
| `ColorPickupInteraction::takeColor` | `medianColor` | `imageView().grab(rect)` then `medianColor(image)` | WIRED |
| `Output Task` | `recreateThumbnail` | `src/core/filters/output/Task.cpp:425`, unchanged | WIRED (see WR-02) |

### Data-Flow Trace (Level 4)

| Artifact | Data variable | Source | Real data | Status |
|----------|---------------|--------|-----------|--------|
| HQ pixmap | `m_hqPixmap` | `imageproc::transform` of `m_image` at device transform | Yes (grab equals reference) | FLOWING |
| Thumbnail | temp pixmap and cache pixmap | `makeThumbnail(image, effectiveSize)` from the output image, file `q<Quality x ratio>` | Yes (live `q500`) | FLOWING |
| Zone mask | `m_screenPictureMask` | `transformToGray` of the picture mask at device transform | Yes | FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Release build is current | `cmake --build --preset macos-release` | exit 0 | PASS |
| Full suite, baselines | `ctest --preset macos-release` | `100% tests passed out of 7`; no file changed in `git status` | PASS |
| Retina tests run and pass | `ctest --preset macos-release -R qt_tests -V` | `TestHiDpiQt` Totals: 26 passed, 0 failed, 0 skipped | PASS |
| Output code untouched | `git diff --stat 059f4800 HEAD` over goldens, `imageproc`, `dewarping`, `foundation`, `math`, `Task.cpp`, `CacheDrivenTask.cpp`, `OutputGenerator.cpp` | empty | PASS |
| Formatting | `xcrun clang-format --dry-run -Werror` on every changed `.cpp`/`.h` | no output | PASS |
| Debug ctest | not re-run by the verifier | orchestrator's `ctest-macos-debug.txt`: 7 of 7 | accepted |

### Probe Execution

SKIPPED (no `probe-*.sh` declared by any plan).

### Requirements Coverage

Every ID in the PLAN frontmatter (`requirements-completed` of plans 01 to 08) appears in REQUIREMENTS.md and is marked Complete in its traceability table. No orphaned requirement is mapped to Phase 4.

| Requirement | Source plans | Description | Status | Evidence |
|-------------|--------------|-------------|--------|----------|
| HIDPI-01 | 04-02 | Ratio measured; Info.plist only if 1.0 | SATISFIED (proxy binary) | `hidpi-01.md`, Mach-O comparison, no Info.plist |
| HIDPI-02 | 04-01, 04-02 | Page view sharp at 2x at every zoom | SATISFIED | HQ pixmap code, `live-page-view.md`, 6 tests |
| HIDPI-03 | 04-03, 04-04, 04-07 | Thumbnails sharp, cache keyed by ratio | SATISFIED (WR-01, WR-02 open) | `q500` file names, tracker, tests, live record |
| HIDPI-04 | 04-05, 04-06, 04-07 | Overlay pixmaps sharp | SATISFIED | Diffs, live overlay record, 5 tests |
| HIDPI-05 | 04-01, 04-04, 04-05 | Re-render on display move | SATISFIED by simulation (D-03) | 5 display-change tests |
| HIDPI-06 | 04-06, 04-07 | Colour picker correct on Retina | SATISFIED | `medianColor`, 2 tests, live record |
| TESTX-03 | 04-01 to 04-08 | Automated HiDPI test checks buffers at 2x | SATISFIED | 26 tests in `qt_tests`, traceability row in REQUIREMENTS.md (v2 entry says it was brought into Phase 4 by D-04) |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `src/app/MainWindow.cpp` | 1416, 1866 | `XXX:` comments | Info | Present in the 1.2.1 base (`git show 5eaac18` has them); none added by this phase (`git diff` of the phase adds no TBD/FIXME/XXX/TODO line). Not a new debt marker. |
| `src/core/ImageViewBase.cpp` | 698 | `XXX: a hack ...` | Info | Same, upstream comment. |
| `src/qt_tests/tst_hidpi_qt.cpp` | several | `...-XXXXXX` | Info | `QTemporaryDir` template, not a marker. |
| (open review) IN-01 to IN-05 | | test-only accessors, tooltip wording, redundant ctest env, planning IDs in a comment, lexical path guard in `retina_check` | Info | Cosmetic or tool-only, no effect on the goal. |

### Warnings (not gaps; recommend fixing before relying on display moves)

**W1 (review WR-01) `removeLoadedItemsLocked()` sets `m_endOfLoadedItems` to `end()`** (`src/core/ThumbnailPixmapCache.cpp:849`). I read the code and confirm the review: after a flush, QUEUED, IN_PROGRESS and LOAD_FAILED items remain in the remove queue while the marker claims every item is LOADED. The marker is re-set only when the next `request()` arrives with the marker at `end()`, and then it points past the surviving non-LOADED items. `removeExcessLocked()` only asserts that the front is LOADED, so once 40 pixmaps are held it can erase a non-LOADED front item, and an IN_PROGRESS item erased this way leaves a pending `LoadResultEvent` with a dangling iterator. The bug predates the phase (it was reachable through the Settings quality change); the phase makes it reachable on every display move, normally while thumbnails are loading. Likelihood is low (needs about 40 completed loads while one older item is still in progress, and the load queue is last-in first-out), but the effect would be a crash. It does not make a truth fail, and no later phase covers it (Phase 6 lists TIFF size, XML escaping, thread cap, OpenGL, splits). Fix is one line, `m_endOfLoadedItems = m_removeQueue.begin();`, plus a test with a queued item present at the flush.

**W2 (review WR-02) Stale thumbnail after reprocessing.** `recreateThumbnail()` (`:514`, called from `output/Task.cpp:425`) rewrites only the file of the ratio in force; `loadSaveThumbnail()` and `ensureThumbnailExists()` accept an existing `q<width>` file as current. After reprocessing a page on one display, the file of the other ratio keeps the old output until it is deleted. A thumbnail with outdated content, no data loss, only for someone who uses displays of two ratios. The same limit existed before for the Quality setting, the phase adds the per-display case.

**W3 (review WR-03) `ImageViewBase::event` may be a no-op** because the viewport can report the old ratio when the scroll area gets the event. I traced the paint path: `validateHqPixmap` then fails on the ratio, `scheduleHqVersionRebuild` and `initiateBuildingHqVersion` cancel any old task and rebuild at the new ratio, and a late old-ratio result is replaced the same way. Correctness holds through the paint-time check; the handler is partly dead code. Not a defect in behaviour.

### Human Verification Required

None required for the verdict. Optional, reversible per D-03: drag the app window between a 1x and a 2x display once a second display is available (assumption A1), and launch `build/macos-release/scantailor-advanced` once and look at it (the harness stood in for the app binary on criterion 1). Judgement of "looks as good as other macOS apps" stays with the user's eye, as `closing-check.md` says.

### Gaps Summary

No gaps. All five roadmap criteria and all seven requirement IDs (HIDPI-01 to HIDPI-06, TESTX-03) are backed by code read in the diff, a fresh passing build and test run, and the live Studio Display records. Output code, goldens and manifests are untouched. Two stated limits are carried openly: criterion 1 was measured with a binary of the same Mach-O kind and not the app itself, and criterion 4 rests on the offscreen simulation agreed in D-03. Three open review warnings are listed above; W1 is the one worth fixing soon.

---

_Verified: 2026-10-25_
_Verifier: Claude (gsd-verifier)_
