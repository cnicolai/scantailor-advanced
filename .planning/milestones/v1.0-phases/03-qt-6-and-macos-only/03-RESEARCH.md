# Phase 3: Qt 6 and macOS Only - Research

**Researched:** 2026-10-24
**Domain:** Mechanical removal of dead platform code from a C++17/Qt 6/CMake desktop app, proven output-neutral by golden tests
**Confidence:** HIGH (every inventory item below was read in the tree this session; the whole strip was prototyped in a scratch copy, built, and run through `ctest`)

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Strip depth**
- **D-01:** The rule is that everything that never runs on the Mac goes. The user stated it in those words ("Everything that never runs on the Mac shall go"). Besides what STRIP-01 to STRIP-06 name, it covers:
  - The `PORTABLE_VERSION` option, `PORTABLE_CONFIG_DIR` in `config.h.in`, `Application::isPortableVersion()` and `getPortableConfigPath()`, and their callers in `src/app/main.cpp:48` and `src/core/DefaultParamsProfileManager.cpp:21`. The presets already set the option OFF, so every build then behaves as today's preset builds do. Settings go to the INI under `~/.config/scantailor-advanced/` and profiles to `~/Library/Application Support/scantailor-advanced/profiles` (Phase 1 amendment to D-01/D-02).
  - Every `install()` rule, the uninstall target (`cmake/cmake_uninstall.cmake.in`), and the Windows runtime-library helpers: `cmake/CopyToBuildDir.cmake`, `cmake/copy_to_build_dir.cmake.in`, `cmake/AddDynamicLibraryLocations.cmake` and `add_runtime_libs_to_install`.
  - The GCC-only flag block under `CMAKE_COMPILER_IS_GNUCC`, including `-march=native`. The Clang block next to it stays byte-for-byte unchanged (Phase 1 D-03 `-ffp-contract=off`, D-08 warnings).
  - The MSVC, MinGW, `WIN_XP` and `DEPENDENCIES_ROOT` blocks.
  - `scripts/post_issue_followups.py`, a one-shot helper that posts comments on upstream ScanTailor-Advanced issues through `gh`.
  - `src/resources/unix/` and `src/resources/win32/`. Also `src/resources/scantailor-advanced.png`, whose only reference is the AppImage step in `.github/workflows/cmake.yml`. `src/resources/appicon.svg` stays because `resources.qrc` lists it.
  - The non-Mac side of platform conditionals: the `#else` side of `Q_OS_MAC` in `src/core/zones/ZoneContextMenuInteraction.{h,cpp}`, and the `#else` side of `Q_OS_UNIX` in `src/app/MainWindow.cpp:1327`. The Mac side of each stays as it is.
  - `TRANSLATION_DIRS` and `PLUGIN_DIRS` in `CMakeLists.txt` and `config.h.in`, once nothing in the Mac build reads them.
- **D-02:** "Runs on the Mac" means the code executes in the macOS build, whatever it was written for. Platform macros (`_WIN32`, `Q_OS_WIN`, `WIN32`, `MSVC`, `MINGW`, `Q_OS_MAC`, `Q_OS_UNIX`) and the Qt 5 version gates are resolved to the side the macOS Qt 6 build takes today. After the strip the macOS build must behave exactly as before.
- **D-03:** Windows-minded code without a platform label that runs on the Mac **stays**. The known case is `RelinkablePath::normalize` (`src/core/RelinkablePath.cpp:12`). It turns every backslash into a slash, and its `..` handling checks for drive letters (`endsWith(QChar(':'))`). A backslash is a legal character in macOS file names, so a name that contains one can be mangled. Removing this logic would change behaviour, so Phase 3 leaves it and files a backlog item (next free 999.x) that describes the case. It is to be fixed with a test in a later phase. `QDir::toNativeSeparators` calls also stay, because they run on the Mac and do nothing there. The labelled `#if _WIN32` branch inside `normalize` (line 23) goes under D-01.
- **D-04:** The batch-finished alarm in `src/app/MainWindow.cpp:1327` keeps its Unix side, the `play /usr/share/sounds/...` command, until Phase 5 replaces it (MAC-10). Phase 3 removes only the `#else` side.

**Translations**
- **D-05:** These go together, so that the CMake is never left half-edited (PITFALLS Pitfall 25):
  - `src/translations/*.ts`, `cmake/UpdateTranslations.cmake`, the `compile_translations` and `update_translations` targets and the `LinguistTools` component
  - `Application::installLanguage`, `getLanguagesList`, `m_translator` and `m_translationsMap`
  - `ApplicationSettings::getLanguage` and `setLanguage`, and the calls in `src/app/main.cpp:53` and `src/app/MainWindow.cpp:1537`
  - the Language row (`languageBox` and its label) in `src/app/SettingsDialog.ui` and `SettingsDialog.cpp`

  No `.qm` file is built or loaded.
- **D-06:** The `tr()` and `QCoreApplication::translate` calls stay, and the `.ui` strings stay translatable. With no translator installed they return the English text unchanged. Stripping about 220 calls in 42 files plus every `.ui` form would make a large diff with no visible effect, and the project convention keeps `tr()` on user-visible strings. Source strings are not edited.
- **D-07:** A `language` value that already sits in the user's settings file is left alone: no code reads it, deletes it or rewrites it. Nothing in this phase writes to the user's settings.

**README and test docs**
- **D-08:** The README gets a new top, and the Features section is kept.
  - The new top says what this fork is: a personal macOS fork of ScanTailor Advanced 1.2.1 for Apple Silicon, with changes not meant for upstream. It links to upstream, lists the Homebrew prerequisites, gives the configure, build and run commands with the `macos-release`, `macos-debug` and `macos-asan` presets, shows how to run the tests, and says where settings and profiles live.
  - The Homebrew list follows Phase 1 D-09: `cmake ninja boost qtbase qtsvg` plus `jpeg-turbo`, `libpng` and `libtiff`, and never the `qt` meta-formula or `jpeg`. Keep `qttools` only if the build still needs it once `LinguistTools` is gone.
  - These sections go: "Building" (Windows, Linux, Wayland and Flatpak notes), the old "About this fork" (the previous fork's notes about translations), and "Building (detailed)" (Ubuntu, MXE cross-compile and native Windows).
  - The Features section (about lines 109–440, with no platform-specific content) stays unchanged as the reference for what each option does. The Contents list is updated to match the new structure.
- **D-09:** `TESTING.md` (currently Spanish, covering only the five old executables) is rewritten in English and stays a separate file, linked from the README. The README holds every command needed to build and run the tests (success criterion 5). `TESTING.md` holds the details:
  - the seven `ctest` suites and what each covers
  - running them through the presets
  - `UPDATE_GOLDENS=1` and its rules (Phase 2 D-06, D-07, D-10)
  - the failure artifacts in `build/<preset>/test-artifacts/<test>/`
  - the toolchain manifest and its warn-only behaviour (Phase 2 D-09)
  - the expected failures and their backlog items 999.1–999.7
  - the `macos-asan` preset
  - how to format locally with `xcrun clang-format`, now that the lint workflow is gone
- **D-10:** Agent docs are brought up to date as the last step of the phase. Patch the stale facts in every `.planning/codebase/*.md` map that mentions something removed: Qt 5 fallback, Windows, Linux, Flatpak/DEB/NSIS/CPack, the CI and lint workflows, translations, portable mode or install rules. All seven maps currently mention at least one of these. Then regenerate the GSD-managed sections of `.claude/CLAUDE.md` with `gsd-tools generate-claude-md`. The `GSD:profile` section is not touched, and there is no full `/gsd-map-codebase` remap.

**Verification**
- **D-11 (carried from Phase 2 and STATE.md):** Every strip commit must pass the gate: it builds and passes `ctest --preset macos-release` and `ctest --preset macos-debug` 7/7, with no golden or manifest file changed, and `UPDATE_GOLDENS` is never set in this phase. Qt 5 conditionals are resolved with `unifdef`, never by hand, and every resulting diff is read. The polarity of the RANSAC and dewarping branches in `src/dewarping/DistortionModelBuilder.cpp` and `src/dewarping/DetectVertContentBounds.cpp` changes dewarp output if it is wrong.
- **D-12:** Verification is fully automated, with no GUI step for the user. Success criterion 4 is checked in two ways:
  - An automated check asserts that the Settings dialog has no language control and that the build neither produces nor loads a `.qm` file.
  - The executor launches the app with the offscreen platform, grabs the main window and the Settings dialog as PNGs, and looks at them itself. It looks for a gap or broken layout where the Language row was, and for any non-English text. The verdict goes into the phase records.
- **D-13:** Success criterion 5 is proven by a fresh clone. The executor clones the repository into a new folder under `build/`, which is gitignored (not `/tmp`). It then runs only the commands the README lists: configure, build and `ctest`. Any step that needs knowledge the README does not give counts as a failure and is fixed in the README. The README's `brew install` line is **not** executed, because it could upgrade Qt or Boost mid-phase and move the toolchain away from the one the goldens were recorded with. Instead, `brew list --versions` confirms that every package the README lists is installed.
- **D-14:** One `macos-asan` run of the full suite follows the last strip commit. Its findings are compared with `.planning/phases/02-golden-baselines/records/sanitizer-findings.md`: 12 UBSan and 0 ASan reports at 9 project locations. A new report blocks the phase until it is explained. Known reports are only recorded, not fixed.
- **D-15:** The success criterion 1 search (`Qt5` and `QT_VERSION` in CMake files and `src/`) must come out empty, test code included. `src/test_support/Manifest.cpp:73` uses `QT_VERSION_STR` for the manifest's `qt_compile` field. Its source changes so that the token disappears, while the string written to the manifest stays identical and no committed golden `MANIFEST.txt` changes.

### Claude's Discretion
- **Plan and commit split.** Use one commit per strip category: Qt 5 C++ conditionals, Qt 5 CMake, Windows, Linux, packaging and install, CI, translations, README and `TESTING.md`, agent docs. Each commit passes D-11. Order them per PITFALLS Pitfall 7 (C++ conditionals, then CMake, then the `main.cpp` Qt 5 block and the `QGLWidget` macro hack at the top of `src/core/ImageViewBase.cpp`, then docs and `flatpak/`) and Pitfall 14 (delete `src/resources/unix/` only after removing the unconditional `configure_file` of the `.desktop.in`).
- **Minimum Qt 6 version** in `find_package(Qt6 …)`. Set a floor, not a pin: a future `brew upgrade` must still configure. Pick the lowest version the code actually needs, no higher than the installed 6.12.
- **Replacing the `Qt_*_lib` indirection variables** (direct `Qt6::` or `Qt::` targets) and whether `cmake/ListItemsPrepend.cmake` and `cmake/SetDefaultBuildType.cmake` are still used.
- **How D-15 removes the token**, for example a compile definition fed from CMake's `Qt6_VERSION`.
- **Where the D-12 screenshots land** and whether they are committed. They show synthetic UI only, with no personal data.
- **Whether `.github/` is deleted as a whole.** It contains only workflows and `dependabot.yml`.
- **README details** not fixed above, such as whether the upstream demo-video link at the top stays, and wording. Planning documents use British spelling; keep README and `TESTING.md` consistent with that.
- **Filing the D-03 backlog item** (next free 999.x) through the GSD backlog command during execution.

### Deferred Ideas (OUT OF SCOPE)
- **Backslash and drive-letter handling in `RelinkablePath::normalize`.** On macOS a file name may contain `\`, which the normaliser turns into a path separator. Filed as a backlog item during Phase 3 (D-03) and to be fixed with a test in a later phase.
- **Local clang-format enforcement** (a `format` target or a pre-commit hook) to replace the removed lint workflow. That is a new capability, so Phase 3 only documents the manual command (D-09).
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| STRIP-01 | Only Qt 6 is supported; Qt 5 CMake branches and `QT_VERSION` `#if` blocks are removed with baseline images unchanged | Section "QT_VERSION inventory": `unifdef -DQT_VERSION_MAJOR=6` resolves all 39 production lines in 19 files, 0 insertions; "Qt 5 in CMake" lists every `Qt5`/`Qt6_FOUND` site; prototype built and passed 7/7 |
| STRIP-02 | Windows-specific code paths and build options are removed | "Platform macro inventory" (7 C++ sites, `-U_WIN32 -UQ_OS_WIN`) and "CMake block inventory" (Windows blocks with exact lines) |
| STRIP-03 | Linux-specific code (Wayland/xcb fallback, desktop and MIME files) is removed | xcb/Wayland block is inside `#if QT_VERSION_MAJOR == 5` (`main.cpp:20-33`), so it leaves with STRIP-01; `src/resources/unix/` plus the unconditional `configure_file` (`CMakeLists.txt:357-360`); `Q_OS_MAC`/`Q_OS_UNIX` guards resolved |
| STRIP-04 | Packaging is removed: Flatpak, DEB, NSIS/CPack, `build-*.sh` scripts and MXE notes | File list with line counts; root `CMakeLists.txt:383-545` tail; `src/app/CMakeLists.txt:82-93` |
| STRIP-05 | All GitHub Actions workflows and the Dependabot configuration are removed | `.github/` holds exactly 6 tracked files (5 workflows, `dependabot.yml`); delete whole directory |
| STRIP-06 | Translations are removed; the UI is English only | Atomic removal set (11 CMake call sites, 8 `.ts` files, 4 C++ files, 1 `.ui`); finding that translations were never loaded on the Mac build today |
| STRIP-08 | The README documents the macOS build and test commands | README target structure, Homebrew list (qttools not needed, proven), fresh-clone proof recipe, two stale README passages in the "kept" Features section |
</phase_requirements>

## Summary

The strip is mechanical and low risk because the macOS build already takes the Qt 6 / non-Windows / non-Linux side of every conditional. I ran the whole phase as a prototype in a scratch copy of the tree (outside the project): `unifdef` on 26 files, hand edits for translations, portable mode and the Language row, a rewritten CMake, and deletion of all packaging, CI, resource and translation files. The result configured with every `qttools` CMake package hidden, built in 62 s, and passed `ctest` 7/7 in Release (59 s) and Debug (268 s) with zero golden files changed. The full set of build commands (compile and link lines of all eight executables) was byte-identical to the unstripped build apart from one intended new define (`ST_QT_COMPILE_VERSION`), which is the strongest available proof that the CMake edit is output-neutral.

Three findings change the plan. First, translations are already dead on the Mac: `Application::initTranslations` scans for `scantailor_*.qm` (`Application.cpp:68`) but the build produces `scantailor-advanced_*.qm`, so no translator was ever installed and the Settings dialog shows a disabled one-entry "English" combo today; removing translations changes nothing at runtime and the offscreen screenshot of the baseline proves it. Second, `qttools` is not needed after `LinguistTools` goes (prototype configured and built with all `qttools` CMake packages on `CMAKE_IGNORE_PATH`), so the README list is `cmake ninja boost qtbase qtsvg jpeg-turbo libpng libtiff`. Third, the README "Features" section that D-08 keeps unchanged contains two passages that describe the portable mode D-01 removes (lines 213-217 and 399); they need an exception.

For D-12 the app sources are not in a library, so no existing test target can construct `MainWindow` or `SettingsDialog`. I built and ran a small `ui_snapshot` executable (same sources as the app except `main.cpp`, offscreen, isolated settings) that writes both PNGs and fails if a language control exists; it produced the "before" image with the Language row (exit 1) and the "after" image without it (exit 0). It should be the first commit of the phase so the "before" shots are taken on the baseline.

**Primary recommendation:** Add the `ui_snapshot` harness and capture "before" artefacts first, then strip in gated commits C1 to C9 (order in "Recommended commit sequence"), resolving every conditional with `unifdef` using only the symbols of the current category, formatting with `xcrun clang-format -i` except `src/dewarping/TextLineTracer.cpp`, and proving each CMake commit with an identical-build-commands diff in addition to the D-11 gate.

## Architectural Responsibility Map

This phase has no client/server tiers; the "tiers" are build-system layers. The map exists so the planner can check that each deletion lands in the layer that owns it.

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Qt version gating (`QT_VERSION*`) | C++ preprocessor (`src/`) | CMake (`find_package`, `Qt_*_lib`) | Both layers carry a Qt 5 branch; C++ goes first (Pitfall 7) |
| Platform gating (`_WIN32`, `Q_OS_*`) | C++ preprocessor | CMake `if (WIN32/UNIX/MSVC/MINGW)` | Resolved to the macOS side, never edited by eye |
| Configuration header (`config.h`) | CMake `configure_file` (`config.h.in`) | Runtime readers in `Application.cpp`, `main.cpp`, `MainWindow.cpp` | Only `APPLICATION_NAME`, `ORGANIZATION_NAME`, `ENABLE_DEBUG_FEATURES` have macOS readers after the strip |
| Translations | `Application` + `ApplicationSettings` + `SettingsDialog` (runtime) | CMake `LinguistTools`/`compile_translations` (build) | Must be removed together (D-05) |
| Packaging and install | CMake `install()`/CPack | `flatpak/`, `build-*.sh`, `.github/` | None of it runs on the Mac |
| Output-equivalence proof | `pipeline_tests` goldens (`src/pipeline_tests/golden`) | `ninja -t commands` diff, `unifdef` insertion count | Goldens prove behaviour, command diff proves the CMake edit |
| UI verification | Offscreen Qt platform plugin | `ui_snapshot` harness | No existing target links `MainWindow`/`SettingsDialog` |

## Standard Stack

### Core (tools this phase uses; no new libraries)

| Tool | Version | Purpose | Why Standard |
|------|---------|---------|--------------|
| `unifdef` | 2.12 at `/usr/bin/unifdef` [VERIFIED: `unifdef -V` printed "Version: unifdef-2.12"; `command -v unifdef` printed `/usr/bin/unifdef`] | Resolve `#if` conditionals | D-11 mandates it; ships with Xcode CLT, nothing to install |
| `xcrun clang-format` | Apple clang-format 21.0.0 (clang-2100.3.34.2) [VERIFIED: `xcrun clang-format --version`] | Format touched C++ files | `clang-format` is not on `PATH`; `xcrun` finds it. `.clang-format` is `BasedOnStyle: Chromium` |
| CMake | 4.4.4 [VERIFIED: `brew list --versions`: "cmake 4.4.4"] | Configure/build/test presets | `cmake_minimum_required(VERSION 3.25)` in `CMakeLists.txt:1` |
| Ninja | 1.13.2 [VERIFIED: `brew list --versions`] | Generator of the presets; `ninja -t commands` for the equivalence proof | |
| Qt (`qtbase`, `qtsvg`) | 6.12.0_1 [VERIFIED: `brew list --versions`] | The only supported Qt | `qttools` 6.12.0_1 is also installed but not needed (see below) |
| Boost / jpeg-turbo / libpng / libtiff | 1.92.0 / 3.2.0 / 1.6.59 / 4.7.2 [VERIFIED: `brew list --versions`] | Unchanged | Goldens were recorded against exactly these |

### Qt 6 components after the strip

Keep `Core Gui Widgets Xml Network OpenGL Svg OpenGLWidgets Test`; drop `LinguistTools`. `OpenGL` and `OpenGLWidgets` stay because STRIP-07 is Phase 6.

Provenance of each component (which Homebrew formula owns it) [VERIFIED: `ls -l /opt/homebrew/lib/cmake` symlink targets]: `Qt6CoreTools`, `Qt6WidgetsTools` (moc/uic/rcc), `Qt6Test`, `Qt6OpenGLWidgets` point into `Cellar/qtbase/6.12.0_1`; `Qt6Svg` into `Cellar/qtsvg/6.12.0_1`. `Qt6LinguistTools`, `Qt6Tools`, `Qt6ToolsTools`, `Qt6Designer*`, `Qt6Help*`, `Qt6Linguist`, `Qt6UiPlugin`, `Qt6UiTools*` are `qttools`.

**`qttools` is not needed.** The stripped prototype was configured with `-DCMAKE_IGNORE_PATH=` set to all twelve `qttools` directories under `/opt/homebrew/lib/cmake/` (Qt6Designer, Qt6DesignerComponentsPrivate, Qt6DesignerPrivate, Qt6Help, Qt6HelpPrivate, Qt6Linguist, Qt6LinguistTools, Qt6Tools, Qt6ToolsTools, Qt6UiPlugin, Qt6UiTools, Qt6UiToolsPrivate), built, and passed 7/7 [VERIFIED: run this session]. README lists `cmake ninja boost qtbase qtsvg jpeg-turbo libpng libtiff`. (`zlib` comes from the SDK: the app links `/usr/lib/libz.1.dylib` [VERIFIED: `otool -L build/macos-release/scantailor-advanced`].)

### Minimum Qt version (Claude's discretion)

Recommend `find_package(Qt6 6.5 COMPONENTS ... CONFIG REQUIRED)`.

- A version argument is a floor, not a pin [VERIFIED: configuring with `find_package(Qt6 6.12 ...)` succeeded and printed `Qt6_VERSION=[6.12.0]`; `find_package(Qt6 6.13 ...)` failed with "The version found is not compatible with the version requested", version found 6.12.0].
- Only 6.12.0 can be tested here, so a lower floor cannot be proven by compiling. The Qt 6 branches that become permanent use `QLineF::intersects`, `Qt::SkipEmptyParts`/`Qt::KeepEmptyParts`, `QAtomicInt::storeRelaxed/loadRelaxed`, `QMouseEvent::position()`, `QEnterEvent`, `QIODeviceBase`, `QRandomGenerator`, `<QtOpenGLWidgets/QOpenGLWidget>` [VERIFIED: unifdef output diffs]. To my knowledge all of these exist since Qt 6.0 or earlier [ASSUMED], and I found no Qt 6.1+ API in a targeted grep of `src/` (QKeyCombination, QPointingDevice, QEventPoint, QStringConverter, `fromString`, moveToTrash, QUtf8StringView, QStringTokenizer, colorScheme, setColorSpace, QNativeInterface, QPermission: no hits) [VERIFIED: grep]. 6.5 is a conservative documented floor with zero practical cost on a Homebrew machine that always has the newest Qt.
- Do NOT use the Qt 6.12 value as the floor; a `brew upgrade` that moves to 6.12.x or later must configure.

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `unifdef` | regex script | D-11 forbids it; `unifdef` resolved all 39 lines with 0 insertions, which is checkable |
| `ST_QT_COMPILE_VERSION="${Qt6_VERSION}"` (D-15) | `QLibraryInfo::version()` | That is the runtime version; the manifest needs the compile-time one (`qt_compile` vs `qt_runtime` keys, `Manifest.cpp:72-73`) |
| `Qt6::Core` etc. | keep `Qt::Core` via `Qt_*_lib` variables | Variables exist only to switch Qt 5/6; replaced targets gave identical link lines |
| Separate `ui_snapshot` executable | extend `qt_tests` | `qt_tests` links only `core foundation Qt6::Test ...` (`src/qt_tests/CMakeLists.txt:9`) and `MainWindow`/`SettingsDialog` live in the app executable's sources, not a library |

**Installation:** nothing to install. `brew list --versions cmake ninja boost qtbase qtsvg jpeg-turbo libpng libtiff` must show all seven (D-13 check).

## Package Legitimacy Audit

No external package is added, removed or upgraded by this phase. All Homebrew formulae above were already installed in Phase 1 (D-09) and unchanged. The `gsd-tools query package-legitimacy` seam covers npm/PyPI/crates only and does not apply. `unifdef` is Apple's `/usr/bin/unifdef` from the Command Line Tools, not a downloaded package.

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

## Architecture Patterns

### System Architecture Diagram

```
 baseline a796c2ae  (goldens: src/pipeline_tests/golden, 38 files, unchanged since)
        |
        v
 C0 harness + "before" artefacts --> records/ (PNG, ninja command lists, nm/strings counts)
        |
        v
 per strip commit N:
   edit (unifdef -D/-U only the category's symbols; hand edit CMake/translations)
        |
        +--> xcrun clang-format -i (not TextLineTracer.cpp)
        |
        v
   cmake --preset macos-release  --> cmake --build --> ctest   (7/7, ~1+1 min)
   cmake --preset macos-debug    --> cmake --build --> ctest   (7/7, ~1+4.5 min)
        |
        +--> git diff --quiet a796c2ae -- src/pipeline_tests/golden     (goldens unchanged)
        +--> CMake commits only: ninja -t commands diff vs baseline     (identical but intended)
        v
   commit
        |
        v  (after last strip commit)
 README + TESTING.md --> fresh clone under build/ --> README commands only --> ctest 7/7
        |
        v
 ui_snapshot "after" PNGs + static checks (nm -u, strings, build.ninja, ui_SettingsDialog.h)
        |
        v
 macos-asan full suite (executables run directly) --> compare 9 locations with sanitizer-findings.md
        |
        v
 agent docs: patch .planning/codebase/*.md, then gsd-tools generate-claude-md
```

### Recommended commit sequence (per Pitfall 7 and 14)

| # | Commit | Contents | CMake touched? |
|---|--------|----------|----------------|
| C0 | test: offscreen UI snapshot harness | `src/app/ui_snapshot/main.cpp` + `BUILD_TESTS` block in `src/app/CMakeLists.txt`; then records: "before" PNGs (exit 1 expected), baseline command lists | yes (app) |
| C1 | refactor: resolve Qt 5 conditionals in C++ | `unifdef -DQT_VERSION_MAJOR=6` on the 19 files, clang-format, plus D-15 (`Manifest.cpp` + `ST_QT_COMPILE_VERSION` in `src/test_support/CMakeLists.txt`) | one line |
| C2 | build: Qt 6 only in CMake | `find_package(Qt5)`, `qt_min_version`, `Qt_*_lib` to `Qt6::`, `Qt6_FOUND` branches (also those inside `if (WIN32)` blocks that later commits delete), `Qt5_add_translation` | yes |
| C3 | chore: remove CI | `git rm -r .github` | no |
| C4 | build: remove packaging and install rules | `flatpak/`, `build-deb.sh`, `build-windows.sh`, `scripts/`, CPack block, all `install()`, uninstall target and `cmake/cmake_uninstall.cmake.in`, `install(FILES ${qm_files} ...)`, `src/app/CMakeLists.txt:82-93` | yes |
| C5 | refactor: remove Windows code paths | `unifdef -U_WIN32 -UQ_OS_WIN` on 7 files, Windows CMake blocks, `src/resources/win32/`, `cmake/CopyToBuildDir.cmake`, `cmake/copy_to_build_dir.cmake.in`, `cmake/AddDynamicLibraryLocations.cmake`, `EXTRA_LIBS` (9 references), `NOMINMAX`, MSVC/WIN_XP flag block | yes |
| C6 | refactor: remove Linux code paths | `unifdef -DQ_OS_MAC -DQ_OS_UNIX` on 4 files, GNU flag block, `if (UNIX)` unwrap, `configure_file(... .desktop.in)` removal then `src/resources/unix/` and `src/resources/scantailor-advanced.png` deletion | yes |
| C7 | refactor: remove portable mode | `Application.{h,cpp}` portable members, `main.cpp:48-50`, `DefaultParamsProfileManager.cpp:20-25`, `PORTABLE_VERSION` option, `config.h.in:9`, `CMakePresets.json:17` | yes |
| C8 | refactor: remove translations | the atomic set of D-05 (list below) plus `TRANSLATION_DIRS`/`PLUGIN_DIRS` | yes |
| C9 | docs: README and TESTING.md | D-08, D-09 | no |
| V | verification records | fresh clone, "after" PNGs, ASan comparison | no |
| C10 | docs: agent docs | D-10 | no |

Reasons for this order: the C2 grep for `Qt5` can only be empty once the `Qt5::` lines inside the Windows install block (`CMakeLists.txt:412-416`) are resolved, which C2 does in place; C4 precedes C5/C6 so that install blocks are already gone when `if (WIN32)`/`if (UNIX)` are unwrapped; C6 removes the unconditional `configure_file` of the `.desktop.in` in the same commit as the directory deletion (Pitfall 14); C7 follows C6 because `config.h.in` lines are removed one category at a time; C8 comes last among code commits because it edits `Application.{h,cpp}` after C7 already trimmed them.

Estimated cost of the D-11 gate [VERIFIED by measurement today unless noted]: Release full build 60 s wall (489 steps, 10 cores), Debug full build 58 s, `ctest` Release 59 s (Phase 2 record: 55.6 s), `ctest` Debug 268 s while another build ran (Phase 2 record: 230 s; `pipeline_tests` alone dominates). A worst-case gate with full rebuilds is about 7 to 8 minutes; incremental builds after a C++-only commit are 10 to 60 s. Ten commits cost about 75 minutes of gate time. The `macos-asan` `ctest` took 661 s in Phase 2 (`records/sanitizer-findings.md`), one run.

### Recommended file layout for the harness

```
src/app/
├── ui_snapshot/main.cpp      # new, test-only; compiled only when BUILD_TESTS is ON
└── CMakeLists.txt            # + `if (BUILD_TESTS) add_executable(ui_snapshot ...)` block
```
Not registered with `add_test` (D-11 says 7/7 suites). It adds one executable, +17 s CPU to a full build.

### Pattern 1: Category-scoped `unifdef`

**What:** Run `unifdef` once per commit with only that commit's symbols. A symbol not named stays untouched (verified: the `-DQT_VERSION_MAJOR=6`-only run left `#ifdef _WIN32` in `main.cpp` and `Utils.cpp`).
**Why it works:** The macOS Qt 6 preprocessor state was measured with clang: `Q_OS_MAC`, `Q_OS_MACOS`, `Q_OS_UNIX`, `__APPLE__` defined; `Q_OS_WIN`, `_WIN32`, `Q_OS_LINUX` not defined; `QT_VERSION_MAJOR` 6, `QT_VERSION_MINOR` 12 [VERIFIED: `clang++ -E` of a probe file including `<QtGlobal>` printed `Q_OS_MAC=defined`, `Q_OS_MACOS=defined`, `Q_OS_UNIX=defined`, `__APPLE__=defined`, `6 12 "6.12.0"`, and no line for `Q_OS_WIN`, `_WIN32`, `Q_OS_LINUX`].

```bash
# C1 (Qt 5 conditionals). zsh does not word-split an unquoted $VAR: use xargs.
grep -rlI 'QT_VERSION' src --exclude-dir=translations | grep -v 'src/test_support/Manifest.cpp' | sort \
  | xargs unifdef -m -DQT_VERSION_MAJOR=6
# exit status of unifdef is 1 when it changed a file (xargs then reports 123): do not use `set -e`.
git diff --numstat   # additions column must be 0 for every file before clang-format
```
Only `QT_VERSION_MAJOR=6` is needed: `QT_VERSION_MINOR` never has to be defined, because every expression is `MAJOR == 5 && ...` (false) or `MAJOR > 5 || ...` (true) [VERIFIED: 0 `QT_VERSION` left in the 19 outputs].

```bash
# C5 (Windows)   files: Utils.cpp DarkColorScheme.cpp LightColorScheme.cpp RelinkablePath.cpp main.cpp ByteOrder.h
unifdef -m -UQ_OS_WIN -U_WIN32 src/core/Utils.cpp src/core/DarkColorScheme.cpp src/core/LightColorScheme.cpp \
  src/core/RelinkablePath.cpp src/app/main.cpp src/imageproc/ByteOrder.h
# C6 (Linux/non-Mac side)   files: ZoneContextMenuInteraction.{h,cpp} MainWindow.cpp
unifdef -m -DQ_OS_MAC -DQ_OS_UNIX src/core/zones/ZoneContextMenuInteraction.h \
  src/core/zones/ZoneContextMenuInteraction.cpp src/app/MainWindow.cpp
```
`#if _WIN32` (bare value form, `RelinkablePath.cpp:23`) is handled by `-U_WIN32` [VERIFIED: removed in the combined run].

**`QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)`** (`main.cpp:29`): `unifdef` cannot evaluate a function-like macro call. It never has to: the line sits inside `#if QT_VERSION_MAJOR == 5` (`main.cpp:20-33`), which `unifdef` removes whole [VERIFIED: diff]. If a future gate has `QT_VERSION_CHECK` outside such a block, resolve it by hand and read the diff.

**Polarity check (D-11) for the two dewarping files** [VERIFIED: unifdef diffs]:

| File:line | Expression | Mac side | Kept |
|-----------|------------|----------|------|
| `DistortionModelBuilder.cpp:9` | `#if QT_VERSION_MAJOR > 5 || QT_VERSION_MINOR > 9` | true | `#include <QRandomGenerator>` |
| `DistortionModelBuilder.cpp:162` | `#if QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR <= 9` | false | `QRandomGenerator prng(0);  // Repeatablity is important.` |
| `DistortionModelBuilder.cpp:169` | same | false | `int i = prng.generate() % numCurves;` and `int j = prng.generate() % numCurves;` |
| `DetectVertContentBounds.cpp:11` | `#if QT_VERSION_MAJOR > 5 || QT_VERSION_MINOR > 9` | true | `#include <QRandomGenerator>` |
| `DetectVertContentBounds.cpp:250` | `#if QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR <= 9` | false | `QRandomGenerator prng(0);  // Repeatablity is important.` |
| `DetectVertContentBounds.cpp:268` | same | false | `auto r = prng.generate();` |

The removed text is `qsrand(0)`, `qrand()` (Qt 5.9 and older). A wrong polarity would swap the RANSAC seed source and change dewarp output; the resulting `git diff` for these two files contains deletions only.

### Pattern 2: Output-equivalence proof for CMake commits

Goldens do not prove a CMake edit changed nothing (a dropped flag could still pass). Compare the complete command set of the build before and after:

```bash
# once, at the start, for each preset build dir that exists (same directory before and after, so no path normalisation)
T="scantailor-advanced pipeline_tests core_tests qt_tests dewarping_tests imageproc_tests math_tests foundation_tests"
ninja -C build/macos-release -t commands $T | sort > build/scratch/cmds-release-baseline.txt
# after a CMake commit and `cmake --preset macos-release`:
ninja -C build/macos-release -t commands $T | sort > build/scratch/cmds-release-after.txt
diff build/scratch/cmds-release-baseline.txt build/scratch/cmds-release-after.txt
```
In the prototype the two lists (481 lines each) differed only by `-DST_QT_COMPILE_VERSION=\"6.12.0\"` on the `test_support` compile lines and by the `CMakeFiles/d/<hash>.d` names of the autogen steps (a hash of the source path; ignore, or normalise with `sed -E 's#CMakeFiles/d/[0-9a-f]{64}\.d#CMakeFiles/d/HASH.d#'`) [VERIFIED: run this session]. That covered: `Qt::Core` to `Qt6::Core`, removal of `${EXTRA_LIBS}`, `WIN32` keyword on `add_executable`, `if (UNIX)` unwrap, `CMAKE_INSTALL_RPATH_USE_LINK_PATH` removal, GNU/MSVC block removal, `PORTABLE_VERSION` removal. Expected intended differences per commit: C0 adds the `ui_snapshot` target (add it to `T` afterwards); C1 adds `ST_QT_COMPILE_VERSION`; every later CMake commit: none.

### Pattern 3: Offscreen snapshot harness (D-12)

Verified behaviour: the harness constructs `Application`, applies the colour scheme and icon pack as `main.cpp` does, creates `MainWindow` (1280x800) and `SettingsDialog`, grabs both widgets, and exits 1 if a `languageBox`/`languageLabel` child or any label containing "Language" exists. Results: baseline tree exit 1, stripped tree exit 0, runtime 1.0 s.

**Isolation is mandatory**: set organisation/application name and `QSettings::setPath(IniFormat, UserScope, <outdir>/settings)` before the `Application` object exists, as `src/qt_tests/main.cpp` does. With the default profile name `"Default"` (`DefaultParamsProvider.cpp:17`), the profile manager never reads the user's `~/Library/Application Support` profile directory [VERIFIED: `DefaultParamsProvider.cpp:12-22` reads a profile file only for names other than "Default"/"Source"], and the isolated settings file starts empty.

The baseline screenshot shows the Language row as a disabled combo with the single entry "English" under "Color Scheme"; the stripped screenshot shows the Color Scheme row directly followed by the group end, with no gap [VERIFIED: both PNGs viewed this session].

### Anti-Patterns to Avoid
- **Resolving conditionals by hand or by regex:** D-11; also hides polarity errors.
- **Running `xcrun clang-format -i` on `src/dewarping/TextLineTracer.cpp`:** the file already violates the style at lines 317 and 328 (`_2 = bind((float (*)(float)) & std::fabs, _1));`); formatting it adds two unrelated hunks. Check it with `--dry-run` and expect exactly those two warnings [VERIFIED].
- **Deleting `src/resources/unix/` before `configure_file(... desktop.in ...)` is gone** (`CMakeLists.txt:357-360`): configure fails (Pitfall 14).
- **Naming a CMake define `ST_QT_VERSION`:** it contains the string `QT_VERSION` and fails criterion 1. Use `ST_QT_COMPILE_VERSION`.
- **Running the `ui_snapshot` harness with the user's real settings or profile directory.**

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Resolving `#if` blocks | sed/regex/python | `unifdef` with `-D`/`-U` | Handles nested conditionals and `#else`; 0 insertions is checkable |
| Compile-time Qt version string | parse headers | `Qt6_VERSION` from `find_package` | Set by the Qt package config; equals `QT_VERSION_STR` byte for byte (`6.12.0`) [VERIFIED] |
| Taking UI screenshots | `screencapture`, Xvfb | Qt offscreen platform + `QWidget::grab()` | No display session, no screen-recording permission, deterministic |
| Next backlog number | counting by hand | `gsd-tools query phase.next-decimal 999 --raw` (returns `999.8`) [VERIFIED], via `/gsd-capture --backlog` | Documented workflow (`add-backlog.md`) |
| Regenerating CLAUDE.md sections | hand edits | `gsd-tools generate-claude-md` after patching `.planning/codebase/*.md` | The `GSD:stack`, `conventions`, `architecture`, `project` sections are generated from those maps; the profile section is not touched |
| Proving a CMake rewrite | reading the diff | `ninja -t commands` diff | Catches dropped flags the tests would not |

## Runtime State Inventory

This is a removal phase, not a rename; the inventory below answers the canonical question "after every file is updated, what runtime systems still carry the removed things".

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | A `language` key may exist in `~/.config/scantailor-advanced/scantailor-advanced.ini` (written by the old `SettingsDialog::commitChanges`, `SettingsDialog.cpp:100`). Not inspected (outside the project, D-07) | None. D-07: left alone, nothing reads or rewrites it. After the strip nothing writes it |
| Live service config | None. No external service references any removed item | None |
| OS-registered state | None. The `.desktop`/MIME files were never installed on this Mac; no `install()` rule was ever run for this fork | None |
| Secrets/env vars | `SCANTAILOR_NO_XCB_FALLBACK` is read only inside the Qt 5 block (`main.cpp:22`); no secret keys | None; the env var simply stops being read |
| Build artifacts | Existing build dirs keep stale outputs the new CMake no longer produces: `build/macos-*/scantailor-advanced_*.qm` (7 files), `scantailor-advanced.desktop`, `CPackConfig.cmake`, `CPackSourceConfig.cmake`, `cmake_uninstall.cmake`, `update_translations_scantailor.pro`, plus the `build/scratch/` dirs | Static checks for "no `.qm`" must run on a fresh dir or fresh clone, or grep `build.ninja`, not `find`. `cmake --fresh --preset <p>` or deleting the preset dir gives a clean state |

## Inventories (current tree, branch `gsd/phase-01-deletion-free-macos-build`, HEAD `e273ca2c`)

### QT_VERSION inventory: 40 lines, 20 files [VERIFIED: `grep -rnI "QT_VERSION" src ...`]

CONTEXT says 39 lines in 19 production files; confirmed, plus `Manifest.cpp:73` (test support) makes 40/20.

| Pattern (quoted) | Mac side | Lines (file:line) |
|------------------|----------|-------------------|
| `#if QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR < 14` (25 lines) | false, keep `#else` | `core/BackgroundTask.h:30,38`; `core/RelinkablePath.cpp:15`; `core/Application.cpp:61` (leaves anyway with C8); `core/filters/output/DewarpingView.cpp:217`; `core/filters/page_split/PageLayoutAdapter.cpp:16,35,71,114`; `core/filters/page_split/PageLayout.cpp:117,354`; `core/filters/page_split/ImageView.cpp:212,244,290,309`; `core/interaction/ZoomHandler.cpp:56`; `app/RelinkablePathVisualization.cpp:56`; `foundation/Proximity.cpp:32`; `math/PolylineIntersector.cpp:90`; `dewarping/TextLineTracer.cpp:443`; `dewarping/CylindricalSurfaceDewarper.cpp:344,367`; `dewarping/DistortionModelBuilder.cpp:299,314`; `dewarping/DetectVertContentBounds.cpp:405` |
| `#if QT_VERSION_MAJOR == 5` (7 lines) | false, keep `#else` | `core/ImageViewBase.cpp:11,530,573`; `core/ImageViewBase.h:250`; `dewarping/Curve.cpp:51,78`; `app/main.cpp:20` |
| `#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)` (1 line) | nested inside `main.cpp:20` block, removed with it | `app/main.cpp:29` |
| `#if QT_VERSION_MAJOR > 5 \|\| QT_VERSION_MINOR > 9` (2 lines, inverted) | true, keep body | `dewarping/DistortionModelBuilder.cpp:9`; `dewarping/DetectVertContentBounds.cpp:11` |
| `#if QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR <= 9` (4 lines) | false, keep `#else` | `dewarping/DistortionModelBuilder.cpp:162,169`; `dewarping/DetectVertContentBounds.cpp:250,268` |
| `QT_VERSION_STR` (1 line, token not a gate) | n/a | `test_support/Manifest.cpp:73`: `values["qt_compile"] = nonEmpty(QString::fromLatin1(QT_VERSION_STR));` |

Behaviour-relevant Qt 6 sides that become permanent (all already what the Mac runs): `event->position()` (`ImageViewBase.cpp:530`, `ZoomHandler.cpp:56`), `enterEvent(QEnterEvent*)` (`ImageViewBase.h:250`, `.cpp:573`), `QIODeviceBase::ReadOnly/WriteOnly` (`Curve.cpp:51,78`), `<QtOpenGLWidgets/QOpenGLWidget>` (`ImageViewBase.cpp:11`), `QRandomGenerator prng(0)` (the two dewarping files). The `main.cpp:20-33` Qt 5 block holds the Wayland-to-xcb fallback, `AA_EnableHighDpiScaling`, `AA_UseHighDpiPixmaps`, and `setHighDpiScaleFactorRoundingPolicy`; Qt 6 does high-DPI scaling by default (Pitfall 7), nothing replaces them.

### Qt 5 and `Qt6_FOUND` in CMake [VERIFIED: grep + Read of `CMakeLists.txt`]

| File:line | Content | Action |
|-----------|---------|--------|
| `CMakeLists.txt:242` | `set(qt_min_version 5.9)` | remove; floor goes into `find_package(Qt6 6.5 ...)` |
| `CMakeLists.txt:244-255` | `find_package(Qt6 COMPONENTS Core Gui Widgets Xml Network LinguistTools OpenGL Svg OpenGLWidgets Test CONFIG)` (no `REQUIRED`) with `Qt5` fallback `REQUIRED`, `QT_BINDIR` | single `find_package(Qt6 6.5 ... CONFIG REQUIRED)`; keep `LinguistTools` until C8 |
| `CMakeLists.txt:257-276` | `Qt_*_lib` variables, Qt 6 branch `Qt::Core` ..., Qt 5 branch `Qt5::Core` ... | remove; 12 users below use `Qt6::X` |
| `CMakeLists.txt:371-375` | `if(Qt6_FOUND) Qt_add_translation(qm_files ...) else() Qt5_add_translation(...)` | C2 keeps one branch; C8 deletes |
| `CMakeLists.txt:406-416` | `if(Qt6_FOUND) set(dep_libs ... Qt::Core ... Qt6::OpenGLWidgets) else() set(dep_libs ... Qt5::Core ...)` inside `if (WIN32)` | C2 resolves; C4 deletes the whole block |
| `CMakeLists.txt:436-464` | `if (NOT Qt6_FOUND)` plugin install block | C2 resolves (drop block); C4 deletes the rest |
| `src/core/CMakeLists.txt:122-135` | `if(Qt6_FOUND) ... PUBLIC Qt::Core Qt::Gui Qt::Widgets Qt::Xml Qt::Network Qt::OpenGL Qt::Svg Qt::OpenGLWidgets ... else() ... Qt5::Core ...` | one `target_link_libraries(core PRIVATE TIFF::TIFF PNG::PNG ZLIB::ZLIB JPEG::JPEG PUBLIC Qt6::Core ... Qt6::OpenGLWidgets imageproc zones ...)` |
| `src/app/CMakeLists.txt:84` | `if (Qt6_FOUND AND WIN32)` windeployqt | C4 deletes |
| `Qt_*_lib` users | `src/foundation/CMakeLists.txt:34` (`Core Xml Gui`); `src/test_support/CMakeLists.txt:6` (`Gui Core`); `src/imageproc/tests/CMakeLists.txt:25` (`Widgets Xml`); `src/dewarping/tests/CMakeLists.txt:6` (`Xml Gui Core`); `src/pipeline_tests/CMakeLists.txt:6` (`Widgets Xml`); `src/qt_tests/CMakeLists.txt:9` (`Test Core Gui Widgets`) | replace each `${Qt_X_lib}` by `Qt6::X` |

`cmake/ListItemsPrepend.cmake` is still used (`src/app/CMakeLists.txt:10`: `list_items_prepend(resource_files "${SCANTAILOR_RESOURCES_DIR}/")`): **keep**. `cmake/SetDefaultBuildType.cmake` is still used (`CMakeLists.txt:95`: `st_set_default_build_type(Release)`) and gives plain `cmake -B x` a Release default: **keep**. After C8 `cmake/` holds exactly these two files.

### Platform macro inventory [VERIFIED: grep, then unifdef run]

| Location | Quoted | Mac side | Commit |
|----------|--------|----------|--------|
| `src/core/Utils.cpp:13` | `#ifdef Q_OS_WIN` (`<windows.h>` / `<stdio.h>`) | else (`stdio.h`) | C5 |
| `src/core/Utils.cpp:21` | `#ifdef Q_OS_WIN` (`MoveFileExW`) | else (`rename(QFile::encodeName(from).data(), QFile::encodeName(to).data()) == 0`) | C5 |
| `src/core/DarkColorScheme.cpp:75`, `LightColorScheme.cpp:75` | `#ifdef _WIN32` (13/4 vs 16/4) | else: `Utils::qssConvertPxToEm(m_styleSheet, 16, 4)` | C5 |
| `src/core/RelinkablePath.cpp:23` | `#if _WIN32` | false | C5 (D-03: the rest of `normalize` stays) |
| `src/app/main.cpp:36` | `#ifdef _WIN32` (`setLibraryPaths`) | false | C5 |
| `src/imageproc/ByteOrder.h:7` | `#ifdef _WIN32` (`winsock2.h` / `netinet/in.h`) | else (`netinet/in.h`) | C5 |
| `src/core/zones/ZoneContextMenuInteraction.cpp:73,141`, `.h:117` | `#ifdef Q_OS_MAC` | true. **There is no `#else`** (CONTEXT D-01 says "`#else` side"); the guarded code is the Mac side and stays, only the guards go | C6 |
| `src/app/MainWindow.cpp:1327` | `#if defined(Q_OS_UNIX)` with `#else QString extPlayCmd;` | true: `QString extPlayCmd("play /usr/share/sounds/freedesktop/stereo/bell.oga");` stays (D-04) | C6 |

Windows-minded code that runs on the Mac and stays (D-03): `RelinkablePath::normalize` from `RelinkablePath.cpp:10` (backslash replacement at line 12: `frontSlashes.replace(QChar('\\'), QLatin1String("/"));`, drive-letter check at line 39: `lastComp.endsWith(QChar(':'))`), `RelinkablePathVisualization.cpp:67-70` (`// That's the Windows \\host\path thing.`), `AtomicFileOverwriter.cpp:39` comment, `QDir::toNativeSeparators` callers. For the 999.8 backlog text: `normalize("/Users/me/scan\\01.tif")` returns `"/Users/me/scan/01.tif"` (a legal macOS name containing a backslash becomes two components) [VERIFIED by reading `RelinkablePath.cpp:11-14`; not executed].

### CMake block inventory with disposition [VERIFIED: Read of `CMakeLists.txt` at HEAD]

| Lines | Content | Disposition |
|-------|---------|-------------|
| 26-62 | `if (NOT DEFAULT_COMPILER_FLAGS_IS_SET)`: 27-46 MSVC/`WIN_XP`/`/D_SECURE_SCL`/`DEBUG_CLI`; 48-52 `CMAKE_COMPILER_IS_GNUCC` incl. `-march=native;-mtune=native`; 54-61 `set(CMAKE_C_FLAGS ... CACHE STRING ... FORCE)` trio + `DEFAULT_COMPILER_FLAGS_IS_SET` | remove 27-52 (C5 MSVC, C6 GNU). Keep the trio and the guard (no-op re-caching of the same values; prototype commands identical) |
| 64-70 | Clang block (`-Wall;-Wextra;-Werror=return-type;-Wno-unused`, `-ffp-contract=off`) | untouched, byte for byte. Its comment "Clang counterpart of the GNU warning set above" becomes stale; leave it (D-01) |
| 72-80 | `SANITIZE` | untouched |
| 82-85 | `if (UNIX) option(CMAKE_POSITION_INDEPENDENT_CODE "" ON) option(CMAKE_INSTALL_RPATH_USE_LINK_PATH "" ON) endif()` | C6: keep the first line unconditionally (`-fPIC` is in the Mac compile lines [VERIFIED: `compile_commands.json` has 338 occurrences]); drop `CMAKE_INSTALL_RPATH_USE_LINK_PATH` (install only; link commands identical in prototype; `LC_RPATH` of the binary is `/opt/homebrew/lib` and comes from the link line) |
| 87-93 | `include()` of SetDefaultBuildType (keep), UpdateTranslations (C8), ListItemsPrepend (keep), `if (WIN32)` CopyToBuildDir + AddDynamicLibraryLocations (C5) | |
| 97-98 | BUILD_TESTS option; comment mentions Flatpak and a test list | C4: reword the comment |
| 100-109 | `CMAKE_RUNTIME_OUTPUT_DIRECTORY` (keep), `CMAKE_INSTALL_BINDIR` (C4) | |
| 113-155 | `if (WIN32)` DEPENDENCIES_ROOT and `*_INSTALL_PREFIX` | C5 |
| 161-163, 169-171, 177-179, 185-190 | `if (WIN32) add_dynamic_library_locations(...)` after each `find_package`; TIFF `USE_LIBTIFF_DLL` | C5 |
| 196-220 | `if (WIN32)` Boost: MinGW, `real_path_find_glob`, `set(Boost_USE_STATIC_LIBS ON)` | C5. **`Boost_USE_STATIC_LIBS` is set only here**, so the Mac links shared Boost and takes the `BOOST_TEST_DYN_LINK` branch |
| 231-238 | `if (BUILD_TESTS) if (NOT Boost_USE_STATIC_LIBS) if (WIN32) add_dynamic_library_locations(...) endif() add_definitions(-DBOOST_TEST_DYN_LINK)` | C5: reduce to `if (BUILD_TESTS) add_definitions(-DBOOST_TEST_DYN_LINK) endif()` (the Mac compile lines contain `-DBOOST_TEST_DYN_LINK` [VERIFIED: command dump]) |
| 281-285 | `if (UNIX)` Threads | C6: unwrap, keep (`link_libraries(Threads::Threads)`) |
| 289-292 | `set(EXTRA_LIBS "")` + Windows libs | C5; remove the 9 `${EXTRA_LIBS}` uses (below) |
| 298-300 | `if (WIN32 AND MSVC) add_definitions(-DNOMINMAX)` | C5 |
| 307-310 | `option(PORTABLE_VERSION ... ON)` | C7 |
| 311, 319-330 | `APPLICATION_NAME`, `configure_file(config.h.in ...)`, `DEVELOPER_VERSION` regex on `ENABLE_DEBUG_FEATURES` | keep unchanged (cross-platform) |
| 312-318 | `TRANSLATION_DIRS`, `PLUGIN_DIRS` (Windows vs else) | C5 resolves `if (WIN32)` to the else side; C8 deletes both |
| 332-349 | `VERSION`/`VERSION_QUAD` extraction with `FATAL_ERROR` guards, `configure_file(version.h.in ... COPYONLY)` | keep |
| 350-352 | `APPLICATION_VERSION_MAJOR/MINOR/PATCH` | their only readers are CPack (502-505) and `src/resources/win32/resources.rc.in:6-7`: delete in C4 |
| 354 | `SCANTAILOR_RESOURCES_DIR` | keep (`src/app/CMakeLists.txt:10`) |
| 355 | `SCANTAILOR_TRANSLATIONS_DIR` | C8 |
| 357-360 | `configure_file("${SCANTAILOR_RESOURCES_DIR}/unix/${target_name}.desktop.in" ... @ONLY)` | **C6, before deleting `src/resources/unix/`** |
| 364-376 | `file(GLOB translation_files ...)`, `finalize_translations`, `update_translations_target`, `Qt_add_translation`, `add_custom_target(compile_translations ALL ...)` | C8 |
| 377-381 | `install(FILES ${qm_files} ...)` | C4 |
| 383-483 | `if (WIN32)` runtime libs, plugins, `copy_to_build_dir`, MinGW | C4 (install) with the remaining WIN32 helpers; includes `Qt5::` lines 412-416 |
| 485-493 | `if (UNIX)` install `.desktop`, `appicon.svg`, MIME xml | C4 |
| 495-499 | `CMAKE_INSTALL_UCRT_LIBRARIES`, `CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION`, `include(InstallRequiredSystemLibraries)` | C4 |
| 501-534 | `CPACK_*`, `include(CPack)` | C4 |
| 536-545 | `uninstall` target from `cmake/cmake_uninstall.cmake.in` | C4 |

`src/app/CMakeLists.txt`: 49-69 (`win32_resource_file`, `.rc`, `windres`) C5; 73 the `WIN32` keyword of `add_executable` and 74 `${win32_resource_file}` C5 (the keyword has no effect on macOS and the binary is not a bundle); 77 `${EXTRA_LIBS}` C5; 82 `install(TARGETS ...)` C4; 84-93 windeployqt C4; 95 `translation_sources(...)` C8.

`${EXTRA_LIBS}` is referenced at [VERIFIED: grep]: `src/app/CMakeLists.txt:77`, `src/core/tests/CMakeLists.txt:18`, `src/pipeline_tests/CMakeLists.txt:6`, `src/foundation/tests/CMakeLists.txt:11`, `src/math/tests/CMakeLists.txt:14`, `src/imageproc/tests/CMakeLists.txt:26`, `src/qt_tests/CMakeLists.txt:9`, `src/dewarping/tests/CMakeLists.txt:7` (8 files; it is empty on macOS).

### Translation removal set (C8) [VERIFIED: grep]

- Files: `src/translations/*.ts` (8 tracked files, 1.3 MB: `de es fr ko pl pt ru untranslated`), `cmake/UpdateTranslations.cmake`.
- `translation_sources(...)` in 10 `CMakeLists.txt`: `src/core/CMakeLists.txt:140`, `src/core/filters/{fix_orientation:22, select_content:28, output:74, page_layout:28, page_split:31, deskew:24}/CMakeLists.txt`, `src/core/zones/CMakeLists.txt:23`, `src/core/interaction/CMakeLists.txt:18`, `src/app/CMakeLists.txt:95`. Several are the last line of the file without a trailing newline (a regex needing `\n` misses them); delete with `sed -i '' '/^translation_sources/d'`.
- Root: `finalize_translations`, `update_translations_target`, `Qt_add_translation`, `compile_translations`, `SCANTAILOR_TRANSLATIONS_DIR`, `TRANSLATION_DIRS`, `PLUGIN_DIRS`, `LinguistTools` component, `include(cmake/UpdateTranslations.cmake)`.
- `config.h.in:6-7` (`TRANSLATION_DIRS`, `PLUGIN_DIRS`). `PLUGIN_DIRS` has no reader anywhere in `src/` [VERIFIED: grep]; `TRANSLATION_DIRS` is read only at `Application.cpp:66`.
- `src/core/Application.h`: `#include <QtCore/QTranslator>` (line 9), `getCurrentLocale` (18), `installLanguage` (20), `getLanguagesList` (22), `initTranslations` (29), `m_translator` (35), `m_currentLocale` (36), `m_translationsMap` (37). `src/core/Application.cpp`: constructor initialiser `m_currentLocale("en")` and `initTranslations()` call (15-16), `installLanguage` (30-47), `getCurrentLocale` (49-51), `getLanguagesList` (53-58), `initTranslations` (60-82); unused includes `<QDir>`, `<QTemporaryDir>` (the latter with C7), `<config.h>` once C7 is done.
- `src/core/ApplicationSettings.h:91,93` (`getLanguage`, `setLanguage`), `:137` (`DEFAULT_LANGUAGE`), `:160` (`LANGUAGE_KEY`); `ApplicationSettings.cpp:28` (`DEFAULT_LANGUAGE = QLocale::system().name()`), `:51` (`LANGUAGE_KEY = "language"`), `:210-216`.
- `src/app/main.cpp:53` (`app.installLanguage(...)`), `src/app/MainWindow.cpp:1537` (`static_cast<Application*>(qApp)->installLanguage(settings.getLanguage());` plus the following blank line).
- `src/app/SettingsDialog.cpp:53-61` (the braced block with `static_cast<Application*>(qApp)` at line 54, `getLanguagesList`, `languageBox` at lines 57, 59, 60) and `:100` (`settings.setLanguage(ui.languageBox->currentData().toString());`), `#include "Application.h"` (line 13).
- `src/app/SettingsDialog.ui:84-110`: the whole `<item>` that wraps `<layout class="QHBoxLayout" name="horizontalLayout_4">` with `languageLabel`, `languageBox` and `horizontalSpacer_4`. Delete the entire `<item>`, not only the widgets; removing only the label and combo leaves an empty layout row.

### `config.h` consumers [VERIFIED: grep of `src`]

| Macro in `config.h.in` | Readers in `src/` | After strip |
|------------------------|-------------------|-------------|
| `TRANSLATION_DIRS` | `Application.cpp:66` | delete (C8) |
| `PLUGIN_DIRS` | none | delete (C8; could go earlier, none needs it) |
| `PORTABLE_CONFIG_DIR` | `Application.cpp:85` | delete (C7) |
| `ENABLE_DEBUG_FEATURES` | `MainWindow.cpp:313` | keep; line must stay `#define ENABLE_DEBUG_FEATURES` because `CMakeLists.txt:327` comments it out with a regex when `DEVELOPER_VERSION` is OFF |
| `APPLICATION_NAME`, `ORGANIZATION_NAME` | `main.cpp:44-45` | keep |

Headers that merely `#include <config.h>` without using a macro (`foundation/NonCopyable.h:8`, `ScopedIncDec.h:8`, `FlagOps.h:8`, `tests/main.cpp` files) need the file to exist, not its content: keep `include_directories(BEFORE "${CMAKE_BINARY_DIR}")`.

### Files to delete (tracked) [VERIFIED: `git ls-files`]

`.github/` (6 files: `dependabot.yml`, `workflows/{ci,cmake,flatpak,lint,release}.yml`), `flatpak/org.scantailor.Advanced.json`, `build-deb.sh`, `build-windows.sh`, `scripts/post_issue_followups.py` (354 lines, `scripts/` then empty), `cmake/AddDynamicLibraryLocations.cmake`, `cmake/CopyToBuildDir.cmake`, `cmake/UpdateTranslations.cmake`, `cmake/cmake_uninstall.cmake.in`, `cmake/copy_to_build_dir.cmake.in`, `src/resources/unix/{scantailor-advanced.desktop.in,mime/scantailor-project.xml}`, `src/resources/win32/{resources.rc.in,scantailor-advanced.ico}`, `src/resources/scantailor-advanced.png`, `src/translations/*.ts` (8). Delete `.github/` as a whole (it contains nothing else). `.gitignore` carries Linux/Windows packaging patterns (`*.deb`, `*.flatpak`, `.flatpak-builder/`, `*.exe` ...); they are harmless, recommend leaving them.

## Common Pitfalls

### Pitfall 1: zsh does not word-split unquoted variables or unmatched globs
**What goes wrong:** `unifdef -m $FILES` receives one argument containing newlines ("can't open src/app/main.cpp\nsrc/app/MainWindow.cpp ..."), and `grep --include=*.cpp` aborts with "no matches found". Both happened in this research [VERIFIED].
**How to avoid:** pipe file lists into `xargs`; quote `--include='*.cpp'`. Executors run in zsh.

### Pitfall 2: `unifdef` exit status 1 means "changed"
**What goes wrong:** `set -e` or `&&` chains stop after a successful run; `xargs` reports 123.
**How to avoid:** judge success by `git diff --numstat` (zero additions before formatting) and by the leftover grep, not by exit status.

### Pitfall 3: Formatting side effects after `unifdef`
**What goes wrong:** `unifdef` leaves cosmetic residue: `BackgroundTask.h` bodies that Chromium style folds to one line (`void cancel() override { m_cancelFlag.storeRelaxed(1); }`), `RelinkablePath.cpp` leaves `if (newComponents.isEmpty()\n      ) {`, `main.cpp` keeps two blank lines.
**How to avoid:** `xcrun clang-format -i` on every touched file except `src/dewarping/TextLineTracer.cpp` (its pre-existing violations at lines 317 and 328 would be reformatted too). In the prototype, formatting touched exactly 4 extra hunks: 1 in `BackgroundTask.h`, 1 in `RelinkablePath.cpp`, 2 pre-existing ones in `TextLineTracer.cpp`. New files (the harness) need the copyright header and `.clang-format` style.

### Pitfall 4: Criterion 1 grep false positives and negatives
**What goes wrong:** `grep -i lupdate` matches `conditionalUpdate` (`page_split/Settings.cpp:124`); a bare `xcb` matches the hex tables `0xcb` in `TiffWriter.cpp:33` and `BitOps.cpp:29`; a define named `ST_QT_VERSION` matches `QT_VERSION`; `Qt 5.4.0` in the comment at `src/core/PixmapRenderer.h:20` does not match `Qt5` and may stay.
**How to avoid:** use the case-sensitive commands with repeated `-e` in "Validation Architecture". Do not use `-i`, do not use `-w` (it applies to every `-e` pattern and would stop `Q_OS_` from matching `Q_OS_MAC`), and do not write alternation as `\|` inside `-E`. Note that `grep` in this environment is ugrep 7.8.4 (`grep --version`), not BSD grep; the repeated-`-e` form behaves the same in both.

### Pitfall 5: Half-removed translation CMake
**What goes wrong:** deleting `cmake/UpdateTranslations.cmake` while any of 10 `translation_sources(...)` calls remain fails configure with "Unknown CMake command".
**How to avoid:** one commit (C8), with `grep -rn "translation_sources\|finalize_translations\|update_translations_target\|Qt_add_translation\|compile_translations" CMakeLists.txt cmake src` empty before building.

### Pitfall 6: Stale preset variable and stale build products
**What goes wrong:** `CMakePresets.json:17` sets `"PORTABLE_VERSION": "OFF"`; after C7 CMake reports a manually specified variable that was not used. Existing build dirs still contain `.qm`, `.desktop`, CPack and uninstall files.
**How to avoid:** remove the preset line in C7; run the "no `.qm`" checks on a fresh directory.

### Pitfall 7: `ctest` hides sanitizer reports of passing tests
**What goes wrong:** `ctest --preset macos-asan --output-on-failure` prints nothing for passing tests, so UBSan lines are invisible; D-14 would then "find" zero reports.
**How to avoid:** run the seven executables directly with `ASAN_OPTIONS=detect_leaks=0:halt_on_error=0 UBSAN_OPTIONS=print_stacktrace=1` as in `records/test-cases-asan.txt` (its header gives the exact commands), or use `ctest -V`.

### Pitfall 8: The "kept" README Features section contains stale portable-mode text
**What goes wrong:** README lines 213-217 say "The settings and program files are stored in the folder with the application" and line 399 says user profiles are stored "in `config/profiles` folder or in an system specific one". Both are false once D-01 removes portable mode. The user's own rule (D-01, D-10) is to patch stale facts.
**How to avoid:** amend those two passages in C9 and record the exception to D-08 (see Open Questions).

### Pitfall 9: The batch alarm must not change
**What goes wrong:** collapsing the `#if defined(Q_OS_UNIX)` also invites tidying `QString cmd = settings.value("main_window/external_alarm_cmd", extPlayCmd)`.
**How to avoid:** C6 diff for `MainWindow.cpp` must be exactly 4 deleted lines (`#if defined(Q_OS_UNIX)`, `#else`, `QString extPlayCmd;`, `#endif`) [VERIFIED: unifdef diff].

### Pitfall 10: `DefaultParamsProfileManager` cast removal changes test code comments only
`static_cast<Application*>(qApp)` in `DefaultParamsProfileManager.cpp:20` disappears with C7 (it was undefined behaviour in programs whose `qApp` is not an `Application`). The comment at `src/pipeline_tests/main.cpp:22` ("not a plain QApplication: DefaultParamsProfileManager casts qApp to it") becomes stale; the fixture still creating `Application` is harmless. Leave the code, optionally fix the comment.

## Code Examples

### Target state of the root `CMakeLists.txt` (prototype, built and tested)
Only the changed regions are shown; everything else (cmake_minimum_required, in-source guard, policies, `project`, Clang block, `SANITIZE`, `DEVELOPER_VERSION` handling, version extraction, `add_subdirectory(src)`) is as today.

```cmake
if (NOT DEFAULT_COMPILER_FLAGS_IS_SET)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}"
      CACHE STRING "Common C flags for all build configurations." FORCE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}"
      CACHE STRING "Common C++ flags for all build configurations." FORCE)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"
      CACHE STRING "Common link flags for all build configurations." FORCE)

  set(DEFAULT_COMPILER_FLAGS_IS_SET YES CACHE INTERNAL "" FORCE)
endif()
# ... Clang block and SANITIZE unchanged ...
option(CMAKE_POSITION_INDEPENDENT_CODE "" ON)

include(cmake/SetDefaultBuildType.cmake)
include(cmake/ListItemsPrepend.cmake)

st_set_default_build_type(Release)

option(BUILD_TESTS "Build the unit and pipeline tests" ON)

if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()

find_package(JPEG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(TIFF REQUIRED)

set(Boost_USE_MULTITHREADED ON)
if (BUILD_TESTS)
  find_package(Boost 1.60 COMPONENTS unit_test_framework REQUIRED)
else()
  find_package(Boost 1.60 REQUIRED)
endif()
include_directories(${Boost_INCLUDE_DIRS})
add_definitions(-DBOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
if (BUILD_TESTS)
  add_definitions(-DBOOST_TEST_DYN_LINK)
endif()

find_package(
    Qt6 6.5
    COMPONENTS Core Gui Widgets Xml Network OpenGL Svg OpenGLWidgets Test
    CONFIG REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

set(CMAKE_AUTOMOC ON)
if (BUILD_TESTS)
  enable_testing()
endif()

set(APPLICATION_NAME "${target_name}")
configure_file(config.h.in "${CMAKE_BINARY_DIR}/config.h" @ONLY)
# ... DEVELOPER_VERSION regex, include_directories(BEFORE ...), VERSION extraction, version.h unchanged ...
set(SCANTAILOR_RESOURCES_DIR "${CMAKE_SOURCE_DIR}/src/resources")

add_subdirectory(src)
```

`config.h.in` after C7 and C8:

```c
/* This file is a template for config.h, for use with CMake. */

#ifndef SCANTAILOR_CONFIG_H_
#define SCANTAILOR_CONFIG_H_

#define ENABLE_DEBUG_FEATURES
#define APPLICATION_NAME "@APPLICATION_NAME@"
#define ORGANIZATION_NAME APPLICATION_NAME

#endif
```

`src/app/CMakeLists.txt` after C4/C5/C8 (executable part):

```cmake
add_executable(
    ${target_name} ${gui_only_sources} ${gui_only_ui_files}
    ${resource_files})
target_link_libraries(
    ${target_name}
    PRIVATE core)
target_include_directories(
    ${target_name}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    "$<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_INCLUDE_DIRECTORIES>")
```

### D-15 token removal

```cmake
# src/test_support/CMakeLists.txt, inside the existing target_compile_definitions(test_support PRIVATE ...)
ST_QT_COMPILE_VERSION="${Qt6_VERSION}"
```
```cpp
// src/test_support/Manifest.cpp, next to the other ST_* fallbacks
#ifndef ST_QT_COMPILE_VERSION
#define ST_QT_COMPILE_VERSION "unknown"
#endif
// ...
values["qt_compile"] = nonEmpty(QString::fromLatin1(ST_QT_COMPILE_VERSION));
```
The produced string is `6.12.0`, identical to the committed manifests (`qt_compile = 6.12.0` in `src/pipeline_tests/golden/{flat,mixed,book}/MANIFEST.txt`) [VERIFIED: manifest files read; `Qt6_VERSION=[6.12.0]` printed; the proto compile line contains `-DST_QT_COMPILE_VERSION=\"6.12.0\"`]. After a Qt upgrade both the old and the new form change together, and a mismatch is a warning only (Phase 2 D-09).

### `ui_snapshot` harness (built and run in the prototype)

```cpp
// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
//
// Offscreen snapshot of the main window and the Settings dialog (Phase 3, D-12).
// usage: ui_snapshot <output-dir> [--assert-no-language]
#include <config.h>
#include <core/Application.h>
#include <core/ApplicationSettings.h>
#include <core/ColorSchemeFactory.h>
#include <core/ColorSchemeManager.h>
#include <core/IconProvider.h>
#include <core/StyledIconPack.h>

#include <QComboBox>
#include <QDir>
#include <QLabel>
#include <QSettings>
#include <cstdio>

#include "MainWindow.h"
#include "SettingsDialog.h"

int main(int argc, char* argv[]) {
  // argument parsing for <output-dir> and --assert-no-language omitted here
  qputenv("QT_QPA_PLATFORM", "offscreen");
  qputenv("LC_ALL", "en_US.UTF-8");
  Application::setApplicationName(APPLICATION_NAME);
  Application::setOrganizationName(ORGANIZATION_NAME);
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, outDir + "/settings");  // never the user's settings
  Application app(argc, argv);
  { std::unique_ptr<ColorScheme> scheme = ColorSchemeFactory().create(ApplicationSettings::getInstance().getColorScheme());
    ColorSchemeManager::instance().setColorScheme(*scheme); }
  IconProvider::getInstance().setIconPack(StyledIconPack::createDefault());
  MainWindow window;  window.resize(1280, 800);  window.show();  QApplication::processEvents();
  window.grab().save(outDir + "/main-window.png");
  SettingsDialog dialog(&window);  dialog.show();  QApplication::processEvents();
  dialog.grab().save(outDir + "/settings-dialog.png");
  // failure condition: dialog.findChild<QComboBox*>("languageBox") || findChild<QLabel*>("languageLabel")
  //                    || any QLabel text containing "Language"
}
```
CMake block (appended to `src/app/CMakeLists.txt`; `main.cpp` is the only app source left out):

```cmake
if (BUILD_TESTS)
  set(snapshot_sources ${gui_only_sources})
  list(REMOVE_ITEM snapshot_sources main.cpp)
  add_executable(ui_snapshot ui_snapshot/main.cpp ${snapshot_sources} ${gui_only_ui_files} ${resource_files})
  target_link_libraries(ui_snapshot PRIVATE core)
  target_include_directories(ui_snapshot PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
      "$<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_INCLUDE_DIRECTORIES>")
endif()
```
Make the "no language control" failure conditional on `--assert-no-language` so the harness also runs on the baseline (exit 0, report `language control present: yes`); the C0 "before" run then needs no failing exit status. `main-window.png` shows the empty-project start screen (menus File/Tools/Help, "New Project ..." and "Open Project ..." panel), which is the only main-window state reachable without a project.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `qsrand()`/`qrand()` | `QRandomGenerator prng(0); prng.generate()` | Qt 5.10 | Already what the Mac runs; seed 0 stays (dewarp determinism) |
| `QString::SkipEmptyParts`, `QLineF::intersect`, `QAtomicInt::store/load` | `Qt::SkipEmptyParts`, `intersects`, `storeRelaxed/loadRelaxed` | Qt 5.14 | Permanent after C1 |
| `QGLWidget` macro hack | `QOpenGLWidget` from `QtOpenGLWidgets` | Qt 6.0 | Hack leaves in C1; the OpenGL viewer itself stays until Phase 6 |
| `AA_EnableHighDpiScaling` | default on in Qt 6 | Qt 6.0 | The Qt 5 block is removed, nothing replaces it (Phase 4 owns Retina) |

**Deprecated/outdated within the removed content:** MXE/MinGW cross-compile notes, Flathub submission notes, `scantailor-libs-build` links in the README.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Every Qt API used by the permanent Qt 6 branches exists since Qt 6.0 or earlier, so a floor of 6.5 is safe and conservative | Standard Stack, Qt floor | Very low: only 6.12.0 can be built here, and Homebrew always provides the newest Qt; a floor too high is not reachable, a floor too low only changes a configure error into a compile error |
| A2 | The user's `language` key (if any) in `~/.config/scantailor-advanced/scantailor-advanced.ini` is harmless once unread | Runtime State Inventory | None to the build; the file was not inspected (outside the project) |
| A3 | `main-window.png` of the empty start screen is a sufficient "main window" check for D-12 (loading a project in the harness is not required) | Code Examples | If the user wants to see a loaded project, a synthetic project from `test_support` would be needed; not required by D-12 wording |

## Open Questions

1. **README Features section vs D-08 "stays unchanged".**
   - What we know: lines 213-217 (Portability) and 399 (profiles in `config/profiles`) describe portable mode, which D-01 removes.
   - What's unclear: whether the user wants them edited or accepts them as historical feature text.
   - Recommendation: edit exactly these two passages in C9 (replace by one sentence each stating the INI location `~/.config/scantailor-advanced/` and profiles under `~/Library/Application Support/scantailor-advanced/profiles`), and note the exception in the plan; D-10 already asks to patch stale facts.

2. **Should `ui_snapshot` become a ctest test?**
   - What we know: D-11 fixes 7/7 suites; the harness takes about 1 s.
   - Recommendation: no for Phase 3 (not registered, built under `BUILD_TESTS`, run by the executor, result in records). Phases 4 and 5 can promote it (TESTX-03 in v2 is related).

3. **Commit the screenshots?**
   - Recommendation: commit "before" and "after" PNGs (11 KB and 30 KB each) under `.planning/phases/03-qt-6-and-macos-only/records/screenshots/{before,after}/`, synthetic UI only.

4. **Stale comment in the untouched Clang block** ("counterpart of the GNU warning set above").
   - D-01 says byte for byte. Recommendation: leave it, mention in the C6 commit message.

5. **`.gitignore`** still lists Linux/Windows packaging patterns. Recommendation: leave (harmless, not in the D-01 list).

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| `unifdef` | C1, C5, C6 | yes | 2.12, `/usr/bin/unifdef` | none needed |
| `xcrun clang-format` | every C++ commit | yes | Apple clang-format 21.0.0 | none; not on `PATH` |
| `xcrun git-clang-format` | would format only changed lines | no | `xcrun: error ... failed with exit code 17664` | `clang-format-diff.py` exists at `.../XcodeDefault.xctoolchain/usr/bin/`; or skip formatting `TextLineTracer.cpp` |
| CMake / Ninja / ctest | gate | yes | 4.4.4 / 1.13.2 | none |
| Qt 6 (`qtbase`, `qtsvg`) | build | yes | 6.12.0_1 | none |
| `qttools` | nothing after C8 | installed, not needed | 6.12.0_1 | n/a |
| Boost, jpeg-turbo, libpng, libtiff | build | yes | 1.92.0, 3.2.0, 1.6.59, 4.7.2 | none |
| Xcode (for `xcodebuild -version` in manifest) | manifest `xcode` key | yes | Xcode 27.1 | manifest value "unknown" (warn-only) |
| `git`, `python3` | records, parsing | yes | | |
| `gsd-tools` | backlog number, `generate-claude-md` | yes | `node ~/.claude/gsd-core/bin/gsd-tools.cjs` | |
| Offscreen Qt platform plugin | harness, qt_tests | yes | ships with `qtbase` | |

Reads by build tools under `/opt/homebrew` and the Xcode/CLT installation are approved for Phases 3-6 (`.planning/phases/02-golden-baselines/records/access-approval.md`). Scratch files go to `build/scratch/` (gitignored), never `/tmp`; the fresh clone for D-13 goes to `build/fresh-clone/` (any new empty folder under `build/`).

**Missing dependencies with no fallback:** none.

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Boost.Test (6 executables) + Qt Test (`qt_tests`), run through CTest presets |
| Config file | `CMakePresets.json` (`macos-release`, `macos-debug`, `macos-asan`), per-directory `CMakeLists.txt` |
| Quick run command | `cmake --build --preset macos-release && ctest --preset macos-release` (about 2 min after a full build; `ctest --preset macos-release -E pipeline_tests` skips the 55 s `pipeline_tests` for C++-only iterations, never for the gate) |
| Full suite command | the Release command above plus `cmake --preset macos-debug && cmake --build --preset macos-debug && ctest --preset macos-debug` (Debug `ctest` 230-270 s) |

### Phase Requirements to Test Map

| Req ID / criterion | Behaviour | Test type | Automated command | Exists? |
|--------------------|-----------|-----------|-------------------|---------|
| SC1 / STRIP-01 | no `Qt5`, no `QT_VERSION` | grep | `grep -rnI -e Qt5 -e QT_VERSION CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in src` prints nothing (exit 1) [prototype of the whole phase: exit 1] | to run each commit |
| SC1 / STRIP-01 | output unchanged | golden | `ctest --preset macos-release` and `macos-debug`, 7/7; `git diff --quiet a796c2ae -- src/pipeline_tests/golden` and `git status --porcelain -- src/pipeline_tests/golden` empty (goldens unchanged since baseline: 0 files differ today) | yes |
| SC1 / D-11 polarity | unifdef commits delete only | diff | `git diff --numstat` additions equal 0 before clang-format | yes |
| SC2 / STRIP-02, 03 | no Windows/Linux code or options | grep | `grep -rnI -e _WIN32 -e Q_OS_ -e WIN32 -e MSVC -e MINGW -e __APPLE__ -e UNIX -e PORTABLE -e TRANSLATION_DIRS -e PLUGIN_DIRS -e LinguistTools -e lupdate -e lrelease -e QTranslator -e XDG_SESSION_TYPE -e SCANTAILOR_NO_XCB_FALLBACK -e Wayland -e wayland CMakeLists.txt CMakePresets.json cmake config.h.in src` prints nothing at the end of the phase (prototype: exit 1, no output; the same command prints 60 lines on today's tree with `--exclude-dir=translations`, so it can fail). Use repeated `-e`, not `\|`, and no `-w` (it applies to every pattern) | to run |
| SC2 / STRIP-04 | no packaging files | file test + grep | `test ! -e flatpak -a ! -e build-deb.sh -a ! -e build-windows.sh -a ! -e scripts -a ! -e src/resources/unix -a ! -e src/resources/win32 -a ! -e cmake/cmake_uninstall.cmake.in`; and `grep -rnI -e 'install(' -e CPack -e CPACK -e uninstall CMakeLists.txt cmake src --include=CMakeLists.txt --include='*.cmake'` empty | to run |
| C8 atomicity | no half-removed translation CMake | grep | `grep -rnI -e translation_sources -e finalize_translations -e update_translations_target -e Qt_add_translation -e compile_translations CMakeLists.txt cmake src --include=CMakeLists.txt --include='*.cmake'` empty before building C8 | to run |
| SC2 | clean build configures, builds, passes | fresh | `cmake --fresh --preset macos-release` (or delete the preset dir), build, `ctest`; the D-13 fresh clone covers it end to end | yes |
| SC3 / STRIP-05 | no CI | file test | `test ! -e .github && test -z "$(git ls-files .github)"` | to run |
| SC4 / STRIP-06 | no language control in the dialog | static | `grep -c languageBox src/app/SettingsDialog.ui` is 0; in a fresh build `grep -c languageBox <build>/src/app/scantailor-advanced_autogen/include/ui_SettingsDialog.h` is 0 (baseline: 4) | yes |
| SC4 | no translator code in the binary | static | `nm -u build/macos-release/scantailor-advanced` piped to `grep -ci translator` gives 0 (baseline: 5, e.g. `__ZN11QTranslator4loadERK7QStringS2_S2_S2_`); `strings build/macos-release/scantailor-advanced` piped to `grep -c 'scantailor_\*\.qm'` gives 0 (baseline: 1) | yes |
| SC4 | no `.qm` target | static | `grep -ci -e '\.qm' -e lrelease -e lupdate -e translations <build>/build.ninja` gives 0 (baseline: 40); in a fresh dir `find <build> -name '*.qm'` prints nothing | yes |
| SC4 | UI renders without gap, English only | harness + view | `build/macos-release/ui_snapshot build/scratch/ui-after --assert-no-language` exits 0; Read both PNGs; verdict in records | to build (C0) |
| SC5 / STRIP-08 | README alone suffices | fresh clone | `git clone . build/fresh-clone` then only README commands: `cmake --preset macos-release && cmake --build --preset macos-release && ctest --preset macos-release` (about 2 min); plus `brew list --versions cmake ninja boost qtbase qtsvg jpeg-turbo libpng libtiff` shows all seven | to run |
| D-15 | manifest unchanged | golden | `qt_compile = 6.12.0` equals `Qt6_VERSION`; no `MANIFEST.txt` modified (`git status --porcelain -- src/pipeline_tests/golden`) | yes |
| D-14 | no new sanitizer location | ASan | build `macos-asan`, run the seven executables directly with the test-preset environment, extract the locations with `grep -h -e 'runtime error:' -e 'ERROR: AddressSanitizer'` and compare file:line with the 9 in `sanitizer-findings.md`; none of the 5 files carrying them (`BinaryImage.cpp`, `Despeckle.cpp`, `InfluenceMap.cpp`, `EstimateBackground.cpp`, `SlicedHistogram.cpp`) is edited by this phase, so line numbers stay stable | to run |
| CMake commits | build commands unchanged | command diff | `ninja -t commands <8 targets>` before/after (Pattern 2) | to run |
| D-03 | backlog item filed | file | `grep -n "Phase 999.8" .planning/ROADMAP.md` | to run |
| D-10 | agent docs current | grep | `grep -rniI -e qt5 -e 'qt 5' -e flatpak -e nsis -e cpack -e windows -e linux -e translation -e portable -e .github .planning/codebase .claude/CLAUDE.md` shows only deliberate historical mentions | to run |

### Sampling Rate
- **Per task commit:** the quick command for C++-only edits; for any CMake edit add the command diff.
- **Per strip commit (D-11):** full Release and Debug gate, goldens-unchanged check, leftover greps.
- **Phase gate:** fresh clone + ASan comparison + screenshots + all greps empty before `/gsd-verify-work`.

### Wave 0 Gaps
- [ ] `src/app/ui_snapshot/main.cpp` and its CMake block (C0), covers SC4 screenshots and the dialog assertion
- [ ] `build/scratch/cmds-*-baseline.txt` (baseline `ninja -t commands` lists for Release and Debug), taken on the unmodified tree before C1
- [ ] "before" artefacts: `nm -u` translator count (5), `strings` count (1), `build.ninja` count (40), `ui_SettingsDialog.h` count (4), baseline PNGs
- No test framework install is needed.

## Security Domain

`security_enforcement` is enabled in `.planning/config.json` (ASVS level 1, block on high). The phase deletes code and adds one test-only executable; there is no new input surface.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication / V3 Session / V4 Access Control | no | no such code |
| V5 Input Validation | no new input | the harness takes one output-directory argument from the executor only |
| V6 Cryptography | no | none |
| V10 Malicious Code / V14 Configuration | yes | removing CI and Dependabot removes automation with repository secrets and write tokens; removing `install()`/CPack removes scripts that run `windeployqt`/`dpkg`; no new external code is added |

### Known Threat Patterns

| Pattern | STRIP/STRIDE | Standard Mitigation |
|---------|--------------|---------------------|
| Harness or tests touching the user's real settings and profile directories | Tampering / Information disclosure | `QSettings::setPath` into the output dir before the `Application` object; default profile name avoids reading the profile directory (`DefaultParamsProvider.cpp:17`) |
| Leaving a `language` key rewritten or deleted | Tampering | D-07: nothing writes it after C8; `SettingsDialog::commitChanges` no longer calls `setLanguage` |
| Shell execution in the batch alarm (`std::system(cmd.toStdString().c_str())`, `MainWindow.cpp:1337`) | Elevation of privilege | Pre-existing and explicitly kept (D-04); replaced by the system beep in Phase 5 (MAC-10, Pitfall 18). Do not touch in this phase |
| Fresh clone under `/tmp` or reading outside the project | Information disclosure | clone to `build/fresh-clone/`; reads by build tools under `/opt/homebrew` and Xcode only (approved) |
| Mass deletion hitting needed files | Denial of service | one category per commit, gate per commit, `git` history for recovery |

## Sources

### Primary (HIGH confidence)
- The repository tree at HEAD `e273ca2c`: `CMakeLists.txt`, `config.h.in`, `version.h.in`, `CMakePresets.json`, `cmake/*`, `src/**/CMakeLists.txt`, the 26 touched C++ files, `README.md`, `TESTING.md`, `.github/*`, `.gitignore`, `.claude/CLAUDE.md`
- `.planning/phases/03-qt-6-and-macos-only/03-CONTEXT.md`, `.planning/REQUIREMENTS.md`, `.planning/STATE.md`, `.planning/research/PITFALLS.md` (Pitfalls 7, 14, 25), Phase 2 records (`ctest-phase2.md`, `sanitizer-findings.md`, `test-cases-asan.txt`, `access-approval.md`)
- Experiments run this session in a scratch copy: `unifdef` runs and diffs; stripped build in Release and Debug with all `qttools` CMake packages ignored; `ctest` 7/7 in both; `ninja -t commands` comparison; clang macro probe; `find_package(Qt6 <version>)` floor test; `ui_snapshot` build and run on baseline and stripped trees; `xcrun clang-format --dry-run` on all touched files
- `~/.claude/gsd-core/workflows/add-backlog.md`, `~/.claude/gsd-core/bin/lib/profile-output.cjs` (`cmdGenerateClaudeMd`), `gsd-tools query phase.next-decimal 999 --raw` (returned `999.8`)

### Secondary (MEDIUM confidence)
- none

### Tertiary (LOW confidence)
- Qt API "since" versions behind the 6.5 floor (A1): training knowledge, not checked against Qt docs this session

## Metadata

**Confidence breakdown:**
- Standard stack and tooling: HIGH, each tool version read from the machine
- Architecture (commit order, dependencies between commits): HIGH, the end state was built and tested; individual intermediate states were reasoned, not each built (the planner's gate will confirm them)
- Pitfalls: HIGH, most were hit during the prototype
- Qt version floor: MEDIUM-LOW (A1), effect negligible

**Scratch material:** the prototype tree lives in the session scratchpad and is not durable; all load-bearing results are quoted above.

**Research date:** 2026-10-24
**Valid until:** 2026-11-24 (toolchain-bound: a `brew upgrade` of Qt or Boost invalidates the timings and the golden manifests)
