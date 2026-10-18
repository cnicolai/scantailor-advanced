---
gsd_state_version: "1.0"
milestone: v1.0
current_phase: 1
current_phase_name: Deletion-Free macOS Build
status: planning
stopped_at: Phase 1 context gathered
last_updated: "2026-10-18T17:27:33.805Z"
last_activity: 2026-10-18
last_activity_desc: Roadmap created (6 phases, 51/51 v1 requirements mapped)
state_head: 94844e68b6750acbcbeacf2808a573b1415c54c8
progress:
  total_phases: 6
  completed_phases: 0
  total_plans: 0
  completed_plans: 0
milestone_name: macOS
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-10-18)

**Core value:** The app builds from source and works reliably for daily scanning work on a current Apple Silicon Mac, with a test suite that makes later changes safe.
**Current focus:** Phase 1: Deletion-Free macOS Build

## Current Position

Phase: 1 of 6 (Deletion-Free macOS Build)
Plan: 0 of TBD in current phase
Status: Ready to plan
Last activity: 2026-10-18 -- Roadmap created (6 phases, 51/51 v1 requirements mapped)

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

Last session: 2026-10-18T17:27:33.792Z
Stopped at: Phase 1 context gathered
Resume file: .planning/phases/01-deletion-free-macos-build/01-CONTEXT.md
