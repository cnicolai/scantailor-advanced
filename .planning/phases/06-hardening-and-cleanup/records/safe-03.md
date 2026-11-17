# SAFE-03: worker threads capped by physical memory (plans 06-08 and 06-09)

Checked on 2026-11-17, Qt 6.12.0, macOS on Apple Silicon, 64 GiB, 10 processors. Code commits: `062765df` (the cap, gate `s10-thread-cap`), `f18fef83` (the slider display and the shared setting reader, gate `s11-load-tooltip`) and `87c61b9b` (the two memory probes, gate `s12-memory-probe`). Evidence files are under `build/scratch/` in `{repo}` (not committed): `core-s10.txt`, `qt-s10.txt`, `qt-s11.txt`, `probe-s12.txt`, `batch-s12.txt`, `probe-s12-list.txt`.

## Decisions

| Decision | Value | Why |
|----------|-------|-----|
| Budget | half of `hw.memsize` (34359738368 B on this Mac) | the rest stays with macOS, the GUI, the thumbnail cache and compressed memory |
| Cost of one page | 40 B per pixel (`MemoryBudget::kBytesPerPagePixel`) of the larger of the source and the estimated output | the research peak was 17 to 33.1 B per pixel of that larger count (table below); 40 B is about 1.2 times the worst value |
| Estimator rule | output pixels at the fixed output dpi 600 (`kAssumedOutputDpi`), a missing source dpi counts as 300, per axis `ceil` in double, never below the source pixels, clamp at 2^40 | needs no access to `output::Settings`, which the batch start cannot reach |
| Limit of the rule | a page whose output dpi the user raises above 600 is estimated too small by `(dpi / 600)^2` | revisit if the user sets such a dpi (research: a 300 dpi source with 1200 dpi output peaked at 361 B per source pixel, 26 B per larger pixel, while the estimate counts only the 600 dpi output of that page) |
| The one cap function | `MemoryBudget::batchMemoryCap(const PageSequence&)` | `MainWindow::startBatchProcessing` and the batch probe call the same function |
| When the cap is set | at every `startBatchProcessing`, after the loop over the pages and before the first `submitTask`; it stays in force for interactive processing afterwards (one page at a time anyway) | the cap depends on the largest page of the batch |
| Slider setting | `WorkerThreadPool::storedThreadCount(settings, max)` clamps to `[1, max]`; the key literal lives in `WorkerThreadPool::threadCountKey()` only; the pool and `SystemLoadWidget` both read through it | one reader, T-06-12 |
| Tooltip | `<n>/<max>`, plus `, limited to <cap> by memory` when the cap is below `n`; English only, no translation update (D-00d) | the slider range and the stored value are untouched (D-00c) |
| 32-bit branches | `sizeof(void*) <= 4` removed from `WorkerThreadPool.cpp` (plan 06-08) and `SystemLoadWidget.cpp` (this plan) | D-07, the only target is arm64 |
| Stored setting | `settings/batch_processing_threads` is written only by the slider, as before; the cap never writes or removes it | D-00c |

## Research table: peak above start per larger pixel

| Page | Source px | Output px | Peak above start | B per larger pixel |
|------|-----------|-----------|------------------|--------------------|
| flat, B/W, 300 dpi | 1.15 MP | 0.69 MP | 31 MB | 27 |
| mixed colour, 600 dpi | 4.6 MP | 3.9 MP | 145 MB | 31.5 |
| mixed colour, 2400 dpi | 73.6 MP | 61.8 MP | 2178 MB | 29.6 |
| mixed 300 dpi source, output 600 dpi | 1.15 MP | 3.9 MP | 117 MB | 30 |
| curved, auto dewarp, 2400 dpi | 73.6 MP | 54.2 MP | 1733 MB | 23.5 |

The worst measured value is 33.1 B per pixel (mixed colour, 600 dpi, 4.6 MP); the full table is in `06-RESEARCH.md`, "Memory model, measured".

## Cap table (pinned by `test_threads_for_memory_table`)

| Page | Pixels | Cap on 64 GiB | Cap on 16 GiB |
|------|--------|---------------|---------------|
| 2480 x 3508 at 300 dpi | 34.8 MP (output) | 24 | 6 |
| 4961 x 7016 at 600 dpi | 34.8 MP | 24 | 6 |
| 7016 x 9921 at 600 dpi | 69.6 MP | 12 | 3 |
| 9921 x 14031 at 1200 dpi | 139.2 MP | 6 | 1 |
| 14031 x 19843 at 1200 dpi | 278.4 MP | 3 | 1 |
| 8000 x 9200 at 2400 dpi | 73.6 MP | 11 | 2 |
| 12000 x 13800 at 3600 dpi (the D-08 page) | 165.6 MP (source) | 5 | 1 |

The pool runs `max(1, min(idealThreadCount, slider, cap))` tasks. On this Mac `idealThreadCount` is 10, so the first two rows run at full parallelism and only the larger pages are throttled (D-06).

## Test evidence

- Plan 06-08: `CoreMemoryBudgetTestSuite` (eight cases) and `TestCoreQt::testWorkerPoolHonoursMemoryCap`; mutation runs: the budget share `/ 2` changed to `/ 3` fails 14 core checks; a pool that ignores its cap fails the pool test (actual 3, expected 1).
- Plan 06-09, `qt-s11.txt`: `PASS   : TestCoreQt::testStoredThreadCountClamps()` (key absent gives max, 0 gives 1, -5 gives 1, 4 gives 4, 999 gives max, the stored 999 is not rewritten) and `PASS   : TestUxQt::testSystemLoadToolTipShowsMemoryLimit()` (no cap: `10/10`; cap 1: `10/10, limited to 1 by memory`, `1/10`; the slider tooltip property follows the slider; range 1..10 and the stored setting unchanged by the cap; a cap equal to the value gives the plain text). All four `Totals:` lines say `0 failed, 0 skipped`.
- Mutation runs (not committed, restored from byte copies): `m_memoryCap < threads` changed to `m_memoryCap < 0` fails the tooltip test (actual `10/10`, expected `10/10, limited to 1 by memory`); `storedThreadCount` without the clamp fails the clamp test (actual 0, expected 1).
- Probe mutation runs (not committed): `pool.setMemoryCap(2)` in place of the computed cap fails `threads == effectiveThreads(...)` (`2 != 10`); `estimate / 100` in the peak check fails (`47808584 > 1840000`); leaving the last page without its input file gives `failed=1` and `FAILURE no output file ...`.
- Gate `s11-load-tooltip` (tested `f18fef83`) and `s12-memory-probe` (tested `87c61b9b`): Release and Debug `100% tests passed out of 7`, goldens unchanged.

## The two probes (disabled by default, never run by ctest)

`pipeline_tests --list_content` lists `test_print_peak_memory` and `test_batch_through_worker_pool` without the enabled marker `*`; `ctest` runs `pipeline_tests` without `--run_test`, so Boost skips both. Both read `ST_PROBE_DPI` (default 300), `ST_PROBE_SCALE` (default 1) and, for the batch probe, `ST_PROBE_PAGES` (default 4).

Smoke run of the peak probe at the defaults (one 1.15 MP MIXED page at 300 dpi, output at the source dpi; the estimate counts the output at the assumed 600 dpi, 4.6 MP):

```
PROBE pixels=4600000 estimate_mb=175 peak_above_start_mb=43 cap16=46 cap64=186 cap_here=186
```

The peak above start is 43 MB against the 175 MB estimate, so the check `peak <= estimate` holds with a margin of four (43 MB is about 39 B per source pixel of this small page, where fixed costs weigh more than at the D-08 size; the estimate is the larger because it assumes the 600 dpi output).

Smoke run of the batch probe at the defaults (4 pages through `LoadFileTask`-wrapped stage chains on a real `WorkerThreadPool` with `setMemoryCap(MemoryBudget::batchMemoryCap(sequence))`):

```
BATCH pages=4 pixels=4600000 estimate_mb=175 budget_mb=32768 ideal=10 slider=10 cap=186 threads=10 max_running=4 done=4 failed=0 peak_above_start_mb=135 swap_before_bytes=0 swap_max_bytes=0 swap_after_bytes=0 elapsed_s=0
```

A small page does not bind the cap (186 against 10 threads), all four pages run at once, none failed, and the swap stayed at 0.

## D-08 run (plan 06-16, only after the user's go)

```
ST_PROBE_DPI=1200 ST_PROBE_SCALE=3 ST_PROBE_PAGES=10 build/macos-release/pipeline_tests --run_test=BatchMemoryProbeTestSuite/test_batch_through_worker_pool --log_level=message
```

Ten pages of 12000 x 13800 px at 3600 dpi: 165600000 estimated pixels each, 6317 MiB per page at 40 B per pixel. On this Mac the budget is 32768 MiB, so `batchMemoryCap` gives 5 while `idealThreadCount` is 10: without the cap all ten pages would run at once. Expected `BATCH` line: `pixels=165600000 estimate_mb=6317 budget_mb=32768 ideal=10 slider=10 cap=5 threads=5 max_running<=5 done=10 failed=0`, `peak_above_start_mb` near 5 x 4.9 GB (research factor 29.6 B per pixel at 73.6 MP) and at most 5 x 6317 = 31585 MiB, inside the 32768 MiB budget, and `swap_max_bytes` and `swap_after_bytes` not above `swap_before_bytes` (0 at research time). The ten inputs are hard links of one PNG, so the input costs the disk space of one page. The uncapped comparison stays forbidden.
