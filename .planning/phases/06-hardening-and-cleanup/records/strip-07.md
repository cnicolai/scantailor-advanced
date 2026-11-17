# STRIP-07 record: the OpenGL viewer option and the Qt OpenGL modules are gone (plans 06-10 and 06-11)

Checked on 2026-11-17 on the gated commit `a024751a` (`build(06-11): stop linking the Qt OpenGL modules`, gate `s14-opengl-modules`); the plan 06-10 commit is `33142442` (gate `s13-opengl-option`). Planning base: `e5319cd9`. Output is normalised: `{repo}` for the repository path, `~` for the home folder.

The requirement is read as: no OpenGL control in Settings, no OpenGL source or setting accessor, no Qt OpenGL module in CMake or in any linked binary, the colour-picker tests unchanged. The system `OpenGL.framework` that `QtGui` links itself is outside the requirement (see "Link proof").

## What was removed

At `e5319cd9`, `git grep -n -i opengl -- src CMakeLists.txt` printed 49 lines in 11 files. Now it prints nothing outside `src/qt_tests` (the two tests that prove the removal).

| File | What went | Plan |
|------|-----------|------|
| `src/app/SettingsDialog.ui` | `enableOpenglCb`, `openglDeviceLabel` and their zorder lines | 06-10 |
| `src/app/SettingsDialog.cpp` | support check, device label fill, `OpenGLSupport.h` include, store of the checkbox state | 06-10 |
| `src/core/ApplicationSettings.{h,cpp}` | `isOpenGlEnabled`, `setOpenGlEnabled`, `DEFAULT_OPENGL_STATE`, `OPENGL_STATE_KEY` | 06-10 |
| `src/core/ImageViewBase.{h,cpp}` | `QOpenGLWidget` and `OpenGLSupport.h` includes, the viewport branch, `sampleImageAtWidgetRect` | 06-10 |
| `src/core/filters/output/ColorPickupInteraction.cpp` | the OpenGL sampling branch of `takeColor` (the view grab and `medianColor` stay, byte-identical) | 06-10 |
| `src/core/OpenGLSupport.h`, `src/core/OpenGLSupport.cpp` | both files (`supported()`, `deviceName()`) | 06-11 |
| `src/core/CMakeLists.txt` | the `OpenGLSupport.cpp OpenGLSupport.h` source line; `Qt6::OpenGL` and `Qt6::OpenGLWidgets` on the core link line | 06-11 |
| `CMakeLists.txt` | `OpenGL` and `OpenGLWidgets` from `find_package(Qt6 6.5 COMPONENTS ...)`, now `Core Gui Widgets Xml Network Svg Test` | 06-11 |

## Colour picker: unchanged

`src/qt_tests/tst_hidpi_qt.cpp` is unedited since `e5319cd9` (`git diff --quiet e5319cd9 -- src/qt_tests/tst_hidpi_qt.cpp` succeeds). Both picker tests pass before the removal (`build/scratch/qt-s13-before.txt`), after plan 06-10 (`build/scratch/qt-s13.txt`) and after plan 06-11 (`build/scratch/qt-s14.txt`):

```
PASS   : TestHiDpiQt::testMedianColorRatioInvariant()
PASS   : TestHiDpiQt::testColorPickerGrabAtRatio2()
```

## Tests

- `TestMacQt::testSettingsHasNoOpenGlControl` (plan 06-10): no child widget named like `opengl` and no label or button text mentioning OpenGL in `SettingsDialog`, with a synthetic positive control. Mutation run in 06-10: putting the checkbox back into `SettingsDialog.ui` made it fail with `OpenGL controls in the Settings dialog: enableOpenglCb, Accelerate user interface with OpenGL`.
- `TestMacQt::testNoOpenGlInSourcesOrBuild` (plan 06-11): the words `QOpenGL`, `QtOpenGL`, `OpenGLSupport`, `isOpenGlEnabled`, `setOpenGlEnabled` occur in no `.cpp`, `.h` or `.mm` file of the scan scope (at least 500 files read, test code excluded as in the neighbouring scans); `CMakeLists.txt` and `src/core/CMakeLists.txt` are non-empty and do not contain `OpenGL`; `src/app/SettingsDialog.ui` does not contain `opengl` case-insensitively. Positive controls: a synthetic `#include <QtOpenGLWidgets/QOpenGLWidget>` line is found, a `QWidget` line is not.
- Mutation runs for the new test (uncommitted, files restored from byte copies, `cmp` identical afterwards; the test reads the files at run time, so no rebuild was needed): appending `// QOpenGLWidget` to `src/core/PageId.h` fails with `src/core/PageId.h: QOpenGL`; appending `# OpenGL` to `src/core/CMakeLists.txt` fails with `src/core/CMakeLists.txt names OpenGL`; appending `# OpenGLWidgets` to `CMakeLists.txt` fails with `CMakeLists.txt names OpenGL`. The unmodified tree passes.
- `build/scratch/qt-s14.txt` (`build/macos-release/qt_tests`, run with the ctest environment `QT_QPA_PLATFORM=offscreen`, `LC_ALL=en_US.UTF-8`, `TMPDIR` under `build/macos-release/test-tmp`): every `Totals:` line says `0 failed, 0 skipped`; it holds `PASS   : TestMacQt::testNoOpenGlInSourcesOrBuild()`.

## Link proof

Both presets were configured again after the CMake edit (`cmake --preset macos-release`, `macos-debug`; no `--fresh` was needed) and built completely.

`otool -L build/macos-release/<binary>` (listings in `build/scratch/otool-s14-<binary>.txt`, Debug app in `build/scratch/otool-s14-debug.txt`), counted lines that mention `QtOpenGL`:

| Binary | QtOpenGL lines at `e5319cd9` | now | `QtGui` lines (positive control) |
|--------|------------------------------|-----|----------------------------------|
| `scantailor-advanced` | 2 | 0 | 1 |
| `qt_tests` | 2 | 0 | 1 |
| `mac_check` | 2 | 0 | 1 |
| `retina_check` | 2 | 0 | 1 |
| `ui_snapshot` | 2 | 0 | 1 |
| `pipeline_tests` | 2 | 0 | 1 |
| `core_tests` | 2 | 0 | 1 |
| Debug `scantailor-advanced` | - | 0 | 1 |

`grep -c QtOpenGL build/macos-release/build.ninja` printed 334 at `e5319cd9` and prints 0 now (Debug: 0).

The system `OpenGL.framework` (`/System/Library/Frameworks/OpenGL.framework/...`) still appears once per binary. `QtGui` links it itself (`otool -L` of `QtGui` lists it), and the 9 link lines that carry `-framework OpenGL` are unchanged. A search for "opengl" over `otool -L` can therefore never come out empty; the checks look for `QtOpenGL` only.

## Gate and command diff

Gate `s14-opengl-modules` (tested commit `a024751a`): Release and Debug `100% tests passed out of 7`, goldens unchanged, command diff 639 / 639 lines. The Qt OpenGL modules disappear from the build, so a non-empty diff is expected. Classification, same in both presets (token comparison of `cmds-<preset>-ref.txt` against `cmds-<preset>.txt`):

- tokens that left: `-DQT_OPENGL_LIB`, `-DQT_OPENGLWIDGETS_LIB`, the `QtOpenGL` and `QtOpenGLWidgets` include directories, the `QtOpenGL` and `QtOpenGLWidgets` framework link items, and the `OpenGLSupport.cpp` compile line, its object and dependency file, and its archive member;
- tokens that were added: none;
- after removing exactly those items from both lists, they are equal line for line except 5 link lines (`scantailor-advanced`, `ui_snapshot`, `core_tests`, `pipeline_tests`, `qt_tests`) that carry the same items with the Qt frameworks in another order (CMake orders the link items again once two modules are gone); the set of items per line is identical. Framework order has no effect on dynamic Qt frameworks.

The normalising script is `build/scratch/norm-s14.py` (scratch, not committed); the comparison files are under `build/scratch/gate/s14-opengl-modules/` (`norm-ref-*.txt`, `norm-new-*.txt`).

The output-path guard of convention 13 prints nothing. Settings are untouched (D-00c): `git grep -n -i -e enable_opengl -e OPENGL_STATE -- src` prints nothing, so no code reads, deletes or rewrites a stored `settings/enable_opengl` value, and there is no migration code. The application binary was not launched.

## Gate rows

| Label | Commit | Result |
|-------|--------|--------|
| s13-opengl-option | `33142442` | PASS, goldens unchanged, command diff 0 / 0 |
| s14-opengl-modules | `a024751a` | PASS, goldens unchanged, command diff 639 / 639, classified above |

## For the Phase 6 transition

`.planning/codebase/STACK.md`, `.planning/codebase/INTEGRATIONS.md` and the generated stack text of `.claude/CLAUDE.md` still mention the OpenGL modules and `src/core/OpenGLSupport.cpp`; they are regenerated through `generate-claude-md` at the Phase 6 transition and were not edited here.
