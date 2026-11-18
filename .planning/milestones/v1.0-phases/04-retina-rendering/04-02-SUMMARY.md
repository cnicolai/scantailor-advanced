---
phase: 04-retina-rendering
plan: 02
subsystem: ui
tags: [qt6, hidpi, retina, cocoa, devicePixelRatio, live-check, smoothing]

requires:
  - phase: 04-retina-rendering
    provides: "plan 04-01: hidpi helpers, ImageViewBase HQ pixmap at device resolution, hqPixmap()/hqPixmapIsCurrent()/displayedSourcePixelWidthMM() diagnostics"
provides:
  - "src/app/retina_check/main.cpp: live Cocoa measurement tool (page section), built under BUILD_TESTS, not a ctest suite"
  - "records/hidpi-01.md: HIDPI-01 measurement (ratio 2), Mach-O comparison, verdict that no Info.plist is needed"
  - "records/live-page-view.md and six PNG crops: page view equals the device-resolution reference at fit, 100% and 200% on the Studio Display"
  - "Measured fact: the 0.5 mm smoothing switch counts physical millimetres on Cocoa (ratio 0.9978), no recalibration"
affects: [04-03, 04-04, 04-05, 04-06, 04-07, 04-08]

plan_head_before: 93b996f33c30c350e1438069756c600021082ff8
plan_head_after: 169295fc67c8af347a8fdc8cc2b55f9f7f32f2d0

actuals:
  tokens: 6200
  tasks: 2
  commits: 3

tech-stack:
  added: []
  patterns:
    - "retina_check: harness that clears every scale-factor variable and QT_QPA_PLATFORM before the application exists, refuses output outside build/ (exit 2) and any platform but cocoa (exit 3), and writes key: value lines to summary.txt"
    - "Live check = same grab-versus-device-reference comparison as testHqGrabMatchesDeviceReference, run on the real display"

key-files:
  created:
    - src/app/retina_check/main.cpp
    - .planning/phases/04-retina-rendering/records/hidpi-01.md
    - .planning/phases/04-retina-rendering/records/live-page-view.md
    - .planning/phases/04-retina-rendering/records/live/page-fit-grab.png
    - .planning/phases/04-retina-rendering/records/live/page-fit-reference.png
    - .planning/phases/04-retina-rendering/records/live/page-100-grab.png
    - .planning/phases/04-retina-rendering/records/live/page-100-reference.png
    - .planning/phases/04-retina-rendering/records/live/page-200-grab.png
    - .planning/phases/04-retina-rendering/records/live/page-200-reference.png
  modified:
    - src/app/CMakeLists.txt
    - .planning/phases/04-retina-rendering/records/gate-log.md

key-decisions:
  - "No Info.plist: the measured window ratio is 2 on the Studio Display for a plain executable with the same Mach-O platform, minos, sdk and no __info_plist as scantailor-advanced (D-00a)"
  - "No smoothing recalibration: widthMM() / width() is millimetres per logical pixel on Cocoa (ratio 0.9978, band 0.98 to 1.02)"

patterns-established:
  - "A live Cocoa run is recorded as summary.txt lines copied into records/*.md, with only crops of generated pages committed"

requirements-completed: [HIDPI-01, HIDPI-02]

coverage:
  - id: D1
    description: "The device pixel ratio of screen, window, page view and viewport is 2 on the Studio Display for a binary of this build; no Info.plist is needed (HIDPI-01)"
    requirement: HIDPI-01
    verification:
      - kind: other
        ref: "build/macos-release/retina_check build/scratch/retina/page --section page (summary.txt: platform cocoa, window_dpr 2, viewport_dpr 2)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The page view's HQ pixmap has ratio 2 and the device-size target rect, and the viewport grab equals the device-resolution reference (100 percent of pixels, maximum channel difference 0) at fit, 100% and 200%; the ratio 1 counterfactual differs in 6.3 percent at fit (HIDPI-02)"
    requirement: HIDPI-02
    verification:
      - kind: other
        ref: "retina_check summary.txt: page_fit_verdict, page_100_verdict, page_200_verdict all PASS"
        status: pass
    human_judgment: false
  - id: D3
    description: "The 0.5 mm smoothing switch flips at the same physical size as at ratio 1 on Cocoa"
    requirement: HIDPI-02
    verification:
      - kind: other
        ref: "retina_check summary.txt: smoothing_ratio 0.9978, smoothing_verdict PASS"
        status: pass
    human_judgment: false
  - id: D4
    description: "The harness commit passes gate.sh with an empty command diff (retina_check is not a gated target)"
    verification:
      - kind: other
        ref: "bash .planning/phases/03-qt-6-and-macos-only/gate.sh check r03-retina-check"
        status: pass
    human_judgment: false
  - id: D5
    description: "Whether the page looks as sharp as Preview to the eye"
    requirement: HIDPI-02
    verification: []
    human_judgment: true
    rationale: "The pixel comparison proves equality with a device-resolution rendering; the perceptual comparison with Preview named in the roadmap criterion is the user's to confirm in verification"

duration: 11min
completed: 2026-10-25
status: complete
---

# Phase 4 Plan 2: Live Retina check on the Studio Display Summary

**`retina_check`, a committed Cocoa harness, measured ratio 2 for screen, window and page view, a pixel-exact match of the page view with a device-resolution reference at fit, 100% and 200%, and a smoothing switch that already counts physical millimetres (ratio 0.9978), so neither the Info.plist nor the smoothing recalibration branch was needed**

## Performance

- **Duration:** 11 min
- **Started:** 2026-10-25T13:34:30Z
- **Completed:** 2026-10-25T13:46:00Z
- **Tasks:** 2
- **Files modified:** 11 (retina_check/main.cpp and src/app/CMakeLists.txt, gate-log, two records, six PNG crops)

## Accomplishments

- `retina_check <output-dir> [--section page]` builds under `BUILD_TESTS` from `src/app/retina_check/main.cpp`, links `core` and `test_support` only, and is not a ctest suite (`Total Tests: 7`). It exits 2 for an output directory outside `build/` (checked with `build/../retina-outside`, nothing created) and 3 off Cocoa, clears the scale-factor variables, and keeps settings and temporary files under its output directory.
- On the Studio Display (Qt 6.12.0, 2560x1440 logical, physical DPI 109.75): screen, window, view and viewport ratio 2, identical to the Phase 1 probe. The Mach-O comparison (platform 1, minos 26.0, sdk 27.0, no `__info_plist`, same Qt frameworks) ties the harness to `scantailor-advanced`.
- Page view versus device-resolution reference: HQ pixmap ratio 2 and size equal to the expected target rect at fit (1565x1800), 100% (2000x1800) and 200% (2526x1766); equal fraction 1.000000 and maximum channel difference 0 in all three; the ratio 1 counterfactual differs in 6.3 percent of the pixels at fit (floor 5 percent).
- Smoothing: `displayedSourcePixelWidthMM()` 0.182812 mm versus 0.183210 mm from the screen's physical size (ratio 0.9978); screen 0.2341 mm per logical pixel, 0.5 percent from the 0.2330 mm panel figure. The 0.5 mm switch flips at zoom 2.1358 on this screen.

## Task Commits

1. **Task 1: the retina_check harness with its page section, built and gated** - `e028d3b1` (test); gate `r03-retina-check` PASS (Release and Debug 100% out of 7, goldens unchanged, command diff empty), gate record `9bad0ace`
2. **Task 2: run on the Studio Display, record HIDPI-01 and the live page view** - `169295fc` (docs); `.planning/` only, no gate

**Plan metadata:** the commit that adds this summary (docs: complete plan)

## Files Created/Modified

- `src/app/retina_check/main.cpp` - the harness: argument and path guard, bare-Cocoa environment, page section, smoothing check, `summary.txt`
- `src/app/CMakeLists.txt` - `retina_check` target inside the `BUILD_TESTS` block, with `RETINA_CHECK_ROOT`
- `.planning/phases/04-retina-rendering/records/hidpi-01.md` - HIDPI-01 measurement, Mach-O table, smoothing verdict
- `.planning/phases/04-retina-rendering/records/live-page-view.md` - Fit, 100%, 200% values and `Success criterion 2 (page view): PASS`
- `.planning/phases/04-retina-rendering/records/live/page-{fit,100,200}-{grab,reference}.png` - 256x256 device-pixel crops of the synthetic FLAT page
- `.planning/phases/04-retina-rendering/records/gate-log.md` - row for `r03-retina-check`

## Decisions Made

- No Info.plist (D-00a): ratio 2 was measured, so `src/app/Info.plist` does not exist and `otool -l build/macos-release/scantailor-advanced` shows no `__info_plist`.
- No smoothing recalibration: the measured ratio 0.9978 lies inside 0.98 to 1.02 and the screen size agrees with the panel, so `src/core/ImageViewBase.cpp` is unchanged by this plan.

## Deviations from Plan

None - plan executed exactly as written. Neither conditional branch (Info.plist, smoothing recalibration) was taken.

## Issues Encountered

None. The Cocoa window exposed at once, no run needed a retry. `timeout` does not exist on this Mac, so the run relied on the tool's own time limit instead of a shell timeout; the run ended in a few seconds.

## Known Stubs

None.

## Threat Flags

None. The only new surface is the harness itself; its file writes stay below `build/` (guard checked), and `grep -c "$HOME"` prints 0 for both records (T-04-05, T-04-06, T-04-08).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 04-03 onward can rely on ratio 2 on this display with no Info.plist, and on the smoothing switch being correct as it is.
- Plan 04-07 extends `retina_check` with a section for thumbnails and overlays (`--section` currently accepts `page` only).
- The Cocoa delivery of `DevicePixelRatioChange` on a real display drag is still not observable on this one-display Mac; the offscreen simulation of plan 04-01 stays the evidence.
- The perceptual "as sharp as Preview" judgement is left to verification (coverage D5).

## Self-Check: PASSED

- `src/app/retina_check/main.cpp`, the two records, the six PNG crops and `gate-log.md` exist on disk.
- Commits `e028d3b1`, `9bad0ace`, `169295fc` are ancestors of HEAD; `build/scratch/gate/r03-retina-check/summary.txt` reads `result: PASS` and `goldens: unchanged`, both command diffs are empty.
- The summary of the run contains `platform: cocoa`, `window_dpr: 2`, `page_fit_verdict: PASS`, `page_100_verdict: PASS`, `page_200_verdict: PASS`, `smoothing_verdict: PASS`, `verdict: PASS`; `src/app/Info.plist` does not exist.

---
*Phase: 04-retina-rendering*
*Completed: 2026-10-25*
