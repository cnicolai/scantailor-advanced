# Phase 3: Qt 6 and macOS Only - Context

**Gathered:** 2026-10-24
**Status:** Ready for planning

<domain>
## Phase Boundary

The phase removes everything the Mac build does not need, and output stays the same. The following go:
- the Qt 5 CMake branches and every `QT_VERSION` gate
- the Windows and Linux code paths, build options and files
- packaging: Flatpak, DEB, NSIS/CPack, `build-*.sh` and the MXE notes
- the `.github/` workflows and the Dependabot configuration
- translations, so the UI is English only

The README is rewritten so that it alone is enough to build the app and run the tests on the Mac. Every strip commit comes after golden baseline `a796c2ae` and passes the full suite with all goldens unchanged. Requirements: STRIP-01 to STRIP-06 and STRIP-08.

Not in this phase:
- the OpenGL viewer (STRIP-07, Phase 6)
- the `play` command of the batch-finished alarm (MAC-10, Phase 5)
- Retina work (Phase 4)
- any change in how the macOS build behaves, including fixes to Windows-minded logic that runs on the Mac (D-03)
- any change to the application name, organisation name or settings location

</domain>

<decisions>
## Implementation Decisions

### Strip depth
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

### Translations
- **D-05:** These go together, so that the CMake is never left half-edited (PITFALLS Pitfall 25):
  - `src/translations/*.ts`, `cmake/UpdateTranslations.cmake`, the `compile_translations` and `update_translations` targets and the `LinguistTools` component
  - `Application::installLanguage`, `getLanguagesList`, `m_translator` and `m_translationsMap`
  - `ApplicationSettings::getLanguage` and `setLanguage`, and the calls in `src/app/main.cpp:53` and `src/app/MainWindow.cpp:1537`
  - the Language row (`languageBox` and its label) in `src/app/SettingsDialog.ui` and `SettingsDialog.cpp`

  No `.qm` file is built or loaded.
- **D-06:** The `tr()` and `QCoreApplication::translate` calls stay, and the `.ui` strings stay translatable. With no translator installed they return the English text unchanged. Stripping about 220 calls in 42 files plus every `.ui` form would make a large diff with no visible effect, and the project convention keeps `tr()` on user-visible strings. Source strings are not edited.
- **D-07:** A `language` value that already sits in the user's settings file is left alone: no code reads it, deletes it or rewrites it. Nothing in this phase writes to the user's settings.

### README and test docs
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

### Verification
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

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase scope and requirements
- `.planning/ROADMAP.md` § Phase 3: goal and the five success criteria
- `.planning/REQUIREMENTS.md`: STRIP-01 to STRIP-06 and STRIP-08. STRIP-07 (OpenGL) is Phase 6 and MAC-10 (alarm) is Phase 5; both are out of scope here
- `.planning/PROJECT.md`: constraints (output must not change unintentionally) and the Key Decisions table (Qt 6 only, macOS only, disable CI, English-only UI)
- `.planning/STATE.md` § Blockers/Concerns: the Phase 3 gate (after `a796c2ae`, ctest 7/7 with goldens unchanged) and the `unifdef` and polarity warning

### Prior phase decisions and records
- `.planning/phases/01-deletion-free-macos-build/01-CONTEXT.md`: D-01/D-02 with the storage amendment (settings INI under `~/.config`, profiles under Application Support), D-03 (`-ffp-contract=off`), D-08 (Clang warnings), D-09 (Homebrew package set, never `qt` or `jpeg`)
- `.planning/phases/01-deletion-free-macos-build/records/toolchain.txt`: toolchain commands and versions
- `.planning/phases/02-golden-baselines/02-CONTEXT.md`: D-06, D-07 and D-10 (golden update rules), D-08 (artifact folder), D-09 (manifest warn-only), D-11 (Release and Debug must pass)
- `.planning/phases/02-golden-baselines/records/closing-check.md`: baseline commit `a796c2ae`
- `.planning/phases/02-golden-baselines/records/sanitizer-findings.md`: the ASan/UBSan reference list for D-14
- `.planning/phases/02-golden-baselines/records/ctest-phase2.md`: the 7/7 suite state that Phase 3 must keep
- `.planning/phases/02-golden-baselines/records/access-approval.md`: build-tool reads under `/opt/homebrew` and Xcode/CLT, approved for Phases 3–6

### Research
- `.planning/research/PITFALLS.md`: Pitfall 7 (Qt 5 removal activates the untested path; `unifdef`; deletion order; leftover-grep check), Pitfall 14 (platform deletions breaking shared configure: the `.desktop.in` `configure_file`, `PORTABLE_VERSION`, `TRANSLATION_DIRS`/`PLUGIN_DIRS`, `ByteOrder.h`, clang-format enforcement), Pitfall 18 (alarm, for the D-04 boundary), Pitfall 25 (translations after the strip)
- `.planning/research/SUMMARY.md` § Phase order item 3 and the P2 strip deliverables list

### Codebase and docs
- `.planning/codebase/STACK.md`, `STRUCTURE.md`, `INTEGRATIONS.md`, `CONVENTIONS.md`, `ARCHITECTURE.md`, `TESTING.md`, `CONCERNS.md`: the maps to patch under D-10
- `.claude/CLAUDE.md`: GSD-managed sections to regenerate under D-10
- `README.md`: Features section (kept), build and fork sections (replaced)
- `TESTING.md`: rewritten under D-09
- `CMakePresets.json`: the `macos-release`, `macos-debug` and `macos-asan` configure, build and test presets the README must use

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `CMakePresets.json` already defines every command the README needs: configure, build and test presets for release, debug and ASan.
- `src/pipeline_tests` and `src/test_support` (Phase 2) give the per-commit output proof: image and text goldens plus the manifest.
- `.planning/phases/02-golden-baselines/records/sanitizer-findings.md` has the format and reference list for the D-14 comparison.

### Established Patterns
- Qt 5 gates: 39 `QT_VERSION` lines in 19 production files (`src/core`, `src/dewarping`, `src/foundation`, `src/math`, `src/app`), all Qt 5 only, mostly `QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR < 14` (`QString::KeepEmptyParts` and similar). `DetectVertContentBounds.cpp:11` and `DistortionModelBuilder.cpp:9` use the inverted form `QT_VERSION_MAJOR > 5 || QT_VERSION_MINOR > 9`. Watch the polarity here.
- `src/app/main.cpp:20-34`: the Qt 5 block holds the Wayland-to-xcb fallback (`SCANTAILOR_NO_XCB_FALLBACK`), `AA_EnableHighDpiScaling`, `AA_UseHighDpiPixmaps` and the rounding policy. All of it goes with Qt 5. `main.cpp:36` has a `_WIN32` library-path block.
- Windows branches: `src/core/Utils.cpp:13,21` (`MoveFileExW`; the `rename()` side stays), `src/imageproc/ByteOrder.h:7` (`winsock2.h`; `netinet/in.h` stays), `src/core/{Dark,Light}ColorScheme.cpp:75` (px-to-em base 13 on Windows; 16 stays), `src/core/RelinkablePath.cpp:23`, `src/app/CMakeLists.txt:50-90` (`.rc` resource, `windres`, `WIN32` executable flag).
- `CMakeLists.txt` (approximate line ranges at `a796c2ae`): `if (UNIX)`/`if (WIN32)` blocks around 82–113, 161–235, 281–320 and 377–500, the Qt 5 branch around 242–275, translations around 355–381, and CPack around 502–534.
- `config.h.in` defines `PORTABLE_CONFIG_DIR`, `TRANSLATION_DIRS` and `PLUGIN_DIRS`.

### Integration Points
- Settings dialog: `src/app/SettingsDialog.ui:94` (`languageBox`) and `src/app/SettingsDialog.cpp:55,100`.
- Translation loading: `src/core/Application.{h,cpp}` (constructor scan at `Application.cpp:61-80`, `installLanguage` at 30) and `ApplicationSettings.cpp:210-216`.
- Settings store: `src/app/main.cpp:47` sets the INI format, which stays. Only the portable `setPath` branch at 48–50 goes.
- Test manifest: `src/test_support/Manifest.cpp:73` (`QT_VERSION_STR`, D-15).

</code_context>

<specifics>
## Specific Ideas

- The user's principle for this phase, in their words: "Everything that never runs on the Mac shall go."
- The user does not want manual verification steps, consistent with Phase 2. Proof comes from `ctest`, grep checks, Claude's own look at offscreen screenshots and committed records. The end-of-phase report is a short written summary.
- No UI design contract is needed (`UI hint: no`). The only visible change is the removed Language row in Settings.

</specifics>

<deferred>
## Deferred Ideas

- **Backslash and drive-letter handling in `RelinkablePath::normalize`.** On macOS a file name may contain `\`, which the normaliser turns into a path separator. Filed as a backlog item during Phase 3 (D-03) and to be fixed with a test in a later phase.
- **Local clang-format enforcement** (a `format` target or a pre-commit hook) to replace the removed lint workflow. That is a new capability, so Phase 3 only documents the manual command (D-09).

</deferred>

---

*Phase: 03-qt-6-and-macos-only*
*Context gathered: 2026-10-24*
