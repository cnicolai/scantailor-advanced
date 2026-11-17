---
phase: 06-hardening-and-cleanup
plan: 10
subsystem: opengl-viewer-option-removal
tags: [cpp, strip-07, qt-widgets, settings, color-picker, qt-test, gate]

requires:
  - phase: 06-hardening-and-cleanup
    provides: "plan 06-01 conventions (gate, label table, output-path guard); plan 06-09 gate-log numbering"
provides:
  - "The OpenGL viewer option is gone from the Settings dialog, ApplicationSettings, the ImageViewBase viewport and the colour picker"
  - "TestMacQt::testSettingsHasNoOpenGlControl: no OpenGL-named child widget and no label or button mentioning OpenGL in SettingsDialog, with a synthetic positive control"
affects: [06-11]

actuals:
  tokens: 3700
  tasks: 1
  commits: 2

plan_head_before: 9acd4bff5327d920e0468e5dd0e8a48edfc84269
plan_head_after: dd4deb191326c3a96419d2dd4faa22aa2cbcef41

tech-stack:
  added: []
  patterns:
    - "Removal slice proven at both ends: a new dialog test at one end, the unchanged colour-picker tests (before and after) at the other"
    - "New test proven to discriminate with an uncommitted mutation run, restored from a byte copy"

key-files:
  created: []
  modified:
    - src/app/SettingsDialog.ui
    - src/app/SettingsDialog.cpp
    - src/core/ApplicationSettings.h
    - src/core/ApplicationSettings.cpp
    - src/core/ImageViewBase.h
    - src/core/ImageViewBase.cpp
    - src/core/filters/output/ColorPickupInteraction.cpp
    - src/qt_tests/tst_mac_qt.h
    - src/qt_tests/tst_mac_qt.cpp
    - .planning/phases/06-hardening-and-cleanup/records/gate-log.md

key-decisions:
  - "A stored settings/enable_opengl value is neither read, deleted nor rewritten; no migration code (D-00c)"
  - "The colour picker keeps only the view grab and ColorPickupInteraction::medianColor, byte-identical; tst_hidpi_qt.cpp is unedited"
  - "OpenGLSupport.{h,cpp} and the Qt OpenGL modules stay in the build until plan 06-11, so the gate's command diff is empty"

requirements-completed: []

coverage:
  - id: D1
    description: "The Settings dialog has no OpenGL control (no child widget named like opengl, no label or button text mentioning OpenGL)"
    requirement: STRIP-07
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_mac_qt.cpp#testSettingsHasNoOpenGlControl"
        status: pass
    human_judgment: false
  - id: D2
    description: "The colour picker is unchanged after the removal"
    requirement: STRIP-07
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_hidpi_qt.cpp#testMedianColorRatioInvariant and testColorPickerGrabAtRatio2 (PASS before and after; file unedited since e5319cd9)"
        status: pass
    human_judgment: false
  - id: D3
    description: "No edited file mentions OpenGL; isOpenGlEnabled, setOpenGlEnabled, the viewport branch and sampleImageAtWidgetRect are gone"
    requirement: STRIP-07
    verification:
      - kind: other
        ref: "git grep -n -i -e opengl over the seven edited source files prints nothing (34 hits at e5319cd9)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The commit passes the gate: 100% tests passed out of 7 in Release and Debug, goldens unchanged, command diff 0 / 0"
    requirement: STRIP-07
    verification:
      - kind: other
        ref: "gate.sh check s13-opengl-option (tested 33142442) -> result: PASS"
        status: pass
    human_judgment: false
  - id: D5
    description: "The Settings dialog still looks right after the two rows left the User Interface group"
    requirement: STRIP-07
    verification:
      - kind: other
        ref: "build/scratch/ui-s13/settings-dialog.png rendered offscreen by ui_snapshot and read: no clipping, group holds auto-save and colour scheme"
        status: pass
    human_judgment: true
    rationale: "Checked on the offscreen render only; the live look on screen is not asserted by a test"

duration: about 35min
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 10: STRIP-07, remove the OpenGL viewer option Summary

**The OpenGL viewer option is removed along its whole path: the two Settings rows, the settings accessors and key, the `QOpenGLWidget` viewport branch with `sampleImageAtWidgetRect`, and the picker's OpenGL sampling branch, proven by a new Settings-dialog test and by the unchanged colour-picker tests passing before and after.**

## Performance

- **Duration:** about 35 min of clock time (one gate run in the background)
- **Completed:** 2026-11-17
- **Tasks:** 1 (tracer)
- **Files:** 7 source files edited, 2 test files edited, 1 record appended

## Accomplishments

- `SettingsDialog.ui` lost `enableOpenglCb`, `openglDeviceLabel` and their zorder lines; `SettingsDialog.cpp` lost the support check, the device label fill, the `OpenGLSupport.h` include and the store of the checkbox state. The offscreen render shows the "User Interface" group with auto-save and colour scheme and no clipping.
- `ApplicationSettings` lost `isOpenGlEnabled`, `setOpenGlEnabled`, `DEFAULT_OPENGL_STATE` and `OPENGL_STATE_KEY`; nothing reads, removes or rewrites a stored `enable_opengl` value (D-00c).
- `ImageViewBase` lost the `QOpenGLWidget` and `OpenGLSupport.h` includes, the `ApplicationSettings.h` include (confirmed unused by the build), the viewport branch and `sampleImageAtWidgetRect`; the background comment keeps its first part and drops the two OpenGL sentences.
- `ColorPickupInteraction::takeColor` samples only through `imageView().grab(rect)`; `medianColor` is untouched.
- `TestMacQt::testSettingsHasNoOpenGlControl` with the file-local helper `openGlControls`: a synthetic widget with a `QCheckBox` named `enableOpenglCb` and the old text gives a non-empty list (control), `SettingsDialog` gives an empty list, and the dialog still holds at least one `QCheckBox`. Mutation run: restoring the checkbox in `SettingsDialog.ui` (uncommitted) made the slot fail with `OpenGL controls in the Settings dialog: enableOpenglCb, Accelerate user interface with OpenGL`; the file was restored from a byte copy (`cmp` identical).
- Evidence: `build/scratch/qt-s13-before.txt` and `qt-s13.txt` both hold `PASS   : TestHiDpiQt::testMedianColorRatioInvariant()` and `PASS   : TestHiDpiQt::testColorPickerGrabAtRatio2()`; `qt-s13.txt` holds `PASS   : TestMacQt::testSettingsHasNoOpenGlControl()` and every `Totals:` line says `0 failed, 0 skipped` (TestMacQt 29 passed, up from 28). `git diff --quiet e5319cd9 -- src/qt_tests/tst_hidpi_qt.cpp` succeeds.
- Gate `s13-opengl-option` (tested `33142442`): Release and Debug `100% tests passed out of 7`, goldens unchanged, command diff 0 / 0 (edits only). Output-path guard prints nothing; all touched C++ files pass `clang-format --dry-run -Werror`.

## Task Commits

1. **Task 1 (tracer): remove the OpenGL viewer option** - `33142442` (refactor), gated as `s13-opengl-option`; gate record `dd4deb19`

The tracer verify (qt_tests with the picker lines before and after, the new slot, gate summary, grep and format checks) passed end to end.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None. The build warnings seen (`-Wunused-parameter` in `InteractionHandler.h` and `PictureZonePropDialog.cpp`) are older and outside this task. The app binary was not launched; the dialog was rendered with `ui_snapshot`, which keeps its own settings under `build/scratch`.

## Known Stubs

None.

## Threat Flags

None beyond the plan's threat model. T-06-14 (colour picker after removing the OpenGL sampling branch) is mitigated: the branch was guarded by a setting that defaulted to false, both picker tests pass before and after, and `tst_hidpi_qt.cpp` is unedited.

## Next Phase Readiness

`OpenGLSupport.{h,cpp}`, the `Qt6::OpenGL` and `Qt6::OpenGLWidgets` links and the `CMakeLists.txt` OpenGL components remain for plan 06-11. After this plan the only remaining OpenGL references in `src/` are `CMakeLists.txt:105`, `src/core/CMakeLists.txt` (source list and link line) and the `OpenGLSupport` files. STRIP-07 stays open until plan 06-11.

## Self-Check: PASSED

- Commits `33142442` and `dd4deb19` are ancestors of HEAD; `git rev-list --count` from the recorded base `9acd4bff` gives 2 (pass).
- Acceptance criteria re-run: the `git grep -n -i -e opengl` count over the seven edited files is 0; `qt-s13-before.txt` and `qt-s13.txt` both hold the two picker PASS lines and `qt-s13.txt` the new slot; gate summary says `result: PASS` and `goldens: unchanged`, the tested commit equals the commit with the plan subject, the gate-log row exists (pass).
- clang-format dry-run clean on all eight touched C++ files; `tst_hidpi_qt.cpp` unchanged against `e5319cd9` (pass).
