---
phase: 06-hardening-and-cleanup
fixed_at: 2026-11-17T00:00:00Z
review_path: .planning/phases/06-hardening-and-cleanup/06-REVIEW.md
iteration: 1
findings_in_scope: 6
fixed: 6
skipped: 0
status: all_fixed
---

# Phase 6: Code Review Fix Report

**Fixed at:** 2026-11-17
**Source review:** .planning/phases/06-hardening-and-cleanup/06-REVIEW.md
**Iteration:** 1

**Summary:**
- Findings in scope: 6 (WR-01 to WR-06; IN-01 to IN-03 are out of scope for `critical_warning`)
- Fixed: 6
- Skipped: 0

**Verification environment:** all edits, builds and tests ran in the main checkout (`/Users/christoph/Developer/scantailor-advanced`, branch `gsd/phase-01-deletion-free-macos-build`), no isolated worktree, with the `macos-release` preset. After the last commit the full `ctest --preset macos-release` passed (`100% tests passed out of 7`). The Debug gate is left to the orchestrator.

## Fixed Issues

### WR-01: The size limit also refuses the app's own output, automask and speckles files

**Files modified:** `src/core/ImageLoader.h`, `src/core/ImageLoader.cpp`, `src/core/TiffReader.h`, `src/core/TiffReader.cpp`, `src/core/LoadFileTask.cpp`, `src/core/ThumbnailPixmapCache.cpp`, `src/core/tests/TestImageSizeLimit.cpp`, `src/qt_tests/tst_core_qt.cpp`, `src/pipeline_tests/HeadlessPipeline.cpp`, `src/pipeline_tests/TestProjectRoundTrip.cpp`
**Commit:** 5e1fd627
**Applied fix:** `ImageLoader::load(...)` no longer applies `ImageSizeLimit`. The new `ImageLoader::loadSource(const ImageId&)` and `loadSource(QIODevice&, int)` do. Both go through one private `loadImpl(..., enforceSizeLimit)`. `TiffReader::readImage` got a third parameter `enforceSizeLimit` (default true, so a direct caller stays safe); the `width <= 0 || height <= 0` sanity check stays for both paths. The one-time Qt allocation limit (2048 MB) is still set on both paths.

Classification of every `ImageLoader` caller (grep over `src/`):

| Caller | Reads | Entry point |
|--------|-------|-------------|
| `src/core/LoadFileTask.cpp:61` | the user's source scan | `loadSource` (changed) |
| `src/core/ThumbnailPixmapCache.cpp:695` | the user's source scan, when no thumbnail file exists | `loadSource` (changed) |
| `src/core/ThumbnailPixmapCache.cpp:690` | the cached thumbnail PNG the app wrote | `load` (unchanged, now unlimited) |
| `src/core/filters/output/Task.cpp:261, 267, 271, 276, 286, 294` | cached output, foreground, background, original background, automask, speckles (all written by the app) | `load` (unchanged, now unlimited, as decided) |
| `src/pipeline_tests/HeadlessPipeline.cpp:197` | generated source page | `loadSource` (changed) |
| `src/pipeline_tests/HeadlessPipeline.cpp:160` | pipeline output file | `load` (unchanged) |
| `src/pipeline_tests/TestProjectRoundTrip.cpp:256` | generated source page | `loadSource` (changed) |
| `src/core/tests/TestImageSizeLimit.cpp` (`loadPage`) and `src/qt_tests/tst_core_qt.cpp` (`loadFrom`) | forged and boundary source files | `loadSource` (changed, they test the limit) |

New test `test_own_files_are_not_limited_but_source_files_are` (`CoreImageSizeLimitTestSuite`): a real 65536 x 1 bilevel TIFF (8 KB) and a real 65536 x 1 grey PNG are one pixel over the side limit. `load` returns the 65536 x 1 image, `loadSource` returns a null image. Nothing is allocated beyond the files themselves; a forged header is never read through the unlimited path, because that read would allocate the claimed size. With the TIFF limit mutated away the `loadSource` half of this test fails (see WR-05 mutation run).

Residuals, not changed: (1) The unlimited path still sits under Qt's allocation limit of 2048 MB for formats read by `QImageReader`, so an own PNG whose decoded size exceeds 2 GB is still refused; TIFF output goes through `TiffReader` and is unlimited. (2) `ThumbnailPixmapCache` would load an oversized source only through `loadSource`, so a forged source file is still refused there.

### WR-02: The memory cap outlives the batch and the project it was computed for

**Files modified:** `src/app/MainWindowBatch.cpp`, `src/app/MainWindowProject.cpp`
**Commit:** 2b398e9b
**Status:** fixed: requires human verification (no automated test)
**Applied fix:** `stopBatchProcessing()` sets `MemoryBudget::kNoCap` on `m_workerThreadPool` and `m_systemLoadWidget` after the batch queue is cleared. `switchToNewProject()` does the same right after its `stopBatchProcessing(CLEAR_MAIN_AREA)` call, so a project switch resets the cap even if the batch ended some other way. `stopBatchProcessing()` returns early when no batch is running, which is why the second reset is needed. No test: the members are private to `MainWindow` and a batch run needs a processing queue; the pool and widget behaviour with a cap is already pinned by `testWorkerPoolHonoursMemoryCap` and `testSystemLoadToolTipShowsMemoryLimit`. A person should check that the slider tooltip loses "limited to N by memory" after a batch finishes.

### WR-03: The cap assumes 600 dpi output, but the output dpi is a per-page setting

**Files modified:** `src/core/MemoryBudget.h`, `src/core/MemoryBudget.cpp`, `src/core/filters/output/Filter.h`, `src/core/filters/output/Filter.cpp`, `src/app/MainWindowBatch.cpp`, `src/core/tests/TestMemoryBudget.cpp`
**Commit:** ee684c7b
**Status:** fixed: requires human verification (the batch call site has no automated test)
**Applied fix:** `MemoryBudget::pagePixelEstimate(metadata, outputDpi = Dpi())`: a side of `outputDpi` that is not positive counts as `kAssumedOutputDpi` (600), so existing callers and the null default give the old numbers. `largestPagePixels` and `batchMemoryCap` take an optional `OutputDpiOf` callback (`std::function<Dpi(const PageInfo&)>`), which keeps `core` free of a dependency on the output filter. `output::Filter` got `Dpi outputDpi(const PageId&) const` (reads `Settings::getParams(pageId).outputDpi()`, which is 600 for a page without parameters). `startBatchProcessing()` passes a lambda that calls it; it runs after `loadDefaultSettings(page)` for every page, so the user's default profile counts. Only the thread cap changes; no output byte changes.
**Tests:** `test_page_pixel_estimate_at_the_output_dpi` (null and non-positive dpi fall back to 600, 1200 dpi gives 139197440 px for an A4 at 300 dpi, below the source dpi the source wins, the cap on 64 GiB is 24 at 600 and 6 at 1200 dpi) and `test_largest_page_pixels_uses_the_output_dpi_of_each_page` (callback per page, empty callback, null-returning callback, `batchMemoryCap` with a callback).

### WR-04: JPEG metadata load still runs `jpeg_start_decompress` for every in-limit file

**Files modified:** `src/core/JpegMetadataLoader.cpp`, `src/core/tests/TestImageSizeLimit.cpp`
**Commit:** 039f2701
**Applied fix:** `jpeg_start_decompress` runs only when `!cinfo->progressive_mode`; progressive files report the size and density that `jpeg_read_header` already filled. Consequence to know: the "unsupported compression type" check no longer fires at metadata time for a progressive file (such a file is reported with its size and fails later in the real load, as before for any other decode error).
**Test:** `test_progressive_jpeg_inside_the_limit_is_not_decompressed_for_metadata`: a progressive forgery of 65500 x 8191 px (536,500,000 px, inside the limit; 65500 is libjpeg's largest side, so the 65535 x 8191 the review suggested is refused by libjpeg itself) returns its size, and the sampled footprint growth stays below 64 MB and the time below 1 s. Measured before the fix: 1536 MB and 292 ms; after: 0 MB and 0 ms. With the fix reverted the test fails (growth 1.6 GB).

### WR-05: The peak-footprint assertions can pass vacuously

**Files modified:** `src/test_support/ProcessMemory.h`, `src/core/tests/TestImageSizeLimit.cpp`, `src/qt_tests/tst_core_qt.cpp`, `src/qt_tests/tst_ux_qt.cpp`
**Commit:** f8946bcf
**Applied fix:** new `test_support::FootprintSampler` (header-only, in `ProcessMemory.h`): a sampler thread reads the current `phys_footprint` of the whole process every millisecond, plus one sample when it stops with the call's result still alive, and `stop()` returns the largest sample above the footprint at construction. All refusal checks now use it instead of two readings of the process high-water mark: `checkRefusedWithoutAllocating` and the progressive JPEG forgery in `TestImageSizeLimit.cpp`, `verifyRefusedWithoutAllocating` in `tst_core_qt.cpp`, and `requestThumbnail` in `tst_ux_qt.cpp` (the thumbnail thread is covered because the footprint is process-wide). The time bound of 1 s stays as the second independent check. A reading that fails (`valid()` false) fails the test instead of passing. The remark that the refusal cases must run first was removed, since the order no longer matters.
**Mutation proof (uncommitted, reverted):**
- Mutation A, `TiffReader.cpp`: `enforceSizeLimit && ImageSizeLimit::exceeds(...)` became `false && ...`. `core_tests` then failed 9 checks, among them `test_forged_mono_tiff_is_refused_without_allocating` (`growth < kFootprintBoundBytes` failed, 1250657168 >= 67108864) and `test_forged_wide_tiff_is_refused_without_allocating` (500269416 >= 67108864), plus the boundary and own-file tests. `qt_tests` then failed `TestUxQt::testOversizedThumbnailFails` (status LOADED instead of LOAD_FAILED) and crashed in `testOversizedImageShowsTooLargeInPageView`.
- Mutation B, `JpegMetadataLoader.cpp` reverted to the old code: only `test_progressive_jpeg_inside_the_limit_is_not_decompressed_for_metadata` failed, and only on the footprint check (growth about 1.6 GB, the reported size was still correct), so the sampler itself discriminates.
- Observation: `test_forged_rgb_tiff_is_refused_without_allocating` and `test_rgb_tiff_at_the_ceiling_is_refused_without_allocating` still passed under mutation A. Without the limit, libtiff fails on the truncated strip before the lazily allocated RGB buffer is touched, so these two cases do not discriminate the limit; they were not changed (they still document the refusal and cost nothing).

### WR-06: Cancelling or failing "Save" while closing a project still closes it (pre-existing, moved)

**Files modified:** `src/app/MainWindow.h`, `src/app/MainWindowProject.cpp`, `src/qt_tests/tst_mac_qt.cpp`, `src/qt_tests/tst_mac_qt.h`
**Commit:** af81849b
**Applied fix:** `saveProjectTriggered()` and `saveProjectAsTriggered()` return `bool`, true only when the file was written. In `closeProjectInteractive()` both `case SAVE` blocks (project without a file, and the backup-write-failed branch) now `return false` when the save did not happen, so the project stays open. Callers: the only two callers are the `actionSaveProject` and `actionSaveProjectAs` connections, made with `SIGNAL`/`SLOT` strings, which ignore a slot's return value, so their behaviour is unchanged; the rest of the code does not call either function.
**Test:** `TestMacQt::testFailedSaveWhileClosingKeepsTheProject`. It writes a project file with `SaveFixture`, opens it in a `MainWindow` (`openProject` is synchronous and offscreen-safe), makes the folder and file read-only (so the backup write fails, which is the second branch), triggers `actionCloseProject` with a message-box answerer that says Yes, and checks that two boxes appeared (the question and the save error) and that the window title still names the project. As a control, with the permissions back the same action closes the project (title "Unnamed"). With the fix reverted the test fails at the first title check. The first branch (a project without a file, cancelled Save As) shares the code but is not reachable offscreen, because such a project only comes from the New Project wizard.

## Skipped Issues

None.

---

_Fixed: 2026-11-17_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
