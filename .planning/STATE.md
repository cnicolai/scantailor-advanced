---
gsd_state_version: "1.0"
milestone: v1.0
current_phase: 05
current_phase_name: Mac Menus and Shortcuts
status: executing
stopped_at: Completed 05-02-PLAN.md
last_updated: "2026-11-14T23:09:01.544Z"
last_activity: 2026-11-14
last_activity_desc: Phase 05 execution started
state_head: 28c6b08d14b31c48550b2ead8c966ac2377a8fb7
progress:
  total_phases: 6
  completed_phases: 4
  total_plans: 49
  completed_plans: 40
milestone_name: macOS
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-10-25)

**Core value:** The app builds from source and works reliably for daily scanning work on a current Apple Silicon Mac, with a test suite that makes later changes safe.
**Current focus:** Phase 05 — Mac Menus and Shortcuts

## Current Position

Phase: 05 (Mac Menus and Shortcuts) — EXECUTING
Plan: 3 of 11
Status: Ready to execute
Last activity: 2026-11-14 — Phase 05 execution started

Progress: [███████░░░] 67%

## Performance Metrics

**Velocity:**
- Total plans completed: 38
- Average duration: -
- Total execution time: 0.0 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 01 | 7 | - | - |
| 02 | 14 | - | - |
| 03 | 9 | - | - |
| 04 | 8 | - | - |

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
| Phase 03 P01 | 50 min | 2 tasks | 9 files |
| Phase 03 P02 | 20 min | 3 tasks | 30 files |
| Phase 03 P03 | 13 min | 2 tasks | 14 files |
| Phase 03 P04 | 25 min | 2 tasks | 20 files |
| Phase 03 P05 | 13min | 2 tasks | 15 files |
| Phase 03 P06 | 7 min | 2 tasks | 34 files |
| Phase 03 P07 | 11 min | 2 tasks | 4 files |
| Phase 03 P08 | 14 min | 2 tasks | 3 files |
| Phase 03 P09 | 7 min | 2 tasks | 9 files |
| Phase 04 P01 | 19 min | 2 tasks | 10 files |
| Phase 04 P02 | 11 min | 2 tasks | 11 files |
| Phase 04 P03 | 17 min | 2 tasks | 10 files |
| Phase 04 P04-04 | 17 min | 2 tasks | 9 files |
| Phase 04 P05 | 23 min | 2 tasks | 8 files |
| Phase 04 P06 | 19 min | 2 tasks | 10 files |
| Phase 04 P07 | 50min | 2 tasks | 10 files |
| Phase 04 P08 | 10 min | 2 tasks | 4 files |
| Phase 05 P01 | 30 min | 2 tasks | 8 files |
| Phase 05 P02 | 35 min | 2 tasks | 10 files |

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
- [Phase 03]: 03-01: ui_snapshot is a separate executable under BUILD_TESTS, not in ctest (suite count stays 7)
- [Phase 03]: 03-01: D-15 define is ST_QT_COMPILE_VERSION; README profiles path is the measured ~/Library/Application Support/scantailor-advanced/scantailor-advanced/profiles
- [Phase 03]: 03-02: Qt 5 removed from C++ by unifdef -DQT_VERSION_MAJOR=6 (dewarping keeps QRandomGenerator prng(0)); CMake finds only Qt6 6.5 with Qt6:: targets; qt_compile fed by ST_QT_COMPILE_VERSION from Qt6_VERSION; criterion-1 search empty
- [Phase 03]: Lint workflow was the only .clang-format enforcement; formatting becomes a documented manual step (TESTING.md, plan 03-07)
- [Phase 03]: 03-04: RelinkablePath::normalize keeps its unlabelled backslash/drive-letter logic (D-03); tracked as backlog 999.8
- [Phase 03]: 03-04: Boost.Test define -DBOOST_TEST_DYN_LINK is unconditional after Windows removal
- [Phase 03]: 03-05: Clang flag block comment (refers to the removed GNU warning set) kept byte-for-byte equal to a796c2ae
- [Phase 03]: 03-05: CMAKE_POSITION_INDEPENDENT_CODE and Threads lines unwrapped, not deleted, because the Mac compile and link commands use them
- [Phase 03]: 03-06: translations removed in one commit (c10); a language key in user settings is left alone (D-07); D-12 after check Verdict PASS
- [Phase 03]: 03-07: README Features unchanged except two portable-mode passages; TESTING.md documents artefacts as test-artifacts/<suite>/<name>
- [Phase 03]: 03-08: ASan+UBSan after the strip equal Phase 2 (0 ASan, 12 UBSan, same 9 locations, 0 new); executables run with their ctest environment so settings stay in build/
- [Phase 03]: 03-09: the D-10 search keeps one generator-owned line in CLAUDE.md (skills fallback text naming .github/skills/); CLAUDE.md is edited only through generate-claude-md
- [Phase 04]: Offscreen test windows are placed by setScreen() plus move() into the screen rectangle; setScreen() alone reverts on resize
- [Phase 04]: Retina tests wait for viewport()->devicePixelRatioF() before the HQ pixmap; the viewport child learns the ratio after its parent
- [Phase 04]: Plan 04-02: no Info.plist (ratio 2 measured on Studio Display) and no smoothing recalibration (ratio 0.9978)
- [Phase 04]: 04-03: Quality spin box range stays 100-1500; thumbnail cache clamps the effective side to 3000 so stored values are never rewritten
- [Phase 04]: 04-03: makeThumbnail left unchanged; ratio 1 thumbnails keep their size (a 1200x900 page gives a 249 px wide q250 file)
- [Phase 04]: Loaded thumbnail pixmap stays at ratio 1; only the ThumbnailBase temp buffer carries the painter's ratio, keyed ThumbnailBase::tempPixmap@<ratio>
- [Phase 04]: ThumbnailDprTracker holds the cache as weak_ptr and relies on setDevicePixelRatio being a no-op for an equal ratio
- [Phase 04]: 04-05: qt_tests links dewarping so the zone-mask tests can build a real output::Settings; gate r08 command diff is therefore non-empty (include flag and libdewarping only)
- [Phase 04]: 04-05: BubbleAnimation::nextFrame gets an explicit logical rect; on Qt 6.12 the default rect of a ratio-2 pixmap is its device size (Pitfall 2 confirmed by a counterfactual)
- [Phase 04]: 04-06: ThumbnailSequence.cpp unchanged; page badges proven at device resolution by testPageBadgeAtDeviceResolution on the real styled icon engine
- [Phase 04]: 04-06: colour picker median moved into ColorPickupInteraction::medianColor, circle counted in logical pixels; identical at ratio 1, OpenGL branch untouched
- [Phase 04]: retina_check links dewarping; stage-list centring is measured on ink united over 12 grabs and list-decoration size on the first ink column run (plan recipes measured a comet fragment and the item text); no threshold changed
- [Phase 04]: Phase 4 closing: sanitizer report at BinaryImage.cpp:1044 (known Phase 1 location) reached through the Phase 4 test helper makeZoneMask is recorded as old code, not fixed
- [Phase 04]: HIDPI-01 recorded as measured with retina_check, which shares the app's Mach-O properties; the app binary itself was not launched (home-folder rule)
- [Phase 05]: 05-01: explicit key sequences Ctrl+, and Ctrl+Shift+S instead of StandardKeys (empty offscreen); Settings sits between Close Project and Quit with no separator; GUI sources are the static library app_gui (WR-01 closed)
- [Phase 05]: 05-02: About moved from the empty Help menu into File and Help is gone; the application menu names ScanTailor Advanced through an embedded Info.plist section (CFBundleName only, settings and profiles paths measured identical)
- [Phase 05]: 05-02: German layout measured live: Option-Command digits and plus/minus fire once each, so plan 05-03 binds the Output tabs and thumbnail keys with Option-Command; native Settings item reads "Preferences..." not "Settings…" (open point, unchanged)

### Pending Todos

None yet.

### Blockers/Concerns

- [Phase 2] `02-UAT.md` still lists gap G-02-3 as `diagnosed`; the re-verification (02-VERIFICATION.md, passed 9/9) confirms it closed. `/gsd-verify-work 02` reconciles the UAT entry.
- [Phase 2] Four info findings of the re-review are deferred (IN-01 to IN-04 in `02-REVIEW-DISPOSITION.md`). IN-01 matters if a later phase adds a scan resolution: the generator refuses 400 and 450 dpi for SPREAD, MIXED and CURVED, not only 150 and 225 (`dpiSupported`).
- [Phase 3] Code review left WR-01 open: `ui_snapshot` recompiles all GUI sources and is built by default under `BUILD_TESTS`, so every preset build compiles the GUI twice. Fix with an OBJECT library plus `EXCLUDE_FROM_ALL`, or drop the tool, in a later phase. Seven info findings are open as well (`03-REVIEW-DISPOSITION.md`); WR-02 is deferred.
- [Phase 3] Existing build directories still hold stale untracked `.qm`, `.desktop` and CPack files from before the strip; nothing produces them any more. `cmake --fresh --preset <name>` or deleting `build/<preset>` clears them.
- Build-tool reads for Phases 4 to 6 are approved in `records/access-approval.md` of Phase 2. `.planning/phases/03-qt-6-and-macos-only/gate.sh` gives later phases the same per-commit gate (Release and Debug ctest, goldens, build-command diff).
- [Phase 4] Code review left WR-01 open and worth fixing soon: `ThumbnailPixmapCache::removeLoadedItemsLocked()` sets `m_endOfLoadedItems` to `end()` instead of `begin()`; display moves now reach this old bug (possible dangling iterator). One-line fix plus a test, e.g. `/gsd-code-review 4 --fix`. WR-02 (stale other-ratio thumbnail after reprocessing) and WR-03 (benign) are open too (`04-REVIEW-DISPOSITION.md`).
- [Phase 4] A real drag between a 1x and a 2x display is unverified (one display attached; criterion 4 rests on the offscreen two-screen simulation). Thumbnails depend on the `DevicePixelRatioChange` event alone. Re-check procedure: `phases/04-retina-rendering/records/closing-check.md`.
- Phase 5: check on the Mac whether Backspace works for deleting zones under Qt 6 (low confidence in research).

## Session Continuity

Last session: 2026-11-14T23:09:01.182Z
Stopped at: Completed 05-02-PLAN.md
Resume file: None
