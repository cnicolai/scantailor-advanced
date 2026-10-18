# Phase 1 fix log

Edit rules for Phase 1: only build and launch fixes plus added build settings are allowed; every edit against `5eaac18` gets one row here and one commit of its own. Anything that could change processing output is stopped and put to the user first (D-07). Plan 01-07 cross-checks this log against `git diff 5eaac18` and the commit list.

| # | File | Change | Class | Commit | Reason | Affects processing? |
|---|------|--------|-------|--------|--------|---------------------|
| 1 | CMakeLists.txt | Line 1 `cmake_minimum_required(VERSION 3.9 FATAL_ERROR)` replaced by `VERSION 3.25 FATAL_ERROR`; policy block gets `if (POLICY CMP0167)` / `cmake_policy(SET CMP0167 NEW)` / `endif()` after the CMP0071 block, before `project(` | build setting | b2d5214 | Matches the schema-v6 minimum of `CMakePresets.json`; removes the CMake 4 warning of the untouched configure ("Compatibility with CMake < 3.10 will be removed from a future version of CMake", quoted from `configure.txt` `## Unmodified tree`). CMP0167 NEW makes `find_package(Boost)` use `BoostConfig.cmake`; the untouched configure warned "Policy CMP0167 is not set: The FindBoost module is removed". The one replaced line in the CMake files; all other edits are additive. | No: compiler and link inputs unchanged apart from the policy-driven flags listed in `flags-compare.txt` |
| 2 | CMakePresets.json (new) | Schema v6 file with configure presets `macos-base` (hidden) and `macos-release`, build preset and test presets (`macos-test-base` hidden, `macos-release`). `macos-base` sets Ninja, `CMAKE_PREFIX_PATH=/opt/homebrew`, `CMAKE_EXPORT_COMPILE_COMMANDS=ON`, `PORTABLE_VERSION=OFF` (D-01), `BUILD_TESTS=ON`, `CMAKE_FIND_FRAMEWORK=LAST` | build setting | d7f2ca2 | One preset per variant to configure against Homebrew Qt 6 and Boost; `CMAKE_FIND_FRAMEWORK=LAST` keeps the Homebrew libraries ahead of macOS frameworks (T-01-02). The configure succeeded with it, so research assumption A5 needed no change. | No: no source file is touched |

## Checked, not changed

- **Boost include path (research Pattern 2, A1).** After row 1, `cmake --preset macos-release --fresh --trace-expand --trace-source=CMakeLists.txt` shows the Boost call (now on line 211 of `CMakeLists.txt`, because row 1 added four lines above it) as `include_directories(/opt/homebrew/include )`. `Boost_INCLUDE_DIRS` is therefore not empty under BoostConfig, and the `link_libraries(Boost::headers)` bridge was not added. The `include_directories(${Boost_INCLUDE_DIRS})` line stays as it is.

## Open errors

None. `foundation_tests` builds with one compiler warning under the default Clang diagnostics, which is not caused by an edit of this plan: `src/foundation/PropertySet.cpp:25:22: warning: expression with side effects will be evaluated despite being used as an operand to 'typeid' [-Wpotentially-evaluated-expression]`. No `src/` file was edited.
