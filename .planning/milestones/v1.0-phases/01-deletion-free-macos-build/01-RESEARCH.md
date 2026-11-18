# Phase 1: Deletion-Free macOS Build - Research

**Researched:** 2026-10-18
**Domain:** Bringing a C++17/Qt/Boost/CMake desktop app (ScanTailor Advanced 1.2.1, commit `5eaac18`) up on Apple Silicon with Apple Clang and Homebrew, plus recording the starting state (ctest, warnings, sanitizers, FP setting)
**Confidence:** MEDIUM-HIGH. Toolchain, Homebrew and CMake mechanics are verified on this machine or against official docs. The first real compile has not happened (cmake, ninja, Qt and Boost are not installed), so the list of compile errors stays a prediction.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Settings location**
- **D-01:** The presets set `PORTABLE_VERSION=OFF`, so the app uses the standard macOS locations: a QSettings plist in `~/Library/Preferences` and profiles in `~/Library/Application Support`, not `<build dir>/config`. This is a CMake option, not a code change, so it stays within the deletion-free rule. Settings must survive deleting the build directory (needed after `brew upgrade boost`) and be shared by the release, debug and ASan builds.
- **D-02:** Before the first launch, the executor asks the user for permission to list ScanTailor entries in `~/Library/Preferences` and `~/Library/Application Support`, which may be left over from an earlier failed build attempt. It reports what it finds, and the user decides whether to keep it or move it aside. Nothing is deleted. (The global rule is to ask before any access outside the project directory.)

**Floating-point rounding**
- **D-03:** Every preset (release, debug, ASan) builds with `-ffp-contract=off`, and the setting is recorded in the phase records. The Mac build is the reference: no older x86 output has to be matched. The reason for `off` is that results then do not depend on build type, optimisation level or code layout, so the Phase 2 baselines survive refactors such as the Phase 6 `OutputGenerator.cpp` split. — **Reversibility:** costly — once Phase 2 records baselines, changing this setting means re-recording and re-validating every golden image.

**End-to-end smoke test (BUILD-03)**
- **D-04:** Input is a few throwaway generated pages in the session scratchpad, never committed: at least a book-spread-like page, a flat text page and a mixed text-and-image page. No personal scans. They are quick to generate; Phase 2 builds the proper seeded generator.
- **D-05:** Smoke-run depth:
  1. Create a project from the generated pages and batch-process all six stages with defaults.
  2. On one page each, switch the output mode (black-and-white, colour/greyscale, mixed), turn dewarping on, and draw one picture zone.
  3. Check that the output TIFFs open in Preview.
  4. Save the project, close it, and reopen it.
  The user drives the GUI. The executor walks them through it one step at a time and says what to look for at each step.
- **D-06:** The smoke run also observes, and only logs, issues that later phases fix: blur on Retina, Ctrl-based shortcuts and odd menus, and the batch-finished alarm (`src/app/MainWindow.cpp:1337` runs a Linux `play` command through `std::system`). It also records the measured device pixel ratio, which sets the Phase 4 Retina scope (HIDPI-01). The pixel ratio is read without committed code changes, for example with Qt logging such as `QT_LOGGING_RULES="qt.highdpi.*=true"` or a probe kept in the scratchpad. Findings go into a phase findings file. Nothing is fixed.

**Fix boundary and setup**
- **D-07:** Runtime failures that block BUILD-03 may get a fix only if the platform or toolchain causes them, for example a libc++ difference, undefined behaviour that Clang exposes, or a `Q_OS_UNIX` branch that picks Linux behaviour on macOS. The fix must be the smallest change that works, in its own commit, with the reason in the message. **If a fix would change processing arithmetic or image output, the executor stops and asks the user first.**
- **D-08:** The executor enables `-Wall -Wextra` (plus `-Werror=return-type -Wno-unused`, matching the existing GCC set) for AppleClang/Clang. It saves the full warning log and a count per warning type, and makes no source edits for warnings. No global `-Werror`.
- **D-09:** The executor installs the missing tools itself with `brew install cmake ninja boost qtbase qtsvg qttools` and records the installed versions (`brew list --versions`, `clang --version`, the CLT/Xcode version, `cmake --version`, the Qt build string). It never installs the `qt` meta-formula or `jpeg`, which conflicts with jpeg-turbo. zlib comes from the SDK. Already installed: jpeg-turbo, libpng, libtiff.

### Claude's Discretion
- **Where the records live** (initial `ctest` pass/fail per test, warning log and counts, sanitizer findings, toolchain versions, smoke findings, DPR measurement). They must be committed. Suggested location: the phase directory, so that `git diff 5eaac18` outside `.planning/` shows only build changes (success criterion 5).
- **Preset layout:** the suggested set is `macos-release`, `macos-debug` and `macos-asan` configure presets plus matching test presets, schema v6, Ninja, `CMAKE_PREFIX_PATH=/opt/homebrew`, `CMAKE_EXPORT_COMPILE_COMMANDS=ON`. The preset with ASan/UBSan builds and runs the existing tests (`ASAN_OPTIONS=detect_leaks=0`; Apple Clang has no LeakSanitizer).
- **CMake minimum:** research recommends 3.25 with `CMP0167` NEW for `BoostConfig.cmake`. Confirm at the first configure, and bump only as far as needed.
- **Commit granularity:** one commit per error class or fix, so each edit can be reviewed against `5eaac18`.
- Whether `qt_tests` needs `QT_QPA_PLATFORM=offscreen` to run under `ctest` in Phase 1. Running it this way is a test setting, not a code change. If it needs a display, record that.

### Deferred Ideas (OUT OF SCOPE)
None. The discussion stayed within the phase scope. Mac issues found during the smoke run are logged for Phases 4 to 6, not fixed here.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| BUILD-01 | One CMake preset configures and builds an arm64 binary against Homebrew Qt 6, Boost, jpeg-turbo, libpng, libtiff | Standard Stack (all bottles exist for `arm64_tahoe`), CMake edits (min 3.25, CMP0167, Boost target bridge), preset skeleton in Code Examples |
| BUILD-02 | Binary links exactly one libjpeg, libpng, libtiff (`otool -L`) | Pitfall on discovery (`CMAKE_FIND_FRAMEWORK`, fresh binaryDir), verification commands in Validation Architecture |
| BUILD-03 | App launches from the build dir, opens a project, processes all six stages, writes output | Settings-location correction (INI, not plist), smoke-run mechanics, page generation without fonts, DPR/path probe |
| BUILD-04 | Existing suite runs under `ctest`, initial pass/fail recorded before any change | Five ctest tests, per-case recording via Boost.Test flags, `.log` is gitignored, Release and Debug both recorded |
| BUILD-05 | Apple Clang builds with `-Wall -Wextra` equivalents | Flag block outside the `DEFAULT_COMPILER_FLAGS_IS_SET` guard, SYSTEM-include handling, `-Werror=return-type` conflict |
| BUILD-06 | CMake option for ASan + UBSan | `SANITIZE` option design, recover-mode behaviour verified by probe (exit code 0 with findings) |
| BUILD-07 | `-ffp-contract` fixed and recorded before baselines | Probe shows default fuses at `-O0` and `-O2`; put the flag in `CMakeLists.txt`, verify in `compile_commands.json` |
</phase_requirements>

## Summary

This phase is mostly build plumbing plus disciplined record-keeping. The code base is portable on the points that normally break a libc++ port: a grep of `src/` found no `char_traits`, `result_of`, `wstring_convert`, `__gnu_cxx`, `<ext/`, `malloc.h`, `endian.h`, `random_shuffle`, `bind1st`, `ptr_fun` or `register`, and the only Boost.Bind form in use is `boost/bind/bind.hpp` (17 files, none use `boost/bind.hpp`). The machine is current (macOS 26.7, Xcode 27.1, Apple clang 21.0.0, SDK 27.0, arm64, 64 GiB, 10 cores), jpeg-turbo 3.2.0, libpng 1.6.59 and libtiff 4.7.2 are installed, and `arm64_tahoe` bottles exist for every formula still to install (qtbase, qtsvg, qttools, boost, cmake, ninja all at the versions research predicted: 6.12.0, 1.92.0, 4.4.4, 1.13.2). The first real compile is the main unknown, because upstream CI only ever built Qt 5 on Linux (the three workflows install `qtbase5-dev` packages), so the Qt 6 branches have never been compiled by upstream CI.

Four findings change or sharpen what the CONTEXT.md decisions assume. (1) **D-01/D-02 describe the settings location wrongly.** `src/app/main.cpp:47` calls `QSettings::setDefaultFormat(QSettings::IniFormat)`, so with `PORTABLE_VERSION=OFF` settings go to `~/.config/scantailor-advanced/scantailor-advanced.ini`, not a plist in `~/Library/Preferences`. Only the profiles directory is in `~/Library/Application Support`. The decision to use OFF still holds; the paths the executor asks permission for must change. (2) **`*.log` is gitignored** (`.gitignore:155`), so a warning log named `*.log` would silently not be committed. (3) **`git diff 5eaac18 -- . ':!.planning'` is already non-empty**: `.claude/CLAUDE.md` (332 added lines) is tracked and differs from the release, so the success-criterion-5 check must also exclude `.claude/`. (4) **`boost@1.85`, the fallback named in STACK.md, is deprecated and disabled** in Homebrew, so there is no installable older Boost; a Boost incompatibility would have to be solved in build settings or escalated.

I measured `-ffp-contract` on this compiler: with Apple clang 21 the default fuses `a*b+c` into `fmadd` at both `-O0` and `-O2`, and `-ffp-contract=off` gives `fmul` plus `fadd`. So the setting matters in Debug as well as Release, which supports D-03 ("every preset"). Sanitizer behaviour was probed too: ASan with `-fsanitize-recover=address` and `halt_on_error=0` reports and continues with exit code 0, so `ctest` will show PASS for a sanitizer build that has findings; the findings must be captured from test output, not from ctest status.

**Primary recommendation:** Do it in this order, one commit per step: (a) capture toolchain versions, then `brew install` the six formulae and capture again; (b) first configure on the untouched tree and save its output; (c) CMake edits (minimum 3.25 + CMP0167 NEW; Boost target bridge; Clang warning, `-ffp-contract=off` and `SANITIZE` blocks outside the cache guard; `CMakePresets.json`); (d) build, fixing only compile errors, each class in its own commit; (e) ctest in Release, Debug and ASan with output captured to `.txt` files in the phase directory; (f) the user-driven smoke run with a scratchpad probe for DPR and settings paths; (g) a closing check of the five success criteria, with the diff command excluding `.planning/` and `.claude/`.

## Architectural Responsibility Map

This phase has no web tiers. The tiers below are the build and run layers a capability can belong to.

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Toolchain selection (Apple Clang, Homebrew prefix) | Preset (`CMakePresets.json`) | `CMAKE_PREFIX_PATH` | One command per variant; no machine paths in `CMakeLists.txt` |
| `-ffp-contract=off`, warnings, sanitizer flags | `CMakeLists.txt` (flag blocks) | Preset chooses `SANITIZE`, build type | A flag in the build file applies to every configure, including one without the preset; the preset cannot be bypassed by accident |
| Dependency discovery (Qt, Boost, jpeg, png, tiff, zlib) | `CMakeLists.txt` `find_package` | Preset cache variables | Existing structure, minimal diff |
| Settings location (`PORTABLE_VERSION`) | CMake option via preset | Runtime (`QSettings` INI, `QStandardPaths`) | Compile-time define `PORTABLE_CONFIG_DIR` empties out and the runtime falls back to standard paths |
| Test execution and recording | CTest + test presets | Boost.Test / Qt Test flags | `ctest` has 5 entries; case-level results need the Boost.Test log flags |
| Sanitizer findings | Test run output capture | `ASAN_OPTIONS` / `UBSAN_OPTIONS` | Exit codes do not show findings in recover mode |
| End-to-end processing | App runtime (GUI, driven by the user) | Scratchpad probe for DPR and paths | Needs a window server and a human; nothing is committed |
| Records | `.planning/phases/01-.../` | Git | Keeps `git diff 5eaac18` outside `.planning/` limited to build changes |

## Standard Stack

### Core
| Library / Tool | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Apple clang + libc++ (Xcode 27.1) | `Apple clang version 21.0.0 (clang-2100.3.34.2)`, target `arm64-apple-darwin25.6.0`, SDK 27.0 | C++17 compiler | Installed. [VERIFIED: `clang --version`, `xcodebuild -version`, `xcrun --show-sdk-version`, this session] |
| CMake | 4.4.4 (not installed) | Build system | [VERIFIED: `brew info --json=v2 cmake`] |
| Ninja | 1.13.2 (not installed) | Generator | [VERIFIED: `brew info --json=v2 ninja`] |
| `qtbase` + `qtsvg` + `qttools` | 6.12.0 (not installed) | Qt Core/Gui/Widgets/Xml/Network/OpenGL/OpenGLWidgets/Test, Svg, LinguistTools | Bottles for `arm64_tahoe` exist. [VERIFIED: `brew info --json=v2`] Qt 6.12 needs "Xcode 16 (macOS 15 SDK) or higher" and runs on "macOS 14.4 or higher, including macOS 26". [CITED: doc.qt.io/qt-6/macos.html] |
| `boost` | 1.92.0 (not installed) | multi_index, intrusive, function, bind, lambda, Boost.Test | [VERIFIED: `brew info --json=v2 boost`] |
| `jpeg-turbo` | 3.2.0 (installed) | libjpeg | [VERIFIED: `brew list --versions`] |
| `libpng` | 1.6.59 (installed) | PNG | [VERIFIED: `brew list --versions`] |
| `libtiff` | 4.7.2 (installed) | TIFF | [VERIFIED: `brew list --versions`] |
| zlib | SDK copy | compression | Not a Homebrew install; `FindZLIB` uses the SDK. [CITED: .planning/research/STACK.md, Homebrew zlib is keg-only] |

### Supporting
| Tool | Version | Purpose | When to Use |
|------|---------|---------|-------------|
| ImageMagick | 7.1.2-34 (installed) | Generate the throwaway smoke pages | Draw glyph-sized rectangles for "text" (needs no font). Text via `-annotate` fails without an explicit `-font <ttf path>` ("unable to read font"); that path is outside the project and needs the user's yes. [VERIFIED: ran in scratchpad] |
| python3 (no Pillow, no numpy) | system Homebrew python | JSON checks of `compile_commands.json` (`python3 -I`) | Verification scripts |
| clang-format | 23.1.3 (not installed) | Format touched lines | Only if the format policy (Open Question 3) says so |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Boost 1.92 | `boost@1.85` | **Not possible**: the formula is `deprecated=True`, `disabled=True`. [VERIFIED: `brew info --json=v2 boost@1.85`] No other versioned Boost formula exists. [VERIFIED: `brew search --formulae /^boost/`] A Boost incompatibility must be handled in build settings or escalated to the user |
| `qttools` (pulls `qtdeclarative`, `litehtml`, `gumbo-parser`) | nothing | `LinguistTools` is required by the existing `find_package`; it is in `qttools`. The extra formulae are expected, not a mistake. [VERIFIED: `brew deps --formula --union`] |

**Installation (D-09):**
```bash
brew install cmake ninja boost qtbase qtsvg qttools
```
Formulae that are missing today (from `brew deps` against the installed list): cmake, ninja, boost, qtbase, qtsvg, qttools, qtdeclarative, double-conversion, md4c, litehtml, gumbo-parser. [VERIFIED: this session]. Do not run `brew install qt` or `brew install jpeg`.

**Version verification:** versions above were read from `brew info --json=v2` on 2026-10-18, so they are current for this machine. Capture `brew list --versions` before and after the install, and set `HOMEBREW_NO_AUTO_UPDATE=1` for the install command so an unrelated upgrade does not move jpeg-turbo, libpng or libtiff between records. [ASSUMED: that `brew install` could otherwise upgrade installed formulae; installed versions already equal the latest stable, so no upgrade is expected]

## Package Legitimacy Audit

The `gsd-tools package-legitimacy check` seam supports npm, PyPI and crates only. This phase installs no packages from those ecosystems. Everything comes from the official `homebrew/core` tap.

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| qtbase, qtsvg, qttools | homebrew/core | long-lived formulae | n/a | qt.io | OK (official tap, not deprecated or disabled) | Approved |
| boost | homebrew/core | long-lived | n/a | boost.org | OK | Approved |
| cmake, ninja | homebrew/core | long-lived | n/a | kitware / ninja-build | OK | Approved |

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none
Evidence: `brew info --json=v2` returned `tap = homebrew/core`, `deprecated = False`, `disabled = False` for all six. [VERIFIED: this session] `boost@1.85` is deprecated and disabled and must not be installed.

## Architecture Patterns

### System Architecture Diagram

```
 brew (homebrew/core, arm64_tahoe bottles)
        |  install cmake ninja boost qtbase qtsvg qttools
        v
 [versions captured before and after] -----------------> records/TOOLCHAIN (txt/md)
        |
        v
 CMakePresets.json ---(preset: macos-release | macos-debug | macos-asan)---+
   cacheVariables: CMAKE_PREFIX_PATH, PORTABLE_VERSION=OFF, BUILD_TESTS,   |
   CMAKE_BUILD_TYPE, SANITIZE (asan only)                                  |
        v                                                                  |
 CMakeLists.txt (min 3.25, CMP0167 NEW)                                    |
   find_package: Qt6 / Boost(config) / JPEG / PNG / TIFF / ZLIB            |
   flag blocks: -Wall -Wextra ..., -ffp-contract=off, -fsanitize=...       |
        v                                                                  |
 ninja build  ---> build/<preset>/scantailor-advanced   (plain Mach-O)     |
        |            build/<preset>/{core,math,foundation,imageproc,qt}_tests
        |--> build output tee'd to records/ (warnings, counts)             |
        v                                                                  |
 ctest --preset <preset> -----> per-test status + Boost.Test case log ---> records/
        |
        +--> asan preset: findings in stdout/stderr (exit code stays 0) --> records/
        v
 user launches build/<preset>/scantailor-advanced (terminal, stderr kept)
        |   settings -> ~/.config/scantailor-advanced/*.ini   (IniFormat)
        |   profiles -> QStandardPaths AppDataLocation/profiles
        v
 six stages -> output TIFFs -> Preview   ---> records/SMOKE findings, DPR
        |
        v
 closing check: file / otool -L / compile_commands / ctest / git diff 5eaac18
```

### Recommended Project Structure
```
CMakePresets.json                         # new, repository root
CMakeLists.txt                            # edited: min version, policy, Boost bridge, flag blocks
.planning/phases/01-deletion-free-macos-build/
├── 01-RESEARCH.md
├── records/
│   ├── toolchain-before.txt / toolchain-after.txt   # brew list --versions, clang, xcodebuild, cmake, Qt build string
│   ├── configure-unmodified.txt              # first configure on the untouched tree
│   ├── fix-log.md                            # one line per edit: file, error class, commit, reason
│   ├── warnings-release.txt                  # NOT *.log
│   ├── warning-counts.txt                    # per [-Wxxx], split: src/ vs generated vs third-party
│   ├── ctest-release.txt, ctest-debug.txt, ctest-asan.txt, ctest-*.junit.xml
│   ├── boost-cases-<exe>-<preset>.txt        # --log_level=test_suite output per Boost.Test exe
│   ├── sanitizer-findings.txt                # deduplicated ASan/UBSan findings with counts
│   ├── fp-contract.md                        # the setting, where it is set, probe evidence
│   └── SMOKE-FINDINGS.md                     # smoke results, DPR, settings paths, Retina/shortcut/alarm notes
```
Do not name a directory `Testing` or `test_results` (both gitignored, `.gitignore` lines in the "Tests / coverage output" block) and do not use `*.log`, `*.tmp`, `*.bak`, `*.cache` (all ignored). [VERIFIED: `git check-ignore -v` returned `.gitignore:155:*.log`; `.txt`, `.md`, `.xml`, `.json` names returned "not ignored"]

### Pattern 1: Flags in `CMakeLists.txt` outside the cache guard
**What:** The existing flag block is wrapped in `if (NOT DEFAULT_COMPILER_FLAGS_IS_SET)` and ends by writing the cache with `FORCE`. New Clang flags go after the block, as `add_compile_options`, not inside it.
**When to use:** Always for this phase. A flag inside the guard is applied only on the first configure of a build directory, so a later edit is silently ignored in an existing directory.
**Quoted evidence:** `CMakeLists.txt:22` `if (NOT DEFAULT_COMPILER_FLAGS_IS_SET)`; `CMakeLists.txt:44` `if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_COMPILER_IS_GNUCXX)`; `CMakeLists.txt:45` `set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Werror=return-type -Wno-unused")`; `CMakeLists.txt:57` `set(DEFAULT_COMPILER_FLAGS_IS_SET YES CACHE INTERNAL "" FORCE)`. [VERIFIED: CMakeLists.txt:22-58]

### Pattern 2: Additive Boost bridge
**What:** Keep `include_directories(${Boost_INCLUDE_DIRS})` (`CMakeLists.txt:207`) and add `link_libraries(Boost::headers)` guarded by `if (TARGET Boost::headers)`, beside the existing `link_libraries(Threads::Threads)` (`CMakeLists.txt:262`). No line is deleted, and Boost headers arrive as SYSTEM includes through the imported target, which keeps third-party warnings out of the warning log. [ASSUMED: that `BoostConfig.cmake` leaves `Boost_INCLUDE_DIRS` empty. The CMake docs for CMP0167 and FindBoost do not say; check `Boost_INCLUDE_DIRS` and `Boost_DIR` at the first configure with CMP0167 NEW, then keep or drop the bridge]

### Pattern 3: Record by capture, not by memory
Every record file is produced by a command whose output is redirected to a `.txt`/`.md`/`.xml` file in the phase directory. Build output: `cmake --build --preset macos-release 2>&1 | tee records/warnings-release.txt`. Do not hand-summarise.

### Anti-Patterns to Avoid
- **Reformatting touched files with `clang-format -i`.** `.claude/CLAUDE.md` says to run it on every touched file, but a whole-file reformat with clang-format 23 against a config written for older versions would put noise into `git diff 5eaac18` and violate success criterion 5. See Open Question 3.
- **`CMAKE_CXX_EXTENSIONS OFF`, `CMAKE_CXX_STANDARD_REQUIRED`, `qt_standard_project_setup`, `Boost::headers` replacing `include_directories`** (all in STACK.md's "modern CMake"). They are Phase 3 cleanup. In Phase 1 they are edits that are not needed to build and that could change `-std=gnu++17` to `-std=c++17`.
- **`_LIBCPP_HARDENING_MODE`** in Debug (STACK.md suggestion). It can turn existing out-of-bounds accesses into aborts and blocks the Debug baseline. Defer to a later phase. [ASSUMED]
- **`-march=native`, `-ffast-math`.** Not for Clang. `CMakeLists.txt:47` applies `-march=native` only under the GNU branch, so Apple Clang is unaffected; leave that alone.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Build variants (release/debug/asan) | A shell script wrapping `cmake` | `CMakePresets.json` schema v6 (needs CMake 3.25; list: v6 = 3.25, v7 = 3.27) | Satisfies "one CMake preset" literally; `ctest --preset` reuses the same variables. [CITED: cmake.org/cmake/help/latest/manual/cmake-presets.7.html] |
| Warning counts by type | Manual counting | `grep -o '\[-W[a-z0-9-]*\]' file \| sort \| uniq -c \| sort -rn`, plus `sort -u` on `file:line:col` for unique sites | Header warnings repeat once per translation unit; raw counts overstate |
| Per-test results | Reading ctest's 5 lines | Boost.Test `--log_level=test_suite --report_level=detailed` (or `--list_content`) per executable; `ctest --output-junit` | ctest registers one test per executable |
| Page generation for the smoke run | Writing a generator (Phase 2 does that properly) | ImageMagick `magick` one-liners with `-draw rectangle` and `-distort`/`-wave`, density set | Throwaway, never committed (D-04) |
| DPR and settings-path measurement | Instrumenting the app | A scratchpad Qt Widgets probe built as a plain executable | No committed code change (D-06) |
| Library "exactly one" check | Eyeballing | `otool -L` plus a script asserting one match per library | Repeatable at the closing check |

**Key insight:** every decision in this phase is reversible except the FP setting, so the work is in getting the evidence recorded, not in clever build engineering.

## Runtime State Inventory

Omitted: this is not a rename, refactor or migration phase. The settings-location question is covered under Pitfall 1.

## Common Pitfalls

### Pitfall 1: The settings location in D-01/D-02 is wrong (INI, not plist)
**What goes wrong:** The executor asks permission to list `~/Library/Preferences`, finds nothing, and the user concludes there is no stale state; meanwhile an INI file under `~/.config` is read and written by every build.
**Why it happens:** `src/app/main.cpp:47` is `QSettings::setDefaultFormat(QSettings::IniFormat);` and the following lines (48-49) only redirect the INI path when the portable config exists: `if (app.isPortableVersion()) {` / `QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, app.getPortableConfigPath());`. [VERIFIED: src/app/main.cpp:47-49] Qt documents IniFormat with UserScope on macOS as `$HOME/.config/MySoft/Star Runner.ini` (base `$HOME/.config`, overridable by `$XDG_CONFIG_HOME`), and NativeFormat as `$HOME/Library/Preferences/com.MySoft.Star Runner.plist`. [CITED: doc.qt.io/qt-6/qsettings.html] With organisation and application name both `scantailor-advanced` (`config.h.in`: `#define ORGANIZATION_NAME APPLICATION_NAME`), the file should be `~/.config/scantailor-advanced/scantailor-advanced.ini`. [VERIFIED: config.h.in; path derived from the Qt docs pattern, so the exact file name is [ASSUMED] until the probe prints `QSettings::fileName()`]
**Profiles:** `src/core/DefaultParamsProfileManager.cpp:24` is `m_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles";` in the non-portable branch, and line 22 is `m_path = app->getPortableConfigPath() + "/profiles";`. [VERIFIED] The macOS AppData path is likely `~/Library/Application Support/scantailor-advanced/scantailor-advanced` or one level up. [ASSUMED] The probe prints it.
**How to avoid:** Keep `PORTABLE_VERSION=OFF` (the decision stands: settings survive deleting the build directory). Change the D-02 permission question to cover `~/.config/scantailor-advanced`, `~/Library/Application Support/scantailor-advanced` and, for completeness, `~/Library/Preferences` entries matching `scantailor*`. The scratchpad probe resolves the real paths first, so the permission request can name exact paths. Ask the user to confirm this correction before the executor acts on it (Open Question 1).
**Warning signs:** Window geometry or "recent" state that persists after `rm -rf build`; settings shared by the debug and ASan builds is the expected result.

### Pitfall 2: `*.log` is gitignored
**What goes wrong:** `warnings.log` and `build.log` are silently dropped from the commit, so the "committed" record does not exist. [VERIFIED: `.gitignore:155` is `*.log`]
**How to avoid:** `.txt`, `.md`, `.xml`, `.json`. After writing the records, `git status --short` and `git check-ignore -v <file>` must show every record as untracked-not-ignored before `git add`.

### Pitfall 3: Success criterion 5 diff is already non-empty
**What goes wrong:** `git diff --stat 5eaac18 -- . ':!.planning'` reports `.claude/CLAUDE.md | 332 +++`, which a literal reading of criterion 5 fails. [VERIFIED: ran this session; `git ls-files .claude` lists `.claude/CLAUDE.md`]
**How to avoid:** Use `git diff --stat 5eaac18 -- . ':!.planning' ':!.claude'`, and for "no deleted code paths" `git diff --numstat 5eaac18 -- src`, which must show 0 deletions except in files listed in `fix-log.md`. CMake files will show a few deleted lines (for example the replaced `3.9` minimum); list them in the closing record.

### Pitfall 4: A partial Qt 6 install produces a Qt 5 error
**What goes wrong:** `CMakeLists.txt:222-225` is `find_package(Qt6 COMPONENTS Core Gui Widgets Xml Network LinguistTools OpenGL Svg OpenGLWidgets Test CONFIG)` (not REQUIRED); if any component is missing (for example `qttools` not installed), `Qt6_FOUND` is false and line 229 `find_package(Qt5 ... REQUIRED)` fails with "Could not find Qt5". [VERIFIED: CMakeLists.txt:222-233]
**How to avoid:** Install all three Qt formulae first; if configure complains about Qt5, read the preceding Qt6 message, not the Qt5 one.

### Pitfall 5: The Qt 6 branches have never been compiled upstream
**What goes wrong:** Hidden compile errors in code that only the Qt 6 arms of 19 files with `QT_VERSION` conditionals reach. The upstream workflows install Qt 5 packages (for example `qtbase5-dev libqt5svg5-dev qttools5-dev` in `.github/workflows/ci.yml:22`). [VERIFIED: grep of `.github/workflows/*.yml`] Qt 6.12 may have removed APIs deprecated in earlier Qt 6 versions. [ASSUMED]
**How to avoid:** Fix in the order the compiler reports, smallest diff, one commit per error class (D-07, `fix-log.md`). Any fix inside `src/dewarping/DetectVertContentBounds.cpp` or `DistortionModelBuilder.cpp` (the two files with `QT_VERSION_MAJOR > 5 || QT_VERSION_MINOR > 9` arms around RANSAC/dewarp code, lines 11/250/268 and 9/162/169) must stop and ask, since the Qt 6 arm is what now runs and a change can alter output.

### Pitfall 6: Sanitizer builds pass in `ctest` while having findings
**What goes wrong:** With `-fsanitize-recover=address` and `ASAN_OPTIONS=halt_on_error=0`, a probe with a heap overflow and a signed overflow printed both findings and exited 0; without recover, ASan aborts at the first error (exit 134); UBSan alone prints and continues. [VERIFIED: probe in scratchpad, Apple clang 21] `detect_leaks=1` prints "detect_leaks is not supported on this platform", so `detect_leaks=0` is correct. [VERIFIED: same probe]
**How to avoid:** Capture each test executable's full output (`ctest --output-log` or run the executables directly with `2>&1 | tee`). Count with `grep -E 'runtime error:|ERROR: AddressSanitizer' | sort | uniq -c`. Record deduplicated findings with file:line, not just totals. Optionally add `--catch_system_errors=no` for Boost.Test so Boost does not swallow a SIGABRT. [ASSUMED] `-fsanitize=undefined` includes `vptr`, which may flag uninstrumented Qt types; if findings flood from Qt headers, record them separately instead of disabling the check. [ASSUMED]

### Pitfall 7: `-Werror=return-type` can force a source edit that D-08 forbids
**What goes wrong:** D-08 asks for `-Werror=return-type` and also "no source edits for warnings". If the code has a missing return, the build fails.
**How to avoid:** If it fires, a missing return in a non-void function is undefined behaviour that Clang exposes, which D-07 allows to be fixed (smallest change, own commit). If the fix could change output, stop and ask. The alternative is to demote the flag to a plain warning and record it; decide with the user, do not choose silently. Also note the GCC set at `CMakeLists.txt:45` contains `-Wpedantic`, which D-08 omits; follow D-08 and record the difference (Open Question 4).

### Pitfall 8: Third-party warnings flooding the count
**What goes wrong:** `include_directories(${Boost_INCLUDE_DIRS})` (`CMakeLists.txt:207`) passes `-I`, not `-isystem`, so Boost header warnings under `-Wall -Wextra` appear in the log as if they were project warnings. Qt and the libraries are imported targets, whose include directories CMake treats as SYSTEM. [CITED: CMake imported-target behaviour; [ASSUMED] for this exact build]
**How to avoid:** Pattern 2 (Boost via the imported target) and a post-processing split of `warning-counts.txt` by path: `src/` (project code), `build/.../*_autogen` (moc output), everything else (third party).

### Pitfall 9: Debug and Release are different programs
**What goes wrong:** Release defines `NDEBUG` (asserts off), and `DEVELOPER_VERSION` defaults to ON only for Debug (`CMakeLists.txt:299-303`), which keeps `#define ENABLE_DEBUG_FEATURES` in `config.h` (otherwise it is commented out, lines 304-307); `MainWindow.cpp:313` branches on it. [VERIFIED: CMakeLists.txt:299-307, src/app/MainWindow.cpp:313] Test results can differ between the two.
**How to avoid:** Record `ctest` for Release and Debug (and ASan, which uses Debug) as separate files. Phase 2's goldens must say which preset made them.

### Pitfall 10: Warning/FP flags and the `build/` directory reuse
**What goes wrong:** The cache guard (Pattern 1) and `CMAKE_CXX_FLAGS` written with `FORCE` (`CMakeLists.txt:52-53`) make an edited flag invisible in an old build directory.
**How to avoid:** One binary directory per preset (`${sourceDir}/build/${presetName}`; `build/` is gitignored), and delete the directory when flags change. Verify flags in `compile_commands.json`, not by reading the CMake file.

### Pitfall 11: `brew upgrade boost` changes the dylib name and `Boost_DIR`
**What goes wrong:** A stale cache fails to configure or tests fail to load `libboost_unit_test_framework`. [CITED: .planning/research/STACK.md, MEDIUM there]
**How to avoid:** Record the Boost version in the toolchain files. Delete `build/<preset>` after any Homebrew upgrade. Settings survive because they live under `~/.config` (Pitfall 1).

### Pitfall 12: qt_tests needs a window server unless offscreen
**What goes wrong:** `src/qt_tests/main.cpp` creates a `QApplication`. [VERIFIED: file read] In a shell without a GUI session the Cocoa plugin aborts.
**How to avoid:** Run `qt_tests` once as registered (`add_test(NAME qt_tests COMMAND qt_tests)`, `src/qt_tests/CMakeLists.txt:12`) from a Terminal in the logged-in session and once with `QT_QPA_PLATFORM=offscreen`, and record both. Put the variable in the test preset `environment` only if the default run fails or flashes windows. Whether Homebrew's `qtbase` ships the offscreen plugin is [ASSUMED] until the run.

### Pitfall 13: Unbundled-binary details
**What goes wrong:** The executable is a plain Mach-O: `add_executable(${target_name} WIN32 ...)` (`src/app/CMakeLists.txt:72-73`) has no `MACOSX_BUNDLE`, and `CMAKE_RUNTIME_OUTPUT_DIRECTORY` is the binary root (`CMakeLists.txt:78-80`), so the app, the five test executables and the `.qm` files all sit in `build/<preset>/`. [VERIFIED: files read] The app name in the menu and Dock will be the executable name; that is cosmetic and for Phases 4/5.
**How to avoid:** Launch from a terminal with `2> smoke-stderr.txt` kept, so "QObject::connect" and Qt warnings are captured. Arm64 binaries need a code signature; the linker adds an ad-hoc one by default. [ASSUMED]

### Pitfall 14: Smoke pages with fonts need a path outside the project
**What goes wrong:** ImageMagick `-annotate` fails here with `unable to read font` unless `-font <ttf>` points at a system font file, which is outside the project and needs the user's yes under the global rule.
**How to avoid:** Generate "text" as rows of small filled rectangles (no font), then apply `-wave` or `-distort` to the book-spread page for curvature, and set density (`-density 300 -units PixelsPerInch`) so ScanTailor does not open the Fix DPI dialog. Check each file with `magick identify -verbose <file> | grep -i resolution` before the run. [ASSUMED: that ScanTailor needs the density to avoid the DPI dialog; the code has `FixDpiDialog` in `src/app/CMakeLists.txt`]

## Code Examples

### Preset skeleton (schema v6)
```json
{
  "version": 6,
  "cmakeMinimumRequired": { "major": 3, "minor": 25, "patch": 0 },
  "configurePresets": [
    {
      "name": "macos-base",
      "hidden": true,
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": {
        "CMAKE_PREFIX_PATH": "/opt/homebrew",
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
        "PORTABLE_VERSION": "OFF",
        "BUILD_TESTS": "ON"
      }
    },
    { "name": "macos-release", "inherits": "macos-base",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Release" } },
    { "name": "macos-debug", "inherits": "macos-base",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" } },
    { "name": "macos-asan", "inherits": "macos-base",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug", "SANITIZE": "address,undefined" } }
  ],
  "buildPresets": [
    { "name": "macos-release", "configurePreset": "macos-release" },
    { "name": "macos-debug", "configurePreset": "macos-debug" },
    { "name": "macos-asan", "configurePreset": "macos-asan" }
  ],
  "testPresets": [
    { "name": "macos-release", "configurePreset": "macos-release",
      "output": { "outputOnFailure": true }, "execution": { "noTestsAction": "error" } },
    { "name": "macos-debug", "configurePreset": "macos-debug",
      "output": { "outputOnFailure": true }, "execution": { "noTestsAction": "error" } },
    { "name": "macos-asan", "configurePreset": "macos-asan",
      "output": { "outputOnFailure": true }, "execution": { "noTestsAction": "error" },
      "environment": { "ASAN_OPTIONS": "detect_leaks=0:halt_on_error=0",
                       "UBSAN_OPTIONS": "print_stacktrace=1" } }
  ]
}
```
Source: preset keys `hidden`, `inherits`, `generator`, `binaryDir`, `cacheVariables`, `environment`, `output.outputOnFailure`, `execution.noTestsAction` and "version 6 = CMake 3.25" are from [CITED: cmake.org/cmake/help/latest/manual/cmake-presets.7.html]. The `CMakePresets.json` file is not gitignored (`CMakeUserPresets.json` is). [VERIFIED: `.gitignore` read] Optional hardening, [ASSUMED] harmless: `"CMAKE_FIND_FRAMEWORK": "LAST"` so `find_package(JPEG/PNG/TIFF)` does not prefer a framework (Mono, other SDKs) over `/opt/homebrew`.

### CMake edits (each its own commit)
```cmake
# 1. Line 1: was cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
# in the existing policy block (CMakeLists.txt:12-15), add:
if (POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)   # use upstream BoostConfig.cmake, not FindBoost
endif()

# 2. After the DEFAULT_COMPILER_FLAGS_IS_SET block (after line 58), outside the guard:
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")   # matches Clang and AppleClang
  add_compile_options(
      "$<$<COMPILE_LANGUAGE:C,CXX>:-Wall;-Wextra;-Werror=return-type;-Wno-unused>"
      "$<$<COMPILE_LANGUAGE:C,CXX>:-ffp-contract=off>")
endif()

set(SANITIZE "" CACHE STRING "Value for -fsanitize=, e.g. address,undefined. Empty disables.")
if (SANITIZE)
  add_compile_options(-fsanitize=${SANITIZE} -fno-omit-frame-pointer)
  add_link_options(-fsanitize=${SANITIZE})
  if (SANITIZE MATCHES "address")
    add_compile_options(-fsanitize-recover=address)   # keep running to collect findings
  endif()
endif()

# 3. Next to link_libraries(Threads::Threads) (CMakeLists.txt:262) or after the Boost block:
if (TARGET Boost::headers)
  link_libraries(Boost::headers)
endif()
```
`COMPILE_LANGUAGE` with a language list needs CMake 3.15, so it is safe at 3.25. [ASSUMED from CMake docs, not run here because CMake is not installed]. Keep `-ffp-contract=off` in `CMakeLists.txt`, not only in presets: a configure without the preset then still builds with the frozen setting. This is a Claude's-discretion call inside D-03; D-03 only requires that every preset builds with it.

### FP-contract evidence (record in `fp-contract.md`)
```
$ clang++ -std=c++17 -O2 -S fp.cpp        # double f(double a,double b,double c){return a*b+c;}
        fmadd   d0, d0, d1, d2            # default: fused (also at -O0)
$ clang++ -std=c++17 -O2 -ffp-contract=off -S fp.cpp
        fmul    d0, d0, d1
        fadd    d0, d0, d2
$ clang++ -std=c++17 -O2 -ffp-contract=fast -S fp.cpp
        fmadd   d0, d0, d1, d2
```
[VERIFIED: probe run in the session scratchpad with Apple clang 21.0.0] Verification after the build: every `command` entry in `compile_commands.json` contains `-ffp-contract=off`, `-Wall` and `-Wextra` (see Validation Architecture).

### Scratchpad probe (not committed): DPR, settings path, app-data path
```cpp
// Build in the scratchpad as a plain executable (no bundle) against Qt6 Widgets.
#include <QApplication>
#include <QDebug>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QWidget>
int main(int argc, char** argv) {
  QApplication app(argc, argv);
  QApplication::setApplicationName("scantailor-advanced");
  QApplication::setOrganizationName("scantailor-advanced");
  QWidget w; w.resize(300, 200); w.show();
  QTimer::singleShot(500, [&] {
    for (QScreen* s : QApplication::screens())
      qInfo() << s->name() << "dpr" << s->devicePixelRatio() << "logicalDPI" << s->logicalDotsPerInch()
              << "physicalDPI" << s->physicalDotsPerInch() << "geometry" << s->geometry();
    qInfo() << "window dpr" << w.devicePixelRatio();
    qInfo() << "ini" << QSettings(QSettings::IniFormat, QSettings::UserScope,
                                  "scantailor-advanced", "scantailor-advanced").fileName();
    qInfo() << "appdata" << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QApplication::quit();
  });
  return app.exec();
}
```
[ASSUMED: written from Qt API knowledge, not compiled here.] It answers HIDPI-01's measurement (the probe is also an unbundled executable, so it sees the same bundle situation as the build-dir binary) and gives the exact paths for the D-02 permission request. The probe does not read the user's settings, only prints the path. The alternative from D-06, `QT_LOGGING_RULES="qt.highdpi=true"` on the real app, is [ASSUMED] to print scale information; use it as a cross-check, not the only source.

### Closing verification commands
```bash
file build/macos-release/scantailor-advanced                       # expect: Mach-O 64-bit executable arm64
otool -L build/macos-release/scantailor-advanced | grep -E 'libjpeg|libpng|libtiff'   # exactly 3 lines, one each
python3 -I - <<'EOF'
import json,sys
cc=json.load(open("build/macos-release/compile_commands.json"))
bad=[e["file"] for e in cc if not all(f in e["command"] for f in ("-Wall","-Wextra","-ffp-contract=off"))]
print(len(cc),"entries;",len(bad),"missing flags"); sys.exit(1 if bad else 0)
EOF
git diff --stat 5eaac18 -- . ':!.planning' ':!.claude'
git diff --numstat 5eaac18 -- src                                   # deletions column: only files named in fix-log.md
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `cmake_minimum_required(VERSION 3.9)` | 3.25 or higher | CMake 3.31 deprecates compatibility below 3.10; 4.0 removed below 3.5 [CITED: cmake.org cmake_minimum_required] | 3.9 still configures on 4.4.4 with a deprecation warning. Raise it so the output is clean and CMP0167 can be set |
| CMake `FindBoost` module | upstream `BoostConfig.cmake` (CMP0167 NEW, CMake 3.30) | 3.30 | The module is "available only if policy CMP0167 is not set to NEW" [CITED: cmake.org FindBoost]; unset, CMake warns and still uses the module |
| `qt_add_translation` | `qt_add_lrelease` / `qt_add_translations` | Deprecated in current Qt 6 docs, not removed [CITED: doc.qt.io/qt-6/qtlinguist-cmake-qt-add-translation.html] | Leave `Qt_add_translation` (`CMakeLists.txt:350`) alone in Phase 1; Phase 3 removes translations anyway |
| `BOOST_AUTO_TEST_MAIN` | `BOOST_TEST_MAIN` | Mapped by Boost's `config.hpp`: `#if !defined(BOOST_TEST_MAIN) && defined(BOOST_AUTO_TEST_MAIN)` then `#define BOOST_TEST_MAIN BOOST_AUTO_TEST_MAIN` [CITED: boostorg/test develop `config.hpp`] | The old macro in `src/*/tests/main.cpp` keeps working |

**Deprecated/outdated:**
- `boost@1.85`: disabled in Homebrew, not an available fallback.
- Xcode 27: STACK.md notes (one developer blog, LOW to MEDIUM) a libc++ `multimap::find` behaviour change; a web search this session found no source-break reports for Boost or Qt on this toolchain. Treat the first build as the test.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `BoostConfig.cmake` leaves `Boost_INCLUDE_DIRS` empty, so `Boost::headers` must be linked | Pattern 2 | Low: the bridge is additive and harmless either way; headers missing would show as `boost/...: file not found` |
| A2 | The Qt 6 arms of the `QT_VERSION` conditionals compile on Qt 6.12 with at most small fixes | Pitfall 5 | Medium: a larger incompatibility changes the size of the phase |
| A3 | The INI file is `~/.config/scantailor-advanced/scantailor-advanced.ini` and AppData is under `~/Library/Application Support/scantailor-advanced/...` | Pitfall 1 | Low: the probe prints the real paths before any access request |
| A4 | Homebrew's `qtbase` ships the `offscreen` platform plugin | Pitfall 12 | Low: otherwise `qt_tests` is run in the GUI session and recorded |
| A5 | `CMAKE_FIND_FRAMEWORK=LAST` is harmless for Qt6 (found through its config packages) | Presets | Low: drop it if configure breaks |
| A6 | `-fsanitize=undefined` `vptr` check may flag uninstrumented Qt types | Pitfall 6 | Low: only noise in the record |
| A7 | Arm64 linker ad-hoc signs the output so the unbundled binary launches | Pitfall 13 | Low: a launch failure would show immediately |
| A8 | ScanTailor needs a DPI in the file to skip the Fix DPI dialog | Pitfall 14 | Low: the dialog appears and is dismissed |
| A9 | `HOMEBREW_NO_AUTO_UPDATE=1` is enough to prevent surprise upgrades of installed formulae | Installation | Low: installed versions already equal latest stable |
| A10 | `COMPILE_LANGUAGE:C,CXX` generator expression and `add_link_options` behave as expected at min 3.25 | CMake edits | Low: found at first configure |
| A11 | `_LIBCPP_HARDENING_MODE` in Debug could abort on existing out-of-bounds accesses | Anti-Patterns | Low: it is deferred, not adopted |
| A12 | Raising the minimum to 3.25 (policies up to 3.25 become NEW) does not change the generated build in a way that affects output | CMake edits | Medium: record the unmodified first-configure output (step b) and compare the `compile_commands.json` flags before and after the bump |

## Open Questions

1. **D-01/D-02 settings location (needs user confirmation).**
   - What we know: `main.cpp:47` forces `IniFormat`; Qt puts IniFormat user settings at `$HOME/.config/<org>/<app>.ini` on macOS; the profile directory is under `QStandardPaths::AppDataLocation`.
   - What's unclear: whether the user wants D-02's permission request rewritten to the `.config` and Application Support paths (recommended).
   - Recommendation: keep `PORTABLE_VERSION=OFF`; the planner adds a decision checkpoint that states the correction and asks permission for exact paths printed by the probe.
2. **Criterion 5 pathspec.** `.claude/CLAUDE.md` is tracked and differs from `5eaac18`. Recommendation: exclude `.claude/` in the closing diff and say so in the record.
3. **Formatting touched lines.** `.claude/CLAUDE.md` says to run `clang-format -i` on touched files; criterion 5 says only build and launch fixes. Recommendation: format only the changed lines (`git clang-format`, which needs `brew install clang-format`, not in D-09) or skip formatting for Phase 1 fixes and say so in `fix-log.md`. User choice.
4. **`-Wpedantic`.** The GCC set (`CMakeLists.txt:45`) has it; D-08 does not. Recommendation: follow D-08, record the difference.
5. **No older Boost fallback exists.** If Boost 1.92 breaks a file, the options are a build-setting workaround, a source fix under D-07, or a user decision. Do not install `boost@1.85`.
6. **Where `-ffp-contract=off` lives.** Recommended: `CMakeLists.txt` (applies to every configure). If the user prefers preset-only, move the line to `cacheVariables.CMAKE_CXX_FLAGS`/`CMAKE_C_FLAGS` in `macos-base`; the CMake cache guard keeps those values.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| macOS on Apple Silicon | all | yes | macOS 26.7 (25G229), arm64, 10 cores (8 performance + 2 efficiency), 64 GiB RAM | none needed |
| Xcode / Apple clang | BUILD-01 | yes | Xcode 27.1 (27A9275), `Apple clang version 21.0.0 (clang-2100.3.34.2)`, SDK 27.0, `xcode-select -p` = `/Applications/Xcode.app/Contents/Developer`, CLT package `27.0.0.0.1788430756` | none needed |
| Homebrew | install | yes | 7.0.9-159-g3073d6a at `/opt/homebrew` | none needed |
| CMake | BUILD-01 | no | 4.4.4 via `brew install cmake` | none |
| Ninja | BUILD-01 | no | 1.13.2 via `brew install ninja` | `-G "Unix Makefiles"` |
| Qt 6 (`qtbase`, `qtsvg`, `qttools`) | BUILD-01 | no | 6.12.0 via brew | none (Qt 5 not to be installed) |
| Boost | BUILD-01, tests | no | 1.92.0 via brew | none (`boost@1.85` disabled) |
| jpeg-turbo, libpng, libtiff | BUILD-01/02 | yes | 3.2.0, 1.6.59, 4.7.2 | none needed |
| zlib | BUILD-01 | yes (SDK) | SDK 27.0 | none needed |
| ImageMagick | D-04 page generation | yes | 7.1.2-34 | python3 without Pillow cannot generate images; install nothing extra, use `magick` |
| clang-format | CLAUDE.md formatting policy | no | 23.1.3 available | skip (Open Question 3) |
| Free disk | install and builds | yes | about 133 GiB on the data volume | none needed |
| Window server session | smoke run, `qt_tests` | not checked | needs the user's logged-in desktop | `QT_QPA_PLATFORM=offscreen` for tests only |

**Missing dependencies with no fallback:** cmake, qt (qtbase, qtsvg, qttools), boost: all installable by D-09.
**Missing dependencies with fallback:** ninja (Makefiles), clang-format (skip).

Note: out-of-project reads performed during this research and the global rule are listed in the hand-off report, not here.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Boost.Test 1.92.0 (core, foundation, math, imageproc), Qt Test 6.12.0 (`qt_tests`), driven by CTest 4.4.4 |
| Config file | `CMakePresets.json` (new), existing `add_test` lines: `src/core/tests/CMakeLists.txt:20`, `src/math/tests/CMakeLists.txt:16`, `src/foundation/tests/CMakeLists.txt:13`, `src/imageproc/tests/CMakeLists.txt:28`, `src/qt_tests/CMakeLists.txt:12` |
| Quick run command | `ctest --preset macos-release -R foundation_tests` |
| Full suite command | `ctest --preset macos-release` |

Quoted registrations: `add_test(NAME core_tests COMMAND core_tests --log_level=message)` (`src/core/tests/CMakeLists.txt:20`), `add_test(NAME qt_tests COMMAND qt_tests)` (`src/qt_tests/CMakeLists.txt:12`). [VERIFIED: grep -n]

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| BUILD-01 | Configures and builds arm64 with the preset | build + smoke | `cmake --preset macos-release && cmake --build --preset macos-release && file build/macos-release/scantailor-advanced` (expect `arm64`) | needs `CMakePresets.json` (Wave 0) |
| BUILD-02 | One libjpeg, libpng, libtiff | script | `otool -L build/macos-release/scantailor-advanced \| grep -E 'libjpeg\|libpng\|libtiff'` and count 3 distinct lines | after build |
| BUILD-03 | Launch, open, six stages, output | manual (user-driven GUI) | one-step-at-a-time walkthrough per D-05; stderr kept in `records/` | no automated form; Phase 2 adds tests |
| BUILD-04 | ctest runs; per-test result recorded | integration | `ctest --preset macos-release --output-junit records/ctest-release.junit.xml`, plus `./<exe> --log_level=test_suite --report_level=detailed` per Boost.Test executable, output to `records/*.txt` | existing tests |
| BUILD-05 | Warnings on | script | the `compile_commands.json` check in Code Examples | after build |
| BUILD-06 | ASan/UBSan build runs the existing tests | integration | `cmake --preset macos-asan && cmake --build --preset macos-asan && ctest --preset macos-asan --output-log records/ctest-asan.txt`, then count findings | needs `SANITIZE` option (Wave 0) |
| BUILD-07 | FP setting fixed and recorded | script | same `compile_commands.json` check (`-ffp-contract=off` in every entry) plus `records/fp-contract.md` | after build |

### Sampling Rate
- **Per task commit:** the narrowest check for that task (a configure, or one target build, or one ctest entry).
- **Per wave merge:** build release plus `ctest --preset macos-release`.
- **Phase gate:** all five success criteria checked from a clean `build/` directory and the closing commands above, before `/gsd-verify-work`.

### Wave 0 Gaps
- [ ] `CMakePresets.json` — covers BUILD-01, BUILD-06
- [ ] `SANITIZE` option in `CMakeLists.txt` — covers BUILD-06
- [ ] `brew install cmake ninja boost qtbase qtsvg qttools` — framework and toolchain install
- [ ] `records/` directory and file names that are not gitignored

No test files need to be written in this phase; existing tests are the subject.

## Security Domain

Config has `security_enforcement` on (ASVS level 1, block on high). This phase adds no network, auth or data-handling code, so most categories do not apply.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | none in scope |
| V3 Session Management | no | none in scope |
| V4 Access Control | no | none in scope |
| V5 Input Validation | observe only | Sanitizer runs may expose input-handling bugs; findings are recorded, not fixed (SAFE-01 in Phase 6) |
| V6 Cryptography | no | none in scope |
| V14 Configuration / supply chain | yes | Official `homebrew/core` formulae only; versions recorded; no `curl | sh`, no disabled formulae |

### Known Threat Patterns for this stack

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Unpinned or wrong-copy native dependencies (two libjpegs) | Tampering | Versions recorded before and after install; `otool -L` check; prefix pinned by preset |
| Shell command from a settings value (`std::system(cmd...)` at `src/app/MainWindow.cpp:1337`) | Elevation of privilege | Observed only in this phase, fixed in Phase 5/6 (MAC-10). The Linux default `play /usr/share/sounds/freedesktop/stereo/bell.oga` is taken from `main_window/external_alarm_cmd` if set |
| Malformed TIFF with huge dimensions | Denial of service | Out of scope here (SAFE-01); do not feed such files in the smoke run |
| Records or probe code leaking personal data | Information disclosure | Throwaway synthetic pages in the scratchpad only; the probe prints paths, not setting contents; no personal scans |

## Project Constraints (from CLAUDE.md)

From `/Users/christoph/.claude/CLAUDE.md` (global):
- Present manual procedures **one step at a time**. For each step say what the command does and what to look for. State thresholds inline. This governs the D-05 smoke run, the D-02 permission request and any other user-executed step.
- **Never access OmniFocus.** No exceptions.
- **Never access files outside the project directory without asking first.** Exempt: `~/.claude/` and the session scratchpad. This applies to the executor's D-02 listing, to a system font path for ImageMagick, and to any `~/.config` or `~/Library` access.

From `./.claude/CLAUDE.md` (project):
- Keep the C++17/Qt/CMake architecture and the six-stage pipeline; this is a port, not a rewrite.
- Output for existing settings must not change unintentionally.
- Synthetic images only in the repository.
- Homebrew-provided Qt 6, Boost, libjpeg, libpng, libtiff, zlib; no manual dependency builds.
- C++17 only; do not introduce C++20 features.
- Run `clang-format -i <file>` on every touched file under `src/` before committing (CI-derived rule). This conflicts with a minimal diff against `5eaac18`; see Open Question 3.
- Header guards, copyright header on new source files (no new source files are expected in this phase; `CMakePresets.json` is not a source file).
- Do not swallow exceptions silently in new code (no new code expected).
- GSD workflow: file-changing work goes through a GSD command (`/gsd-execute-phase`).

## Sources

### Primary (HIGH confidence)
- Local probes this session: `clang --version`, `xcodebuild -version`, `xcrun --show-sdk-version`, `sw_vers`, `sysctl`, `brew --version`, `brew list --versions`, `brew info --json=v2` (qtbase, qtsvg, qttools, boost, cmake, ninja, boost@1.85, clang-format; taps, bottles, deprecation flags), `brew deps`, `brew search`, `magick` text test, scratchpad compile probes for `-ffp-contract` and sanitizers.
- Repository files read: `CMakeLists.txt`, `src/CMakeLists.txt`, `src/app/CMakeLists.txt`, `src/core/CMakeLists.txt`, `src/*/tests/CMakeLists.txt`, `src/qt_tests/CMakeLists.txt`, `src/app/main.cpp`, `src/core/Application.cpp`, `src/core/DefaultParamsProfileManager.cpp`, `src/app/MainWindow.cpp` (lines 300-345, 1320-1350), `config.h.in`, `.gitignore`, `cmake/UpdateTranslations.cmake`, `.github/workflows/*.yml` (grep), `TESTING.md`, `.planning/research/STACK.md`, `.planning/research/PITFALLS.md` (lines 1-568 of 708), `.planning/ROADMAP.md`, `.planning/REQUIREMENTS.md`, `.planning/STATE.md`.
- Qt docs: doc.qt.io/qt-6/qsettings.html (INI and native locations on macOS), doc.qt.io/qt-6/macos.html (Qt 6.12 on macOS), doc.qt.io/qt-6/qtlinguist-cmake-qt-add-translation.html.
- CMake docs: policy CMP0167 (3.30), cmake_minimum_required (3.31 deprecation below 3.10, 4.0 removal below 3.5), FindBoost, cmake-presets(7) (schema versions and test preset keys).
- Boost.Test `boost/test/detail/config.hpp` (develop branch) for `BOOST_AUTO_TEST_MAIN` mapping.

### Secondary (MEDIUM confidence)
- Web search results on CMP0167 and `Boost::headers` (KDE merge request, CMake discourse); no official statement on `Boost_INCLUDE_DIRS` under config mode was found.
- Web search for Xcode 27 / libc++ source breaks: nothing relevant to Boost or Qt was found.

### Tertiary (LOW confidence)
- Items tagged `[ASSUMED]` in the Assumptions Log.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH, versions read from Homebrew on the target machine; bottles exist for `arm64_tahoe`.
- Architecture (build and record flow): HIGH, derived from files read.
- CMake edits: MEDIUM, written against docs but not run (CMake not installed).
- Compile-error prediction: LOW-MEDIUM, clean grep for libc++-gated features but no build yet.
- Pitfalls: HIGH for the repository-specific ones (gitignore, settings format, diff pathspec, cache guard), MEDIUM for platform behaviour.

**Research date:** 2026-10-18
**Valid until:** 2026-11-01 (Homebrew formulae and Xcode move fast; re-run `brew info` before installing if the plan is executed later)
