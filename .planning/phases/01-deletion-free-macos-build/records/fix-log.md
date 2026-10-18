# Phase 1 fix log

Edit rules for Phase 1: only build and launch fixes plus added build settings are allowed; every edit against `5eaac18` gets one row here and one commit of its own. Anything that could change processing output is stopped and put to the user first (D-07). Plan 01-07 cross-checks this log against `git diff 5eaac18` and the commit list.

| # | File | Change | Class | Commit | Reason | Affects processing? |
|---|------|--------|-------|--------|--------|---------------------|
| 1 | CMakeLists.txt | Line 1 `cmake_minimum_required(VERSION 3.9 FATAL_ERROR)` replaced by `VERSION 3.25 FATAL_ERROR`; policy block gets `if (POLICY CMP0167)` / `cmake_policy(SET CMP0167 NEW)` / `endif()` after the CMP0071 block, before `project(` | build setting | b2d5214 | Matches the schema-v6 minimum of `CMakePresets.json`; removes the CMake 4 warning of the untouched configure ("Compatibility with CMake < 3.10 will be removed from a future version of CMake", quoted from `configure.txt` `## Unmodified tree`). CMP0167 NEW makes `find_package(Boost)` use `BoostConfig.cmake`; the untouched configure warned "Policy CMP0167 is not set: The FindBoost module is removed". The one replaced line in the CMake files; all other edits are additive. | No: `flags-compare.txt` shows no flag changed by this edit (no CMP0128 effect, same include paths) |
| 2 | CMakePresets.json (new) | Schema v6 file with configure presets `macos-base` (hidden) and `macos-release`, build preset and test presets (`macos-test-base` hidden, `macos-release`). `macos-base` sets Ninja, `CMAKE_PREFIX_PATH=/opt/homebrew`, `CMAKE_EXPORT_COMPILE_COMMANDS=ON`, `PORTABLE_VERSION=OFF` (D-01), `BUILD_TESTS=ON`, `CMAKE_FIND_FRAMEWORK=LAST` | build setting | d7f2ca2 | One preset per variant to configure against Homebrew Qt 6 and Boost; `CMAKE_FIND_FRAMEWORK=LAST` keeps the Homebrew libraries ahead of macOS frameworks (T-01-02). The configure succeeded with it, so research assumption A5 needed no change. | No: no source file is touched |
| 3 | CMakeLists.txt | New `if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")` block after the flag-cache guard, with `add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:-Wall;-Wextra;-Werror=return-type;-Wno-unused>")` | build setting | e35b498 | BUILD-05, D-08: Clang counterpart of the GNU set at line 45. `-Wpedantic` is left out because D-08 does not list it; the GNU set has it (research Open Question 4). No global `-Werror`. | No: warnings only |
| 4 | CMakeLists.txt | Second `add_compile_options` in the same Clang block: `$<$<COMPILE_LANGUAGE:C,CXX>:-ffp-contract=off>` (line 69) | build setting | 606bea0 | BUILD-07, D-03: contraction frozen before any Phase 2 baseline. Evidence in `fp-contract.md`. | Yes, by design: Apple Clang otherwise fuses `a * b + c` into `fmadd`; fixed before baselines exist |
| 5 | CMakeLists.txt | New cache variable `SANITIZE` (STRING, empty) and an `if (SANITIZE)` block: `add_compile_options(-fsanitize=${SANITIZE} -fno-omit-frame-pointer)`, `add_link_options(-fsanitize=${SANITIZE})`, and `-fsanitize-recover=address` when the value contains `address` | build setting | 8670057 | BUILD-06: the sanitizer preset needs one switch; recover mode lets one run collect every finding (research Pitfall 6). Empty by default, so release and debug trees are unchanged. | No: off unless a preset sets it |
| 6 | CMakePresets.json | Configure, build and test presets `macos-debug` and `macos-asan` (`SANITIZE=address,undefined`, `ASAN_OPTIONS=detect_leaks=0:halt_on_error=0` and `UBSAN_OPTIONS=print_stacktrace=1` for the ASan test preset) | build setting | 11bc573 | BUILD-06; Apple Clang has no LeakSanitizer, so leak detection is off. All presets inherit `PORTABLE_VERSION=OFF` (D-01). | No: separate build directories |

## Checked, not changed

- **Boost include path (research Pattern 2, A1).** After row 1, `cmake --preset macos-release --fresh --trace-expand --trace-source=CMakeLists.txt` shows the Boost call (now on line 211 of `CMakeLists.txt`, because row 1 added four lines above it) as `include_directories(/opt/homebrew/include )`. `Boost_INCLUDE_DIRS` is therefore not empty under BoostConfig, and the `link_libraries(Boost::headers)` bridge was not added. The `include_directories(${Boost_INCLUDE_DIRS})` line stays as it is.

## Open errors

None. `foundation_tests` builds with one compiler warning under the default Clang diagnostics, which is not caused by an edit of this plan: `src/foundation/PropertySet.cpp:25:22: warning: expression with side effects will be evaluated despite being used as an operand to 'typeid' [-Wpotentially-evaluated-expression]`. No `src/` file was edited.

## Build attempt 1

Source: `build-attempt-1.txt` (full keep-going output of `cmake --build --preset macos-release -- -k 0` and `cmake --build --preset macos-debug -- -k 0`). Tree state: `src/` identical to `5eaac18`; `CMakeLists.txt` and `CMakePresets.json` as left by plan 01-01.

Result: release 434 of 434 steps, `exit status: 0`; debug 456 of 456 steps, `exit status: 0`. Searching both outputs for `error:` and `FAILED` finds nothing.

No compile or link errors.

| Class | Root cause | Example | Sites | Files | Origin | In processing code? |
|-------|------------|---------|-------|-------|--------|---------------------|
| (none) | no `error:` diagnostic in either build | - | 0 | - | - | - |

src/ edits needed: no

Formatting decision: not needed (no src/ edits).

Fix loop: no fixes needed.

### Warnings recorded, not fixed (D-08)

The release tree already held the objects of `foundation` and `foundation_tests` from plan 01-01, so its 434 steps are the remaining ones; the debug tree was built from nothing (456 steps). Counts are warning lines in the build output (a header warning repeats once per translation unit that includes it); release and debug differ by one line.

| Warning | Lines (release) | Example | Note |
|---------|-----------------|---------|------|
| `-Wunused-parameter` | 1125 | many files | unused parameters; from `-Wextra` (D-08) |
| `-Winconsistent-missing-override` | 7 | `src/core/filters/select_content/Filter.h:50` (`pageOrderOptions`) | repeats per includer |
| `-Wdeprecated-pragma` | 6 | `src/core/zones/ZoneContextMenuInteraction.cpp:73`, `:141`, `ZoneContextMenuInteraction.h:117` (`Q_OS_MAC` deprecated in Qt 6.12) | `Q_OS_MAC` is still defined; Qt 6 recommends `Q_OS_MACOS` |
| `-Wreorder-ctor` | 1 | `src/core/filters/page_layout/OptionsWidget.cpp:32` | initialiser order differs from declaration order |
| `-Wdeprecated-declarations` | 1 | `src/core/filters/select_content/ContentBoxFinder.cpp:771` (`qAsConst`) | Qt suggests `std::as_const` |
| `-Wpotentially-evaluated-expression` | 0 | `src/foundation/PropertySet.cpp:25` | seen by plan 01-01 in the untouched-tree configure; the release tree had already built `foundation` in plan 01-01, and the debug compile of `PropertySet.cpp` printed no warning. Stays under "Open errors" |
| linker `ignoring duplicate libraries` | 1 | final link of `scantailor-advanced` | static libraries listed twice by `target_link_libraries`; harmless |

None of these stops the build. Phase 1 records warnings only; nothing was edited.

### Artifacts of the build

`build/macos-release/` holds `scantailor-advanced`, `core_tests`, `foundation_tests`, `imageproc_tests`, `math_tests` and `qt_tests` (the plan's verify command lists all six).
