---
phase: 03-qt-6-and-macos-only
plan: 03
subsystem: build
tags: [cmake, packaging, github-actions, flatpak, cpack, strip, gate]

requires:
  - phase: 03-qt-6-and-macos-only
    provides: gate.sh with build-command diff and Qt 6 only CMake (plans 03-01, 03-02)
provides:
  - No GitHub automation left (workflows, Dependabot, upstream issue helper script)
  - No packaging, install or uninstall rule left in any CMake file
  - Flatpak manifest, build-deb.sh, build-windows.sh and the uninstall template deleted
  - BUILD_TESTS option reworded without Flatpak
affects: [03-04, 03-05, 03-06, 03-07, 03-08, 03-09]

actuals:
  tokens: 11700
  tasks: 2
  commits: 4
plan_head_before: 8edaa7806ee20a81445f8d634fb4c21e4d6a3137
plan_head_after: 9c38fb8c6fe65dab5f2042d68a1c5f072ff1c253

tech-stack:
  added: []
  patterns:
    - "Delete whole directories only after checking git ls-files against the expected list"
    - "A removal commit is gated with an empty build-command diff as proof that nothing the build reads was touched"

key-files:
  created: []
  modified:
    - CMakeLists.txt
    - src/app/CMakeLists.txt
    - .planning/phases/03-qt-6-and-macos-only/records/gate-log.md
  deleted:
    - .github/dependabot.yml
    - .github/workflows/ci.yml
    - .github/workflows/cmake.yml
    - .github/workflows/flatpak.yml
    - .github/workflows/lint.yml
    - .github/workflows/release.yml
    - scripts/post_issue_followups.py
    - flatpak/org.scantailor.Advanced.json
    - build-deb.sh
    - build-windows.sh
    - cmake/cmake_uninstall.cmake.in

key-decisions:
  - "The lint workflow was the only enforcement of .clang-format; formatting becomes a documented manual step in TESTING.md (plan 03-07)"
  - "CMAKE_RUNTIME_OUTPUT_DIRECTORY, compile_translations, the .desktop configure_file and APPLICATION_VERSION_* stay for c07, c08 and c10"

requirements-completed: [STRIP-05, STRIP-04]

coverage:
  - id: D1
    description: "No workflow, Dependabot configuration or upstream issue helper remains; .github and scripts are gone from disk and index (ROADMAP criterion 3)"
    requirement: STRIP-05
    verification:
      - kind: other
        ref: "git ls-files .github scripts prints nothing; test ! -e .github; gate.sh check c04-ci (result: PASS, empty command diff)"
        status: pass
    human_judgment: false
  - id: D2
    description: "No Flatpak manifest, DEB or Windows build script, CPack or NSIS setting, install() rule, InstallRequiredSystemLibraries call, uninstall target or windeployqt step remains"
    requirement: STRIP-04
    verification:
      - kind: other
        ref: "grep -rnI -e 'install(' -e CPack -e CPACK -e uninstall -e InstallRequiredSystemLibraries -e CMAKE_INSTALL -e windeployqt -e QT_BINDIR CMakeLists.txt cmake src (no output); gate.sh check c05-packaging (result: PASS, empty command diff)"
        status: pass
    human_judgment: false

duration: 13min
completed: 2026-10-24
status: complete
---

# Phase 3 Plan 03: GitHub automation and packaging removal Summary

**The fork carries no CI, Dependabot, Flatpak, DEB, Windows installer or CMake install/uninstall rule any more; two gated commits remove 1,098 lines with an empty build-command diff in Release and Debug**

## Performance

- **Duration:** about 13 min (two gates of about 4 min each)
- **Started:** 2026-10-24T20:14:58Z
- **Completed:** 2026-10-24T20:27:30Z
- **Tasks:** 2
- **Files modified:** 14 (11 deleted, 2 CMake files edited, the gate log)

## Accomplishments

- c04: `git ls-files .github` listed exactly the six expected files and `git ls-files scripts` only `post_issue_followups.py`, so `git rm -r .github scripts` removed nothing beyond STRIP-05. Neither directory exists on disk any more.
- c05: deleted `flatpak/org.scantailor.Advanced.json`, `build-deb.sh`, `build-windows.sh` and `cmake/cmake_uninstall.cmake.in`. In `CMakeLists.txt` removed `CMAKE_INSTALL_RPATH_USE_LINK_PATH`, `CMAKE_INSTALL_BINDIR`, `QT_BINDIR`, the translation, desktop file, icon and MIME install rules, the whole Windows runtime-library block (`add_runtime_libs_to_install`, `set_if_undefined`, `dep_libs`, `copy_to_build_dir`, MinGW runtime), the UCRT/system-runtime install, the CPack/NSIS section and the uninstall target. In `src/app/CMakeLists.txt` removed `install(TARGETS)` and the `windeployqt` block.
- `option(BUILD_TESTS "Build the unit and pipeline tests" ON)` replaces the Flatpak-flavoured description.
- Kept on purpose: `CMAKE_RUNTIME_OUTPUT_DIRECTORY`, `add_custom_target(compile_translations ALL ...)`, the `.desktop.in` `configure_file` (c08) and the `APPLICATION_VERSION_*` lines (c07). `cmake --preset macos-release` configures cleanly.
- Both commits passed the gate: `100% tests passed out of 7` in Release and Debug, goldens unchanged against a796c2ae, both `cmds-*.diff` empty. Rows are in `records/gate-log.md`.

## Task Commits

1. **Task 1 (c04): remove GitHub automation** - `462f22cd` (chore), gate row `3a71b334` (docs)
2. **Task 2 (c05): remove packaging and install rules** - `017e977c` (build), gate row `9c38fb8c` (docs)

**Plan metadata:** added by the final docs commit of this plan.

## Files Created/Modified

- `CMakeLists.txt` - 134 lines of packaging, install and uninstall logic removed from the tail, plus four smaller removals and the BUILD_TESTS rewording
- `src/app/CMakeLists.txt` - `install(TARGETS)` and the Windows deployment block removed
- `.planning/phases/03-qt-6-and-macos-only/records/gate-log.md` - rows for c04-ci and c05-packaging

## Decisions Made

- `.gitignore` keeps its packaging patterns (harmless, ED-12), as flagged in the plan.
- Existing build directories keep a stale cache entry for the removed RPATH option; the clean-clone configure in plan 03-07 is the check for that (recorded in the c05 commit body).

## Deviations from Plan

None - plan executed exactly as written.

One environment note, not a deviation: the plan's `git log --grep` verify commands rely on `^...$` anchors, which work with the default regex mode; combining them with `--fixed-strings` returns nothing, so the plan's original form was used.

## Issues Encountered

None.

## Known Stubs

None.

## Threat Flags

None. T-03-06 mitigated (directory content checked before deletion, no automation with repository or `gh` credentials remains). T-03-07 mitigated (blocks still read by the Mac build kept, configure run before commit, command diff empty in both presets).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 03-04 can start. The gate reference for the current tree is written by the c05 gate.
- Still present for later commits: the Windows helper includes and `cmake/CopyToBuildDir.cmake`, `cmake/AddDynamicLibraryLocations.cmake` (c07), the `.desktop.in` configure_file and `src/resources/unix` (c08), the translation targets (c10).

## Self-Check: PASSED

- Commits `462f22cd`, `3a71b334`, `017e977c`, `9c38fb8c` are ancestors of HEAD (`git rev-list --count 8edaa780..HEAD` = 4).
- Gate summaries of c04-ci and c05-packaging end with `result: PASS`; `tested-commit.txt` equals the task commits; all automated verify commands of both tasks pass.
- `git ls-files .github scripts flatpak build-deb.sh build-windows.sh cmake/cmake_uninstall.cmake.in` prints nothing; the packaging search prints nothing; `src/pipeline_tests/golden` has no change against a796c2ae.

---
*Phase: 03-qt-6-and-macos-only*
*Completed: 2026-10-24*
