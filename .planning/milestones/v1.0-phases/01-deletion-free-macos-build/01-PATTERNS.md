# Phase 1: Deletion-Free macOS Build - Pattern Map

**Mapped:** 2026-10-18
**Files analyzed:** 9 (2 modified, 1 new config, 6 new records)
**Analogs found:** 8 / 9

All analog paths below are git-tracked (checked: `CMakeLists.txt`, `cmake/*`, `src/*/tests/CMakeLists.txt`, `TESTING.md`, `.planning/*`). No gitignored mirror paths are used. Line numbers refer to commit state at HEAD (unmodified CMake files = `5eaac18` content).

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `CMakeLists.txt` (min version + CMP0167) | config | build-time | same file lines 1, 12-15 (policy block) | exact |
| `CMakeLists.txt` (Clang warning + `-ffp-contract=off` block) | config | build-time | same file lines 44-48 (GNU flag block) | exact (placement differs: must sit OUTSIDE the guard) |
| `CMakeLists.txt` (`SANITIZE` option) | config | build-time | `option(BUILD_TESTS ...)` line 76, `option(PORTABLE_VERSION ...)` line 285, `option(DEVELOPER_VERSION ...)` line 303 | role-match |
| `CMakeLists.txt` (Boost bridge `Boost::headers`) | config | build-time | lines 257-263 (`link_libraries(Threads::Threads)`) + 207 | role-match |
| `CMakePresets.json` (new) | config | build-time | none in repo; use RESEARCH.md "Preset skeleton" | no analog |
| ctest/test preset wiring (no new `add_test`) | test config | request-response | `src/core/tests/CMakeLists.txt:20`, `src/qt_tests/CMakeLists.txt:12` | exact (read-only; do not edit) |
| `records/*.txt`, `records/*.xml` (toolchain, warnings, ctest, sanitizer) | record | captured command output | none (no existing captured-output files); follow RESEARCH Pattern 3 | no analog |
| `records/fix-log.md`, `records/fp-contract.md`, `records/SMOKE-FINDINGS.md` | doc | hand-written markdown | `.planning/phases/01-.../01-CONTEXT.md` (front matter-less sections, bold D-xx labels), `TESTING.md` | role-match |
| `.gitignore` (read-only check; not modified) | config | n/a | `.gitignore:155` `*.log` | n/a |

## Pattern Assignments

### `CMakeLists.txt` - minimum version and policy bump (config)

**Analog:** `CMakeLists.txt` lines 1 and 12-15.

```cmake
cmake_minimum_required(VERSION 3.9 FATAL_ERROR)      # line 1  -> 3.25

# Set cmake policies.
if (POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW)
endif()
```

Copy: add a sibling `if (POLICY CMP0167) cmake_policy(SET CMP0167 NEW) endif()` inside this same block, before `project(...)` at line 17 (policies must be set before `project()`/`find_package`). Style: two-space indent, space after `if`, no `else`. Bump minimum only as far as needed (CONTEXT: confirm at first configure). Own commit.

---

### `CMakeLists.txt` - Clang flags and `-ffp-contract=off` (config)

**Analog:** `CMakeLists.txt` lines 19-58.

```cmake
if (NOT DEFAULT_COMPILER_FLAGS_IS_SET)            # line 22
  ...
  if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_COMPILER_IS_GNUCXX)      # line 44
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Werror=return-type -Wno-unused")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror=return-type -Wno-unused")
    add_compile_options("$<$<CONFIG:RELEASE>:-march=native;-mtune=native>")   # line 47
  endif()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "Common C flags for all build configurations." FORCE)
  ...
  set(DEFAULT_COMPILER_FLAGS_IS_SET YES CACHE INTERNAL "" FORCE)   # line 57
endif()                                           # line 58
```

Rules for the planner:
- Do NOT delete or edit the GNU block (deletion-free). `CMAKE_COMPILER_IS_GNUCC` is false for AppleClang, so it never fires on the Mac.
- Add the new Clang block AFTER line 58 (outside the guard), because flags inside the guard apply only on the first configure of a build dir and are cached with `FORCE` (lines 50-55).
- Use the existing `add_compile_options("$<...>")` generator-expression style from line 47, not `CMAKE_CXX_FLAGS` appends. Condition: `CMAKE_CXX_COMPILER_ID MATCHES "Clang"` (matches AppleClang).
- Flag set per D-08: `-Wall -Wextra -Werror=return-type -Wno-unused` (note: GCC set has `-Wpedantic`, D-08 omits it; record the difference). D-03: `-ffp-contract=off` for all build types.
- Verify in `compile_commands.json`, not by reading the file (RESEARCH "Closing verification commands").

---

### `CMakeLists.txt` - `SANITIZE` option (config)

**Analog:** option declarations with a comment above, same file.

```cmake
# Must be set before Boost (Flatpak and other chroot builds use -DBUILD_TESTS=OFF; issue #61).
option(BUILD_TESTS "Build unit tests (math_tests, imageproc_tests, core_tests, foundation_tests, qt_tests)" ON)   # line 75-76
...
option(PORTABLE_VERSION "Whether to build the portable version." ON)           # line 285
option(DEVELOPER_VERSION "Whether to enable the internal debug features." ${enable_developer_version_default})   # line 303
```

Copy: one descriptive-string option/cache var, default off, declared before `add_subdirectory(src)` (line 340) so all targets inherit `add_compile_options`/`add_link_options`. Place it next to the new Clang block (global, before any `add_subdirectory`). RESEARCH Code Examples gives the body (`set(SANITIZE "" CACHE STRING ...)`, `-fsanitize=${SANITIZE}`, `-fsanitize-recover=address`). `CMAKE_BUILD_TYPE` default handling is in `cmake/SetDefaultBuildType.cmake` (`st_set_default_build_type(Release)`, line 73); presets set `CMAKE_BUILD_TYPE` explicitly so it is bypassed.

---

### `CMakeLists.txt` - Boost bridge (config)

**Analog:** lines 199-216 (current Boost discovery) and 257-263 (global `link_libraries`).

```cmake
set(Boost_USE_MULTITHREADED ON)
if (BUILD_TESTS)
  find_package(Boost 1.60 COMPONENTS unit_test_framework REQUIRED)
else()
  find_package(Boost 1.60 REQUIRED)
endif()

include_directories(${Boost_INCLUDE_DIRS})                    # line 207
add_definitions(-DBOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
if (BUILD_TESTS)
  if (NOT Boost_USE_STATIC_LIBS)
    ...
    add_definitions(-DBOOST_TEST_DYN_LINK)                    # line 214
  endif()
endif()
...
if (UNIX)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  link_libraries(Threads::Threads)                            # line 262
endif()
```

Copy: after line 207, add `if (TARGET Boost::headers) link_libraries(Boost::headers) endif()` (additive, keeps line 207). The tests link `Boost::unit_test_framework` as an imported target already (`src/core/tests/CMakeLists.txt`), so the config-mode targets are the expected naming. Note `Boost_USE_STATIC_LIBS` is only ON for WIN32 (line 197), so on macOS `BOOST_TEST_DYN_LINK` is defined. Check `Boost_INCLUDE_DIRS`/`Boost_DIR` at the first configure before deciding to keep the bridge.

---

### `CMakePresets.json` (config, build-time) - NO ANALOG

No existing preset file in the repo (`CMakeUserPresets.json` is gitignored; `CMakePresets.json` is not). Use RESEARCH.md "Preset skeleton (schema v6)" as the template: hidden `macos-base` (Ninja, `binaryDir: ${sourceDir}/build/${presetName}`, `CMAKE_PREFIX_PATH=/opt/homebrew`, `CMAKE_EXPORT_COMPILE_COMMANDS=ON`, `PORTABLE_VERSION=OFF`, `BUILD_TESTS=ON`), `macos-release`/`macos-debug`/`macos-asan` inheriting from it, matching build and test presets, ASan test preset `environment` with `ASAN_OPTIONS=detect_leaks=0:halt_on_error=0`. Option names must match the CMake `option()` names above (`PORTABLE_VERSION`, `BUILD_TESTS`, new `SANITIZE`). `build/` is already gitignored, so per-preset binary dirs need no new ignore line. Do not add `-ffp-contract` only here; keep it in `CMakeLists.txt` (RESEARCH Open Question 6) unless the user prefers otherwise.

---

### Test registration (read-only reference for test presets)

**Analogs:** the five existing `add_test` lines; do not edit.

```cmake
add_test(NAME core_tests COMMAND core_tests --log_level=message)        # src/core/tests/CMakeLists.txt:20
add_test(NAME foundation_tests COMMAND foundation_tests --log_level=message)   # src/foundation/tests/CMakeLists.txt:13
add_test(NAME qt_tests COMMAND qt_tests)                                # src/qt_tests/CMakeLists.txt:12
```

Pattern: `add_executable(<name>_tests ${sources})` + `target_link_libraries(<name>_tests PRIVATE <module> Boost::unit_test_framework ${EXTRA_LIBS})` + `add_test(NAME <name>_tests COMMAND <name>_tests --log_level=message)`. `qt_tests` links `${Qt_Test_lib}` and has no log flag, and creates a `QApplication`, so it may need `QT_QPA_PLATFORM=offscreen` in the test preset `environment` (record the result either way). One ctest entry per executable; case-level results need the Boost.Test flags run directly (RESEARCH "Don't Hand-Roll").

---

### Records under `.planning/phases/01-deletion-free-macos-build/records/` (record)

**Analog for naming and ignore safety:** `.gitignore:155` is `*.log`; also ignored: `*.tmp`, `*.bak`, `*.cache`, directories `Testing` and `test_results`. Use `.txt`, `.md`, `.xml`, `.json` only. After writing: `git check-ignore -v <file>` must print nothing.

**Capture pattern** (RESEARCH Pattern 3): redirect real command output, never hand-summarise.

```bash
cmake --build --preset macos-release 2>&1 | tee .planning/phases/01-deletion-free-macos-build/records/warnings-release.txt
grep -o '\[-W[a-z0-9-]*\]' records/warnings-release.txt | sort | uniq -c | sort -rn > records/warning-counts.txt
ctest --preset macos-release --output-junit records/ctest-release.junit.xml 2>&1 | tee records/ctest-release.txt
```

Also from TESTING.md (tracked, Spanish): per-suite direct runs `./core_tests --log_level=message`, `./qt_tests`.

### Hand-written markdown records (`fix-log.md`, `fp-contract.md`, `SMOKE-FINDINGS.md`)

**Analog:** `.planning/phases/01-deletion-free-macos-build/01-CONTEXT.md` and `01-RESEARCH.md` conventions: `#` title with phase name, `**Label:**` bold metadata lines, `##` sections, backticked paths with line numbers, decision IDs (`D-03`) cited, table for per-item results, no emoji, end with `---` and a `*Phase: ...*` footer line. `fix-log.md` is one line/row per edit: file, error class, commit hash, reason (D-07). `fp-contract.md` uses the three-compile evidence block from RESEARCH "FP-contract evidence". `SMOKE-FINDINGS.md` holds smoke results, measured DPR, settings paths (INI under `~/.config/scantailor-advanced`, not plist: RESEARCH Pitfall 1), and logged-only observations (Retina blur, shortcuts, alarm at `src/app/MainWindow.cpp:1337`).

## Shared Patterns

### One commit per fix, outside-guard, additive
**Apply to:** every CMake edit. Existing lines are not deleted except `cmake_minimum_required` line 1 (list in the closing record); new content mirrors the two-space `if (...)` / `endif()` style with a space after `if`, as throughout `CMakeLists.txt`.

### Success-criterion-5 diff
**Apply to:** closing check. Use `git diff --stat 5eaac18 -- . ':!.planning' ':!.claude'` (`.claude/CLAUDE.md` already differs) and `git diff --numstat 5eaac18 -- src`.

### Source edits are out of scope except D-07 fixes
**Apply to:** all of `src/`. No formatting passes (`clang-format -i` would add noise); no warning-driven edits (D-08); stop and ask if a fix could change arithmetic or output.

### Include-root / target conventions (reference for any D-07 fix)
**Source:** `src/app/CMakeLists.txt` lines 72-84: `add_executable(${target_name} WIN32 ...)` has no `MACOSX_BUNDLE`; output dir is `CMAKE_RUNTIME_OUTPUT_DIRECTORY` = binary root (`CMakeLists.txt:78-80`), so the app, the five test executables and `.qm` files all land in `build/<preset>/`. `TRANSLATION_DIRS` and `PLUGIN_DIRS` (lines 290-296) are relative to that binary.

## No Analog Found

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| `CMakePresets.json` | config | build-time | No presets in the repo; use RESEARCH.md skeleton |
| `records/*.txt`, `records/*.xml` | record | captured output | No captured-output records exist yet; follow Pattern 3 and the ignore rules |

## Metadata

**Analog search scope:** `CMakeLists.txt`, `cmake/`, `src/{core,foundation,qt_tests}/tests/CMakeLists.txt`, `src/app/CMakeLists.txt`, `TESTING.md`, `.planning/`
**Files scanned:** about 12
**Pattern extraction date:** 2026-10-18
