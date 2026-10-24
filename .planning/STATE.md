---
gsd_state_version: "1.0"
milestone: v1.0
current_phase: 3
current_phase_name: Qt 6 and macOS Only
status: planning
stopped_at: Phase 3 context gathered
last_updated: "2026-10-24T18:14:25.513Z"
last_activity: 2026-10-24
last_activity_desc: Phase 02 complete, transitioned to Phase 3
state_head: 7b8c380cec5916fc6234385748cc587435216247
progress:
  total_phases: 6
  completed_phases: 2
  total_plans: 21
  completed_plans: 21
milestone_name: macOS
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-10-24)

**Core value:** The app builds from source and works reliably for daily scanning work on a current Apple Silicon Mac, with a test suite that makes later changes safe.
**Current focus:** Phase 3 — Qt 6 and macOS Only

## Current Position

Phase: 3 — Qt 6 and macOS Only
Plan: Not started
Status: Ready to plan
Last activity: 2026-10-24 — Phase 02 complete, transitioned to Phase 3

Progress: [███░░░░░░░] 33%

## Performance Metrics

**Velocity:**
- Total plans completed: 21
- Average duration: -
- Total execution time: 0.0 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 01 | 7 | - | - |
| 02 | 14 | - | - |

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
| Phase 02 P05 | 12 min | 2 tasks | 12 files |
| Phase 02 P06 | 29min | 3 tasks | 19 files |
| Phase 02 P07 | 15 min | 3 tasks | 15 files |
| Phase 02 P08 | 11 min | 2 tasks | 3 files |
| Phase 02 P09 | 22 min | 3 tasks | 6 files |
| Phase 02 P10 | 27 min | 3 tasks | 17 files |
| Phase 02 P11 | 5 min | 2 tasks | 1 files |
| Phase 02 P12 | 12 min | 2 tasks | 4 files |
| Phase 02 P13 | 4 min | 3 tasks | 5 files |
| Phase 02 P14 | 9 min | 3 tasks | 4 files |

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
- [Phase 02]: 02-05: flat ground-truth tolerances by D-15: deskew 0.15 deg kept (worst 0.125), content IoU 0.96, binarization 0.005 kept with flat_600_bw an XFAIL_CHECK (F-05-1: about 0.1 deg residual skew in the 600 dpi output) — Measured worst cases in records/scenario-flat.md; D-18 mark records current behaviour in the golden
- [Phase 02]: 02-05: orientation sanity of flat outputs is checked on the glyph ink box, not the page, because default hard margins make the 1676 x 1636 px output wider than high — Page-level portrait check failed on all scenarios although content is upright; other sanity conditions unchanged
- [Phase 02]: 02-06: D-15 outcomes for book scenarios: cutter outside gutter 0.5 px, sub-page deskew 0.01 deg, content IoU 0.95 kept, dewarp residual bound 1.5 px at 300 dpi kept (measured 1.04 px)
- [Phase 02]: 02-06: F-06-1 page_split cutter is 30.4 px (300 dpi) and 55.7 px (600 dpi) from the gutter centre at mid-height; XFAIL_CHECK in all four spread scenarios, no requirement
- [Phase 02]: 02-06: F-06-2 TruthMetrics straightnessResidual cannot measure the curved page (no valid OFF strip at 8 strips, 4-9 px noise); dewarp checks use a truth-registered line offset measure local to TestBookSpread.cpp, noise floor 0.58 px
- [Phase 02]: 02-07: despeckle fixture sanity counts only specks inside the content box (15 of 15 at 300 dpi, 11 of 11 at 600 dpi; 24 and 28 specks are cropped by the output stage), departing from the plan's 90%-of-all-specks stop condition
- [Phase 02]: 02-07: F-07-1 page_split AUTO cuts the 300 dpi mixed page and keeps a 386x542 px picture fragment without text; XFAIL_CHECK, coverage scenarios force SINGLE_PAGE_UNCUT, no golden for AUTO
- [Phase 02]: 02-07: D-15 applied to shortfall from the ideal; bounds tightened to speck removal 0.9995, glyph change 0.0005, chroma ratio 0.995, ink/paper contrast 173, picture non-bi-level 0.999, outside bi-level 0.9995
- [Phase 02]: 02-08: SAFE-02 premise not reproduced for file and directory names; five-page special-character and NFC/NFD project round-trips byte-identically, no expected-failure mark
- [Phase 02]: 02-08: F-08-1 DefaultParams layout type reaches the page_split element only after the stage ran; the round-trip test runs page_split for every page
- [Phase 02]: Plan 02-09: image-kind comparator limits frozen at 0.0001 / 0.001 / 0.05 / 1 (measured Release/Debug spread 0, first grid step above it)
- [Phase 02]: Plan 02-09: the comparator self-test (planned alterations) also fails under the D-12 ceilings; the freeze rests on the measurement record, an extra +2 level case would discriminate
- [Phase 02]: 02-10: Baseline commit is a796c2ae (newest commit touching src/pipeline_tests/golden); every Phase 3 strip commit follows it; non-test paths equal cde6825 apart from three add_subdirectory lines
- [Phase 02]: 02-10: all seven findings (F-04-1 to F-08-1) have backlog items 999.1 to 999.7; D7 and the comparator self-test observation are listed as not-defects without backlog items
- [Phase 02]: 02-10: ASan/UBSan run of the full suite passes 7 of 7 with 12 UBSan and 0 ASan reports at 9 project locations (7 new), recorded and not fixed
- [Phase 02]: 02-11: build-tool reads under /opt/homebrew and Xcode/CLT approved for plans 02-11 to 02-14 and Phases 3 to 6 of v1.0 (decided by orchestrator on the user's instruction to decide); exclusions still need their own yes
- [Phase 02]: 02-12: dpi fit rule computed from the scaled geometry (ink box inside page and documented frame), not a divisibility rule; raster guards compare coordinates because a side-0 speck at 75 dpi is a null QRect
- [Phase 02]: 02-13: textRoundTrips checks each text golden entry on its own; GoldenStore::checkText refuses non-round-tripping values in both modes before any read or write
- [Phase 02]: 02-13: SuiteManifest marks a suite handled only after its manifest is on disk, so a failed manifest write fails every later golden of the suite
- [Phase 02]: 02-14: page-split value asserted against the file-format string single-uncut in both written files; spread page cache is a std::map so handed-out references stay valid

### Pending Todos

None yet.

### Blockers/Concerns

- [Phase 2] `02-UAT.md` still lists gap G-02-3 as `diagnosed`; the re-verification (02-VERIFICATION.md, passed 9/9) confirms it closed. `/gsd-verify-work 02` reconciles the UAT entry.
- [Phase 2] Four info findings of the re-review are deferred (IN-01 to IN-04 in `02-REVIEW-DISPOSITION.md`). IN-01 matters if a later phase adds a scan resolution: the generator refuses 400 and 450 dpi for SPREAD, MIXED and CURVED, not only 150 and 225 (`dpiSupported`).
- Phase 3: every strip commit follows golden baseline `a796c2ae` and must pass `ctest --preset macos-release` and `macos-debug` 7/7 with goldens unchanged. Build-tool reads for Phases 3 to 6 are approved in `records/access-approval.md` of Phase 2.
- Phase 4 research flag: a Retina experiment on the Mac, covering the unbundled DPR, `widthMM()` and the smoothing heuristic under Qt 6. Verifying Retina is mostly manual (100%/200% zoom, thumbnails, zone mask, colour picker, toolbar icons, moving between displays).
- Phase 5: check on the Mac whether Backspace works for deleting zones under Qt 6 (low confidence in research).
- Phase 3: remove Qt 5 code with `unifdef`, not by hand. The polarity of the RANSAC/dewarp branches in `DistortionModelBuilder.cpp` and `DetectVertContentBounds.cpp` changes output if it is wrong.

## Session Continuity

Last session: 2026-10-24T18:14:25.469Z
Stopped at: Phase 3 context gathered
Resume file: .planning/phases/03-qt-6-and-macos-only/03-CONTEXT.md
