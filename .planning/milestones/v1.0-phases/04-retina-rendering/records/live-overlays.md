# Live check of thumbnails and overlays on the Studio Display (plan 04-07)

## Method

Measured 2026-10-25 with `retina_check` (commit `9f9fdabe`, gate `r12-retina-check-overlays` PASS) on the Cocoa platform, one run of all sections:

```
build/macos-release/retina_check build/scratch/retina/all --section all -ApplePersistenceIgnoreState YES
```

The run reports `platform: cocoa`, Qt 6.12.0, screen `Studio Display` 2560x1440 logical at ratio 2, window ratio 2, no Info.plist (see `hidpi-01.md`). Settings, temporary files, the generated page and the thumbnail cache live below the output directory under `build/`; the cursor is not moved and the app binary is not launched. Every element is shown in its own top-level window, grabbed with `grab()` as `Format_RGB32` (the ratio of the grab is checked), and a crop of at most 256x256 device pixels is saved. The crops show only the generated FLAT page, generated masks and the project's own icons.

Device-detail measure (the one of `minBlockNonUniformFraction` in `src/qt_tests/tst_hidpi_qt.cpp`): over a region of interest of a device-pixel grab, for each of the four offsets (0 or 1 in x and y), the fraction of 2x2 blocks inside the region whose four pixels are not all equal within 2 levels per channel; the measure is the minimum over the offsets. A buffer rendered at ratio 1 and shown 2x with nearest sampling has uniform 2x2 blocks at its aligned offset and so scores 0; a rendering with device-resolution edges or gradients scores clearly above 0. A threshold that is missed would be recorded as FAIL; none was missed and no threshold was changed.

## Thumbnails

A `ThumbnailPixmapCache` (Quality 250) and a `ThumbnailDprTracker` on a 400x300 `QGraphicsView`, wired as `MainWindow` does (tracker with the view, cache handed over with `setCache` after the window is shown, item added afterwards); one `ThumbnailBase` of the FLAT page (2480x3508) with `maxSize` 250x160.

```
thumb_cache_dpr: 2
thumb_loaded: yes
thumb_file_suffix: q500
thumb_pixmap_long_side: 500
thumb_grab_dpr: 2
thumb_detail_fraction: 0.1824
thumbnails_verdict: PASS
```

Thresholds: cache and grab ratio 2, a single file with suffix `q500`, long side 500, detail at least 0.02 over the item's bounding rect (inset by 2 device pixels). Crop: `live/thumb-grab.png`.

What this does not show: the cache ratio comes from `setCache` reading the view's `devicePixelRatioF()` on the real display. Cocoa's delivery of `DevicePixelRatioChange` to the child `thumbView` when the window is dragged to a display with another ratio cannot be observed on this one-display Mac (as for the page view in plan 04-02); the offscreen simulation of plan 04-04 (`testThumbnailsFollowScreenChange`) remains the evidence for that event.

## Zone mask

The picture-zone editor of `testZoneMaskAtDeviceResolution` (white 1200x900 page, a 200x200 square rotated by 30 degrees in the picture mask) at 400x300.

```
mask_dpr: 2
mask_size: 747x560
mask_expected_size: 747x560
mask_grab_dpr: 2
mask_band_detail_fraction: 0.3844
mask_verdict: PASS
```

Thresholds: mask ratio 2, mask size equal to `hidpi::targetRect` with the viewport ratio, detail at least 0.02 over the blocks whose centre lies in a 6-device-pixel stroke along the square's outline. Crop: `live/mask-grab.png` (the tinted square on the white page).

## Page badges

The badge as `ThumbnailSequence.cpp` builds it, with the real default icon pack: `getIcon("left_page_thumb").pixmap(QSize(23, 17))` in a `QGraphicsPixmapItem` at (10, 10) of a scene shown in a frameless 120x80 view (the Cocoa viewport came out 118x78; the item lies fully inside).

```
badge_view_dpr: 2
badge_pixmap_dpr: 2
badge_pixmap_size: 46x34
badge_bounding_rect: 23x17
badge_viewport_size: 118x78
badge_grab_dpr: 2
badge_device_rect: 18,18 46x34
badge_grab_match_fraction: 1.0000
badge_grab_max_diff: 0
badge_detail_fraction: 0.0000
badge_verdict: PASS
```

Thresholds: pixmap ratio equal to the view's ratio and 2, 46x34 device pixels, 23x17 logical bounding rect, at least 99 percent of the 46x34 device pixels equal to the pixmap drawn 1:1, maximum channel difference at most 2. The block measure is recorded but not required: the shipped `left_page_thumb.svg` draws only lines and bars on whole logical pixels, so at ratio 2 its edges fall on even device pixels and a ratio-1 rendering upscaled 2x looks the same pixel for pixel (block measure 0.0000 here). The ratio-1 discrimination is carried by the offscreen test `testPageBadgeAtDeviceResolution` (plan 04-06), which uses a badge with a diagonal edge. Crop: `live/badge-grab.png` (46x34 device pixels).

## Handles

`deskew::ImageView` on the FLAT page at 600x500, two runs. Placement run with a scratch icon sheet whose `aqua-sphere` is a 16x16 opaque magenta square (so the handles cannot be confused with the blue grid and arcs); sharpness run with the real icon.

```
handle_blob_count: 4
handle_blob_max_side: 32
handle_horizontal_offset: 0.000
handle_vertical_offset: 0.000
handle_viewport_size: 600x500
handle_pixmap_dpr: 2
handle_logical_size: 16x16
handle_detail_fraction: 0.8224
handle_verdict: PASS
```

Thresholds: pixmap ratio 2 and logical size 16x16, four blobs, longest blob side between 30 and 34 device pixels, both offsets (left/right blob centres against the viewport's horizontal centre, top/bottom against the vertical centre) at most 2 logical pixels, detail at least 0.02 over the four blob boxes. With the pre-phase bug every handle sat 8 logical pixels up and left of its anchor; here the offsets are 0.000. Crop: `live/handles-grab.png` (the right handle of the sharpness run, real icon).

## Stage-list animation

A `StageListView` 300x400 over a real `StageSequence`, row 0 selected, batch processing in progress.

```
stage_frame_dpr: 2
stage_frame_size: 26x26
stage_frame_logical_size: 13x13
stage_cell_rect: 263,0 18x18
stage_ink_box: 530,4 26x26
stage_ink_center_offset: 0.707
stage_detail_fraction: 0.1598
stage_verdict: PASS
```

Thresholds: frame ratio 2, device size twice the logical size, centre of the ink box within 1.5 logical pixels of the cell centre, detail at least 0.02. Measurement note: a single frame shows only a comet of about four bubbles (the tail colour is close to the cell background in this colour scheme), so the ink box of one grab is lopsided (the first trial run measured an 8x20 box 4.0 logical pixels off centre with one grab). The recorded values therefore unite the ink of 12 grabs one animation step apart, in which each of the 8 ring positions carries the head colour once, and the detail value is the best of the 12 grabs over that united box. Crop: `live/stage-grab.png` (best grab, united box plus 8 pixels).

## List decorations

A `QListView` 200x60 over a `QStandardItemModel` with one item whose `Qt::DecorationRole` is `getIcon("file").pixmap(16, 16)` (the `RelinkingModel` pattern).

```
decoration_pixmap_dpr: 2
decoration_drawn_size: 20x24
decoration_pixmap_size: 32x32
decoration_pixmap_ink_size: 20x24
decoration_detail_fraction: 0.0250
decoration_verdict: PASS
```

Thresholds: pixmap ratio 2, each drawn side between 20 and 34 device pixels (10 to 17 logical), detail at least 0.02. The drawn ink (20x24 device pixels) equals the opaque part of the 32x32 pixmap itself (20x24), so the item view draws the model-data pixmap 1:1 at device resolution, at 16x16 logical pixels. Measurement note: the plan's "left 24 logical pixels" box also took in the first letter of the item text (the first trial run measured 36x36); the recorded box is the ink of the first run of adjacent columns, which is the icon alone. Crop: `live/decoration-grab.png`.

Assumption A5 (RESEARCH): confirmed. A pixmap with ratio 2 given as decoration data is drawn at its logical size, so `RelinkingModel`'s `getIcon("file").pixmap(16, 16)` needs no change.

## Colour picker

The colour-patch `BasicImageView` of `testColorPickerGrabAtRatio2` at 400x300 (page of colour `#283cc8` with a `#dc781e` patch); a 15x15-logical rect centred in the view is grabbed with `view.grab(rect)` and passed to `ColorPickupInteraction::medianColor`. The real mouse cursor is not moved.

```
picker_grab_size: 30x30
picker_grab_dpr: 2
picker_color: #dc781e
picker_expected: #dc781e
picker_verdict: PASS
```

Thresholds: 30x30 pixels at ratio 2, picked colour equal to the patch colour. The mapping from the cursor position to the grabbed rect (`mapFromGlobal`, logical) is unchanged by the phase and is not driven here.

## Page view (re-run)

The page section ran again in the same process after plans 04-03 to 04-06; every value equals the record of plan 04-02 (`live-page-view.md`).

```
page_fit_equal_fraction: 1.000000    page_fit_max_diff: 0    page_fit_counterfactual_diff_fraction: 0.062592
page_100_equal_fraction: 1.000000    page_100_max_diff: 0
page_200_equal_fraction: 1.000000    page_200_max_diff: 0
smoothing_ratio: 0.9978
page_verdict: PASS
```

## Verdicts

Overall run: `verdict: PASS`, with `page_verdict`, `thumbnails_verdict`, `overlays_verdict` (mask, badge, handle, stage, decoration) and `picker_verdict` all PASS.

Success criterion 2 (thumbnails): PASS

Success criterion 3: PASS

Success criterion 5 (live picker): PASS

Left to the user's eye (not decidable by pixel measurement): whether thumbnails and overlays look as sharp as other macOS apps, and the Cocoa delivery of `DevicePixelRatioChange` on a real display move (needs a second display).
