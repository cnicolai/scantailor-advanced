---
phase: 04-retina-rendering
plan: 07
subsystem: ui
tags: [qt6, hidpi, retina, cocoa, live-check, thumbnails, overlays, colour-picker]

requires:
  - phase: 04-retina-rendering
    provides: "plan 04-02: retina_check harness with the page section, its path guard and Cocoa-only exit"
  - phase: 04-retina-rendering
    provides: "plans 04-03 to 04-06: ratio-aware thumbnail cache, ThumbnailDprTracker, zone mask and stage animation at device resolution, logical icon geometry, ColorPickupInteraction::medianColor"
provides:
  - "retina_check --section page|thumbnails|overlays|picker|all, with 12 elements measured on the real display and a verdict line each"
  - "records/live-overlays.md: live verdicts for success criteria 2 (thumbnails), 3 and 5, assumption A5 confirmed"
  - "six PNG crops of generated pages and project icons in records/live/"
affects: [04-08]

plan_head_before: 2110a9ec0e8ce6a58fa93c4eb8845cae9f5ccb79
plan_head_after: 081f8fe0cb0fa52d48a3330e592e7a5c7b8a91ba

actuals:
  tokens: 9000
  tasks: 2
  commits: 3

tech-stack:
  added: []
  patterns:
    - "Live Cocoa measurement elements that each write prefix_* lines and a prefix_verdict, so one failing element does not hide the others"
    - "Device-detail measure (minimum over the four 2x2 block offsets of the non-uniform block fraction) with an optional block filter, shared by all elements"
    - "Colour-keyed placement run: a scratch icon sheet with an opaque magenta handle makes the handle blobs unambiguous in a grab"

key-files:
  created:
    - .planning/phases/04-retina-rendering/records/live-overlays.md
    - .planning/phases/04-retina-rendering/records/live/thumb-grab.png
    - .planning/phases/04-retina-rendering/records/live/mask-grab.png
    - .planning/phases/04-retina-rendering/records/live/badge-grab.png
    - .planning/phases/04-retina-rendering/records/live/handles-grab.png
    - .planning/phases/04-retina-rendering/records/live/stage-grab.png
    - .planning/phases/04-retina-rendering/records/live/decoration-grab.png
  modified:
    - src/app/retina_check/main.cpp
    - src/app/CMakeLists.txt
    - .planning/phases/04-retina-rendering/records/gate-log.md

key-decisions:
  - "retina_check links dewarping: the zone-mask element needs output::Settings, whose header includes DistortionModel.h (same cause as the qt_tests link in plan 04-05)"
  - "The thumbnails section hands the cache to the tracker after the window is shown, as MainWindow does on a project switch, so the measured cache ratio comes from the real display"
  - "Stage-list centring is measured on the ink united over 12 grabs, and list-decoration size on the first run of adjacent ink columns, because the plan's single-grab and left-24-pixel recipes measure the comet fragment and the item text instead of the element"

patterns-established:
  - "A measurement recipe that cannot see the whole element is corrected in the harness and the correction is recorded with the first trial values, not by loosening a threshold"

requirements-completed: [HIDPI-03, HIDPI-04, HIDPI-06]

coverage:
  - id: D1
    description: "On the Studio Display a thumbnail cache at Quality 250 with a ThumbnailDprTracker has ratio 2, builds a single q500 file with a 500 px long side, and the grab has ratio 2 with a detail fraction of 0.1824 (HIDPI-03, success criterion 2 for thumbnails)"
    requirement: HIDPI-03
    verification:
      - kind: other
        ref: "build/macos-release/retina_check build/scratch/retina/all --section all (summary.txt: thumbnails_verdict PASS)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The picture-zone mask has ratio 2 and the expected device size (747x560) and a band detail fraction of 0.3844; handles are centred on the rotation origin (offsets 0.000, 32 px blobs); the stage-list frame has ratio 2 and is centred within 0.707 logical pixels (HIDPI-04, success criterion 3)"
    requirement: HIDPI-04
    verification:
      - kind: other
        ref: "retina_check summary.txt: mask_verdict, handle_verdict, stage_verdict PASS"
        status: pass
    human_judgment: false
  - id: D3
    description: "The shipped page badge has ratio 2 equal to the view's, 46x34 device pixels, a 23x17 bounding rect, and the view grab equals it drawn 1:1 in 100 percent of its pixels"
    requirement: HIDPI-04
    verification:
      - kind: other
        ref: "retina_check summary.txt: badge_verdict PASS (badge_grab_match_fraction 1.0000, badge_grab_max_diff 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "A 16x16 file icon pixmap given to a QListView as decoration data is drawn 1:1 at device resolution (20x24 device ink equals the pixmap's own ink), so RelinkingModel needs no change (RESEARCH A5 confirmed)"
    requirement: HIDPI-04
    verification:
      - kind: other
        ref: "retina_check summary.txt: decoration_verdict PASS"
        status: pass
    human_judgment: false
  - id: D5
    description: "A 15x15-logical grab of a page view is 30x30 at ratio 2 and medianColor returns exactly the patch colour #dc781e (HIDPI-06, success criterion 5 live)"
    requirement: HIDPI-06
    verification:
      - kind: other
        ref: "retina_check summary.txt: picker_verdict PASS"
        status: pass
    human_judgment: false
  - id: D6
    description: "The page section still passes at fit, 100% and 200% in the same all-sections run with the values of plan 04-02"
    requirement: HIDPI-04
    verification:
      - kind: other
        ref: "retina_check summary.txt: page_verdict PASS"
        status: pass
    human_judgment: false
  - id: D7
    description: "The harness commit passes gate.sh: Release and Debug 100% tests passed out of 7, goldens unchanged, empty command diff"
    verification:
      - kind: other
        ref: "bash .planning/phases/03-qt-6-and-macos-only/gate.sh check r12-retina-check-overlays"
        status: pass
    human_judgment: false
  - id: D8
    description: "Cocoa delivers DevicePixelRatioChange to the child thumbView on a real display move, and thumbnails and overlays look as sharp as other macOS apps"
    requirement: HIDPI-05
    verification: []
    human_judgment: true
    rationale: "The live run measures the ratio the cache takes from the view on the one attached display; the event on a display drag needs a second display, and the perceptual comparison is the user's to confirm in verification"

duration: 50min
completed: 2026-10-25
status: complete
---

# Phase 4 Plan 7: Live check of thumbnails and overlays Summary

**`retina_check` now measures thumbnails (q500 cache, device-resolution grab), zone mask, page badge, deskew handles, stage-list animation, list decoration and the colour picker on the real Studio Display, and one all-sections Cocoa run passes every element, which closes success criteria 2 (thumbnails), 3 and 5 and confirms that model-data icon pixmaps are drawn 1:1**

## Performance

- **Duration:** about 50 min (estimated from the commit times)
- **Started:** 2026-10-25T14:35Z (estimated)
- **Completed:** 2026-10-25T15:25Z
- **Tasks:** 2 (both auto)
- **Files modified:** 10 (retina_check/main.cpp, src/app/CMakeLists.txt, gate log, the record and six crops)

## Accomplishments

- `retina_check` takes `--section page|thumbnails|overlays|picker|all`; any other value prints the usage and exits 2 before a directory is created. Each section writes `key: value` lines and a verdict; the final `verdict` is PASS only if every section passed. `platform` and `qt_runtime` moved to `main` so every section run records them.
- Thumbnails: a `ThumbnailPixmapCache` at Quality 250 plus `ThumbnailDprTracker` on a `QGraphicsView` give cache ratio 2, one `q500` file, a 500 px long side, a ratio-2 grab and a detail fraction of 0.1824.
- Overlays: zone mask 747x560 at ratio 2 (expected size equal, band detail 0.3844); page badge 46x34 at ratio 2, 23x17 logical, grab equal to the 1:1 reference in 100 percent of the pixels; four 32 px handle blobs centred on the rotation origin (offsets 0.000, detail 0.8224); stage frame 26x26 at ratio 2 centred within 0.707 logical pixels (detail 0.1598); list decoration drawn 1:1 (20x24 device ink equals the pixmap's own ink), which confirms RESEARCH A5.
- Picker: the 15x15-logical grab is 30x30 at ratio 2 and `medianColor` returns `#dc781e`, the patch colour.
- The page section ran again in the same run and equals plan 04-02 (equal fraction 1.000000, maximum difference 0 at fit, 100% and 200%).

## Task Commits

1. **Task 1: add the thumbnails, overlays and picker sections to retina_check** - `9f9fdabe` (test); gate `r12-retina-check-overlays` PASS (Release and Debug 100% out of 7, goldens unchanged, command diff 0 / 0), gate record `46478e13`
2. **Task 2: run every section on the Studio Display and record the live verdicts** - `081f8fe0` (docs); `.planning/` only, no gate

**Plan metadata:** the commits that add this summary and update state and roadmap.

## Files Created/Modified

- `src/app/retina_check/main.cpp` - sections, 12 elements, shared device-detail measure, blob finder, summary keys
- `src/app/CMakeLists.txt` - `dewarping` added to the `retina_check` link libraries
- `.planning/phases/04-retina-rendering/records/live-overlays.md` - method, nine element sections, verdict lines
- `.planning/phases/04-retina-rendering/records/live/{thumb,mask,badge,handles,stage,decoration}-grab.png` - crops of generated pages and project icons
- `.planning/phases/04-retina-rendering/records/gate-log.md` - row `r12-retina-check-overlays`

## Decisions Made

- `retina_check` links `dewarping` (the zone-mask element builds an `output::PictureZoneEditor`, whose `output::Settings` header includes `DistortionModel.h`).
- The thumbnails section creates the tracker with the view, shows the window, then calls `setCache`, which is the production order in `MainWindow`; the item is added afterwards so the first request is already for `q500` and the thumbs directory holds a single file.
- Two measurement recipes of the plan were corrected in the harness (see Deviations); no pass threshold was changed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] retina_check could not include output/Settings.h**
- **Found during:** Task 1 (design of the zone-mask element; same cause as plan 04-05's deviation 1)
- **Issue:** `retina_check` linked `core` and `test_support` only; `output/Settings.h` needs the dewarping include directory.
- **Fix:** `dewarping` added to `target_link_libraries(retina_check ...)`.
- **Consequence:** none for the gate: `retina_check` is not a gated target and the command diff is empty (0 / 0).
- **Files modified:** `src/app/CMakeLists.txt`
- **Committed in:** `9f9fdabe`

**2. [Rule 1 - Bug in the plan's measurement recipe] Stage-list centring measured on one grab**
- **Found during:** Task 1 (first trial run on the display; the trial run was a debugging run of the harness, the recorded run is the one of Task 2)
- **Issue:** one frame of the animation shows a comet of about four bubbles whose tail fades into the cell background (background 107, tail 136 against the 30-level ink threshold), so the ink box was 8x20 device pixels and 4.0 logical pixels off the cell centre although the frame itself is centred (plan 04-05 pins that offscreen).
- **Fix:** the ink of 12 grabs, one animation step apart, is united (every one of the 8 ring positions carries the head colour once); the detail value is the best of the 12 grabs over the united box. Thresholds unchanged (offset at most 1.5, detail at least 0.02). Result: ink box 26x26 device pixels, offset 0.707.
- **Files modified:** `src/app/retina_check/main.cpp`
- **Committed in:** `9f9fdabe`

**3. [Rule 1 - Bug in the plan's measurement recipe] List-decoration box included the item text**
- **Found during:** Task 1 (first trial run: drawn size 36x36, detail 0.0772, verdict FAIL)
- **Issue:** the plan restricts the ink box to the left 24 logical pixels of the item, which also contains the first letter of the item text.
- **Fix:** the box is the ink of the first run of adjacent columns (the icon, which is separated from the text by a gap); the opaque part of the pixmap itself is recorded for comparison. Thresholds unchanged. Result: 20x24 device pixels, equal to the pixmap's own 20x24, detail 0.0250 (floor 0.02).
- **Files modified:** `src/app/retina_check/main.cpp`
- **Committed in:** `9f9fdabe`

---

**Total deviations:** 3 auto-fixed (1 blocking, 2 measurement-recipe corrections)
**Impact on plan:** no production code changed; both corrected measurements are described with their first trial values in `live-overlays.md`, so the change cannot be mistaken for a retuned threshold.

## Issues Encountered

- The harness was run on the display three times before the gate (two debugging runs, one of them with the two failing elements above) and once more for the record; the display was awake and the session unlocked each time, every window was exposed at once, and each run took about 8 seconds.
- The decoration detail fraction (0.0250) is close to its floor (0.02): the document icon is small and mostly flat-coloured. It is the measured value and is recorded as such.
- Uncommitted `.planning/config.json`, `.planning/milestone.lock` and `.planning/state.json` predate this plan and were left out of every commit.

## Known Stubs

None.

## Threat Flags

None. No new network, auth or file-access surface in production code. T-04-22: every path of the new sections (generated page, thumbnail cache, scratch icon sheet, crops) lies below the output directory under `build/`; the cursor was not moved and the app binary was not launched. T-04-23: crops show only the generated page, generated masks and the project's own icons, and `grep -c "$HOME"` prints 0 for the record. T-04-24: thresholds are those of the plan; the two recipe corrections are documented above.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 04-08 can cite `records/live-overlays.md` and `records/live-page-view.md` for success criteria 2, 3 and 5.
- Still the user's to confirm in verification: the perceptual sharpness against other macOS apps, and the Cocoa delivery of `DevicePixelRatioChange` to `thumbView` on a real display move (needs a second display; the offscreen simulation of plan 04-04 stays the evidence).

## Self-Check: PASSED

- `src/app/retina_check/main.cpp`, `records/live-overlays.md` and the six crops in `records/live/` exist on disk.
- Commits `9f9fdabe`, `46478e13`, `081f8fe0` are ancestors of HEAD; `build/scratch/gate/r12-retina-check-overlays/summary.txt` reads `result: PASS`, `goldens: unchanged`, `diff_lines=0` for both presets.
- `build/scratch/retina/all/summary.txt` contains `platform: cocoa`, `window_dpr: 2`, PASS for page, thumbnails, overlays, picker, mask, badge, handle, stage and decoration, `thumb_file_suffix: q500`, `badge_pixmap_size: 46x34` and `verdict: PASS`; `build/macos-release/retina_check ... --section nosuch` exits 2.

---
*Phase: 04-retina-rendering*
*Completed: 2026-10-25*
