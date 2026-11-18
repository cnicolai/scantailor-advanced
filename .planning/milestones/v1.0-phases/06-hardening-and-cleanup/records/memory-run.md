# Memory run of Phase 6 (plan 06-16, D-08, roadmap criterion 3)

Run on 2026-11-17 from 14:00 to 14:03 on HEAD `dfe5ee69`. The release binaries were built at 13:15 from source identical to `a9d17d1c` (the last commit that changed `src/` or a build file). The commands below are the exact commands of plan 06-16 Task 3. They were started by the orchestrator, not by the plan executor, after the user's own go in the main conversation (D-00b, D-08). Only synthetic pages were used; the application binary `scantailor-advanced` was not launched and no real scan was read. Output is normalised: `{repo}` for the repository path, `~` for the home folder. Evidence files (not committed): `build/scratch/mem/` in `{repo}`.

## Machine

```
$ sysctl hw.memsize hw.ncpu                      (build/scratch/mem/machine.txt)
hw.memsize: 68719476736
hw.ncpu: 10
$ df -h build                                    (build/scratch/mem/disk.txt)
/dev/disk3s5   1,8Ti   1,7Ti    86Gi    96%    ...    /System/Volumes/Data
```

64 GiB of RAM, 10 processors (`idealThreadCount` 10), 86 GiB free on the build volume (the plan needs 10 GB). `vm.swapusage` read `total = 0.00M used = 0.00M free = 0.00M` before the batch: this Mac had no swap in use.

## Why this size

Ten pages of 12000 x 13800 px (`ST_PROBE_DPI=1200 ST_PROBE_SCALE=3`) are 165600000 pixels each, estimated at 40 B per pixel = 6317 MiB per page. The budget is half of `hw.memsize` = 32768 MiB, so `MemoryBudget::batchMemoryCap` gives 5 while `idealThreadCount` is 10: the cap binds. Without the cap, all ten pages would run at once (up to 10 x 6317 MiB by the estimate).

## What the run exercises, and what it leaves out

The batch probe (`BatchMemoryProbeTestSuite/test_batch_through_worker_pool`) uses the app's batch path: each page is a `LoadFileTask`-wrapped chain of the six stage tasks, run on a real `WorkerThreadPool` after `setMemoryCap(MemoryBudget::batchMemoryCap(sequence))`, where `batchMemoryCap` is the function `MainWindow::startBatchProcessing` calls, computed from this Mac's real `hw.memsize`. The probe copies the submit loops of `startBatchProcessing` and the refill of `filterResult`. It leaves out the `MainWindow` shell (the window, the progress widgets, the event wiring), because the application binary is never launched (D-00e).

An uncapped batch was not run, nor a batch with a changed cap or with more pages at once than the computed cap. That ban is a safety limit and not a D-08 rule: an uncapped run of ten 165.6 MP pages could push this Mac into swap, the very harm SAFE-03 prevents. The research's earlier design (independent processes with a simulated 16 GiB budget) was replaced at plan revision, because it never exercised `startBatchProcessing`'s cap on this Mac.

## How D-08's "before and after" and "peak resident memory" are read

- Before and after: read as the run window of the capped batch, as roadmap criterion 3 says ("Activity Monitor shows no growth in swap during the run"). `vm.swapusage` is read right before the batch starts (`BATCH swap_before_bytes` and the external `swap-before.txt`), every 2 s during it (every `SAMPLE swap_used_bytes`) and after it ends (`swap_after_bytes` and the external `swap-after.txt`). The batch passes only if no value is above the one before. The footprint is read at the start, in every `SAMPLE footprint_mb` and as its peak after the run (`peak_above_start_mb`).
- Metric: D-08's "peak resident memory" is measured as the peak `phys_footprint` from `task_vm_info`, not RSS. macOS compresses memory, and compressed pages leave a process's RSS while still taking RAM, so RSS is misleading; `phys_footprint` counts them.

## Calibration of one page

```
$ env ST_PROBE_DPI=1200 ST_PROBE_SCALE=3 build/macos-release/pipeline_tests --run_test=PeakMemoryProbeTestSuite/test_print_peak_memory --log_level=message > build/scratch/mem/probe-single.txt 2>&1
Running 1 test case...
PROBE pixels=165600000 estimate_mb=6317 peak_above_start_mb=4909 cap16=1 cap64=5 cap_here=5
*** No errors detected
```

(14:00:45 to 14:01:12, 27 s.) The peak above start is 4909 MiB, at or below the 6317 MiB estimate: 31.1 B per pixel of the 165.6 MP page, against the factor of 40. `cap_here=5` on this Mac, `cap64=5`, `cap16=1`. No factor fix was needed, so `s24-factor-fix` does not exist.

## Batch through the worker pool

```
$ LC_ALL=C sysctl vm.swapusage > build/scratch/mem/swap-before.txt
vm.swapusage: total = 0.00M  used = 0.00M  free = 0.00M  (encrypted)
$ env ST_PROBE_DPI=1200 ST_PROBE_SCALE=3 ST_PROBE_PAGES=10 build/macos-release/pipeline_tests --run_test=BatchMemoryProbeTestSuite/test_batch_through_worker_pool --log_level=message > build/scratch/mem/batch.txt 2>&1
$ LC_ALL=C sysctl vm.swapusage > build/scratch/mem/swap-after.txt
vm.swapusage: total = 0.00M  used = 0.00M  free = 0.00M  (encrypted)
```

The batch started at 14:01:24 (`batch-start.txt`), in the Bash tool's background mode. Final lines of `batch.txt`:

```
BATCH pages=10 pixels=165600000 estimate_mb=6317 budget_mb=32768 ideal=10 slider=10 cap=5 threads=5 max_running=5 done=10 failed=0 peak_above_start_mb=17737 swap_before_bytes=0 swap_max_bytes=0 swap_after_bytes=0 elapsed_s=39
*** No errors detected
```

- The cap bound: `cap=5`, `threads=5` equal to the cap and below `ideal=10`, `max_running=5` equal to `threads`. All ten pages finished (`done=10 failed=0`).
- Peak: 17737 MiB above the start, at most the 32768 MiB budget and below the 31585 MiB that five pages at the estimate would take (about 3.5 GiB per running page, 22.5 B per pixel of the 165.6 MP page, below the 31.1 B of the calibration: pages in different stages do not all hit their peak together).
- `SAMPLE` series: 19 lines, one every 2 s from 2100 ms to 38103 ms. `running` is 5 in every line and never above 5. The highest `footprint_mb` is 18451 (at 20 s). `swap_used_bytes` is 0 in every line.
- External `vm.swapusage`: `used = 0.00M` before and after; the probe's `swap_before_bytes`, `swap_max_bytes` and `swap_after_bytes` are all 0. No growth in swap during the run.
- Duration: 39 s, far below the plan's estimate of 10 to 15 minutes. The ten pages ran as two rounds of five, so one round took about 20 s. The estimate was a planning guess and was wrong by a factor of about 20; the memory figures, not the duration, are what the run measures.

## Verdict

PASS. On the app's batch path, the memory cap binds on this Mac (5 of 10 threads for the 165.6 MP page), all ten pages are processed without failure, the peak footprint stays inside the half-of-RAM budget (17737 of 32768 MiB), and swap does not grow. The estimate of 40 B per pixel held for the single page (4909 of 6317 MiB) and for the batch. Roadmap criterion 3 is met on synthetic pages; the optional Activity Monitor check on real scans (D-08) stays with the user and is described in `records/closing-check.md`.
