---
gsd_state_version: "1.0"
milestone: v1.0
current_phase: 01
current_phase_name: Deletion-Free macOS Build
status: executing
stopped_at: Completed 01-06-PLAN.md (smoke verdict FAIL, awaiting decision)
last_updated: "2026-10-23T13:04:42.020Z"
last_activity: 2026-10-23
last_activity_desc: Phase 01 execution started
state_head: 7de8359bd20135f9e281770e96e01babbf6f0c5e
progress:
  total_phases: 6
  completed_phases: 0
  total_plans: 7
  completed_plans: 6
milestone_name: macOS
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-10-18)

**Core value:** The app builds from source and works reliably for daily scanning work on a current Apple Silicon Mac, with a test suite that makes later changes safe.
**Current focus:** Phase 01 — Deletion-Free macOS Build

## Current Position

Phase: 01 (Deletion-Free macOS Build) — EXECUTING
Plan: 6 of 7 summarised (01-01 to 01-05 complete; 01-06 halted: BUILD-03 smoke verdict FAIL); 01-07 waits for the decision on 01-06
Status: Blocked on the user's decision about 01-06 (see records/SMOKE-FINDINGS.md `## What is open`)
Last activity: 2026-10-23 — Phase 01 execution started

Progress: [░░░░░░░░░░] 0%

## Performance Metrics

**Velocity:**
- Total plans completed: 0
- Average duration: -
- Total execution time: 0.0 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| - | - | - | - |

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
- [Phase 01]: 01-06: BUILD-03 smoke verdict is FAIL: saved project has dewarping mode off only and 0 picture zones; cause open (walkthrough gap, wrong file, or save bug) — Plan sets PASS only when the project-file check meets every expectation; not softened
- [Phase 01]: 01-06: project-file check ran on the user-named Desktop file after the user saved there at step 8 — User chose 'Read the Desktop file'; only the allowed fields are recorded
- [Phase 01]: 01-06: HIDPI-01 provisionally needs no embedded Info.plist (probe ratio 2.0, user reports sharp window) — No qt.highdpi line reached the stderr file, so the app's own confirmation is missing

### Pending Todos

None yet.

### Blockers/Concerns

- Phase 1 decisions to make before any golden is captured: the `-ffp-contract` value (research suggests `off`), how `PORTABLE_VERSION` handles the settings location, and the CMake minimum (research says 3.25 for Qt 6.12; confirm at first configure).
- Phase 1 may need a launch fix as well as compile fixes, for example if a `Q_OS_UNIX` branch picks a Linux platform default on macOS. Log each such edit on its own; do not delete anything.
- Phase 2 research flag: a short spike to set the comparator tolerances by measuring the spread between two builds (with and without `-ffp-contract=off`). It also has to check `ApplicationSettings` isolation and `ThumbnailPixmapCache` under a test app.
- Phase 4 research flag: a Retina experiment on the Mac, covering the unbundled DPR, `widthMM()` and the smoothing heuristic under Qt 6. Verifying Retina is mostly manual (100%/200% zoom, thumbnails, zone mask, colour picker, toolbar icons, moving between displays).
- Phase 5: check on the Mac whether Backspace works for deleting zones under Qt 6 (low confidence in research).
- Phase 3: remove Qt 5 code with `unifdef`, not by hand. The polarity of the RANSAC/dewarp branches in `DistortionModelBuilder.cpp` and `DetectVertContentBounds.cpp` changes output if it is wrong.
- BUILD-03: saved project lacks dewarping auto and a picture zone; repeat steps 4, 6, 8 on the scratchpad path or decide (see SMOKE-FINDINGS.md)

## Session Continuity

Last session: 2026-10-23T13:04:42.001Z
Stopped at: Completed 01-06-PLAN.md (smoke verdict FAIL, awaiting decision)
Resume file: None
