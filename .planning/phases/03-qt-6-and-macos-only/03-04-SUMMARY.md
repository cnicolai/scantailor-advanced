---
phase: 03-qt-6-and-macos-only
plan: 04
subsystem: build
tags: [unifdef, cmake, windows-removal, strip, gate, backlog]

requires:
  - phase: 03-qt-6-and-macos-only
    provides: gate.sh with build-command diff, Qt 6 only CMake, packaging removed (plans 03-01 to 03-03)
provides:
  - No _WIN32 or Q_OS_WIN conditional left in any .cpp or .h file
  - No Windows option, block, helper, variable or resource file left in the CMake build
  - Backlog item 999.8 for the backslash handling of RelinkablePath::normalize
affects: [03-05, 03-06, 03-07, 03-08, 03-09]

actuals:
  tokens: 6600
  tasks: 2
  commits: 5
plan_head_before: fd5c84ffcbf7f942e0a2e45d6687ddae04aea319
plan_head_after: f3b4b020ccf355bbd5763b8cae7f26731ae95176

tech-stack:
  added: []
  patterns:
    - "Platform conditionals are resolved with unifdef -U for the macros that are undefined on the Mac; the numstat (0 additions, 29 deletions) is the proof that nothing else moved"
    - "Windows-minded code without a platform label that still runs on the Mac stays and is tracked in the backlog instead"

key-files:
  created:
    - .planning/phases/999.8-relinkablepath-normalize-turns-a-backslash-in-a-macos-file-n/.gitkeep
  modified:
    - src/core/Utils.cpp
    - src/core/DarkColorScheme.cpp
    - src/core/LightColorScheme.cpp
    - src/core/RelinkablePath.cpp
    - src/app/main.cpp
    - src/imageproc/ByteOrder.h
    - CMakeLists.txt
    - src/app/CMakeLists.txt
    - src/core/tests/CMakeLists.txt
    - src/pipeline_tests/CMakeLists.txt
    - src/foundation/tests/CMakeLists.txt
    - src/math/tests/CMakeLists.txt
    - src/imageproc/tests/CMakeLists.txt
    - src/qt_tests/CMakeLists.txt
    - src/dewarping/tests/CMakeLists.txt
    - .planning/ROADMAP.md
    - .planning/phases/03-qt-6-and-macos-only/records/gate-log.md
  deleted:
    - cmake/CopyToBuildDir.cmake
    - cmake/copy_to_build_dir.cmake.in
    - cmake/AddDynamicLibraryLocations.cmake
    - src/resources/win32/resources.rc.in
    - src/resources/win32/scantailor-advanced.ico

key-decisions:
  - "RelinkablePath::normalize keeps its unlabelled backslash replacement and drive-letter check (D-03); only the labelled #if _WIN32 lines went, and backlog item 999.8 records the case"
  - "The Boost.Test define is now unconditional: -DBOOST_TEST_DYN_LINK stays because the Mac never set Boost_USE_STATIC_LIBS"
  - "The empty Dependencies section header went with the DEPENDENCIES_ROOT block it introduced"

requirements-completed: [STRIP-02]

coverage:
  - id: D1
    description: "No _WIN32 or Q_OS_WIN conditional remains in C++ sources and headers; the macOS sides (rename(), base 16 in both colour schemes, netinet/in.h) are intact"
    requirement: STRIP-02
    verification:
      - kind: other
        ref: "grep -rnI -e _WIN32 -e Q_OS_WIN src --include='*.cpp' --include='*.h' prints nothing; numstat 0 additions / 29 deletions in six files; gate.sh check c06-windows-cpp (result: PASS, empty command diff)"
        status: pass
    human_judgment: false
  - id: D2
    description: "No Windows build option, block, helper or resource file remains; cmake/ lists only ListItemsPrepend, SetDefaultBuildType and UpdateTranslations"
    requirement: STRIP-02
    verification:
      - kind: other
        ref: "Windows search over CMakeLists.txt, CMakePresets.json, cmake, config.h.in and src prints nothing; gate.sh check c07-windows-cmake (result: PASS, empty command diff)"
        status: pass
    human_judgment: false
  - id: D3
    description: "Backlog item 999.8 describes the kept backslash handling of RelinkablePath::normalize"
    verification:
      - kind: other
        ref: "grep -c '^### Phase 999.8:' .planning/ROADMAP.md prints 1; the backlog commit removes no ROADMAP line"
        status: pass
    human_judgment: true
    rationale: "Whether the wording of the backlog goal is adequate is a reading judgment; no test asserts it"

duration: 25min
completed: 2026-10-24
status: complete
---

# Phase 3 Plan 04: Windows removal Summary

**Windows is gone from the C++ code (unifdef, 29 deletions, 0 additions) and from the CMake build (options, blocks, helpers, resources, EXTRA_LIBS); both gated commits leave the macOS build commands byte-identical, and the kept backslash handling of RelinkablePath::normalize is filed as backlog item 999.8**

## Performance

- **Duration:** about 25 min (two gates of about 4 min each)
- **Completed:** 2026-10-24
- **Tasks:** 2
- **Files modified:** 14 edited, 5 deleted, 1 created (.gitkeep), plus ROADMAP.md and the gate log

## Accomplishments

- c06: `unifdef -UQ_OS_WIN -U_WIN32` over six files removed exactly 29 lines (Utils.cpp 8, DarkColorScheme 4, LightColorScheme 4, RelinkablePath 3, main.cpp 4, ByteOrder.h 6) and added none. The macOS sides remain: `rename(...)` in `Utils::overwritingRename`, base 16 in both colour schemes, `<netinet/in.h>`. `RelinkablePath::normalize` lost only its `#if _WIN32` clause inside the empty-component test.
- Backlog 999.8: `phase.next-decimal 999` returned 999.8; the entry is appended under `## Backlog` with a scoped Edit (no removed line), plus `.planning/phases/999.8-relinkablepath-normalize-turns-a-backslash-in-a-macos-file-n/.gitkeep`.
- c07: removed the MSVC/WIN_XP flag block, the helper includes, the DEPENDENCIES_ROOT block, the four `add_dynamic_library_locations` blocks (TIFF's `USE_LIBTIFF_DLL` with it), the MinGW Boost block, the `System` section with `EXTRA_LIBS`, `NOMINMAX`, the Windows `TRANSLATION_DIRS`/`PLUGIN_DIRS` branch and the three `APPLICATION_VERSION_*` lines. `src/app/CMakeLists.txt` lost the win32 resource block, the `WIN32` keyword and `${EXTRA_LIBS}`; seven test CMake files lost `${EXTRA_LIBS}`. Deleted the three Windows cmake helpers and `src/resources/win32/`.
- The Clang block of `CMakeLists.txt` is byte-identical to a796c2ae.
- Both commits passed the gate: `100% tests passed out of 7` in Release and Debug, goldens unchanged, `cmds-*.diff` empty for both presets. Rows are in `records/gate-log.md`.

## Task Commits

1. **Task 1 (c06): remove Windows code paths** - `ad15b3ef` (refactor), backlog item `0312ab01` (docs), gate row `ff6488c4` (docs)
2. **Task 2 (c07): remove Windows build options and files** - `85e095d4` (build), gate row `f3b4b020` (docs)

**Plan metadata:** added by the final docs commit of this plan.

## Files Created/Modified

- `src/core/Utils.cpp`, `DarkColorScheme.cpp`, `LightColorScheme.cpp`, `RelinkablePath.cpp`, `src/app/main.cpp`, `src/imageproc/ByteOrder.h` - Windows branches resolved
- `CMakeLists.txt`, `src/app/CMakeLists.txt`, seven test `CMakeLists.txt` - Windows options and `EXTRA_LIBS` removed
- `.planning/ROADMAP.md` - backlog item 999.8 appended

## Decisions Made

- D-03 honoured: the unlabelled backslash replacement and the `endsWith(':')` drive-letter check stay in `RelinkablePath::normalize`; `QDir::toNativeSeparators` calls untouched.
- `DEBUG_CLI` was read only inside the removed MSVC block and has no other reader.
- The Dependencies section header was removed together with its only content.

## Deviations from Plan

None - plan executed exactly as written.

Environment notes, not deviations:
- `git diff --numstat` also listed the unrelated pre-existing change to `.planning/config.json`; the six-line numstat file was filtered to the source files before the check.
- After unifdef, `main.cpp` kept two consecutive blank lines (clang-format allows two); one was removed by hand, so the committed `main.cpp` diff shows 5 deletions and the whole commit 32 deletions and 1 insertion (the `RelinkablePath.cpp` condition re-wrapped by clang-format). The unifdef step itself measured 29 deletions and 0 additions.
- The plan's verify command `grep -q 'add_executable(ui_snapshot' src/app/CMakeLists.txt` does not match the multi-line layout `add_executable(` / `ui_snapshot ...` that plan 03-01 wrote; the target is intact (checked with `ui_snapshot ui_snapshot/main.cpp`).

## Issues Encountered

None.

## Known Stubs

None.

## Threat Flags

None. T-03-08 mitigated (unifdef -U only, zero additions, kept macOS lines checked by grep, goldens and command diff through the gate). T-03-09 accepted and tracked as backlog 999.8. T-03-SC: no package installed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 03-05 can start. The gate reference for the current tree is written by the c07 gate.
- Still present for later commits: the GNU compiler block and `.desktop.in` configure_file (c08), `TRANSLATION_DIRS`/`PLUGIN_DIRS` (c10), the translation targets (c10).

## Self-Check: PASSED

- Commits `ad15b3ef`, `0312ab01`, `ff6488c4`, `85e095d4`, `f3b4b020` are ancestors of HEAD (`git rev-list --count fd5c84ff..HEAD` = 5 before this SUMMARY).
- Gate summaries of c06-windows-cpp and c07-windows-cmake end with `result: PASS`; `tested-commit.txt` equals the task commits.
- `git ls-files cmake` lists the three expected files; `src/resources/win32` does not exist; `git diff a796c2ae -- src/pipeline_tests/golden` is empty; `xcrun clang-format --dry-run -Werror` passes for the six C++ files.

---
*Phase: 03-qt-6-and-macos-only*
*Completed: 2026-10-24*
