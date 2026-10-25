# HIDPI-01 record: device pixel ratio on the Studio Display (plan 04-02)

Measured 2026-10-25 with `retina_check` (commit e028d3b1, gate `r03-retina-check` PASS).

## Method

- The measurement is made by `retina_check` (`src/app/retina_check/main.cpp`), a harness built into `{repo}/build/macos-release/` by the same toolchain as `scantailor-advanced`. It uses the same `Application` class (`src/core/Application.h`) and the same colour scheme and icon pack set-up as `src/app/main.cpp`, and it shows a `BasicImageView` on the real Cocoa display.
- The harness clears `QT_QPA_PLATFORM`, `QT_SCALE_FACTOR`, `QT_SCREEN_SCALE_FACTORS`, `QT_ENABLE_HIGHDPI_SCALING`, `QT_SCALE_FACTOR_ROUNDING_POLICY` and `QT_USE_PHYSICAL_DPI` before the application object exists and exits with 3 unless the platform is `cocoa`, so it sees bare Cocoa behaviour (D-00c).
- `build/macos-release/scantailor-advanced` itself was not launched: it reads and writes `~/.config/scantailor-advanced/` and can read `~/Library/Application Support/...`, which the home-folder rule forbids without the user's yes. The harness keeps its settings and temporary files below its output directory under `{repo}/build/` and was started with `-ApplePersistenceIgnoreState YES`.
- `qt.highdpi` logging is not used as evidence (RESEARCH Pitfall 11; Phase 1 also got no such line from the app).
- Run command: `build/macos-release/retina_check build/scratch/retina/page --section page -ApplePersistenceIgnoreState YES`, exit status 0, `verdict: PASS`.

## Harness facts

```
platform: cocoa
qt_runtime: 6.12.0
screen_name: Studio Display
screen_dpr: 2
screen_logical_dpi: 72
screen_physical_dpi: 109.75
screen_geometry: 0,0 2560x1440
window_dpr: 2
view_dpr: 2
viewport_dpr: 2
```

Screen, window, page view and its viewport all report device pixel ratio 2.

## Mach-O comparison

`otool -l` and `otool -L` of the two binaries in `{repo}/build/macos-release/`:

| Property | scantailor-advanced | retina_check |
|----------|---------------------|--------------|
| LC_BUILD_VERSION platform | 1 (macOS) | 1 (macOS) |
| minos | 26.0 | 26.0 |
| sdk | 27.0 | 27.0 |
| `__info_plist` section | none (0 matches) | none (0 matches) |
| QtCore, QtGui, QtWidgets | `/opt/homebrew/opt/qtbase/lib/*.framework`, 6.12.0 | the same three frameworks, 6.12.0 |

Same build platform, minimum OS and SDK, no embedded Info.plist in either binary, the same Qt frameworks. AppKit therefore treats the two executables alike, and the ratio of the harness window is the ratio the app gets.

## Comparison with Phase 1

The Phase 1 probe (`.planning/phases/01-deletion-free-macos-build/records/probe-output.txt`, plain executable, no bundle) measured: screen ratio 2, window ratio 2, geometry 2560x1440, physical DPI 109.75, Qt 6.12.0. The harness reproduces every one of these values.

## Smoothing heuristic

`ImageViewBase::paintEvent` compares `displayedSourcePixelWidthMM()` (the zoom times `widthMM() / width()`) with 0.5 mm to decide between smooth and non-smooth pixmap drawing. Measured on Cocoa at fit:

```
widthMM: 299
width: 1280
mm_per_logical_px: 0.2336
screen_physical_size_mm: 599.3x329.5
screen_mm_per_logical_px: 0.2341
smoothing_fit_source_px_mm: 0.182812
smoothing_fit_reference_mm: 0.183210
smoothing_ratio: 0.9978
smoothing_switch_xscale: 2.1358
smoothing_verdict: PASS
```

- Band: 0.98 to 1.02. `widthMM()` is an integer, so plus or minus 1 mm on a view about 299 mm wide is under 0.4 percent; a wider band would hide a real error, a narrower one would fail on the rounding alone.
- Panel plausibility: the Studio Display shows 5120 device pixels across at 218 pixels per inch as 2560 logical pixels, 0.2330 mm per logical pixel. The measured `screen_mm_per_logical_px` of 0.2341 is 0.5 percent above it (limit 2 percent), so Qt's screen size agrees with the panel. The Phase 1 probe's 109.75 physical DPI gives 0.2314 mm, 0.7 percent below 0.2330.
- The 0.5 mm switch of `ImageViewBase::paintEvent` flips where one source pixel is 0.5 mm wide, at zoom 2.1358 logical pixels per source pixel on this screen (RESEARCH expected about 2.16 from 0.2314 mm).
- What a contrary ratio would have meant: a ratio near 0.5 would put the switch at 1.0 mm, a ratio near 2 at 0.25 mm. The measured ratio of 0.9978 shows `widthMM() / width()` is millimetres per logical pixel on Cocoa at ratio 2, as RESEARCH derived, so no recalibration was made and `src/core/ImageViewBase.cpp` is unchanged by this plan.

Smoothing switch: no Retina correction needed (ratio 0.9978)

HIDPI-01 verdict: ratio 2 measured; Info.plist not needed (D-00a)
