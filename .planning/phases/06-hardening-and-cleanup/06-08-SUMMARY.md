---
phase: 06-hardening-and-cleanup
plan: 08
subsystem: worker-pool-memory-cap
tags: [cpp, safety, memory, worker-pool, sysctl, boost-test, qt-test, gate]

requires:
  - phase: 06-hardening-and-cleanup
    provides: "plan 06-01 conventions (gate, label table, output-path guard) and the gate log; plan 06-04 image size limit"
provides:
  - "MemoryBudget (src/core): physicalMemoryBytes (sysctl hw.memsize), pagePixelEstimate, largestPagePixels, threadsForMemory, batchMemoryCap(const PageSequence&), effectiveThreads, constants kBytesPerPagePixel 40, kAssumedOutputDpi 600, kFallbackSourceDpi 300, kNoCap, kMaxEstimatePixels 2^40"
  - "WorkerThreadPool::setMemoryCap(int), maxThreadCount() const, member m_memoryCap; updateNumberOfThreads applies MemoryBudget::effectiveThreads(ideal, slider, cap); the 32-bit branch is gone"
  - "MainWindow::startBatchProcessing sets the cap of the largest page before the first submitTask"
  - "CoreMemoryBudgetTestSuite (8 cases) and TestCoreQt::testWorkerPoolHonoursMemoryCap"
affects: [06-09, 06-16]

actuals:
  tokens: 3500
  tasks: 1
  commits: 2

plan_head_before: 067a74146aad59465ab9f830b6241c2b7e84de92
plan_head_after: ae3c0186421e2456a4c89411b3aa6cae4b0a0054

tech-stack:
  added: []
  patterns:
    - "Pure cap arithmetic in one class, the one batchMemoryCap function shared by the app and the later memory probe"
    - "Cap applied on top of the stored slider value; the stored setting is never rewritten"
    - "Every new test proven to discriminate with an uncommitted mutation run, restored from byte copies"

key-files:
  created:
    - src/core/MemoryBudget.h
    - src/core/MemoryBudget.cpp
    - src/core/tests/TestMemoryBudget.cpp
  modified:
    - src/core/CMakeLists.txt
    - src/core/WorkerThreadPool.h
    - src/core/WorkerThreadPool.cpp
    - src/app/MainWindow.cpp
    - src/core/tests/CMakeLists.txt
    - src/qt_tests/tst_core_qt.h
    - src/qt_tests/tst_core_qt.cpp
    - .planning/phases/06-hardening-and-cleanup/records/gate-log.md

key-decisions:
  - "Budget is half of hw.memsize at 40 B per pixel of the larger of source and estimated output pixels (output at the fixed 600 dpi rule, no coupling to output::Settings); revisit if the user sets an output dpi above 600"
  - "The cap is set at every startBatchProcessing and stays in force for interactive processing afterwards (one page at a time anyway)"
  - "The D-08 run page 12000 x 13800 at 3600 dpi (165.6 MP) is pinned as a test vector: cap 5 on 64 GiB, 1 on 16 GiB"

requirements-completed: []

coverage:
  - id: D1
    description: "Cap arithmetic is pinned: 64 GiB gives 24, 24, 12, 6, 3, 11, 5 and 16 GiB gives 6, 6, 3, 1, 1, 2, 1 for the seven reference pages including the D-08 run page; floor at each threshold; never below 1; kNoCap for non-positive input; no overflow for 2^62 bytes or pixels"
    requirement: SAFE-03
    verification:
      - kind: unit
        ref: "src/core/tests/TestMemoryBudget.cpp#test_threads_for_memory_table, test_threads_for_memory_boundaries, test_threads_for_memory_no_cap"
        status: pass
    human_judgment: false
  - id: D2
    description: "Page estimate follows the precision contract (per-axis ceil in double, never below the source pixels, clamp to 2^40, missing dpi counts as 300)"
    requirement: SAFE-03
    verification:
      - kind: unit
        ref: "src/core/tests/TestMemoryBudget.cpp#test_page_pixel_estimate"
        status: pass
    human_judgment: false
  - id: D3
    description: "The pool runs min(idealThreadCount, slider, cap) tasks, returns to min(ideal, slider) when the cap is lifted, and never rewrites settings/batch_processing_threads"
    requirement: SAFE-03
    verification:
      - kind: unit
        ref: "src/qt_tests/tst_core_qt.cpp#testWorkerPoolHonoursMemoryCap"
        status: pass
    human_judgment: false
  - id: D4
    description: "Small scans are not throttled: A4 at 300 and 600 dpi give a cap of 24 on 64 GiB, above this Mac's 10 threads; one function (batchMemoryCap) combines RAM and page sizes"
    requirement: SAFE-03
    verification:
      - kind: unit
        ref: "src/core/tests/TestMemoryBudget.cpp#test_threads_for_memory_table, test_batch_memory_cap, test_largest_page_pixels"
        status: pass
    human_judgment: false
  - id: D5
    description: "The batch start sets the cap before the first task is submitted (setMemoryCap at MainWindow.cpp line 1259, first submitTask at line 1270)"
    requirement: SAFE-03
    verification:
      - kind: other
        ref: "grep -n 'setMemoryCap(\\|submitTask(' src/app/MainWindow.cpp"
        status: pass
    human_judgment: true
    rationale: "No automated test drives startBatchProcessing (needs a loaded project and the main window); the wiring is checked by reading the code and the large memory run of plan 06-16 exercises it through the same batchMemoryCap"
  - id: D6
    description: "The commit passes the gate: 100% tests passed out of 7 in Release and Debug, goldens unchanged"
    requirement: SAFE-03
    verification:
      - kind: other
        ref: "gate.sh check s10-thread-cap (tested 062765df) -> result: PASS"
        status: pass
    human_judgment: false

duration: 25min
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 08: SAFE-03, cap the worker threads by physical memory Summary

**A batch run now limits parallel page tasks to half of the Mac's RAM divided by 40 B per pixel of the largest page (24 for A4 on this 64 GiB Mac, 5 for the 165.6 MP D-08 page), applied on top of the thread slider without ever rewriting the stored setting, through one `MemoryBudget::batchMemoryCap` function that the later memory probe shares.**

## Performance

- **Duration:** about 25 min of clock time (one gate run in the background)
- **Completed:** 2026-11-17
- **Tasks:** 1 (tracer)
- **Files:** 3 created, 8 modified

## Accomplishments

- `MemoryBudget` (new, `src/core/`): `physicalMemoryBytes` reads `hw.memsize` with `sysctlbyname`; `pagePixelEstimate` computes per axis in double with `ceil`, takes the larger of source and 600 dpi output pixels, clamps to 2^40, and counts a missing dpi as 300; `threadsForMemory` is `(bytes / 2) / (40 * min(pixels, 2^40))` clamped to [1, `kNoCap`] and `kNoCap` for non-positive input; `batchMemoryCap(pages)` joins RAM and the largest page; `effectiveThreads` is `max(1, min(ideal, slider, cap))`.
- `WorkerThreadPool` gained `setMemoryCap`, `maxThreadCount` and `m_memoryCap`; `updateNumberOfThreads` uses `effectiveThreads`, reads the slider with the old key and clamp, and no longer has the `sizeof(void*) <= 4` branch.
- `MainWindow::startBatchProcessing` calls `m_workerThreadPool->setMemoryCap(MemoryBudget::batchMemoryCap(sequence))` after the loop over the pages and before the first `submitTask`.
- Tests: `CoreMemoryBudgetTestSuite` has eight cases (estimate, table for 64 and 16 GiB over seven pages, threshold boundaries, no-cap and overflow vectors, `effectiveThreads`, `largestPagePixels`, `batchMemoryCap`, physical memory above 1 GiB). `TestCoreQt::testWorkerPoolHonoursMemoryCap` checks the pool at slider 3, cap 1, lifted cap, removed key, and that the key keeps its value under a cap.
- Mutation runs (not committed, restored from byte copies, `cmp` identical afterwards): the budget share changed from `/ 2` to `/ 3` makes 14 core checks fail; the pool ignoring its cap (`kNoCap` passed to `effectiveThreads`) fails `testWorkerPoolHonoursMemoryCap` with actual 3, expected 1.
- Gate `s10-thread-cap` (tested `062765df`, command diff 6 / 6: the two new compile lines, the `libcore.a` archive line and the `core_tests` link line): Release and Debug `100% tests passed out of 7`, goldens unchanged. Every touched C++ file passes `clang-format --dry-run -Werror`. No file under imageproc or dewarping was added, so the output-path allowlist is unchanged.

## Task Commits

1. **Task 1 (tracer): a batch run caps the worker pool by physical memory and the largest page** - `062765df` (feat), gated as `s10-thread-cap`; gate record `ae3c0186`

The tracer verify (core suite, qt_tests, gate summary) passed end to end before any later work; this plan has no expansion task.

## Deviations from Plan

None - plan executed as written. Small notes, none changing the outcome:

- The page-estimate overflow vector (2,000,000,000 x 2,000,000,000 px at 1 dpi gives exactly 2^40) sits inside `test_page_pixel_estimate`, so the suite keeps the eight cases the plan names.
- `qt_tests` was run with the ctest environment of `build/macos-release` (`QT_QPA_PLATFORM=offscreen`, `LC_ALL=en_US.UTF-8`, `TMPDIR=build/macos-release/test-tmp`), not bare.
- The gate was run through the Bash tool's background mode and waited for with a polling loop.

## Issues Encountered

None open. No large memory run was started (plan 06-16, after the user's go); the app binary was not launched.

## Known Stubs

None.

## Threat Flags

None beyond the plan's threat model. T-06-03 (self-inflicted swap) is mitigated by the cap at batch start, T-06-13 (overflow) by the qint64 arithmetic, the 2^40 estimate clamp and the [1, INT_MAX] result clamp, with overflow vectors in the tests. The new system call is read-only (`sysctlbyname("hw.memsize")`).

## Next Phase Readiness

SAFE-03 stays unmarked in REQUIREMENTS.md until every plan that declares it has finished. Plan 06-09 (slider display, the shared setting reader, the memory probes that call `batchMemoryCap`) is next; the large memory run is plan 06-16 after the user's go.

## Self-Check: PASSED

- Created files exist (`MemoryBudget.h`, `MemoryBudget.cpp`, `TestMemoryBudget.cpp`); commits `062765df` and `ae3c0186` are ancestors of HEAD and `git rev-list --count` from the recorded base gives 2 (pass).
- Acceptance criteria re-run: `setMemoryCap(` (line 1259) precedes the first `submitTask(` (line 1270) in `startBatchProcessing`; `git grep -F 'sizeof(void*) <= 4' -- src/core/WorkerThreadPool.cpp` prints nothing; `core-s10.txt` holds `No errors detected` and the eight case names; `qt-s10.txt` holds `PASS   : TestCoreQt::testWorkerPoolHonoursMemoryCap()`; gate summary has `result: PASS` and `goldens: unchanged`, the tested commit equals the commit with the plan subject, the gate-log row exists (pass).
- clang-format dry-run clean on all touched C++ files (pass).
