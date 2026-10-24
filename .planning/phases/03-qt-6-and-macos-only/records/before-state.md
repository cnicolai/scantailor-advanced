# Phase 3 before state

Measured on the c00 tree (Release build, `build/macos-release`). Every value comes from real command output; the repository path is written `{repo}`. The "after" checks of Phase 3 compare with these numbers.

## Commit measured

c00 = `8d0105ab` (`test(03-01): add offscreen UI snapshot harness`). Measured with the Release build the c00 gate produced.

| Tool | Version |
|------|---------|
| `brew list --versions` | boost 1.92.0, jpeg-turbo 3.2.0, libpng 1.6.59, libtiff 4.7.2, ninja 1.13.2, qtbase 6.12.0_1, qtsvg 6.12.0_1 |
| `cmake --version` | cmake version 4.4.4 |
| `xcrun clang --version` | Apple clang version 21.0.0 (clang-2100.3.34.2) |
| `unifdef -V` | Version: unifdef-2.12 |
| `xcrun clang-format --version` | Apple clang-format version 21.0.0 (clang-2100.3.34.2) |

Note: `brew list --versions` prints no line for `cmake`, so CMake 4.4.4 is not a Homebrew formula install on this Mac. The README rewrite (plan 03-07) must not claim that `brew install cmake` was what provided it; the fresh-clone check there shows what the README commands need.

## Static counts

| Check | Command | Before | Expected after the strip |
|-------|---------|--------|--------------------------|
| `languageBox` in the generated dialog header | `grep -c languageBox build/macos-release/src/app/scantailor-advanced_autogen/include/ui_SettingsDialog.h` | 4 | 0 |
| Qt translator symbols in the binary | `nm -u build/macos-release/scantailor-advanced \| grep -ci translator` | 5 | 0 |
| `.qm` file pattern string in the binary | `strings build/macos-release/scantailor-advanced \| grep -c 'scantailor_\*\.qm'` | 1 | 0 |
| translation references in `build.ninja` | `grep -ci -e '\.qm' -e lrelease -e lupdate -e translations build/macos-release/build.ninja` | 40 | 0 |
| `.qm` files in the build directory | `find build/macos-release -name '*.qm' \| wc -l` | 7 | not meaningful here (see note) |

All four D-12 counts equal the planning values (4, 5, 1, 40). Note on the last row: `.qm` files left by earlier builds stay in an existing build directory, so the "no `.qm` file" check after the strip runs in the fresh clone of plan 03-07, not here.

## Leftover searches

| Search | Before (measured) | Expected after |
|--------|-------------------|----------------|
| Criterion 1: `grep -rnI -e Qt5 -e QT_VERSION CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in src` | 54 lines | 0 |
| Criterion 2: the SC2 command of RESEARCH "Validation Architecture" (`_WIN32`, `Q_OS_`, `WIN32`, `MSVC`, `MINGW`, `__APPLE__`, `UNIX`, `PORTABLE`, `TRANSLATION_DIRS`, `PLUGIN_DIRS`, `LinguistTools`, `lupdate`, `lrelease`, `QTranslator`, `XDG_SESSION_TYPE`, `SCANTAILOR_NO_XCB_FALLBACK`, `Wayland`, `wayland`) with `--exclude-dir=translations` | 60 lines | 0 |
| Packaging search: `grep -rnI -e 'install(' -e CPack -e CPACK -e uninstall -e InstallRequiredSystemLibraries -e CMAKE_INSTALL -e windeployqt -e QT_BINDIR CMakeLists.txt cmake src --include=CMakeLists.txt --include='*.cmake' --include='*.in'` | 54 lines | 0 |
| D-06 translatable calls: `git grep -h -o -E '(^\|[^A-Za-z0-9_])tr\(\|QCoreApplication::translate\(' HEAD -- 'src/*.cpp' 'src/*.h' \| wc -l` | 249 | 249 |
| `<string>` entries in `.ui` files: `git grep -h -o '<string>' HEAD -- 'src/*.ui' \| wc -l` | 574 (also 574 at a796c2ae) | 573 |

The harness added no match to any of these searches. The expected 573 after the strip: the removed Language label is the only `.ui` string that goes.

## Formatting baseline

`xcrun clang-format --dry-run src/dewarping/TextLineTracer.cpp 2>&1 | grep 'warning:'` prints exactly two warnings, at `src/dewarping/TextLineTracer.cpp:317:49` and `:328:55`. The file is not reformatted in this phase.

## Manifest baseline (D-15)

`build/macos-release/pipeline_tests --run_test=FlatDocumentTestSuite --log_level=message` (without `UPDATE_GOLDENS`): exit status 0, output ends with `*** No errors detected`. Lines containing `TOOLCHAIN DIFFERS`: 0. Of those, mentioning `qt_compile`: 0. Plan 03-02 compares with these two zeros. `git status --porcelain -- src/pipeline_tests/golden` stays empty after the run.

## Screenshots

- `records/screenshots/before/main-window.png`
- `records/screenshots/before/settings-dialog.png`
- `records/screenshots/before/texts.txt`

Main window (1280x800, offscreen, isolated settings so the dark scheme is the default): a menu bar with File, Tools and Help, a dark empty central area, and in its middle the New/Open project panel with the two links "New Project ..." and "Open Project ...". A status bar with a size grip sits at the bottom. No project is open, so no thumbnail strip or filter panel appears.

Settings dialog, two tabs (General, Processing), General tab row by row:
- Group "User Interface": a greyed-out checkbox "Accelerate user interface with OpenGL" with the note "Your hardware / driver don't provide the necessary features" (offscreen has no OpenGL context); the checkbox "Auto-save the existing project"; "Color Scheme:" with a combo showing Dark; and "Language:" with a greyed-out combo showing the single entry English, directly below Color Scheme.
- Group "Thumbnails": "Quality:" 200 and "Size:" 250 as spin boxes; checkboxes "Single column thumbnail display" (off) and "Show question on canceling multi page selection" (on).
- Group "Saving": "B&W Compression:" CCITT G4 and "Color Compression:" LZW.
- Buttons OK and Cancel at the bottom right.

`texts.txt` confirms the Language row: `label: Language: ` and `combo languageBox: English`. The harness prints `language control present: yes` and exits 0 on this tree; with `--assert-no-language` it exits 1.

Verdict: before screenshots show the Language row; text is English only.
