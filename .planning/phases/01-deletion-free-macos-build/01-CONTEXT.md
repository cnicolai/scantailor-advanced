# Phase 1: Deletion-Free macOS Build - Context

**Gathered:** 2026-10-18
**Status:** Ready for planning

<domain>
## Phase Boundary

The unmodified ScanTailor Advanced 1.2.1 code (commit `5eaac18`) builds with Apple Clang as a native arm64 binary against Homebrew Qt 6, Boost, jpeg-turbo, libpng and libtiff. It runs from the build directory and takes a project through all six stages to written output. The phase also records the starting state: existing `ctest` results, the warning wave, ASan/UBSan findings, toolchain versions and the frozen floating-point setting.

Allowed edits: build and launch fixes plus added build settings (CMake minimum/policy bump, Boost config discovery, Clang warning flags, sanitizer option, `CMakePresets.json`). No code paths are deleted. Qt 5, Windows and Linux code stays until Phase 3. Requirements: BUILD-01 to BUILD-07.

</domain>

<decisions>
## Implementation Decisions

### Settings location
- **D-01:** The presets set `PORTABLE_VERSION=OFF`, so the app uses the standard macOS locations: a QSettings plist in `~/Library/Preferences` and profiles in `~/Library/Application Support`, not `<build dir>/config`. This is a CMake option, not a code change, so it stays within the deletion-free rule. Settings must survive deleting the build directory (needed after `brew upgrade boost`) and be shared by the release, debug and ASan builds.
- **D-02:** Before the first launch, the executor asks the user for permission to list ScanTailor entries in `~/Library/Preferences` and `~/Library/Application Support`, which may be left over from an earlier failed build attempt. It reports what it finds, and the user decides whether to keep it or move it aside. Nothing is deleted. (The global rule is to ask before any access outside the project directory.)

### Floating-point rounding
- **D-03:** Every preset (release, debug, ASan) builds with `-ffp-contract=off`, and the setting is recorded in the phase records. The Mac build is the reference: no older x86 output has to be matched. The reason for `off` is that results then do not depend on build type, optimisation level or code layout, so the Phase 2 baselines survive refactors such as the Phase 6 `OutputGenerator.cpp` split. — **Reversibility:** costly — once Phase 2 records baselines, changing this setting means re-recording and re-validating every golden image.

### End-to-end smoke test (BUILD-03)
- **D-04:** Input is a few throwaway generated pages in the session scratchpad, never committed: at least a book-spread-like page, a flat text page and a mixed text-and-image page. No personal scans. They are quick to generate; Phase 2 builds the proper seeded generator.
- **D-05:** Smoke-run depth:
  1. Create a project from the generated pages and batch-process all six stages with defaults.
  2. On one page each, switch the output mode (black-and-white, colour/greyscale, mixed), turn dewarping on, and draw one picture zone.
  3. Check that the output TIFFs open in Preview.
  4. Save the project, close it, and reopen it.
  The user drives the GUI. The executor walks them through it one step at a time and says what to look for at each step.
- **D-06:** The smoke run also observes, and only logs, issues that later phases fix: blur on Retina, Ctrl-based shortcuts and odd menus, and the batch-finished alarm (`src/app/MainWindow.cpp:1337` runs a Linux `play` command through `std::system`). It also records the measured device pixel ratio, which sets the Phase 4 Retina scope (HIDPI-01). The pixel ratio is read without committed code changes, for example with Qt logging such as `QT_LOGGING_RULES="qt.highdpi.*=true"` or a probe kept in the scratchpad. Findings go into a phase findings file. Nothing is fixed.

### Fix boundary and setup
- **D-07:** Runtime failures that block BUILD-03 may get a fix only if the platform or toolchain causes them, for example a libc++ difference, undefined behaviour that Clang exposes, or a `Q_OS_UNIX` branch that picks Linux behaviour on macOS. The fix must be the smallest change that works, in its own commit, with the reason in the message. **If a fix would change processing arithmetic or image output, the executor stops and asks the user first.**
- **D-08:** The executor enables `-Wall -Wextra` (plus `-Werror=return-type -Wno-unused`, matching the existing GCC set) for AppleClang/Clang. It saves the full warning log and a count per warning type, and makes no source edits for warnings. No global `-Werror`.
- **D-09:** The executor installs the missing tools itself with `brew install cmake ninja boost qtbase qtsvg qttools` and records the installed versions (`brew list --versions`, `clang --version`, the CLT/Xcode version, `cmake --version`, the Qt build string). It never installs the `qt` meta-formula or `jpeg`, which conflicts with jpeg-turbo. zlib comes from the SDK. Already installed: jpeg-turbo, libpng, libtiff.

### Claude's Discretion
- **Where the records live** (initial `ctest` pass/fail per test, warning log and counts, sanitizer findings, toolchain versions, smoke findings, DPR measurement). They must be committed. Suggested location: the phase directory, so that `git diff 5eaac18` outside `.planning/` shows only build changes (success criterion 5).
- **Preset layout:** the suggested set is `macos-release`, `macos-debug` and `macos-asan` configure presets plus matching test presets, schema v6, Ninja, `CMAKE_PREFIX_PATH=/opt/homebrew`, `CMAKE_EXPORT_COMPILE_COMMANDS=ON`. The preset with ASan/UBSan builds and runs the existing tests (`ASAN_OPTIONS=detect_leaks=0`; Apple Clang has no LeakSanitizer).
- **CMake minimum:** research recommends 3.25 with `CMP0167` NEW for `BoostConfig.cmake`. Confirm at the first configure, and bump only as far as needed.
- **Commit granularity:** one commit per error class or fix, so each edit can be reviewed against `5eaac18`.
- Whether `qt_tests` needs `QT_QPA_PLATFORM=offscreen` to run under `ctest` in Phase 1. Running it this way is a test setting, not a code change. If it needs a display, record that.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase scope and requirements
- `.planning/ROADMAP.md` § Phase 1: goal and the five success criteria (arm64 with `file`, `otool -L` showing one each of libjpeg/libpng/libtiff, committed `ctest` results, `-ffp-contract` and sanitizer, `git diff 5eaac18` showing only build fixes)
- `.planning/REQUIREMENTS.md`: BUILD-01 to BUILD-07
- `.planning/PROJECT.md`: constraints (Homebrew only, output must not change, synthetic test data only) and the Key Decisions table
- `.planning/STATE.md` § Blockers/Concerns: open Phase 1 items (`-ffp-contract`, `PORTABLE_VERSION`, CMake minimum, possible launch fixes with every edit logged)

### Toolchain and build research
- `.planning/research/STACK.md`: Homebrew package set and versions, CMake 3.25/CMP0167, Boost config discovery, Clang warning flags, sanitizer preset, bundle-vs-bare-executable analysis, open items for the first configure
- `.planning/research/SUMMARY.md`: phase-order rationale (deletion-free bring-up before goldens) and the list of edits allowed in Phase 1
- `.planning/research/PITFALLS.md`: platform defaults (`Q_OS_UNIX` alarm, `PORTABLE_VERSION`), smallest diffs, one commit per error class
- `.planning/research/ARCHITECTURE.md`: build order and seams

### Codebase maps
- `.planning/codebase/STACK.md`, `.planning/codebase/TESTING.md`, `.planning/codebase/CONCERNS.md`
- `TESTING.md` (repo root): existing test run instructions

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- Existing CTest setup: `add_test` in `src/{core,foundation,math,imageproc}/tests/CMakeLists.txt` and `src/qt_tests/CMakeLists.txt` (five executables). `BUILD_TESTS` defaults to ON.
- `DEVELOPER_VERSION` already defaults to ON for Debug builds (`CMakeLists.txt:298-306`).
- `BOOST_TEST_DYN_LINK` is already added when `Boost_USE_STATIC_LIBS` is off (`CMakeLists.txt:209-215`).

### Established Patterns
- `CMakeLists.txt:1` has `cmake_minimum_required(VERSION 3.9)`. CMake 4.x warns below 3.10, and CMP0167 is unset.
- `CMakeLists.txt:44-47`: warnings and `-march=native` apply only under `CMAKE_COMPILER_IS_GNUCC`, so Apple Clang currently builds with no warnings. Add the Clang flags beside this block, never by removing it.
- `CMakeLists.txt:197-207`: Boost is found through the old FindBoost module plus `include_directories(${Boost_INCLUDE_DIRS})`. `BoostConfig.cmake` may not define `Boost_INCLUDE_DIRS`, so check this at the first configure.
- `CMakeLists.txt:285-288` and `src/core/Application.cpp:84-95`: `PORTABLE_VERSION` ON sets `PORTABLE_CONFIG_DIR "config"` relative to `applicationDirPath()`. With OFF the define is empty, `initPortableVersion()` returns early, and QSettings uses the native store (`APPLICATION_NAME`/`ORGANIZATION_NAME` from `src/app/main.cpp:44-45`, `config.h.in:12`).
- `CMakeLists.txt:290-295`: on non-WIN32, `TRANSLATION_DIRS` is `.:translations:../share/...`, relative to the build-dir binary.

### Integration Points
- New file: `CMakePresets.json` at the repository root.
- `src/app/MainWindow.cpp:1337`: batch alarm through `std::system`. Only observe it in Phase 1; Phase 5 (MAC-10) fixes it.

</code_context>

<specifics>
## Specific Ideas

- The user is the one who runs the GUI. Smoke-test instructions must come one step at a time, and each step names the concrete thing to check (the global interaction rule).
- Toolchain versions must be captured before the first build so that Phase 2's `MANIFEST.txt` can reuse them.

</specifics>

<deferred>
## Deferred Ideas

None. The discussion stayed within the phase scope. Mac issues found during the smoke run are logged for Phases 4 to 6, not fixed here.

</deferred>

---

*Phase: 01-deletion-free-macos-build*
*Context gathered: 2026-10-18*
