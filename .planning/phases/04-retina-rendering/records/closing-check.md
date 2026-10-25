# Closing check of Phase 4 (plan 04-08)

Checked on HEAD after the documentation commit `28b70784` (`r13-docs`) and its gate record `0f57be04`. Nothing in `src/` changed after `r12-retina-check-overlays` (`9f9fdabe`), so the last source commit and the last gated commit are the ones in `records/gate-log.md`. This plan fixed no code. Output is normalised: `{repo}` for the repository path. The evidence is in the records of this directory (`gate-log.md`, `hidpi-01.md`, `live-page-view.md`, `live-overlays.md`), in the permanent tests of `TestHiDpiQt` (`src/qt_tests/tst_hidpi_qt.cpp`, 26 tests) and in the commits named.

Phase 4 base (parent of the first Phase 4 code commit `7c730ab2`): `059f4800`.

## Success criteria

ROADMAP Phase 4, criteria 1 to 5.

| # | Verdict | Evidence |
|---|---------|----------|
| 1 | PASS | `records/hidpi-01.md`: screen, window, page view and viewport report device pixel ratio 2 on the Studio Display (Qt 6.12.0, Cocoa). The measuring binary `retina_check` has the same Mach-O platform, minos, sdk, Qt frameworks and no `__info_plist` section as `scantailor-advanced`, so no `Info.plist` was added (D-00a); `src/app/Info.plist` does not exist. The app binary itself was not launched, because it reads and writes `~/.config` and `~/Library`, which the home-folder rule forbids without a yes; the harness stands in for it and the record says so. |
| 2 | PASS | Page view: `records/live-page-view.md`: at fit, 100% and 200% the viewport grab equals the device-resolution reference (equal fraction 1.000000, maximum channel difference 0), HQ pixmap ratio 2 and device size; tests `testHqGrabMatchesDeviceReference` (pixel-exact on `TwoX`, a ratio-1 buffer fails the same comparison) and `testHqPixmapAtDeviceResolution`. Zoomed-in look: `testSmoothingSwitchPhysicalSize` and the live `smoothing_verdict: PASS` (ratio 0.9978) in `records/hidpi-01.md` section "Smoothing heuristic": the 0.5 mm switch counts physical millimetres on Cocoa, no recalibration. Thumbnails: `records/live-overlays.md` section "Thumbnails" (cache ratio 2, one `q500` file, 500 px long side, ratio-2 grab, detail fraction 0.1824); tests `testThumbnailPaintAtRatio`, `testThumbnailFilesKeyedByRatio`, `testThumbnailEffectiveSize`, `testOldThumbnailLeftInPlace`, `testStaleLoadDiscarded`, `testThumbnailQualityDefault`. "As sharp as Preview" is judged by the reference comparison (D-02), not by opening Preview. |
| 3 | PASS | `records/live-overlays.md`: zone mask 747x560 at ratio 2 equal to the expected size (band detail 0.3844); page badge 46x34 at ratio 2, grab equals the 1:1 device pixmap in 100 percent of its pixels; deskew handles four 32 px blobs centred on the rotation origin (offsets 0.000, detail 0.8224); stage-list frame 26x26 at ratio 2, centred within 0.707 logical pixels; list decoration drawn 1:1 (assumption A5 confirmed). Tests: `testZoneMaskAtDeviceResolution`, `testZoneMaskOverlayConfined`, `testStageAnimationFrames`, `testIconPixmapLogicalGeometry` (unremove button), and `testPageBadgeAtDeviceResolution` for the page badges (built at the view's ratio, `src/app/ThumbnailSequence.cpp` unchanged since before the phase). |
| 4 | PASS (simulated, see below) | `testHqRebuildsOnScreenChange`, `testRepeatedRatioEventKeepsHqPixmap`, `testThumbnailsFollowScreenChange`, `testZoneMaskRebuildsOnScreenChange`, `testStageAnimationFollowsScreenChange`: a window moved between the offscreen screens `OneX` (ratio 1) and `TwoX` (ratio 2) rebuilds the HQ page pixmap, the thumbnails, the zone mask and the stage-list frames at the new ratio without a restart. No physical 1x display was available (D-03). |
| 5 | PASS | `testMedianColorRatioInvariant` (the median of a ratio-1 grab and of its 2x replication agree, a misread device-pixel crop is detected), `testColorPickerGrabAtRatio2` (a 15x15-logical grab of the page view is 30x30 at ratio 2 and `medianColor` returns the patch colour on `TwoX` and on `OneX`), the live picker in `records/live-overlays.md` section "Colour picker" (`picker_color: #dc781e`, equal to the patch colour, `picker_verdict: PASS`), and the gate log: every gated Phase 4 commit has `100% tests passed out of 7` in Release and Debug with goldens unchanged (section "Gate log"). |

## Requirements

| ID | Plans | Tests | Records |
|----|-------|-------|---------|
| HIDPI-01 | 04-02 | none (a measurement on the display) | `hidpi-01.md`: ratio 2, Mach-O comparison, no Info.plist |
| HIDPI-02 | 04-01, 04-02 | `testHqPixmapAtDeviceResolution`, `testHqPixmapAtOneXUnchanged`, `testHelpersMatchLegacyAtRatio1`, `testHelpersAtFractionalRatio`, `testHqGrabMatchesDeviceReference`, `testSmoothingSwitchPhysicalSize` | `live-page-view.md`, `hidpi-01.md` |
| HIDPI-03 | 04-03, 04-04, 04-07 | `testThumbnailEffectiveSize`, `testThumbnailFilesKeyedByRatio`, `testOldThumbnailLeftInPlace`, `testStaleLoadDiscarded`, `testThumbnailQualityDefault`, `testThumbnailPaintAtRatio` | `live-overlays.md` "Thumbnails" |
| HIDPI-04 | 04-05, 04-06, 04-07 | `testZoneMaskAtDeviceResolution`, `testZoneMaskOverlayConfined`, `testStageAnimationFrames`, `testIconPixmapLogicalGeometry`, `testPageBadgeAtDeviceResolution` | `live-overlays.md` "Zone mask", "Page badges", "Handles", "Stage-list animation", "List decorations" |
| HIDPI-05 | 04-01, 04-04, 04-05 | `testHqRebuildsOnScreenChange`, `testRepeatedRatioEventKeepsHqPixmap`, `testThumbnailsFollowScreenChange`, `testZoneMaskRebuildsOnScreenChange`, `testStageAnimationFollowsScreenChange` | simulated only, see "Criterion 4 by simulation" |
| HIDPI-06 | 04-06, 04-07 | `testMedianColorRatioInvariant`, `testColorPickerGrabAtRatio2` | `live-overlays.md` "Colour picker" |
| TESTX-03 | 04-01, then every plan that added a `TestHiDpiQt` test | all 26 tests of `TestHiDpiQt`, run by `qt_tests` on every `ctest` (offscreen, two screens) | `gate-log.md` |

TESTX-03 was brought into Phase 4 by D-04; its traceability row is `| TESTX-03 | Phase 4 | Pending |` in `.planning/REQUIREMENTS.md`, committed with this record.

## Decisions

- D-00a: ratio 2 measured with a binary of the same build kind (no `__info_plist`), so no `Info.plist` was added; the Phase 1 probe values were reproduced.
- D-00b: every code commit passed the Phase 3 gate (Release and Debug `100% tests passed out of 7`, goldens equal to `a796c2ae`); `UPDATE_GOLDENS` was never set; no file under `src/pipeline_tests/golden`, `src/imageproc`, `src/dewarping`, any `Task.cpp` of a filter or `OutputGenerator.cpp` changed (section "Static checks").
- D-00c: no scale factor is used as the fix; the variables appear only in `src/qt_tests/main.cpp` as a test aid and in `src/app/retina_check/main.cpp` as `qunsetenv` calls (section "Static checks").
- D-00d: the OpenGL path is untouched; `sampleImageAtWidgetRect` and `OpenGLSupport.cpp` have no diff since the base. `ColorPickupInteraction::medianColor` is independent of that branch, so Phase 6 can remove it.
- D-01: no GUI walkthrough and no human-verify checkpoint; proof came from ctest, the live harness and these records. What only a person can judge is listed in "Not decided by measurement".
- D-02: the page view, thumbnails and overlays were measured on the real Cocoa display with `grab()` against device-resolution references (`records/live-*.md`); only synthetic pages and the project's own icons were used.
- D-03: the 1x to 2x move is proven by a two-screen offscreen simulation, and the record says no 1x display was attached (next section).
- D-04: all 26 `TestHiDpiQt` tests run in `qt_tests` on every ctest; TESTX-03 moved to Phase 4.
- D-05: thumbnails are built at Quality times `devicePixelRatioF()` (never a constant); the stored setting is not rewritten (range stays 100 to 1500, the cache clamps each side to 3000).
- D-06: the Quality default is 250 and the tooltip names the multiplication; live result `thumb_file_suffix: q500`.
- D-07: the file name keeps encoding the effective width (`q500` on Retina, `q250` at ratio 1); the `QPixmapCache` key of the temp pixmap carries the ratio (`ThumbnailBase::tempPixmap@2.00`).
- D-08: old thumbnail files are left in place; `testOldThumbnailLeftInPlace` hashes a `q200` file before and after.
- D-09: no stretching of an old file; the request returns a null pixmap until the background loader has built the new size (same test).

## Criterion 4 by simulation

Criterion 4 asks that dragging the window from a Retina to a non-Retina display and back re-renders the page view and thumbnails at the right resolution. This Mac has only the Studio Display (ratio 2), so the criterion rests on the offscreen two-screen simulation of `qt_tests` (D-03): `OneX` at ratio 1 and `TwoX` at ratio 2, a window placed on one screen and moved to the other, and assertions that the HQ pixmap, thumbnails, zone mask and stage-list frames are rebuilt with the new ratio and size. It does not show what the Cocoa plugin does when a window is dragged between two real displays.

- Assumption A1: Qt's Cocoa plugin delivers `QEvent::DevicePixelRatioChange` when a window is dragged between displays of different backing scale. It cannot be observed here. Mitigation: every rebuilt buffer also compares its stored ratio with the widget's at paint time (`ImageViewBase::validateHqPixmap`, `PictureZoneEditor::validateScreenPictureMask`), so a missed event is caught at the next paint; the handlers are idempotent (`testRepeatedRatioEventKeepsHqPixmap`).
- Assumption A7: the offscreen platform keeps honouring `QT_SCREEN_SCALE_FACTORS`. After a Qt upgrade, `TestHiDpiQt::initTestCase` fails loudly if the two screens do not come up with ratios 1 and 2.
- To re-check live when a 1x display is attached: drag the app window between the displays, and run `build/macos-release/retina_check build/scratch/retina/all --section all -ApplePersistenceIgnoreState YES` on each display (with the window on that display), then compare `window_dpr` and the verdicts of the two `summary.txt` files.

## Static checks

Commands run from the repository root; outputs are in `build/scratch/closing/`.

```
$ git diff --stat 059f4800 -- src/pipeline_tests/golden src/imageproc src/dewarping 'src/core/filters/*/Task.cpp' src/core/filters/output/OutputGenerator.cpp
(no output)
exit status: 0
$ git diff --stat a796c2ae -- src/pipeline_tests/golden
(no output)
exit status: 0
$ git diff --stat 059f4800 -- src | tail -1        (positive control: the command does see the phase's changes)
 33 files changed, 3181 insertions(+), 118 deletions(-)
```

```
$ grep -rln -e QT_SCALE_FACTOR -e QT_ENABLE_HIGHDPI_SCALING -e QT_SCREEN_SCALE_FACTORS -e AA_EnableHighDpiScaling -e AA_UseHighDpiPixmaps src | sort
src/app/retina_check/main.cpp
src/qt_tests/main.cpp
$ grep -rn (same patterns) src
src/qt_tests/main.cpp:38:  qputenv("QT_SCREEN_SCALE_FACTORS", "OneX=1;TwoX=2");
src/app/retina_check/main.cpp:1092:  qunsetenv("QT_SCALE_FACTOR");
src/app/retina_check/main.cpp:1093:  qunsetenv("QT_SCREEN_SCALE_FACTORS");
src/app/retina_check/main.cpp:1094:  qunsetenv("QT_ENABLE_HIGHDPI_SCALING");
src/app/retina_check/main.cpp:1095:  qunsetenv("QT_SCALE_FACTOR_ROUNDING_POLICY");
```

Only the two allowed files match, and the harness lines are `qunsetenv` calls only.

```
$ grep -rn devicePixelRatio src | wc -l
84          (0 before the phase; 17 of them outside src/qt_tests and the harness)
$ ctest --preset macos-release -N | tail -1
Total Tests: 7
```

`src/app/Info.plist` does not exist; `otool -l` of the app showed no `__info_plist` section in plan 04-02 and nothing in `src/app/CMakeLists.txt` adds one.

## Sanitizer run of qt_tests

The `macos-asan` preset (Debug, `SANITIZE=address,undefined`) was built for the `qt_tests` target (`build/scratch/closing/asan-build.txt`, exit 0) and `qt_tests` was run directly with `ASAN_OPTIONS=detect_leaks=0:halt_on_error=0 UBSAN_OPTIONS=print_stacktrace=1` (`build/scratch/closing/asan-qt-tests.txt`, exit status 0, Totals 8 + 7 + 26 passed, 0 failed, 0 skipped).

- `ERROR: AddressSanitizer` lines: 0.
- `runtime error:` lines: 1, `src/imageproc/BinaryImage.cpp:1044:37: member access within null pointer of type 'SharedData'`.

Attribution: the report is located in `src/imageproc/BinaryImage.cpp`, the location the Phase 1 record and the Phase 2 baseline (`.planning/phases/02-golden-baselines/records/sanitizer-findings.md`, re-measured in Phase 3 `records/sanitizer-compare.md`) list as known, with four triggering tests in four other suites. This time it is raised through a test helper of Phase 4 (`makeZoneMask` at `src/qt_tests/tst_hidpi_qt.cpp:256`, which constructs a `BinaryImage` from a `QImage`): the Phase 4 test calls the old code and merely adds one more caller, `qt_tests`, which had 0 reports in the baseline. No report is located in a file Phase 4 created or changed, so there is nothing to fix under `r14-sanitizer-fix`; the finding is recorded and left alone (Phase 2 rule). Unchanged elsewhere: no new report in `src/core/HiDpi.cpp`, `ImageViewBase.cpp`, `ThumbnailPixmapCache.cpp`, `ThumbnailBase.cpp`, `ThumbnailDprTracker.cpp`, `Utils.cpp`, `StageListView.cpp`, `PictureZoneEditor.cpp`, `ColorPickupInteraction.cpp`, the deskew and page-split views, `Thumbnail.cpp`, `UnremoveButton.cpp`, `ZoneContextMenuInteraction.cpp`, `qt_tests/main.cpp` and `tst_hidpi_qt.cpp`.

## Gate log

Each row of `records/gate-log.md` reads `100% tests passed out of 7` for Release and for Debug and `unchanged` goldens.

| Label | Commit | Subject |
|-------|--------|---------|
| r01-hq-tracer | 7c730ab2 | feat(04-01): build the page view HQ pixmap at device resolution |
| r02-hq-dpr-change | 288ea84b | feat(04-01): rebuild the HQ pixmap when the device pixel ratio changes |
| r03-retina-check | e028d3b1 | test(04-02): add the live Retina check harness |
| r04-thumb-cache | 7ccaa186 | feat(04-03): key the thumbnail cache by device pixel ratio |
| r05-thumb-quality | 18371848 | feat(04-03): raise the thumbnail quality default to 250 |
| r06-thumb-paint | a2396af5 | feat(04-04): draw thumbnails at device resolution |
| r07-thumb-dpr-follow | 489165ea | feat(04-04): follow display changes in the thumbnail cache |
| r08-zone-mask | 6e7a4eff | feat(04-05): build the picture zone mask at device resolution |
| r09-stage-animation | 3899d951 | feat(04-05): render the stage list animation at device resolution |
| r10-icon-geometry | 3064968e | fix(04-06): place icon pixmaps by their logical size |
| r11-color-picker | 6dac9b1c | feat(04-06): sample the colour picker by logical pixels |
| r12-retina-check-overlays | 9f9fdabe | test(04-07): extend the live Retina check to thumbnails and overlays |
| r13-docs | 28b70784 | docs(04-08): document the Retina tests and the live check |

`r03p-info-plist` and `r03s-smoothing` do not exist: neither conditional branch was taken (ratio 2 measured, smoothing ratio 0.9978). `r14-sanitizer-fix` does not exist: no fix was needed. One row carries a deviation: `r08-zone-mask` has a non-empty build-command diff (the `-I src/dewarping` flag on five `qt_tests` compile lines and `libdewarping.a` on the `qt_tests` link line, because `output/Settings.h` includes `DistortionModel.h`); the diff is explained in the log row and in the plan 04-05 summary, and it changes no output code.

## Not decided by measurement

These stay with the user's eye or need hardware this Mac does not have; none of them changes a verdict above.

- Whether the page, thumbnails and overlays look as sharp as other macOS apps. The pixel comparisons prove equality with a device-resolution rendering of the same page, which is the measure D-02 fixed.
- The Cocoa delivery of `DevicePixelRatioChange` on a real drag between a 1x and a 2x display (needs a second display, assumption A1).
- The deskew and page-split handle centring in the page-split view, the trash icon in split thumbnails and the zone context-menu swatch were edited by the same logical-size rule and checked by search that no device-size read remains, but have no pixel test of their own; the deskew handles are measured live, the unremove button by `testIconPixmapLogicalGeometry`.
- The animated zone overlay's per-frame copy: `testZoneMaskOverlayConfined` fails for a mask without the ratio but not for a per-frame copy that loses only resolution; that line is covered by a source check and the live zone-mask measurement.

Phase 4 verdict: PASS
