---
phase: 03-qt-6-and-macos-only
plan: 07
subsystem: docs
tags: [readme, testing-md, homebrew, cmake-presets, fresh-clone, gate]

requires:
  - phase: 03-qt-6-and-macos-only
    provides: gate.sh and the strip commits c00 to c10 (03-01 to 03-06), CMakePresets.json (Phase 1)
provides:
  - README.md with the macOS fork intro, Requirements, Build, Run, Tests and Settings and profiles sections; MXE, Ubuntu, Flatpak, DEB and Windows notes gone
  - TESTING.md in English (ten sections, seven suites, goldens, artefacts, manifest, expected failures, sanitizer, ui_snapshot, formatting)
  - D-13 proof records/fresh-clone.md: a fresh clone built and tested from README lines only
affects: [03-08, 03-09]

actuals:
  tokens: 9900
  tasks: 2
  commits: 3
plan_head_before: 653b7440adfe302b46a39361c9aa0692041939fe
plan_head_after: e057718896cd5d941d69bf8fc1c67a9762e6f4e2

tech-stack:
  added: []
  patterns:
    - "D-13: a README is proved by running only its own whole command lines in a fresh clone inside build/"

key-files:
  created:
    - .planning/phases/03-qt-6-and-macos-only/records/fresh-clone.md
  modified:
    - README.md
    - TESTING.md
    - .planning/phases/03-qt-6-and-macos-only/records/gate-log.md

key-decisions:
  - "The Features section of the README is unchanged except the Portability bullet (now 'Settings location', three lines) and profile item 2, as D-08 and ED-07/ED-08 allow"
  - "TESTING.md names the failure artefacts as build/<preset>/test-artifacts/<suite>/<name>/ with actual, expected and diff files, as the GoldenStore code writes them"

requirements-completed: [STRIP-08, STRIP-04]

coverage:
  - id: D1
    description: "README starts with the personal macOS fork, lists the Homebrew line and the whole configure, build, run and test lines of the three presets, names the settings and profiles locations and links TESTING.md; the old Building, About this fork, Building (detailed) and video sections and every MXE, Ubuntu, Flatpak, DEB and Windows note are gone"
    requirement: STRIP-08
    verification:
      - kind: other
        ref: "Task 1 automated checks 1 and 2 (heading, command-line and operating-system greps) exit 0; grep -c $HOME README.md prints 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "The README Features section equals a796c2ae except the five Portability lines and profile item 2"
    requirement: STRIP-04
    verification:
      - kind: other
        ref: "Task 1 automated check 3: build/scratch/features.diff has 6 removed lines, all from the two portable-mode passages, and 4 added lines about settings and profiles"
        status: pass
    human_judgment: false
  - id: D3
    description: "TESTING.md is in English with the ten required sections and names all seven suites, UPDATE_GOLDENS=1, test-artifacts, backlog items 999.1 to 999.7, macos-asan, ui_snapshot, TOOLCHAIN DIFFERS and xcrun clang-format"
    requirement: STRIP-08
    verification:
      - kind: other
        ref: "Task 1 automated check 4 exit 0; no Spanish word of the old file remains"
        status: pass
    human_judgment: false
  - id: D4
    description: "c11-docs passes the gate"
    requirement: STRIP-04
    verification:
      - kind: other
        ref: "gate.sh check c11-docs: result PASS, tested-commit cf4da61c, 100% tests passed out of 7 in Release and Debug, goldens unchanged, command diff 0 / 0; row in records/gate-log.md"
        status: pass
    human_judgment: false
  - id: D5
    description: "A fresh clone built, configured and tested with only the README's macos-release lines passes; its clean build directory has no .qm file or translation step and its build commands equal those of the existing build directory"
    requirement: STRIP-08
    verification:
      - kind: other
        ref: "records/fresh-clone.md Verdict: PASS; status.txt 1 0, 2 0, 3 0; ctest 100% tests passed out of 7; find *.qm empty; build.ninja references 0; nm translator 0; cmds.diff empty (509 lines each)"
        status: pass
    human_judgment: false

duration: 11min
completed: 2026-10-24
status: complete
---

# Phase 3 Plan 07: README and TESTING.md for the macOS build Summary

**README and TESTING.md now describe the Apple Silicon build (Homebrew line, three CMake presets, run, ctest, settings and profile paths, golden and sanitizer rules), c11 passed the gate with an empty command diff, and a fresh clone built from the README's three macos-release lines alone passed `100% tests passed out of 7` with no .qm file and identical build commands**

## Performance

- **Duration:** about 11 min (the c11 gate took about 4 min, the fresh-clone build and ctest about 3 min)
- **Started:** 2026-10-24T21:08:19Z
- **Completed:** 2026-10-24T21:19:30Z
- **Tasks:** 2
- **Files modified:** 4 (README.md, TESTING.md, gate-log.md, new fresh-clone.md), plus this SUMMARY and the state files

## Accomplishments

- README (c11, `cf4da61c`): new title and paragraph, `## Contents` (no colon, Features sub-entries kept), `## Requirements` (the exact line `brew install cmake ninja boost qtbase qtsvg jpeg-turbo libpng libtiff`, the note about `qt` and `jpeg`, zlib from the SDK), `## Build`, `## Run`, `## Tests`, `## Settings and profiles`; `## Description`, `## Features` and `## License` kept. Building, About this fork, the video section and Building (detailed) are removed, and the README names no other operating system.
- Features diff against a796c2ae: six removed lines (the five Portability lines and profile item 2) and four added ones, all about the settings and profiles locations.
- TESTING.md rewritten in English: running the tests, a suite table, golden rules (D-06, D-07, D-10), failure artefacts, toolchain manifest with the warn-only `TOOLCHAIN DIFFERS`, expected failures with the backlog items 999.1 to 999.7, sanitizer build with the direct-run command, `ui_snapshot`, and formatting with `xcrun clang-format`.
- Gate `c11-docs`: Release and Debug `100% tests passed out of 7`, goldens unchanged against a796c2ae, command diff 0 / 0. Row in `records/gate-log.md` (`8f578ff4`).
- D-13: `git clone --quiet . build/fresh-clone`, then only the three whole README lines (`cmake --preset macos-release`, `cmake --build --preset macos-release`, `ctest --preset macos-release`), all exit 0, `100% tests passed out of 7`. The clean directory has no `.qm`, `build.ninja` has 0 references to translations, the app has 0 translator symbols, and the 509 build commands equal the reference of the existing build directory (empty diff). `brew install` was not run; `brew list --versions` shows all eight packages. Record `records/fresh-clone.md` ends with `Verdict: PASS`.

## Task Commits

1. **Task 1 (c11): README and TESTING.md** - `cf4da61c` (docs), gate row `8f578ff4` (docs)
2. **Task 2: fresh-clone proof** - `e0577188` (docs)

**Plan metadata:** added by the final docs commit of this plan.

## Files Created/Modified

- `README.md` - macOS build, run, test and settings documentation; Features kept except two passages
- `TESTING.md` - English test guide
- `.planning/phases/03-qt-6-and-macos-only/records/gate-log.md` - row for c11-docs
- `.planning/phases/03-qt-6-and-macos-only/records/fresh-clone.md` - D-13 proof

## Decisions Made

- Failure artefacts are documented as `build/<preset>/test-artifacts/<suite>/<name>/` with `actual`, `expected` and `diff` files because that is what `GoldenStore.cpp` writes; the plan's `<test>` placeholder was read as suite and name.
- TESTING.md states that goldens are recorded with the Release build, matching the README preset table; the plan gave no stronger rule.

## Deviations from Plan

### Orchestrator-approved check adjustment that turned out not to be needed

**1. [Plan check adjusted to a measured fact] CMake and `brew list --versions`**
- **Found during:** Task 2 precondition
- **Issue:** 03-01's `records/before-state.md` measured that `brew list --versions` printed no `cmake` line, so the orchestrator pre-approved replacing the eight-line check by seven `brew list --versions` lines plus `cmake --version`. When this plan ran, `brew list --formula --versions cmake` printed `cmake 4.4.4` (`/opt/homebrew/bin/cmake`), so the original eight-line check holds as written. The plan's original precondition and acceptance check were therefore used unchanged, and the approved replacement was also satisfied (seven lines, exit 0; `cmake --version` is 4.4.4, above the presets' minimum 3.25).
- **Fix:** `records/fresh-clone.md` § Homebrew packages states the eight formulae and that the earlier 03-01 note and today's output disagree. It does not claim that seven of eight are Homebrew installs, because that is not what was measured now. Nothing was installed or upgraded in this phase, so the cause of the earlier output (a different query, or an out-of-date note) was not investigated.
- **Files modified:** `records/fresh-clone.md`
- **Committed in:** `e0577188`

---

**Total deviations:** 1 (a check adjusted by the orchestrator that proved unnecessary; no code or scope change)
**Impact on plan:** None. The README keeps the planned brew line and makes no claim about how CMake was installed. The 03-01 note in `records/before-state.md` is left as recorded; whoever relies on it should read the line above.

## Issues Encountered

- In zsh a variable holding the target list is not word-split, so the first command-list extraction for the build-command comparison produced an empty file and a 510-line diff. Re-running with the targets written out gave 509 lines each and an empty diff. Only scratch evidence was affected.

## Known Stubs

None.

## Threat Flags

None. T-03-15 mitigated (no `brew install`, `upgrade` or removal; `brew list --versions` is read-only). T-03-16 mitigated (`grep -c "$HOME"` prints 0 for README.md, TESTING.md and fresh-clone.md; only `~`-relative locations documented). T-03-17 mitigated (clone in `build/fresh-clone`, scratch in `build/scratch/fresh-clone`; nothing outside the project). T-03-SC: no package installed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plans 03-08 and 03-09 can start. `src/pipeline_tests/golden` is unchanged against a796c2ae; `UPDATE_GOLDENS` was never set.
- The clone `build/fresh-clone` and the scratch files are gitignored and may be deleted at any time.
- README and TESTING.md are in their final shape for the phase; the leftover searches of the next plan may still look at other files for stale OS or translation words.

## Self-Check: PASSED

- `README.md`, `TESTING.md`, `records/fresh-clone.md` exist; `cf4da61c`, `8f578ff4`, `e0577188` are ancestors of HEAD (`git rev-list --count 653b7440..HEAD` = 3 before this SUMMARY).
- Gate summary of c11-docs ends with `result: PASS` and `tested-commit.txt` equals `cf4da61c`; the gate-log row exists.
- All automated checks of both tasks exit 0; `git diff a796c2ae -- src/pipeline_tests/golden` is empty.

---
*Phase: 03-qt-6-and-macos-only*
*Completed: 2026-10-24*
