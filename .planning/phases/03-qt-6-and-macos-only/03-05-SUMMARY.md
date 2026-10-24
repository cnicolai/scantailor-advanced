---
phase: 03-qt-6-and-macos-only
plan: 05
subsystem: build
tags: [unifdef, cmake, linux-removal, portable-mode, strip, gate]

requires:
  - phase: 03-qt-6-and-macos-only
    provides: gate.sh with build-command diff, Windows removal (plans 03-01 to 03-04)
provides:
  - No Q_OS_ conditional, UNIX block, GCC flag block, .desktop.in reference or Linux resource file left in the tree
  - Portable mode removed (option, define, preset variable, Application API, two branches)
affects: [03-06, 03-07, 03-08, 03-09]

actuals:
  tokens: 2700
  tasks: 2
  commits: 4
plan_head_before: d800dfa84d4d38191da0523bac84ae76f90f89a4
plan_head_after: 2ca1c42b1a4d094d65ed1399d70ba52e017739ab

tech-stack:
  added: []
  patterns:
    - "Platform conditionals resolved with unifdef -D for the macros that are defined on the Mac; the numstat (0 additions, 10 deletions) is the proof that nothing else moved"
    - "A CMake resource file and the configure_file that reads it leave in one commit"

key-files:
  created: []
  modified:
    - CMakeLists.txt
    - CMakePresets.json
    - config.h.in
    - src/app/MainWindow.cpp
    - src/app/main.cpp
    - src/core/Application.h
    - src/core/Application.cpp
    - src/core/DefaultParamsProfileManager.cpp
    - src/core/zones/ZoneContextMenuInteraction.h
    - src/core/zones/ZoneContextMenuInteraction.cpp
    - src/pipeline_tests/main.cpp
    - .planning/phases/03-qt-6-and-macos-only/records/gate-log.md
  deleted:
    - src/resources/unix/scantailor-advanced.desktop.in
    - src/resources/unix/mime/scantailor-project.xml
    - src/resources/scantailor-advanced.png

key-decisions:
  - "The Clang block comment of CMakeLists.txt still says 'Clang counterpart of the GNU warning set above'; the GNU set is gone but the block stays byte-for-byte equal to a796c2ae (D-01)"
  - "CMAKE_POSITION_INDEPENDENT_CODE and the Threads lines are unwrapped, not deleted: -fPIC and pthread linkage are part of the Mac build commands"

requirements-completed: [STRIP-03, STRIP-02]

coverage:
  - id: D1
    description: "No Linux code path, flag block, option or resource file remains; src/resources/appicon.svg stays"
    requirement: STRIP-03
    verification:
      - kind: other
        ref: "grep -rnI -e Q_OS_ -e UNIX -e CMAKE_COMPILER_IS_GNU -e march=native -e desktop.in -e 'unix/' -e scantailor-advanced.png CMakeLists.txt cmake src --exclude-dir=translations prints nothing; gate.sh check c08-linux (result: PASS, empty command diff)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The .desktop.in configure_file and src/resources/unix leave in the same commit; macOS sides (batch alarm play command and lookup, m_extraDelaysDone code, Clang and SANITIZE blocks) unchanged"
    requirement: STRIP-03
    verification:
      - kind: other
        ref: "git show --name-only 450ff19f lists CMakeLists.txt and src/resources/unix/scantailor-advanced.desktop.in; unifdef numstat 0 additions / 10 deletions; Clang and SANITIZE blocks diff empty against a796c2ae"
        status: pass
    human_judgment: false
  - id: D3
    description: "Portable mode is gone; INI format line and AppData profile path unchanged"
    requirement: STRIP-02
    verification:
      - kind: other
        ref: "portable-mode search prints nothing; QSettings::setDefaultFormat(QSettings::IniFormat) and m_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + \"/profiles\" present; gate.sh check c09-portable (result: PASS, empty command diff)"
        status: pass
    human_judgment: false

duration: 13min
completed: 2026-10-24
status: complete
---

# Phase 3 Plan 05: Linux and portable-mode removal Summary

**Linux is gone from code, CMake and resources (unifdef, 10 deletions, 0 additions; GCC flag block, UNIX wrappers, .desktop.in and src/resources/unix removed in one commit) and portable mode is gone from Application, main.cpp, the profile manager, CMake, config.h.in and the presets; both gated commits leave the macOS build commands byte-identical**

## Performance

- **Duration:** about 13 min (two gates of about 4 min each)
- **Completed:** 2026-10-24
- **Tasks:** 2
- **Files modified:** 12 edited, 3 deleted

## Accomplishments

- c08: `unifdef -m -DQ_OS_MAC -DQ_OS_UNIX` over `ZoneContextMenuInteraction.{h,cpp}` and `MainWindow.cpp` removed 2, 4 and 4 guard lines and added none. The batch alarm keeps `QString extPlayCmd("play /usr/share/sounds/freedesktop/stereo/bell.oga");` and its `main_window/external_alarm_cmd` lookup (D-04); only the `#if`, `#else`, empty `QString extPlayCmd;` and `#endif` lines went. `m_extraDelaysDone` code is intact.
- c08 CMake: deleted the GCC flag block (`-Wpedantic`, `-march=native`), unwrapped `option(CMAKE_POSITION_INDEPENDENT_CODE "" ON)` and the three Threads lines, deleted the `.desktop.in` `configure_file`. `src/resources/unix/` and `src/resources/scantailor-advanced.png` were deleted in the same commit (Pitfall 14); `cmake --preset macos-release` configured before the commit.
- c09: removed `Application::isPortableVersion`, `getPortableConfigPath`, `initPortableVersion`, `m_portableConfigPath`, `#include <QTemporaryDir>` from Application.cpp, the portable `setPath` branch of `main.cpp`, the portable branch and `qApp` cast of `DefaultParamsProfileManager`, the `PORTABLE_VERSION` option and block, `PORTABLE_CONFIG_DIR` in `config.h.in`, the `PORTABLE_VERSION` preset variable, and the removed-cast wording in the `pipeline_tests/main.cpp` comment (`(the class the application itself uses)`).
- Both commits passed the gate: `100% tests passed out of 7` in Release and Debug, goldens unchanged, `cmds-*.diff` empty for both presets. Rows are in `records/gate-log.md`.

## Task Commits

1. **Task 1 (c08): remove Linux code paths and files** - `450ff19f` (refactor), gate row `4b25cd02` (docs)
2. **Task 2 (c09): remove portable mode** - `1b1bec27` (refactor), gate row `2ca1c42b` (docs)

**Plan metadata:** added by the final docs commit of this plan.

## Files Created/Modified

- `CMakeLists.txt`, `CMakePresets.json`, `config.h.in` - Linux flag block, `.desktop.in`, portable option, define and preset variable removed
- `src/core/zones/ZoneContextMenuInteraction.{h,cpp}`, `src/app/MainWindow.cpp` - Q_OS_ guards resolved
- `src/core/Application.{h,cpp}`, `src/app/main.cpp`, `src/core/DefaultParamsProfileManager.cpp`, `src/pipeline_tests/main.cpp` - portable mode removed
- Deleted: `src/resources/unix/` (two files), `src/resources/scantailor-advanced.png`

## Decisions Made

- The Clang flag block and its now-stale comment stay byte-for-byte (D-01, ED-12), as the commit body of c08 says.
- Settings and profile locations of preset builds were verified by source reading only: INI format from `QSettings::setDefaultFormat(QSettings::IniFormat)` and profiles from `QStandardPaths::AppDataLocation + "/profiles"`. No real settings or profiles were read or written (D-07).

## Deviations from Plan

None - plan executed exactly as written.

Environment notes, not deviations:
- Hand edits after `unifdef` were none; `xcrun clang-format -i` made no change to the three unifdef-touched files (the dry-run was clean before and after).
- `git diff --numstat` also listed the unrelated pre-existing `.planning/config.json` change; the unifdef numstat file was restricted to `src`.
- The plan's verify command for Task 1 uses two `<automated>` blocks per `<verify>`; all four were run individually and passed.

## Issues Encountered

None.

## Known Stubs

None.

## Threat Flags

None. T-03-10 mitigated (only the branch that never ran in preset builds was removed; INI format line and AppData profile path asserted; nothing read or wrote user files). T-03-11 mitigated (`.desktop.in` configure_file and `src/resources/unix/` in one commit; configure ran before commit; gate built both presets). T-03-12 accepted (alarm kept for Phase 5). T-03-SC: no package installed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 03-06 can start. The gate reference for the current tree is written by the c09 gate.
- Still present for later commits: `TRANSLATION_DIRS`/`PLUGIN_DIRS` and the translation targets (c10).

## Self-Check: PASSED

- Commits `450ff19f`, `4b25cd02`, `1b1bec27`, `2ca1c42b` are ancestors of HEAD (`git rev-list --count d800dfa8..HEAD` = 4 before this SUMMARY).
- Gate summaries of c08-linux and c09-portable end with `result: PASS`; `tested-commit.txt` equals the task commits.
- `git ls-files src/resources/unix src/resources/scantailor-advanced.png` prints nothing; `src/resources/appicon.svg` exists; `git diff a796c2ae -- src/pipeline_tests/golden` is empty; `xcrun clang-format --dry-run -Werror` passes for all eight touched C++ files.

---
*Phase: 03-qt-6-and-macos-only*
*Completed: 2026-10-24*
