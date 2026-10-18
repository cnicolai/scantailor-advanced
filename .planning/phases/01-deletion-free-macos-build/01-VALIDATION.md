---
phase: "1"
slug: "deletion-free-macos-build"
# status lifecycle: draft (seeded by plan-phase) → validated (set by validate-phase §6)
# audit-milestone §5.5 distinguishes NOT-VALIDATED (draft) from PARTIAL (validated + nyquist_compliant: false) (#2117)
status: draft
nyquist_compliant: false
wave_0_complete: false
created: "2026-10-18"
---

# Phase 1 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Boost.Test (core, foundation, math, imageproc tests) and Qt Test (`qt_tests`), driven by CTest; versions recorded in `records/toolchain.txt` by plan 01-01 (research: Boost 1.92.0, Qt 6.12.0, CMake 4.4.4) |
| **Config file** | `CMakePresets.json` (created by plan 01-01, Wave 1); existing `add_test` lines in `src/{core,foundation,math,imageproc}/tests/CMakeLists.txt` and `src/qt_tests/CMakeLists.txt` |
| **Quick run command** | `ctest --preset macos-release -R foundation_tests --output-on-failure` |
| **Full suite command** | `cmake --build --preset macos-release && ctest --preset macos-release` |
| **Estimated runtime** | not yet measured; plan 01-03 records it (ctest prints "Total Test time") |

---

## Sampling Rate

- **After every task commit:** the task's own `<automated>` commands (a configure, one target build, one ctest entry, or a record check)
- **After every plan wave:** from wave 2 on, `cmake --build --preset macos-release && ctest --preset macos-release`; wave 1 only has `foundation_tests`
- **Before `/gsd-verify-work`:** plan 01-07's closing check from fresh build directories. In this phase failing existing tests are recorded, not fixed (BUILD-04), so "green" means every check in `records/closing-check.md` has a verdict and the build succeeds
- **Max feedback latency:** one incremental build plus one ctest run

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 1-01-01 | 01 | 1 | BUILD-01 | T-01-SC, T-01-01 | only homebrew/core formulae installed; `qt`, `jpeg`, `boost@1.85` absent | tracer (build + ctest) | `ctest --preset macos-release -R foundation_tests --output-on-failure` | ❌ W0 (`CMakePresets.json` created here) | ⬜ pending |
| 1-01-02 | 01 | 1 | BUILD-05, BUILD-06, BUILD-07 | T-01-02 | flags in every compile command; sanitizer flags only in the asan tree | script (jq on compile_commands.json) | `cmake --list-presets=all` plus the jq loops in plan 01-01 Task 2 | ❌ W0 (`SANITIZE` option created here) | ⬜ pending |
| 1-02-01 | 02 | 2 | BUILD-01 | T-01-04 | no source edit before the error classes are recorded | record check | `grep -q '^## Build attempt 1' .planning/phases/01-deletion-free-macos-build/records/fix-log.md` | ✅ after 01-01 | ⬜ pending |
| 1-02-02 | 02 | 2 | BUILD-01 | — | user chooses the formatting policy | checkpoint:decision | n/a (decision recorded as `Formatting decision:` in fix-log.md) | n/a | ⬜ pending |
| 1-02-03 | 02 | 2 | BUILD-01 | T-01-04, T-01-05 | every src/ change logged; stop-and-ask before output changes | build | `cmake --build --preset macos-release && cmake --build --preset macos-debug` | ✅ | ⬜ pending |
| 1-03-01 | 03 | 3 | BUILD-01, BUILD-02, BUILD-05 | T-01-06 | exactly one libjpeg, libpng, libtiff, statically and at runtime | script | `file`, `otool -L` and `DYLD_PRINT_LIBRARIES` loops in plan 01-03 Task 1 | ✅ | ⬜ pending |
| 1-03-02 | 03 | 3 | BUILD-04 | T-01-07 | records captured, not hand-written | integration (ctest) | `ctest --preset macos-release -V` (record check: summary over 5 tests) | ✅ existing tests | ⬜ pending |
| 1-03-03 | 03 | 3 | BUILD-04 | T-01-07 | — | integration (ctest) | `ctest --preset macos-debug -V` (record check: summary over 5 tests) | ✅ existing tests | ⬜ pending |
| 1-04-01 | 04 | 3 | BUILD-06 | T-01-09 | ASan runtime linked | build + script | `otool -L build/macos-asan/core_tests` (expects `libclang_rt.asan`) | ✅ after 01-01 | ⬜ pending |
| 1-04-02 | 04 | 3 | BUILD-06 | T-01-08, T-01-09 | findings recorded, not fixed or suppressed | integration (ctest under ASan/UBSan) | `ctest --preset macos-asan -V` plus `sanitizer-findings.md` totals check | ✅ existing tests | ⬜ pending |
| 1-05-01 | 05 | 2 | BUILD-03 | T-01-12, T-01-13 | probe builds no QSettings; no image committed | script | grep checks on `records/probe-output.txt` and `records/smoke-pages.md`; git image check | ❌ W0 (probe and pages created here, scratchpad only) | ⬜ pending |
| 1-05-02 | 05 | 2 | BUILD-03 | T-01-10 | permission before any out-of-project listing | checkpoint:decision (blocking-human) | n/a (`Decision:` line in prior-state.md) | n/a | ⬜ pending |
| 1-05-03 | 05 | 2 | BUILD-03 | T-01-10, T-01-11 | names/sizes/dates only; move aside, never delete | record check | `grep -qE '^Decision: ' .planning/phases/01-deletion-free-macos-build/records/prior-state.md` | ✅ | ⬜ pending |
| 1-06-01 | 06 | 4 | BUILD-03 | — | D-02 outcome exists before the first launch | build + record check | `test -x build/macos-release/scantailor-advanced` plus findings-file check | ✅ | ⬜ pending |
| 1-06-02 | 06 | 4 | BUILD-03 | T-01-14, T-01-15 | user-driven, one step per message | checkpoint:human-action (blocking-human) | n/a (user reports per step) | n/a | ⬜ pending |
| 1-06-03 | 06 | 4 | BUILD-03 | T-01-16 | stderr normalised; no image or project file committed | script | verdict-line check on `records/SMOKE-FINDINGS.md` (plan 01-06 Task 3) | ✅ | ⬜ pending |
| 1-07-01 | 07 | 5 | BUILD-01, BUILD-02, BUILD-04, BUILD-05, BUILD-06, BUILD-07 | — | — | script (fresh trees) | `file`, `otool -L` and jq checks in plan 01-07 Task 1 | ✅ | ⬜ pending |
| 1-07-02 | 07 | 5 | all | T-01-17, T-01-18 | every non-planning commit has a fix-log row; no remove-only hunk | script (git) | commit-to-fix-log loop and pathspec check in plan 01-07 Task 2 | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `brew install cmake ninja boost qtbase qtsvg qttools` (plan 01-01 Task 1) — toolchain and test framework (Boost.Test, Qt Test)
- [ ] `CMakePresets.json` with `macos-release`, `macos-debug`, `macos-asan` configure/build/test presets (plan 01-01) — covers BUILD-01, BUILD-06
- [ ] `SANITIZE` cache variable in `CMakeLists.txt` (plan 01-01 Task 2) — covers BUILD-06
- [ ] `.planning/phases/01-deletion-free-macos-build/records/` with names that are not gitignored (`.txt`, `.md`; never `*.log`) — plan 01-01

No test files are written in this phase; the existing five ctest registrations are the subject.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| App launches from the build directory, project goes through all six stages, outputs open in Preview, project saves and reopens | BUILD-03 | Needs the GUI and a person; Claude cannot operate the app | Plan 01-06 Task 2: nine steps presented one per message, each naming what to do and what to look for; Task 3 then checks the outputs (`magick identify` depth per mode) and the project XML by script |
| Retina sharpness, shortcut spelling, menu layout, batch-finished alarm, UI language (logged only) | BUILD-03 (D-06) | Visual and audible observations | Plan 01-06 steps 1, 3 and 9; recorded in `records/SMOKE-FINDINGS.md` under "Logged for later phases (not fixed)" |
| Device pixel ratio on the Retina screen | BUILD-03 (D-06), feeds HIDPI-01 | Needs the window server; measured by the scratchpad probe and cross-checked with `qt.highdpi` logging | Plan 01-05 Task 1 (probe run by Claude); plan 01-06 step 1 (logging during the user's launch) |
| D-02 permission and keep/move-aside choice | BUILD-03 (D-02) | The user's consent is required for any access outside the project | Plan 01-05 Tasks 2 and 3, one question per message |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 1s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** {pending / approved YYYY-MM-DD}
