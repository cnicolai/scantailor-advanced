---
phase: 01-deletion-free-macos-build
plan: 01
subsystem: infra
tags: [cmake, cmake-presets, homebrew, qt6, boost, apple-clang, ninja, fp-contract, sanitizers]

requires:
  - phase: none
    provides: first plan of the first phase
provides:
  - "Homebrew toolchain (cmake 4.4.4, ninja 1.13.2, boost 1.92.0, qtbase/qtsvg/qttools 6.12.0) installed and recorded before and after"
  - "CMakePresets.json with macos-release, macos-debug and macos-asan configure, build and test presets (schema v6, Ninja)"
  - "CMakeLists.txt: CMake 3.25 minimum, CMP0167 NEW, Clang warning set, frozen -ffp-contract=off, SANITIZE cache variable"
  - "Passing foundation_tests (arm64) built through the macos-release preset"
  - "Records: access-approval, toolchain, configure, fix-log, fp-contract, flags-compare"
affects: [01-02, 01-03, 01-04, 01-05, 01-06, 01-07, phase-02-baselines]

actuals:
  tokens: 7800
  tasks: 3
  commits: 9

tech-stack:
  added: [cmake 4.4.4, ninja 1.13.2, boost 1.92.0, qtbase 6.12.0, qtsvg 6.12.0, qttools 6.12.0]
  patterns:
    - "Phase 1 build edits are additive only; each edit gets one commit and one fix-log row"
    - "Records are captured from real command output and path-normalised ({repo}, ~, {scratch})"

key-files:
  created:
    - CMakePresets.json
    - .planning/phases/01-deletion-free-macos-build/records/access-approval.md
    - .planning/phases/01-deletion-free-macos-build/records/toolchain.txt
    - .planning/phases/01-deletion-free-macos-build/records/configure.txt
    - .planning/phases/01-deletion-free-macos-build/records/fix-log.md
    - .planning/phases/01-deletion-free-macos-build/records/fp-contract.md
    - .planning/phases/01-deletion-free-macos-build/records/flags-compare.txt
  modified:
    - CMakeLists.txt

key-decisions:
  - "User approved build-tool reads under /opt/homebrew and the Xcode/CLT SDK for plans 01-01 to 01-07; recorded as the first commit touching records/"
  - "-ffp-contract=off is frozen in CMakeLists.txt (not only in presets) for C and C++ under Clang, for every build type (D-03, BUILD-07)"
  - "No Boost::headers bridge: under BoostConfig, include_directories(${Boost_INCLUDE_DIRS}) expands to /opt/homebrew/include"
  - "Preset flag set differs from the untouched configure by exactly five tokens (-Wall, -Wextra, -Werror=return-type, -Wno-unused, -ffp-contract=off)"

patterns-established:
  - "Compile-flag verification: jq over build/<preset>/compile_commands.json, entries and missing counts"
  - "Flag-set comparison: flagset.py style extraction (option tokens, build dir as {build}) applied to both trees"

requirements-completed: [BUILD-01, BUILD-05, BUILD-06, BUILD-07]

coverage:
  - id: D1
    description: "macos-release preset configures with Apple Clang against Homebrew Qt 6 and Boost, builds foundation_tests as arm64, and ctest passes it"
    requirement: BUILD-01
    verification:
      - kind: integration
        ref: "ctest --preset macos-release -R foundation_tests --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "Every compile command in the release, debug and ASan trees carries -Wall -Wextra -Werror=return-type -Wno-unused"
    requirement: BUILD-05
    verification:
      - kind: other
        ref: "jq check over build/macos-{release,debug,asan}/compile_commands.json: entries=408 missing=0 each"
        status: pass
    human_judgment: false
  - id: D3
    description: "SANITIZE option exists; only the macos-asan tree carries -fsanitize=address,undefined and -fsanitize-recover=address"
    requirement: BUILD-06
    verification:
      - kind: other
        ref: "jq checks: asan missing=0; macos-release sanitized=0; macos-debug sanitized=0"
        status: pass
    human_judgment: false
  - id: D4
    description: "-ffp-contract=off is set in CMakeLists.txt for C and C++ and recorded with compiler evidence"
    requirement: BUILD-07
    verification:
      - kind: other
        ref: "records/fp-contract.md (fmadd by default, fmul+fadd with the flag); jq check missing=0 in all three trees"
        status: pass
    human_judgment: false

duration: 20min
completed: 2026-10-18
status: complete
commits: 9
plan_head_before: cf03dc8642daa4bab4296e62dd95119783182a97
plan_head_after: 20502a313fafcb881ff51b1160299e8d33ee2684
---

# Phase 1 Plan 01: Toolchain, presets, warnings and frozen FP setting Summary

**Homebrew Qt 6.12 / Boost 1.92 toolchain installed, three CMake presets (release, debug, ASan) configure and build `foundation_tests` as arm64 with Apple Clang, and `-ffp-contract=off` plus the D-08 warning set are frozen in `CMakeLists.txt` before any Phase 2 baseline exists.**

## Performance

- **Duration:** about 20 min for this continuation (Tasks 2 and 3 after the Task 1 approval)
- **Completed:** 2026-10-18
- **Tasks:** 3 (Task 1 decision, Task 2 tracer, Task 3 auto)
- **Files modified:** `CMakeLists.txt`, new `CMakePresets.json`, six records

## Accomplishments

- Task 1: the user approved the build-tool reads for plans 01-01 to 01-07; the answer is committed first in `records/access-approval.md` (4dbcfdf), as the verify gate requires.
- Task 2 (tracer): supply-chain gate passed for cmake, ninja, boost, qtbase, qtsvg and qttools (all `homebrew/core`, not deprecated, not disabled); installed; jpeg-turbo 3.2.0, libpng 1.6.59 and libtiff 4.7.2 did not change. The untouched tree configured cleanly (exit 0, 408 compile commands) and its flag set is on record. `cmake --preset macos-release` then built `foundation_tests` (arm64) and `ctest` passed it.
- Task 3: Clang warnings, `-ffp-contract=off`, the `SANITIZE` variable, and the debug and ASan presets, each in its own commit. All three trees configure with `--fresh`; the jq checks show 408 of 408 compile commands carrying the full flag set in each tree, sanitizer flags only in the ASan tree (also on its link lines), and none in release or debug.
- The preset flag set differs from the untouched configure by exactly the five added tokens; no `-O`, `-D`, `-march`, `-std` or include-path token changed (A12 cleared).
- `-ffp-contract=off` evidence: this Apple Clang prints `fmadd` for `a*b+c` at `-O0`, `-O2` and `-ffp-contract=fast`, and `fmul` then `fadd` with `-ffp-contract=off`.

## Task Commits

1. Task 1 (decision): no commit; answer recorded by the first commit below
2. Task 2 Step 0: `4dbcfdf` docs(01-01): record approval for build-tool reads outside the project
3. Task 2 Step 6a: `b2d5214` build(cmake): require CMake 3.25 and use BoostConfig (CMP0167)
4. Task 2 Step 6b: `d7f2ca2` build: add macOS CMake presets
5. Task 2 records: `a10271e` docs(01-01): record toolchain and first configure
6. Task 3 2a: `e35b498` build(cmake): enable Clang warnings (D-08)
7. Task 3 2b: `606bea0` build(cmake): freeze -ffp-contract=off for C and C++ (D-03)
8. Task 3 2c: `8670057` build(cmake): add SANITIZE option for ASan/UBSan builds
9. Task 3 2d: `11bc573` build: add debug and ASan presets
10. Task 3 records: `20502a3` docs(01-01): record FP setting and flag comparison

The plan-level metadata commit follows this summary. The measured count in the frontmatter (`commits: 9`) counts the nine commits from `cf03dc8` to `20502a3`; the list above shows ten lines because Task 1 has no commit of its own.

## Files Created/Modified

- `CMakeLists.txt` - line 1 minimum 3.25, CMP0167 NEW, Clang warning and FP block (lines 64-70), `SANITIZE` block (lines 72-80); GNU, MSVC/WIN32 and Qt 5 code untouched
- `CMakePresets.json` - new, schema v6, release/debug/ASan configure, build and test presets
- `records/*` - access-approval, toolchain, configure, fix-log, fp-contract, flags-compare under `.planning/phases/01-deletion-free-macos-build/records/`

## Decisions Made

- `-ffp-contract=off` lives in `CMakeLists.txt` so a configure without a preset gets it too (research Open Question 7).
- The Boost bridge was not added: the trace shows `include_directories(/opt/homebrew/include )`.
- `CMAKE_FIND_FRAMEWORK=LAST` stayed in `macos-base`; the configure worked with it.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical functionality, privacy] Formula-only package list in toolchain.txt**
- **Found during:** Task 2 Step 1
- **Issue:** `brew list --versions` also lists installed casks (personal applications). The record is committed to a fork that may be pushed.
- **Fix:** Recorded `brew list --formula --versions` for both the before and after sections and added a `NOTE:` line saying so. Every check in the plan (cmake, ninja, boost, qtbase, qtsvg, qttools lines, absence of jpeg/qt/boost@1.85) uses formulae only.
- **Files modified:** `records/toolchain.txt`
- **Commit:** a10271e

**2. [Rule 3 - Wording] ctest summary line differs from the plan text**
- **Found during:** Task 2 Step 7
- **Issue:** ctest 4.4.4 prints `100% tests passed out of 1`, not `100% tests passed, 0 tests failed out of 1`.
- **Fix:** None needed; the exit status is 0 and the test shows `Passed`. Plan 01-02 onward should match on the exit status or on `100% tests passed`.

**3. [Process] Line number of the Boost call moved**
- The plan names line 207; after the CMP0167 block (+4 lines) the call is on line 211. Recorded as such in `fix-log.md`.

**4. [Process slip] One configure output briefly written to /tmp**
- During Task 2 Step 5 the first configure run wrote its output to `/tmp/x` instead of the session scratchpad. I deleted the file at once and re-ran the configure with the output in the scratchpad. Nothing from it was committed.

### Not changed, on purpose

- The pre-existing compiler warning `src/foundation/PropertySet.cpp:25:22 [-Wpotentially-evaluated-expression]` is not caused by this plan and `src/` is out of scope here; it is noted under `## Open errors` in `fix-log.md`.

## Authentication Gates

None. The Task 1 approval was the plan's own blocking-human decision, answered `approve`.

## Issues Encountered

- My first FP-evidence loop passed the flag set as one word, so the `-O2 -ffp-contract=...` runs failed with "invalid integral value". I reran them with explicit arguments; the recorded evidence comes from the second run.

## Known Stubs

None.

## Threat Flags

None. The only new surface is the build configuration; no network endpoint, auth path or file access pattern was added.

## Next Phase Readiness

- Plans 01-02 to 01-07 can build through `cmake --preset macos-release|macos-debug|macos-asan`; the first task of each checks `Decision: approve` in `records/access-approval.md`, which is now committed.
- Unfinished here: nothing blocks. Plan 01-02 owns the source-edit formatting policy; the one default-Clang warning in `PropertySet.cpp` is for whichever later plan builds with `-Werror` decisions.
- Work is on the phase branch `gsd/phase-01-deletion-free-macos-build`, created by the orchestrator because `master` is protected.

## Self-Check: PASSED

All created files exist (CMakePresets.json, six records, build/macos-release/foundation_tests, compile_commands.json of the debug and ASan trees) and all nine commits (4dbcfdf, b2d5214, d7f2ca2, a10271e, e35b498, 606bea0, 8670057, 11bc573, 20502a3) are ancestors of HEAD.
