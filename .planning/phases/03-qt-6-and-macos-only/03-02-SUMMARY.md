---
phase: 03-qt-6-and-macos-only
plan: 02
subsystem: build
tags: [qt6, unifdef, cmake, strip, golden-manifest]

requires:
  - phase: 03-qt-6-and-macos-only
    provides: gate.sh with build-command diff, command reference for the parent tree (plan 03-01)
provides:
  - All Qt 5 conditionals removed from C++ (19 files) by unifdef with Qt 6 polarity
  - CMake that finds Qt with the single line find_package(Qt6 6.5 ... CONFIG REQUIRED) and links Qt6:: targets
  - Golden manifest qt_compile fed by ST_QT_COMPILE_VERSION from CMake (D-15)
  - Empty criterion-1 search (Qt5 and QT_VERSION) over CMake files and src/
affects: [03-03, 03-04, 03-05, 03-06, 03-07, 03-08, 03-09]

actuals:
  tokens: 8800
  tasks: 3
  commits: 6
plan_head_before: 0e8d712386b3d6753c1519f94ed5f0abde31ef9b
plan_head_after: 73ae39879dc1305a00875d933c884d66605dddc0

tech-stack:
  added: []
  patterns:
    - "Resolve preprocessor gates with unifdef -m -DQT_VERSION_MAJOR=6 and check numstat for zero additions"
    - "Commit first, gate the committed tree, record the row in gate-log.md in its own docs commit"

key-files:
  created: []
  modified:
    - src/app/RelinkablePathVisualization.cpp
    - src/core/Application.cpp
    - src/core/BackgroundTask.h
    - src/core/RelinkablePath.cpp
    - src/core/filters/output/DewarpingView.cpp
    - src/core/filters/page_split/ImageView.cpp
    - src/core/filters/page_split/PageLayout.cpp
    - src/core/filters/page_split/PageLayoutAdapter.cpp
    - src/core/interaction/ZoomHandler.cpp
    - src/dewarping/Curve.cpp
    - src/dewarping/CylindricalSurfaceDewarper.cpp
    - src/dewarping/DetectVertContentBounds.cpp
    - src/dewarping/DistortionModelBuilder.cpp
    - src/dewarping/TextLineTracer.cpp
    - src/foundation/Proximity.cpp
    - src/math/PolylineIntersector.cpp
    - src/test_support/Manifest.cpp
    - src/test_support/CMakeLists.txt
    - CMakeLists.txt
    - src/core/CMakeLists.txt
    - src/app/CMakeLists.txt
    - src/foundation/CMakeLists.txt
    - src/imageproc/tests/CMakeLists.txt
    - src/dewarping/tests/CMakeLists.txt
    - src/pipeline_tests/CMakeLists.txt
    - src/qt_tests/CMakeLists.txt
    - src/app/main.cpp
    - src/core/ImageViewBase.cpp
    - src/core/ImageViewBase.h
    - .planning/phases/03-qt-6-and-macos-only/records/gate-log.md

key-decisions:
  - "Gate lines of the dewarping files resolved by unifdef alone; QRandomGenerator prng(0) is kept, so the RANSAC seed source is unchanged"
  - "qt_compile in the golden manifest comes from ST_QT_COMPILE_VERSION=${Qt6_VERSION} (6.12.0), no MANIFEST.txt changed"
  - "Qt floor is find_package(Qt6 6.5 ...) with Qt6:: targets; QT_BINDIR kept for the Windows block that c05 removes"

requirements-completed: [STRIP-01]

coverage:
  - id: D1
    description: "Qt 5 conditionals in 16 C++ files resolved by unifdef with 0 additions and 135 deletions; polarity of DistortionModelBuilder.cpp and DetectVertContentBounds.cpp checked line by line"
    requirement: STRIP-01
    verification:
      - kind: other
        ref: "build/scratch/c01-numstat-unifdef.txt (16 lines, 0 / 135); gate.sh check c01-qt5-cpp (result: PASS)"
        status: pass
    human_judgment: false
  - id: D2
    description: "qt_compile of the golden manifest is fed from CMake; no TOOLCHAIN DIFFERS line, no golden changed"
    requirement: STRIP-01
    verification:
      - kind: other
        ref: "build/macos-release/pipeline_tests --run_test=FlatDocumentTestSuite (rc 0, 0 TOOLCHAIN DIFFERS lines); git status of src/pipeline_tests/golden empty"
        status: pass
    human_judgment: false
  - id: D3
    description: "CMake requires Qt 6 only with one find_package line; build commands of the nine targets identical in Release and Debug"
    requirement: STRIP-01
    verification:
      - kind: other
        ref: "gate.sh check c02-qt6-cmake (cmds-*.diff empty, result: PASS)"
        status: pass
    human_judgment: false
  - id: D4
    description: "main.cpp Qt 5 block and the QGLWidget hack of ImageViewBase removed; criterion-1 search empty"
    requirement: STRIP-01
    verification:
      - kind: other
        ref: "grep -rnI -e Qt5 -e QT_VERSION CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in src (no output); gate.sh check c03-qt5-main-imageview (result: PASS)"
        status: pass
    human_judgment: false

duration: 20min
completed: 2026-10-24
status: complete
---

# Phase 3 Plan 02: Qt 5 removal from C++ and CMake Summary

**Qt 5 is gone from the code and the build: 19 C++ files resolved with `unifdef -DQT_VERSION_MAJOR=6`, CMake finds only `Qt6 6.5` and links `Qt6::` targets, and the manifest's `qt_compile` comes from a CMake-fed define, with three gated commits and unchanged goldens**

## Performance

- **Duration:** about 20 min (three gates of about 4 min each)
- **Completed:** 2026-10-24T20:13:00Z
- **Tasks:** 3
- **Files modified:** 30 (29 source and CMake files, the gate log)

## Accomplishments

- c01: unifdef over 16 files removed 135 lines and added none. The kept code in both dewarping files is `#include <QRandomGenerator>` and `QRandomGenerator prng(0);`; no `qsrand` or `qrand` remains. Only `BackgroundTask.h` was reflowed by clang-format (two one-line bodies); `TextLineTracer.cpp` was not reformatted and still shows its two original warnings at lines 317 and 328.
- D-15: `Manifest.cpp` reads `ST_QT_COMPILE_VERSION` (fallback `"unknown"`), `src/test_support/CMakeLists.txt` defines it as `"${Qt6_VERSION}"`. The gate's command diff shows only that define on the seven `test_support` compile lines per preset (value 6.12.0). `FlatDocumentTestSuite` exits 0 with no `TOOLCHAIN DIFFERS` line, as in the before state.
- c02: `CMakeLists.txt` has the single line `find_package(Qt6 6.5 COMPONENTS ... CONFIG REQUIRED)`; `qt_min_version`, the Qt 5 fallback, the `Qt_*_lib` variables, every `Qt6_FOUND` test and the Qt 5 plugin-install block are gone. Six CMake files now name `Qt6::<Component>`. The build commands of all nine targets are identical in both presets (empty diff).
- c03: unifdef removed 14, 13 and 4 lines from `main.cpp`, `ImageViewBase.cpp` and `ImageViewBase.h` (0 additions). The `#ifdef _WIN32` block of `main.cpp` is untouched for plan 03-04. The criterion-1 search now prints nothing.
- Each commit passed the gate: `100% tests passed out of 7` in Release and Debug, goldens unchanged against a796c2ae. Rows are in `records/gate-log.md`.

## Task Commits

1. **Task 1 (c01): resolve Qt 5 conditionals in C++** - `9a68f985` (refactor), gate row `a6cdcbbd` (docs)
2. **Task 2 (c02): Qt 6 only in CMake** - `c63a74c2` (build), gate row `efd05867` (docs)
3. **Task 3 (c03): main.cpp and ImageViewBase** - `81a27cb1` (refactor), gate row `73ae3987` (docs)

**Plan metadata:** added by the final docs commit of this plan.

## Decisions Made

- Hand edits were limited to the D-15 manifest change and the CMake files; every C++ conditional was resolved by unifdef output alone (D-11).
- `QT_BINDIR` stays as `set(QT_BINDIR "${QT6_INSTALL_PREFIX}/bin")`; its only reader is the Windows deployment block removed in c05.
- Unused includes left over by the removals were not touched (Phase 6 cleanup).

## Deviations from Plan

None - plan executed exactly as written.

Two environment notes, neither a deviation: (1) the plan's automated check `grep -q 'ST_QT_COMPILE_VERSION="${Qt6_VERSION}"' src/test_support/CMakeLists.txt` returns no match under the ugrep `grep` of this machine because of the `${` in the pattern; the same check with `grep -F` matches line 43, so the file content is as planned. (2) The acceptance wording "`src/core/CMakeLists.txt` contains no `if(`" matches the `endif()` of the unrelated `BUILD_TESTS` block; no Qt conditional is left in that file.

## Issues Encountered

None.

## Known Stubs

None.

## Threat Flags

None. T-03-03 (wrong gate polarity) is mitigated as planned: zero additions by numstat, polarity table checked against the diff, dewarp goldens unchanged in both presets. T-03-04 (CMake edits): command diff empty for c02 and c03 and limited to the D-15 define for c01. T-03-05: no TOOLCHAIN DIFFERS line for `qt_compile`, golden directory clean.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 03-03 can start; the gate reference for the current tree is written by the c03 gate (`build/scratch/gate/ref/`).
- Remaining Windows-only CMake blocks (`dep_libs` install loop, `windeployqt` in `src/app/CMakeLists.txt`, `if (WIN32)` install rules) are left for the Windows removal plans.

## Self-Check: PASSED

- Commits `9a68f985`, `a6cdcbbd`, `c63a74c2`, `efd05867`, `81a27cb1`, `73ae3987` are ancestors of HEAD (`git rev-list --count 0e8d7123..HEAD` = 6).
- Gate summaries of c01, c02, c03 end with `result: PASS`; all automated verify commands of the three tasks pass (the one `grep -q` with `${` re-run with `-F`).
- Criterion-1 search empty; `src/pipeline_tests/golden` has no change against a796c2ae.

---
*Phase: 03-qt-6-and-macos-only*
*Completed: 2026-10-24*
