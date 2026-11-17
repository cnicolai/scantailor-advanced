---
phase: 06-hardening-and-cleanup
plan: 16
subsystem: closing-check
tags: [documentation, closing-check, memory-run, mac-check, sanitizers, gate]

requires:
  - phase: 06-hardening-and-cleanup
    provides: "plans 06-01 to 06-15: every Phase 6 change, its records and gate rows; the opt-in probes of plans 06-09 and 06-13"
provides:
  - "TESTING.md: the Phase 6 suites and Qt slots in the suite table, and the section Opt-in probes with the run commands and environment variables of the three probes"
  - "records/closing-check.md: roadmap criteria 1 to 5 and the nine requirements mapped to tests, records and gate rows; automated, sanitizer and live evidence; For the user; For the transition"
  - "records/memory-run.md: the D-08 run on the app's batch path (cap 5 of 10 threads, peak 17737 MiB, no swap growth)"
  - "gate row s23-docs"
affects: [phase-6-verification, phase-6-transition]

actuals:
  tokens: 8700
  tasks: 3
  commits: 4

plan_head_before: f2c5a732b2750340936b948b4e98a04fe93ba717
plan_head_after: dc837bd89df2d6ad972a959110572affe27c0d32

tech-stack:
  added: []
  patterns:
    - "Closing record in the Phase 5 layout: closing runs, criteria table, requirements table, gate log, static checks with positive controls, sanitizer attribution, For the user, For the transition"
    - "Memory run read as the run window of the capped batch, with swap read before, every 2 s and after, and phys_footprint instead of RSS"

key-files:
  created:
    - .planning/phases/06-hardening-and-cleanup/records/closing-check.md
    - .planning/phases/06-hardening-and-cleanup/records/memory-run.md
  modified:
    - TESTING.md
    - .planning/phases/06-hardening-and-cleanup/records/gate-log.md

key-decisions:
  - "The factor of 40 B per pixel stays: one 165.6 MP page peaked at 4909 MiB (31.1 B per pixel) against the 6317 MiB estimate, and the batch at 17737 MiB against the 32768 MiB budget, so s24-factor-fix was not needed"
  - "The live runs were started by the orchestrator after the user's own go, with the plan's exact commands; this executor wrote the records from the files on disk"
  - "Backslash, UBSan and backlog items 999.1 to 999.8 stay untouched (D-11); the only UBSan location, BinaryImage.cpp:1044:37, is recorded as older code"

requirements-completed: [SAFE-01, SAFE-02, SAFE-03, STRIP-07, CLEAN-01, CLEAN-02, CLEAN-03, CLEAN-04, CLEAN-05]

coverage:
  - id: D1
    description: "TESTING.md names the Phase 6 suites and slots and documents the three opt-in probes with run commands, environment variables and the statement that ctest never runs them"
    requirement: CLEAN-01
    verification:
      - kind: other
        ref: "grep checks of the Task 1 verify block; gate s23-docs result: PASS (100% of 7 in both presets, goldens unchanged, command diff 0 / 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Every automated closing check passes at HEAD: both ctest presets 100% of 7, goldens equal a796c2ae, seven output digests equal to before the split, no #if 0, no foreach include, no QtOpenGL in binaries or build.ninja, no OpenGL in the CMake files"
    requirement: CLEAN-03
    verification:
      - kind: other
        ref: "build/scratch/closing6/ctest-release.txt, ctest-debug.txt, digests.txt, static-checks.txt"
        status: pass
    human_judgment: false
  - id: D3
    description: "core_tests, foundation_tests, qt_tests and ProjectRoundTripTestSuite pass under the macos-asan preset with no AddressSanitizer report; the two UBSan lines are the known BinaryImage.cpp:1044:37 location"
    requirement: SAFE-02
    verification:
      - kind: other
        ref: "build/scratch/closing6/asan-ctest.txt, asan-roundtrip.txt"
        status: pass
    human_judgment: false
  - id: D4
    description: "D-08 memory run: ten 165.6 MP pages through WorkerThreadPool with batchMemoryCap give cap=5 threads=5 below ideal=10, done=10 failed=0, peak 17737 MiB at most the 32768 MiB budget, and swap 0 before, in all 19 samples and after"
    requirement: SAFE-03
    verification:
      - kind: other
        ref: "build/scratch/mem/probe-single.txt, batch.txt, swap-before.txt, swap-after.txt; Task 3 verify block exit 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "Both live mac_check invocations end verdict: PASS at the closing HEAD with section verdicts equal to the Phase 5 closing run"
    requirement: CLEAN-02
    verification:
      - kind: other
        ref: "build/scratch/mac/phase6-input/summary.txt and phase6-panels/summary.txt against closing-input and closing-panels; Task 3 verify block exit 0"
        status: pass
    human_judgment: false
  - id: D6
    description: "Swap does not grow on the user's real scans in Activity Monitor (the optional check of D-08)"
    requirement: SAFE-03
    verification: []
    human_judgment: true
    rationale: "The memory run uses synthetic pages only and never launches the app; the real-scan check is offered to the user in records/closing-check.md and was not done by Claude (D-00e)"

duration: about 1h 5min of clock time, including the wait for the user's go and the live runs
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 16: Closing check of Phase 6 Summary

**Phase 6 closes with evidence for all five roadmap criteria and nine requirements: TESTING.md documents the new tests and the opt-in probes, every automated check passes at HEAD (ctest 7 of 7 in both presets, goldens and seven output digests unchanged, no AddressSanitizer report), the D-08 batch through the worker pool ran at the memory cap of 5 threads with a 17737 MiB peak and no swap, and both live `mac_check` runs equal the Phase 5 verdicts.**

## Performance

- **Duration:** about 1h 5min (13:10 to 14:15, 2026-11-17), of which the gate (about 6 min), the Debug ctest (4 min) and the sanitizer build (about 10 min) are waiting time, plus the wait for the user's go
- **Tasks:** 3 (tracer, checkpoint, auto)
- **Files:** 2 created, 2 modified

## Accomplishments

- Task 1 (tracer): `TESTING.md` lists the Phase 6 suites (`CoreImageSizeLimitTestSuite`, `CoreMemoryBudgetTestSuite`, `FoundationXmlEscapeTestSuite`, the round-trip cases and `ProjectRoundTripBackslashTestSuite`) and the new Qt slots by topic, and has the new section Opt-in probes (digest, peak memory and batch probes with `ST_PROBE_DPI`, `ST_PROBE_SCALE` and `ST_PROBE_PAGES`, each statement checked against the probe sources). Gate `s23-docs` on `26702f8e` passed.
- Automated closing checks at HEAD, outputs under `build/scratch/closing6/`: Release (66.5 s) and Debug (247.3 s) ctest `100% tests passed out of 7`; goldens equal a796c2ae; the seven `DIGEST` lines equal those from before the split; no dead `#if 0` block, no `boost/foreach` include, no `QtOpenGL` in eleven release binaries or either `build.ninja` (with positive controls); 24 of 24 gate-log rows read 100% of 7 and `unchanged`.
- Sanitizers: the `macos-asan` build, `core_tests`, `foundation_tests` and `qt_tests` (100% of 3) and the two round-trip suites: 0 `ERROR: AddressSanitizer` lines. The 2 `runtime error:` lines are the known `BinaryImage.cpp:1044:37` location of Phase 1, older code, not fixed.
- Task 2: the blocking checkpoint was returned with the exact commands, memory use, duration and what the user should avoid; the user then said go.
- Task 3: calibration of one 165.6 MP page peaked at 4909 MiB (estimate 6317). The capped batch (`cap=5 threads=5 ideal=10 max_running=5 done=10 failed=0`) peaked at 17737 MiB above start (budget 32768) in 39 s, and swap stayed at 0 before, in all 19 samples and after. `mac_check` input and panels runs both ended `verdict: PASS` with section verdicts equal to the Phase 5 closing run. `records/memory-run.md` and the completed `records/closing-check.md` hold the evidence.

## Task Commits

1. **Task 1 (tracer): the documentation and the automated closing record** - `26702f8e` (docs, gate `s23-docs`), `c167b12a` (docs, gate record), `dfe5ee69` (docs, closing check, automated part)
2. **Task 2: checkpoint** - no commit (the user's go)
3. **Task 3: memory run, live mac_check, completed records** - `dc837bd8` (docs, closing check)

User's answer at Task 2: relayed by the orchestrator as the user's own "go" for both live runs (no skip), given in the main conversation; the runs were started by the orchestrator, not by this executor.

## Deviations from Plan

### Auto-fixed Issues

None for code. Two points that differ from the plan text:

**1. [Plan wording] The live runs were started by the orchestrator**
- **Found during:** Task 3
- **Issue:** the plan has the executor run the memory run and `mac_check`; after the checkpoint the orchestrator ran the plan's exact commands itself on the user's go and handed the files on disk to this executor, which declined to start anything without a user go of its own and wrote the records and ran both `<verify>` blocks against the files.
- **Impact:** none on the evidence; `records/memory-run.md` states who ran the commands.

**2. [Plan estimate] The batch took 39 s, not 10 to 15 minutes**
- **Issue:** the plan estimated 10 to 15 minutes; the ten pages ran as two rounds of five in 39 s.
- **Fix:** recorded in `records/memory-run.md`; the memory figures are what the run measures.

**Total deviations:** 2 (wording and an estimate). **Impact:** none.

## Issues Encountered

- A scripted wait loop was moved to the background by the Bash tool after 180 s; it ended when the gate summary appeared, and none outlives this run.
- The sanitizer ctest hides reports of passing tests, so it was run with `-V`.
- No check failed.

## Known Stubs

None.

## Threat Flags

None. T-06-22 (memory run) is mitigated: blocking checkpoint with the memory use stated, single-page calibration first, capped batch only (`cap=5`), no uncapped comparison, swap sampled every 2 s, and the run stayed 15000 MiB inside the budget. T-06-23 (panels reaching the home folder) is mitigated: the panels run used `HOME` and `CFFIXED_USER_HOME` below `build/`, and that home stayed empty. T-06-24 (paths in records): `grep -c "$HOME"` prints 0 for `closing-check.md`, `memory-run.md` and `TESTING.md`.

## Next Phase Readiness

Phase 6 has all 16 plans executed. Next are phase verification and the transition: the notes of `records/closing-check.md` section "For the transition" (PROJECT.md Context and STATE.md Blockers still call Phase 3 WR-01 and Phase 4 WR-01 to WR-03 open; the OpenGL mentions in `.planning/codebase` and the generated `.claude/CLAUDE.md`; the STATE.md line about live-run checkpoints) are for the transition, per D-11. The optional Activity Monitor check on real scans stays with the user.

## Self-Check: PASSED

- `records/closing-check.md`, `records/memory-run.md` and the `## Opt-in probes` section of `TESTING.md` exist (pass).
- Commits `26702f8e`, `c167b12a`, `dfe5ee69` and `dc837bd8` are ancestors of HEAD; `git rev-list --count` from the recorded base `f2c5a732` gives 4 before this SUMMARY (pass).
- Gate summary `s23-docs` says `result: PASS` and `goldens: unchanged`; its gate-log row exists (pass).
- Task 1 and Task 3 `<verify>` blocks exit 0; the Task 2 acceptance check (no `build/scratch/mem/` or `phase6-*` before the answer) held at the checkpoint (pass).
