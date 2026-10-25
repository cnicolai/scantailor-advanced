# Live page view on the Studio Display (plan 04-02)

Measured 2026-10-25 with `retina_check --section page` on Cocoa (screen, window and viewport ratio 2; see `hidpi-01.md`). The page is the synthetic FLAT page `PageSpec{FLAT, 300, 1, 0.0}` (2480x3508), shown in a 1280x900 `BasicImageView`. For each zoom the view's HQ pixmap and the grab of its viewport are compared, over the target rect shrunk by 3 pixels on each side, with a device-resolution reference: the same page transformed by `imageproc::transform` at the device transform `hidpi::toDevice(imageToWidget(), 2)`. The counterfactual is the ratio 1 rendering upscaled 2x with nearest-neighbour filtering.

Zooms: fit is `setZoomLevel(1.0)`; 100% is `setZoomLevel(1.0 / fit)` (one source pixel per logical pixel); 200% is `setZoomLevel(2.0 / fit)`; `fit` is `virtualToWidget().m11()` at zoom 1.0.

Thresholds for a zoom to pass: window ratio at least 2; HQ pixmap ratio equals viewport ratio; HQ pixmap size equals the expected device size from `hidpi::targetRect`; equal fraction at least 0.999; maximum channel difference at most 2; at fit only, counterfactual differing fraction at least 0.05. At 100% and 200% a sharp device rendering of an axis-aligned page equals pixel replication, so the counterfactual is recorded but not required there.

## Fit

```
page_fit_hq_dpr: 2
page_fit_hq_size: 1565x1800
page_fit_expected_size: 1565x1800
page_fit_equal_fraction: 1.000000
page_fit_max_diff: 0
page_fit_counterfactual_diff_fraction: 0.062592
page_fit_verdict: PASS
```

The counterfactual differs in 6.3 percent of the compared pixels (floor 5 percent). The FLAT page is mostly paper, so the share is lower than for the noise page of the offscreen test (94 percent); it is still above the floor, so a ratio 1 buffer scaled up would fail this comparison.

Crops (256x256 device pixels, centre of the compared region): `live/page-fit-grab.png`, `live/page-fit-reference.png`.

## 100%

```
page_100_hq_dpr: 2
page_100_hq_size: 2000x1800
page_100_expected_size: 2000x1800
page_100_equal_fraction: 1.000000
page_100_max_diff: 0
page_100_counterfactual_diff_fraction: 0.000000
page_100_verdict: PASS
```

Crops: `live/page-100-grab.png`, `live/page-100-reference.png`.

## 200%

```
page_200_hq_dpr: 2
page_200_hq_size: 2526x1766
page_200_expected_size: 2526x1766
page_200_equal_fraction: 1.000000
page_200_max_diff: 0
page_200_counterfactual_diff_fraction: 0.000000
page_200_verdict: PASS
```

Crops: `live/page-200-grab.png`, `live/page-200-reference.png`.

Success criterion 2 (page view): PASS
