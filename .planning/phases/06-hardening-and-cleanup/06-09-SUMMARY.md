---
phase: 06-hardening-and-cleanup
plan: 09
subsystem: batch-thread-slider-and-memory-probes
tags: [cpp, safety, memory, worker-pool, qt-widgets, qt-test, boost-test, gate]

requires:
  - phase: 06-hardening-and-cleanup
    provides: "plan 06-08 MemoryBudget, WorkerThreadPool::setMemoryCap and the cap at batch start; plan 06-04 ProcessMemory.h; plan 06-01 conventions (gate, label table, output-path guard)"
provides:
  - "WorkerThreadPool::threadCountKey() and storedThreadCount(const QSettings&, int): the one clamped reader of settings/batch_processing_threads, used by the pool and by SystemLoadWidget"
  - "SystemLoadWidget::setMemoryCap(int), toolTipText(int) const, member m_memoryCap; MainWindow::m_systemLoadWidget; startBatchProcessing passes the cap to the pool and the slider"
  - "TestCoreQt::testStoredThreadCountClamps, TestUxQt::testSystemLoadToolTipShowsMemoryLimit"
  - "src/pipeline_tests/TestPeakMemoryProbe.cpp: PeakMemoryProbeTestSuite (PROBE line) and BatchMemoryProbeTestSuite (SAMPLE and BATCH lines), both disabled by default"
  - "records/safe-03.md: decisions, cap table, evidence, smoke lines and the expected D-08 numbers"
affects: [06-16]

actuals:
  tokens: 7800
  tasks: 2
  commits: 5

plan_head_before: 5d1ac12e5f3970319d029feedcce0e8d43b303bc
plan_head_after: 631c50f38e3d8ed5a07ff12dfcc05d3c85ed3a71

tech-stack:
  added: []
  patterns:
    - "One shared function for a settings key and its clamp, read by every consumer"
    - "Opt-in Boost.Test probes decorated disabled(): listed without the enabled marker, run only when named"
    - "Batch probe mirrors the app's batch path (createCompositeTask chain wrapped in LoadFileTask, submit loop, filterResult refill) without editing the frozen HeadlessPipeline files"
    - "Every new test proven to discriminate with an uncommitted mutation run, restored from byte copies"

key-files:
  created:
    - src/pipeline_tests/TestPeakMemoryProbe.cpp
    - .planning/phases/06-hardening-and-cleanup/records/safe-03.md
  modified:
    - src/app/SystemLoadWidget.h
    - src/app/SystemLoadWidget.cpp
    - src/core/WorkerThreadPool.h
    - src/core/WorkerThreadPool.cpp
    - src/app/MainWindow.h
    - src/app/MainWindow.cpp
    - src/qt_tests/tst_core_qt.h
    - src/qt_tests/tst_core_qt.cpp
    - src/qt_tests/tst_ux_qt.h
    - src/qt_tests/tst_ux_qt.cpp
    - .planning/phases/06-hardening-and-cleanup/records/gate-log.md

key-decisions:
  - "Tooltip wording: '<n>/<max>, limited to <cap> by memory', English only, no translation update (D-00d); the slider range and the stored value are untouched by the cap (D-00c)"
  - "The D-08 proof runs on the real batch path: one process, one shared project, LoadFileTask-wrapped stage chains on a real WorkerThreadPool capped by MemoryBudget::batchMemoryCap from this Mac's hw.memsize; only the MainWindow shell is left out"
  - "A page that fails to load gives an error result and no exception, so the batch probe counts a page as failed when its output file is missing"
  - "D-08 run size ST_PROBE_DPI=1200 ST_PROBE_SCALE=3 ST_PROBE_PAGES=10: ten 165.6 MP pages, cap 5 against idealThreadCount 10, at most 31585 MiB inside the 32768 MiB budget; run only in plan 06-16 after the user's go"

requirements-completed: [SAFE-03]

coverage:
  - id: D1
    description: "The batch thread slider shows '<n>/<max>, limited to <cap> by memory' when the cap is below n; range and stored value are untouched by the cap"
    requirement: SAFE-03
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_ux_qt.cpp#testSystemLoadToolTipShowsMemoryLimit"
        status: pass
    human_judgment: true
    rationale: "The tooltip text and property are tested offscreen; how the tooltip looks while the user drags the slider in the batch screen is not asserted by any test"
  - id: D2
    description: "WorkerThreadPool::storedThreadCount clamps the stored slider value to [1, max]; the pool and the slider both read through it and the key literal exists in one source file"
    requirement: SAFE-03
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_core_qt.cpp#testStoredThreadCountClamps"
        status: pass
      - kind: other
        ref: "git grep -l -F 'batch_processing_threads' -- src/core src/app prints only src/core/WorkerThreadPool.cpp"
        status: pass
    human_judgment: false
  - id: D3
    description: "The last 32-bit branches (sizeof(void*) <= 4) are gone from WorkerThreadPool.cpp and SystemLoadWidget.cpp"
    requirement: SAFE-03
    verification:
      - kind: other
        ref: "git grep -n -F 'sizeof(void*) <= 4' -- src/app/SystemLoadWidget.cpp src/core/WorkerThreadPool.cpp prints nothing"
        status: pass
    human_judgment: false
  - id: D4
    description: "Opt-in peak probe: one MIXED page through all six stages prints a PROBE line and checks the peak stays at or below the 40 B per pixel estimate; disabled by default, absent from ctest"
    requirement: SAFE-03
    verification:
      - kind: other
        ref: "build/macos-release/pipeline_tests --run_test=PeakMemoryProbeTestSuite/test_print_peak_memory -> PROBE pixels=4600000 estimate_mb=175 peak_above_start_mb=43 cap16=46 cap64=186 cap_here=186, No errors detected"
        status: pass
      - kind: other
        ref: "pipeline_tests --list_content lists both probe cases without the enabled marker"
        status: pass
    human_judgment: false
  - id: D5
    description: "Opt-in batch probe drives N synthetic pages through LoadFileTask-wrapped stage chains on a real WorkerThreadPool with the app's cap function and prints SAMPLE and BATCH lines with threads, max running, peak above start and swap values"
    requirement: SAFE-03
    verification:
      - kind: other
        ref: "build/macos-release/pipeline_tests --run_test=BatchMemoryProbeTestSuite/test_batch_through_worker_pool -> BATCH pages=4 ... threads=10 max_running=4 done=4 failed=0 swap_before_bytes=0 swap_max_bytes=0 swap_after_bytes=0, No errors detected"
        status: pass
    human_judgment: true
    rationale: "Only the small default size ran here; whether the cap binds and keeps the peak inside the budget at 165.6 MP is the D-08 run of plan 06-16 after the user's go"
  - id: D6
    description: "Both commits pass the gate: 100% tests passed out of 7 in Release and Debug, goldens unchanged"
    requirement: SAFE-03
    verification:
      - kind: other
        ref: "gate.sh check s11-load-tooltip (tested f18fef83) and s12-memory-probe (tested 87c61b9b) -> result: PASS"
        status: pass
    human_judgment: false

duration: about 95min
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 09: SAFE-03, slider display and the D-08 memory probes Summary

**The batch thread slider now says "limited to N by memory" when the memory cap binds, the pool and the slider read the stored thread setting through one clamped function (the last 32-bit branches are gone), and two opt-in probes drive synthetic pages through the app's real batch path and cap function for the D-08 run.**

## Performance

- **Duration:** about 95 min of clock time (two gate runs in the background)
- **Completed:** 2026-11-17
- **Tasks:** 2 (1 tracer, 1 auto)
- **Files:** 1 source file created, 10 source files modified, 1 record created, 1 record appended

## Accomplishments

- `WorkerThreadPool::threadCountKey()` and `storedThreadCount(settings, max)` (clamped to `[1, max]`); `updateNumberOfThreads` and `SystemLoadWidget` read through them, and the key literal now exists in `WorkerThreadPool.cpp` only.
- `SystemLoadWidget` lost its file-local key and its 32-bit clamp; `setMemoryCap(int)` and `toolTipText(int) const` add `, limited to <cap> by memory` when the cap is below the thread value, for the dragged tooltip and for the slider's tooltip property. `MainWindow::startBatchProcessing` computes `memoryCap` once and passes it to the pool and to the slider; the slider range 1..idealThreadCount and the stored value are untouched.
- Tests: `TestCoreQt::testStoredThreadCountClamps` (absent, 0, -5, 4, 999 against max 10; the stored 999 is not rewritten) and `TestUxQt::testSystemLoadToolTipShowsMemoryLimit` (plain text without a cap, limited text after `setMemoryCap(1)`, tooltip property follows the slider, range and settings unchanged, a stored value survives a new widget). Mutation runs fail both (tooltip condition replaced, clamp removed), restored from byte copies.
- `PeakMemoryProbeTestSuite/test_print_peak_memory`: MIXED page through `runPipeline`, one `PROBE` line, check `peak <= 40 B x estimate`.
- `BatchMemoryProbeTestSuite/test_batch_through_worker_pool`: hard-linked PNG inputs, one shared `ProjectPages`, `ThumbnailPixmapCache`, `OutputFileNameGenerator` and set of stage `Settings`, per page the six stage tasks wrapped in `LoadFileTask(BATCH)` and a counting task, a real `WorkerThreadPool` with `setMemoryCap(MemoryBudget::batchMemoryCap(sequence))`, the submit loop of `startBatchProcessing` and the refill of `filterResult`; `SAMPLE` lines every 2 s and one `BATCH` line with `vm.swapusage` values. Mutation runs fail it for a wrong pool cap (`2 != 10`), for a missing input (`failed=1`, `FAILURE no output file`) and the peak probe for a too small estimate.
- `records/safe-03.md` holds the decisions, the 64 and 16 GiB cap table including the D-08 page, the test evidence, both smoke lines and the expected D-08 numbers.
- Gates `s11-load-tooltip` (tested `f18fef83`, command diff 0 / 0, edits only) and `s12-memory-probe` (tested `87c61b9b`, command diff 3 / 3: the new `TestPeakMemoryProbe.cpp` compile line and the `pipeline_tests` link line): Release and Debug `100% tests passed out of 7`, goldens unchanged. Every touched C++ file passes `clang-format --dry-run -Werror`; the output-path guard prints nothing; `HeadlessPipeline.h` and `.cpp` are unedited.

## Task Commits

1. **Task 1 (tracer): the slider shows the memory limit, one shared reader, no 32-bit branch** - `f18fef83` (feat), gated as `s11-load-tooltip`; gate record `b8048f9b`
2. **Task 2: opt-in peak and batch memory probes** - `87c61b9b` (test), gated as `s12-memory-probe`; gate record `8d516712`; SAFE-03 record `631c50f3`

The tracer verify (qt_tests with the three PASS lines, gate summary) passed end to end before the probe task started.

## Deviations from Plan

None - plan executed as written. Small notes, none changing the outcome:

- `toolTipText` appends the limit when `m_memoryCap < threads`, so a stored thread value of 1 with cap 1 shows the plain `1/<max>`, as the behaviour block asks.
- The smoke page's estimate is 4.6 MP (the 1.15 MP source counted at the assumed 600 dpi output), which is why `cap` is 186 and `estimate_mb` 175; the plan's "1.15 MP pages" is the source size.
- `qt_tests` was run with the ctest environment of `build/macos-release`; the gates were run through the Bash tool's background mode and waited for with an until-loop.
- Initial compile of the probe failed on missing complete types (`DefaultParams.h`, three `Params.h` headers); the includes were added before the first commit.

## Issues Encountered

None open. No large memory run was started (plan 06-16, after the user's go); the app binary was not launched.

## Known Stubs

None.

## Threat Flags

None beyond the plan's threat model. T-06-12 (stored slider value out of range) is mitigated by `storedThreadCount` (clamp test) and `effectiveThreads`; T-06-27 (probe started large by accident) by `disabled()`, the absence from `--list_content`'s enabled set, small defaults and the 3600 s submit deadline. The new system call `sysctlbyname("vm.swapusage")` is read-only and lives in test code.

## Next Phase Readiness

SAFE-03 stays shared with plan 06-16 (the memory run); the large D-08 run uses the batch probe of this plan with `ST_PROBE_DPI=1200 ST_PROBE_SCALE=3 ST_PROBE_PAGES=10` and starts only after the user's go. Ready for plan 06-10.

## Self-Check: PASSED

- Created files exist: `src/pipeline_tests/TestPeakMemoryProbe.cpp`, `.planning/phases/06-hardening-and-cleanup/records/safe-03.md`; commits `f18fef83`, `b8048f9b`, `87c61b9b`, `8d516712`, `631c50f3` are ancestors of HEAD and `git rev-list --count` from the recorded base gives 5 (pass).
- Acceptance criteria re-run: `sizeof(void*) <= 4` absent from both files; `batch_processing_threads` only in `src/core/WorkerThreadPool.cpp`; `qt-s11.txt` has the three PASS lines and `0 failed, 0 skipped` Totals; `disabled()` appears twice in the probe file; `probe-s12.txt` and `batch-s12.txt` hold the PROBE and BATCH lines and `No errors detected`; `probe-s12-list.txt` lists both cases without `*`; gate summaries say `result: PASS` and `goldens: unchanged`, the tested commits equal the commits with the plan subjects, both gate-log rows exist; `git diff --quiet e5319cd9 -- src/pipeline_tests/HeadlessPipeline.h src/pipeline_tests/HeadlessPipeline.cpp` succeeds; the record has the factor, both lines, `165600000`, `batchMemoryCap` and no home path (pass).
- clang-format dry-run clean on all touched C++ files (pass).
