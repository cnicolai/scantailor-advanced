---
phase: 03-qt-6-and-macos-only
verified: 2026-10-24T22:30:00Z
status: passed
score: 5/5 must-haves verified
covered_files:
  - ".planning/phases/03-qt-6-and-macos-only/03-01-PLAN.md"
  - ".planning/phases/03-qt-6-and-macos-only/03-01-SUMMARY.md"
  - ".planning/phases/03-qt-6-and-macos-only/03-02-PLAN.md"
  - ".planning/phases/03-qt-6-and-macos-only/03-02-SUMMARY.md"
  - ".planning/phases/03-qt-6-and-macos-only/03-03-PLAN.md"
  - ".planning/phases/03-qt-6-and-macos-only/03-03-SUMMARY.md"
  - ".planning/phases/03-qt-6-and-macos-only/03-04-PLAN.md"
  - ".planning/phases/03-qt-6-and-macos-only/03-04-SUMMARY.md"
  - ".planning/phases/03-qt-6-and-macos-only/03-05-PLAN.md"
  - ".planning/phases/03-qt-6-and-macos-only/03-05-SUMMARY.md"
  - ".planning/phases/03-qt-6-and-macos-only/03-06-PLAN.md"
  - ".planning/phases/03-qt-6-and-macos-only/03-06-SUMMARY.md"
  - ".planning/phases/03-qt-6-and-macos-only/03-07-PLAN.md"
  - ".planning/phases/03-qt-6-and-macos-only/03-07-SUMMARY.md"
  - ".planning/phases/03-qt-6-and-macos-only/03-08-PLAN.md"
  - ".planning/phases/03-qt-6-and-macos-only/03-08-SUMMARY.md"
  - ".planning/phases/03-qt-6-and-macos-only/03-09-PLAN.md"
  - ".planning/phases/03-qt-6-and-macos-only/03-09-SUMMARY.md"
  - "CMakeLists.txt"
  - "CMakePresets.json"
  - "README.md"
  - "TESTING.md"
  - "config.h.in"
  - "src/CMakeLists.txt"
  - "src/app/CMakeLists.txt"
  - "src/app/main.cpp"
  - "src/app/ui_snapshot/main.cpp"
  - "src/test_support/Manifest.cpp"
covered_digest: "v3:sha256:652b0db1704e1b14f20404f33baf6b1b6e5e86ffc89e33e8b723c2a3e37789fd"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 3: Qt 6 and macOS Only Verification Report

**Phase Goal:** The codebase contains only what the Mac build needs: Qt 6, macOS code paths, an English UI, no packaging and no CI. The baselines prove that output is unchanged.
**Verified:** 2026-10-24
**Status:** passed
**Re-verification:** No, initial verification

All evidence below was re-run or re-read on HEAD (`639e1a97`) by the verifier. The phase records were used as leads only.

## Goal Achievement

### Observable Truths (ROADMAP success criteria)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Searching CMake files and `src/` for `Qt5` and `QT_VERSION` finds nothing, and full `ctest` passes with every baseline unchanged | VERIFIED | `grep -rnI -e Qt5 -e QT_VERSION CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in src` prints nothing, exit 1. `CMakeLists.txt:105` is `find_package(Qt6 6.5 ... CONFIG REQUIRED)` with no Qt 5 fallback. `cmake --build --preset macos-release` gave "no work to do"; `ctest --preset macos-release` gave `100% tests passed out of 7` (54.80 s, pipeline_tests 53.72 s, so the golden comparison really ran). `git diff a796c2ae HEAD` over `src/pipeline_tests/golden/` is empty, so no golden image or MANIFEST changed. Test code touched by Phase 3 is limited to 10 files, mostly CMake link lines. The only logic change is `Manifest.cpp`, which swaps `QT_VERSION_STR` for the CMake-fed `ST_QT_COMPILE_VERSION`. `Tolerances.h` and the comparator were not touched in Phase 3, so the gate was not weakened. |
| 2 | No Windows or Linux code paths, build options or files remain, no packaging remains; a clean build directory configures, builds and passes `ctest` | VERIFIED | The platform grep (`_WIN32`, `Q_OS_`, `WIN32`, `MSVC`, `MINGW`, `__APPLE__`, `UNIX`, `PORTABLE`, `XDG_SESSION_TYPE`, `Wayland`, ...) over `CMakeLists.txt CMakePresets.json cmake config.h.in src` prints nothing. The top level has no `flatpak/`, `scripts/`, `build-*.sh`; `cmake/` holds only `ListItemsPrepend.cmake` and `SetDefaultBuildType.cmake`; `src/resources` has no `unix/`, `win32/` or `.png`. The `install(`/CPack/uninstall/windeployqt grep over CMake files is empty. `git ls-files` has no flatpak, desktop, mime, `.ts`, `.qm`, `build-*.sh`, NSIS or `.github` entries. Remaining "Windows" words in `src` are comments and unlabelled code kept on purpose (D-03, backlog 999.8). A clean directory: `records/fresh-clone.md` shows a fresh clone of the commit, 510 build steps from nothing, `100% tests passed out of 7`, build-command list identical to the main tree. |
| 3 | `.github/` contains no workflows and no Dependabot configuration | VERIFIED | `ls .github` fails with "No such file or directory"; `git ls-files` shows no `.github` path. |
| 4 | App starts with an English UI, no translation files built or loaded, no translation targets in CMake | VERIFIED | `src/translations` is gone. Grep for `QTranslator`, `installTranslator`, `translations`, `.qm`, `setLanguage`, `"language"` over `src`, `CMakeLists.txt`, `cmake` finds only the harness assertion string. I re-ran `build/macos-release/ui_snapshot build/scratch/verify-ui --assert-no-language`: exit 0, "language control present: no", and `texts.txt` is byte-identical to the committed `records/screenshots/after/texts.txt`. The 249 `tr()` calls and 573 `.ui` strings stay (D-06), so the English strings are the source strings. |
| 5 | Someone following only the README can build the app on the Mac and run the test suite | VERIFIED | README has Requirements (Homebrew line), Build, Run and Tests sections with whole preset commands (`cmake --preset macos-release`, `cmake --build --preset macos-release`, `ctest --preset macos-release`). `CMakePresets.json` defines all three presets. A README grep for MXE, Ubuntu, Flatpak, DEB, NSIS, Windows, Linux and Qt 5 finds nothing relevant. `records/fresh-clone.md` ran exactly those lines in a fresh clone and passed with no step outside the README. |

**Score:** 5/5 truths verified (0 present but behavior-unverified)

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `CMakeLists.txt`, `CMakePresets.json` | Qt 6 only, three macOS presets | VERIFIED | `find_package(Qt6 6.5 ...)`; presets `macos-release`, `macos-debug`, `macos-asan` |
| `src/app/ui_snapshot/main.cpp` | Offscreen UI harness with language assertion | VERIFIED | Built and run by the verifier, exit 0, not registered with ctest (suite count stays 7) |
| `README.md`, `TESTING.md` | macOS build and test documentation | VERIFIED | Content read; fresh-clone proof |
| Removed trees (`.github`, `flatpak`, `scripts`, `src/translations`, `src/resources/{unix,win32}`) | Absent | VERIFIED | Absent on disk and in the index |
| `records/*` (gate-log, fresh-clone, ui-screenshots, sanitizer-compare, leftover-grep, closing-check) | Evidence | VERIFIED | Present. Gate log has twelve rows (c00 to c11), each Release and Debug `100% tests passed out of 7`, goldens unchanged. |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `Manifest.cpp` | `src/test_support/CMakeLists.txt` | `ST_QT_COMPILE_VERSION="${Qt6_VERSION}"` | WIRED | Define present; the `#ifndef` fallback to "unknown" is covered by the unchanged golden MANIFEST (`qt_compile = 6.12.0`) passing. |
| `ctest --preset macos-release` | seven suites | `CMakePresets.json` test presets | WIRED | `ctest -N` lists all seven; the run passed. |

### Data-Flow Trace (Level 4)

Not applicable: the phase removes code and docs. The one data path touched, the manifest `qt_compile` value, is proven by the golden manifest check inside `pipeline_tests`.

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Full suite on HEAD | `cmake --build --preset macos-release; ctest --preset macos-release` | no work to do; 100% passed, 7 of 7 | PASS |
| English UI, no Language control | `build/macos-release/ui_snapshot build/scratch/verify-ui --assert-no-language` | exit 0; texts identical to committed record | PASS |
| Goldens unchanged | `git diff a796c2ae HEAD -- src/pipeline_tests/golden` | empty | PASS |

### Probe Execution

SKIPPED: the phase declares no probe scripts.

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| STRIP-01 | 03-02, 03-08, 03-09 | Qt 6 only, baselines unchanged | SATISFIED | Truth 1 |
| STRIP-02 | 03-04, 03-05, 03-08, 03-09 | Windows paths and options removed | SATISFIED | Truth 2 (platform grep empty; Windows files gone) |
| STRIP-03 | 03-05, 03-08, 03-09 | Linux code, Wayland/xcb, desktop and MIME files removed | SATISFIED | Truth 2 |
| STRIP-04 | 03-03, 03-07, 03-08, 03-09 | Packaging removed | SATISFIED | Truth 2 (no `install(`, CPack, scripts, Flatpak) |
| STRIP-05 | 03-03, 03-08, 03-09 | Workflows and Dependabot removed | SATISFIED | Truth 3 |
| STRIP-06 | 03-01, 03-06, 03-08, 03-09 | Translations removed, English only | SATISFIED | Truth 4 |
| STRIP-08 | 03-07, 03-09 | README documents macOS build and test | SATISFIED | Truth 5 |
| STRIP-07 | none | OpenGL viewer removal | OUT OF SCOPE | REQUIREMENTS.md maps it to Phase 6 (Pending); no Phase 3 plan claims it. Not an orphan. |

All seven phase IDs appear in REQUIREMENTS.md and are marked Complete in its traceability table. No requirement mapped to Phase 3 is unclaimed.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| (modified files) | - | `TBD`/`FIXME`/`XXX` markers | none found by the Phase 3 leftover sweeps; no blocker | - |
| `.claude/CLAUDE.md` | 298 | `.github/skills/` mentioned | Info | Generic generator boilerplate, accepted in 03-09 |

Code-review items (`03-REVIEW.md`: 0 critical, 2 warning, 7 info) are recorded in `03-REVIEW-DISPOSITION.md`. WR-01 (`ui_snapshot` recompiles the GUI sources and is built by default) is a build-time cost and does not affect any success criterion. WR-02 (orphaned comment in the kept Clang block) is deferred on purpose. None blocks the goal.

### Advisory notes (non-blocking)

- The existing `build/macos-release` directory still holds stale `scantailor-advanced_*.qm`, `.desktop`, `CPackConfig.cmake` and `cmake_uninstall.cmake` files from earlier builds. They are untracked and ignored, and no rule in the tree produces them. The fresh clone confirms a clean directory has none. Deleting `build/macos-release` would remove the confusion.
- Success criterion 4 ("the app starts with an English UI") rests on the offscreen harness and the executor's reading of the PNGs, not on a launch of the real window with a display. The user chose this explicitly (no manual GUI step). I re-ran the harness and compared its text dump to the committed one. I did not view the PNGs.

### Human Verification Required

None. The orchestrator recorded that the user chose no manual GUI step.

### Gaps Summary

No gaps. All five ROADMAP success criteria hold on HEAD, and all seven requirement IDs (STRIP-01 to STRIP-06, STRIP-08) are accounted for. The regression gate was re-run and passes with the golden baselines byte-identical to the Phase 2 baseline commit `a796c2ae`.

---

_Verified: 2026-10-24_
_Verifier: Claude (gsd-verifier)_
