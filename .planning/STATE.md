---
gsd_state_version: "1.0"
milestone: v1.0
current_phase: 02
current_phase_name: Golden Baselines
status: executing
stopped_at: Completed 02-04-PLAN.md
last_updated: "2026-10-23T19:00:46.499Z"
last_activity: 2026-10-23
last_activity_desc: Phase 02 execution started
state_head: a232c1b5912b985b65078bd9dd47ba550545a2a5
progress:
  total_phases: 6
  completed_phases: 1
  total_plans: 17
  completed_plans: 11
milestone_name: macOS
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-10-18)

**Core value:** The app builds from source and works reliably for daily scanning work on a current Apple Silicon Mac, with a test suite that makes later changes safe.
**Current focus:** Phase 02 — Golden Baselines

## Current Position

Phase: 02 (Golden Baselines) — EXECUTING
Plan: 5 of 10
Status: Ready to execute
Last activity: 2026-10-23 — Phase 02 execution started

Progress: [██░░░░░░░░] 17%

## Performance Metrics

**Velocity:**
- Total plans completed: 7
- Average duration: -
- Total execution time: 0.0 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 01 | 7 | - | - |

**Recent Trend:**
- Last 5 plans: -
- Trend: -

*Updated after each plan completion*
**Per-Plan Metrics:**

| Plan | Duration | Tasks | Files |
|------|----------|-------|-------|
| Phase 01 P01 | 20min | 3 tasks | 8 files |
| Phase 01 P02 | 5min | 3 tasks | 2 files |
| Phase 01 P05 | 3min | 3 tasks | 3 files |
| Phase 01 P03 | 25 min | 3 tasks | 9 files |
| Phase 01 P04 | 5 min | 2 tasks | 4 files |
| Phase 01 P06 | 44 min | 3 tasks | 2 files |
| Phase 01 P07 | 5 min | 2 tasks | 3 files |
| Phase 02 P01 | 10 min | 2 tasks | 12 files |
| Phase 02 P02 | 19 min | 3 tasks | 14 files |
| Phase 02 P03 | 24 min | 3 tasks | 8 files |
| Phase 02 P04 | 11 min | 2 tasks | 10 files |

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table.
Recent decisions affecting current work:

- [Roadmap]: Phase order is fixed. (1) a deletion-free build; (2) goldens from that unmodified code; (3) the Qt 5/Windows/Linux/CI/translations strip; (4) Retina; (5) menus and shortcuts; (6) splits, safety fixes and cleanup. The reference for "unmodified" is commit `5eaac18` (release 1.2.1).
- [Roadmap]: HIDPI-01 (DPR measurement and the `Info.plist` fix if needed) is in Phase 4, not Phase 1, so that Phase 1 stays limited to build fixes. Measuring the DPR is a read-only observation, so it can be done as soon as the Phase 1 binary runs. Phase 4's HiDPI scope is decided by that measurement.
- [Roadmap]: STRIP-07 (OpenGL removal) is in Phase 6 because it needs the colour-picker sampling test that Phase 4 adds (HIDPI-06). Retina work in Phase 4 ignores the OpenGL viewport path, which is off by default.
- [Roadmap]: TEST-10 (project round-trip test, including special characters) is in Phase 2 and SAFE-02 (the fix) is in Phase 6. If the unmodified code fails the special-character case, Phase 2 marks it as an expected failure that names SAFE-02, and Phase 6 removes that mark.
- [Roadmap]: CLEAN-02 (`MainWindow.cpp` split) and CLEAN-05 (shared "Save project as" flow) are in Phase 6, after the Phase 5 menu work.
- [Roadmap]: Every phase from Phase 3 on must pass the full `ctest` suite with baselines unchanged.
- [Phase 01]: -ffp-contract=off frozen in CMakeLists.txt for C and C++ under Clang, every build type (D-03, BUILD-07)
- [Phase 01]: No Boost::headers bridge needed: BoostConfig leaves include_directories at /opt/homebrew/include
- [Phase 01]: No src/ edit was needed for the first Qt 6.12 / Boost 1.92 / Apple Clang build; formatting decision recorded as not needed, clang-format not installed
- [Phase 01]: D-02 answered list-corrected; no ScanTailor settings exist in INI, AppData or Preferences, so first launch starts from defaults
- [Phase 01]: 01-03: qt_tests passes under the default Cocoa platform in Release and Debug, so no QT_QPA_PLATFORM=offscreen entry was added to the test preset
- [Phase 01]: 01-03: imageproc_tests ScaleTestSuite/test_random_image fails in Release and Debug on the unmodified sources; recorded as the BUILD-04 baseline and deferred, not fixed (D-08)
- [Phase 01]: 01-04: ASan+UBSan build needed no CMakeLists.txt change; 0 ASan and 3 UBSan reports (BinaryImage.cpp:1044, SlicedHistogram.cpp:49) recorded, not fixed
- [Phase 01]: 01-06: first-run BUILD-03 smoke verdict was FAIL (dewarping off only, 0 picture zones); superseded by the PASS entry below after the re-check — kept for history
- [Phase 01]: 01-06: project-file check ran on the user-named Desktop file after the user saved there at step 8 — User chose 'Read the Desktop file'; only the allowed fields are recorded
- [Phase 01]: 01-06: HIDPI-01 provisionally needs no embedded Info.plist (probe ratio 2.0, user reports sharp window) — No qt.highdpi line reached the stderr file, so the app's own confirmation is missing
- [Phase 01]: 01-06: BUILD-03 smoke verdict is PASS after the re-check of steps 4, 6, 8; first run FAIL kept in the record — Re-saved project holds dewarping auto (page 02 left) and one picture zone (page 03); the first run's settings never took effect; no app defect found in save and load
- [Phase 01]: 01-06: macOS save dialog cannot take the scratchpad path, so the smoke project stays at the user-named Desktop file — Practical dialog constraint, not a user error; a later walkthrough should name a reachable path
- [Phase 01]: Phase 1 verdict PASS: all five success criteria PASS on cde6825; imageproc_tests ScaleTestSuite/test_random_image remains the recorded baseline failure
- [Phase 01]: Phase 2 baseline commit is cde6825 (src, CMakeLists.txt, CMakePresets.json equal 11bc573)
- [Phase 02]: Deskew angle stored by the unmodified stage is the negative of the generator skewDeg (1.5 gives -1.5); GroundTruth textBlock is the tight ink box; runPipeline runs up to page_split first to learn the sub-page count
- [Phase 02]: 02-02: Comparator reports the size metric always (value 0 when sizes match); on a mismatch it is the only metric
- [Phase 02]: 02-02: Update mode refreshes a suite MANIFEST.txt on the first check that passes or passes sanity; blocked checks write nothing; an unchanged manifest is not rewritten
- [Phase 02]: 02-02: A golden replaced in update mode reports no metrics; expected/actual/diff stay in the artifact folder
- [Phase 02]: 02-02: Non-numeric value under a _deg/_px/_mm key fails; numeric limit gets 1e-9 slack (kTextToleranceSlack)
- [Phase 02]: 02-02: GoldenStore clearArtifacts re-checks that the path is below the artifact root (a mutation run with validation off deleted build/macos-release)
- [Phase 02]: 02-03: GroundTruth text boxes are tight ink boxes for every kind; the nominal block frames only place the text
- [Phase 02]: 02-03: Curl profile is evaluated at the image x coordinate, so mapImageToFlat is closed form (the renderer map) and mapFlatToImage is a fixed-point iteration; mapImageToFlat added to the public interface
- [Phase 02]: 02-03: straightnessResidual returns valid=false, linesFound 0 and an infinite residual with fewer than two usable strips; a symmetric arc of sagitta h leaves 0.4375 h at 8 strips
- [Phase 02]: 02-03: HeadlessPipeline needed no change for spread, AUTO dewarp and MIXED output; it is frozen and scenarios use PipelineOptions::configure
- [Phase 02]: D-17: scaleToGray equals the exact area average, Qt 6.12 smooth scaling moved; TestScale compares against an area-average reference
- [Phase 02]: Binarization tests: every binarizer matches the ink mask exactly, bounds at the D-15 minimum 0.05% of ink area
- [Phase 02]: Global thresholds reject Format_Grayscale8 (F-04-1, XFAIL); model top curve skips the first line of the curved page, seed 1 (F-04-2, XFAIL)

### Pending Todos

None yet.

### Blockers/Concerns

- Phase 1 decisions to make before any golden is captured: the `-ffp-contract` value (research suggests `off`), how `PORTABLE_VERSION` handles the settings location, and the CMake minimum (research says 3.25 for Qt 6.12; confirm at first configure).
- Phase 1 may need a launch fix as well as compile fixes, for example if a `Q_OS_UNIX` branch picks a Linux platform default on macOS. Log each such edit on its own; do not delete anything.
- Phase 2 research flag: a short spike to set the comparator tolerances by measuring the spread between two builds (with and without `-ffp-contract=off`). It also has to check `ApplicationSettings` isolation and `ThumbnailPixmapCache` under a test app.
- Phase 4 research flag: a Retina experiment on the Mac, covering the unbundled DPR, `widthMM()` and the smoothing heuristic under Qt 6. Verifying Retina is mostly manual (100%/200% zoom, thumbnails, zone mask, colour picker, toolbar icons, moving between displays).
- Phase 5: check on the Mac whether Backspace works for deleting zones under Qt 6 (low confidence in research).
- Phase 3: remove Qt 5 code with `unifdef`, not by hand. The polarity of the RANSAC/dewarp branches in `DistortionModelBuilder.cpp` and `DetectVertContentBounds.cpp` changes output if it is wrong.

## Session Continuity

Last session: 2026-10-23T19:00:46.469Z
Stopped at: Completed 02-04-PLAN.md
Resume file: None
