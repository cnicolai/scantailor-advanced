---
phase: 06-hardening-and-cleanup
plan: 11
subsystem: opengl-module-removal
tags: [cpp, cmake, strip-07, qt-test, link-proof, gate]

requires:
  - phase: 06-hardening-and-cleanup
    provides: "plan 06-10 removal of every OpenGL use in the app code; plan 06-01 conventions (gate, labels, output-path guard)"
provides:
  - "src/core/OpenGLSupport.{h,cpp} deleted; Qt6::OpenGL and Qt6::OpenGLWidgets gone from find_package and the core link line"
  - "TestMacQt::testNoOpenGlInSourcesOrBuild: source and CMake scan with positive controls, at least 500 files read"
  - "records/strip-07.md: removal inventory, picker evidence, otool link proof, command-diff classification"
affects: [06-12]

actuals:
  tokens: 3000
  tasks: 1
  commits: 3

plan_head_before: 62268fcb8f492be98c0daab4c6226dca62e6379f
plan_head_after: bcb1ea2920ad35a0c8d7c547919827419688f52b

tech-stack:
  added: []
  patterns:
    - "Build-removal slice proven at three ends: a source and CMake scan test, otool -L on every executable, and a classified command diff"
    - "New scan test proven to discriminate with three uncommitted mutations (source, core CMake, top CMake), restored from byte copies"

key-files:
  created:
    - .planning/phases/06-hardening-and-cleanup/records/strip-07.md
  modified:
    - CMakeLists.txt
    - src/core/CMakeLists.txt
    - src/qt_tests/tst_mac_qt.h
    - src/qt_tests/tst_mac_qt.cpp
    - .planning/phases/06-hardening-and-cleanup/records/gate-log.md
  deleted:
    - src/core/OpenGLSupport.h
    - src/core/OpenGLSupport.cpp

key-decisions:
  - "The system OpenGL.framework stays: QtGui links it itself, so the link checks look for QtOpenGL only (the requirement covers the Qt modules, not the system framework)"
  - "A stored settings/enable_opengl value is neither read, deleted nor rewritten; no code mentions the key (D-00c)"
  - "The command diff is classified at token level, not with the plan's sed: the plan's normalisation missed the QT_OPENGL_LIB defines and CMake's reordering of Qt frameworks on five link lines"

requirements-completed: [STRIP-07]

coverage:
  - id: D1
    description: "No C++ source in the scan scope names QOpenGL, QtOpenGL, OpenGLSupport, isOpenGlEnabled or setOpenGlEnabled; the CMake files name no OpenGL; SettingsDialog.ui has no opengl"
    requirement: STRIP-07
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#testNoOpenGlInSourcesOrBuild"
        status: pass
    human_judgment: false
  - id: D2
    description: "No Qt OpenGL framework is linked by scantailor-advanced, qt_tests, mac_check, retina_check, ui_snapshot, pipeline_tests, core_tests (Release) or the Debug app"
    requirement: STRIP-07
    verification:
      - kind: other
        ref: "otool -L listings build/scratch/otool-s14-*.txt: QtOpenGL 0 lines, QtGui 1 line each; grep -c QtOpenGL build.ninja prints 0 in both presets"
        status: pass
    human_judgment: false
  - id: D3
    description: "The colour picker is unchanged without the OpenGL modules"
    requirement: STRIP-07
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#testColorPickerGrabAtRatio2 and testMedianColorRatioInvariant (PASS in qt-s13-before, qt-s13 and qt-s14; file unedited since e5319cd9)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The commit passes the gate and its command diff holds only the removed OpenGL items"
    requirement: STRIP-07
    verification:
      - kind: other
        ref: "gate.sh check s14-opengl-modules (tested a024751a) -> result: PASS, goldens unchanged, diff 639 / 639 classified in records/strip-07.md"
        status: pass
    human_judgment: false

duration: about 40min
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 11: STRIP-07, stop linking the Qt OpenGL modules Summary

**`OpenGLSupport.{h,cpp}` and the Qt OpenGL and OpenGLWidgets modules are out of `find_package`, the core link line and every binary, proven by a new source and CMake scan test with positive controls, `otool -L` on eight binaries (QtOpenGL 2 lines to 0 each), and a command diff that holds only the removed OpenGL items.**

## Performance

- **Duration:** about 40 min of clock time (two builds per preset, one gate run in the background)
- **Completed:** 2026-11-17
- **Tasks:** 1 (tracer)
- **Files:** 2 source files deleted, 2 CMake files and 2 test files edited, 1 record created, 1 record appended

## Accomplishments

- `src/core/OpenGLSupport.h` and `.cpp` removed with `git rm`; their line left `common_sources`; `Qt6::OpenGL` and `Qt6::OpenGLWidgets` left the core link line; `OpenGL` and `OpenGLWidgets` left the `find_package(Qt6 ...)` components. `grep -c OpenGL` prints 0 for both CMake files (1 and 2 before). Both presets reconfigured and rebuilt without `--fresh`.
- `TestMacQt::testNoOpenGlInSourcesOrBuild`: forbidden words `QOpenGL`, `QtOpenGL`, `OpenGLSupport`, `isOpenGlEnabled`, `setOpenGlEnabled`; controls (a synthetic `#include <QtOpenGLWidgets/QOpenGLWidget>` line is found, a `QWidget` line is not); every `.cpp`, `.h` and `.mm` file of the scan scope read, at least 500 files, no hits; both CMake files non-empty and free of `OpenGL`; `SettingsDialog.ui` free of `opengl` (case-insensitive).
- Mutation runs for the new test (uncommitted, restored from byte copies, `cmp` identical): `// QOpenGLWidget` appended to `src/core/PageId.h` fails with `src/core/PageId.h: QOpenGL`; `# OpenGL` appended to `src/core/CMakeLists.txt` and `# OpenGLWidgets` to `CMakeLists.txt` fail with `... names OpenGL`; the clean tree passes.
- Link proof: for `scantailor-advanced`, `qt_tests`, `mac_check`, `retina_check`, `ui_snapshot`, `pipeline_tests` and `core_tests` (Release) and the Debug app, `otool -L` lists `QtGui` (control) and no `QtOpenGL`; `grep -c QtOpenGL build.ninja` went from 334 to 0 in both presets. The system `OpenGL.framework` stays because `QtGui` links it.
- `qt-s14.txt` (`qt_tests` run with the ctest environment): every `Totals:` line says `0 failed, 0 skipped`; it holds `PASS   : TestMacQt::testNoOpenGlInSourcesOrBuild()` and both colour-picker PASS lines; `tst_hidpi_qt.cpp` is unedited since `e5319cd9`.
- Gate `s14-opengl-modules` (tested `a024751a`): Release and Debug `100% tests passed out of 7`, goldens unchanged, command diff 639 / 639, classified in the gate-log row and in `records/strip-07.md`. Output-path guard prints nothing; `tst_mac_qt.{cpp,h}` pass `clang-format --dry-run -Werror`.
- `records/strip-07.md` covers both plans: removal inventory, picker evidence before and after, tests, link proof, gate rows `s13` and `s14`, and the note that `.planning/codebase/STACK.md`, `INTEGRATIONS.md` and the generated stack text of `.claude/CLAUDE.md` still mention the OpenGL modules (update through `generate-claude-md` at the Phase 6 transition).

## Task Commits

1. **Task 1 (tracer): drop OpenGLSupport and the Qt OpenGL modules from the build** - `a024751a` (build), gated as `s14-opengl-modules`; gate record `9bfb1cd1`; STRIP-07 record `bcb1ea29`

The tracer verify (otool listings, `build.ninja`, `qt_tests` lines, gate summary, source and CMake greps, record checks) passed end to end, with the command-diff check done at token level (see Deviations).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The plan's normalised command-diff comparison cannot pass as written**
- **Found during:** Task 1, step 4 (gate classification)
- **Issue:** the `sed` normalisation in the plan's second verify command strips `QtOpenGL` include and link items and `-framework OpenGL`, then expects `cmp` equal. Two real effects it does not cover: (a) the `-DQT_OPENGL_LIB` and `-DQT_OPENGLWIDGETS_LIB` compile definitions leave with the modules, and the archive line of `libcore.a` loses its `OpenGLSupport.cpp.o` member (the `grep -v` drops that line only from the reference); (b) after the two modules are gone CMake orders the Qt frameworks differently on five link lines (`scantailor-advanced`, `ui_snapshot`, `core_tests`, `pipeline_tests`, `qt_tests`), the same items in another order. The first is a missing pattern, the second is not removable by any substitution.
- **Fix:** classified the diff at token level with `build/scratch/norm-s14.py` (scratch): only OpenGL tokens left, none was added, and after removing them the lists are equal except for the order of Qt frameworks on those five link lines (same multiset of items). Written into the gate-log row and `records/strip-07.md`. The rest of the plan's verify (otool, `build.ninja`, qt_tests lines, gate summary, tested commit, gate-log row, absent files, CMake greps, record content, no home path) was run as written.
- **Files modified:** none (classification only)
- **Commit:** `9bfb1cd1` (gate record carries the classification)

**Total deviations:** 1 auto-fixed (1 blocking, in the verification method only). **Impact:** none on the code; the diff expectation "only OpenGL items" holds, with the framework order as a harmless extra.

## Issues Encountered

None in the code. `-framework OpenGL` appears on 9 link lines both before and after, because `QtGui` brings it; it is outside the requirement. The application binary was not launched, and no live harness was run. The scratch listing "Function not found" lines in the mutation runs come from running the slot name against the other three suites of `qt_tests`; they are not failures of the new test.

## Known Stubs

None.

## Threat Flags

None beyond the plan's threat model. T-06-15 (module removal changing every link line) is mitigated: the command diff contains only OpenGL tokens and a link-item order change, Release and Debug ctest pass, goldens are unchanged.

## Next Phase Readiness

STRIP-07 is complete (plans 06-10 and 06-11). `records/strip-07.md` is the evidence. Plan 06-12 (shared Save Project As flow) is next; it adds `src/app/ProjectSaveFlow.{h,cpp}` and edits `src/app/CMakeLists.txt`, so it needs its own gate with a non-empty command diff.

## Self-Check: PASSED

- Commits `a024751a`, `9bfb1cd1` and `bcb1ea29` are ancestors of HEAD; `git rev-list --count` from the recorded base `62268fcb` gives 3 (pass).
- `src/core/OpenGLSupport.cpp` and `.h` do not exist; `grep -c OpenGL` prints 0 for `CMakeLists.txt` and `src/core/CMakeLists.txt` (pass).
- Gate summary says `result: PASS` and `goldens: unchanged`; the tested commit equals the commit with the plan subject; the gate-log row exists; `records/strip-07.md` exists, names `otool` and has no home path (pass).
- `qt-s14.txt` holds `PASS   : TestMacQt::testNoOpenGlInSourcesOrBuild()`, both picker PASS lines and no failed or skipped `Totals:` line (pass).
