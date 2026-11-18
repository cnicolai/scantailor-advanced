---
phase: 06-hardening-and-cleanup
reviewed: 2026-11-17T00:00:00Z
depth: standard
files_reviewed: 23
files_reviewed_list:
  - src/app/MainWindow.h
  - src/app/MainWindowBatch.cpp
  - src/app/MainWindowProject.cpp
  - src/core/ImageLoader.cpp
  - src/core/ImageLoader.h
  - src/core/JpegMetadataLoader.cpp
  - src/core/LoadFileTask.cpp
  - src/core/MemoryBudget.cpp
  - src/core/MemoryBudget.h
  - src/core/ThumbnailPixmapCache.cpp
  - src/core/TiffReader.cpp
  - src/core/TiffReader.h
  - src/core/filters/output/Filter.cpp
  - src/core/filters/output/Filter.h
  - src/core/tests/TestImageSizeLimit.cpp
  - src/core/tests/TestMemoryBudget.cpp
  - src/pipeline_tests/HeadlessPipeline.cpp
  - src/pipeline_tests/TestProjectRoundTrip.cpp
  - src/qt_tests/tst_core_qt.cpp
  - src/qt_tests/tst_mac_qt.cpp
  - src/qt_tests/tst_mac_qt.h
  - src/qt_tests/tst_ux_qt.cpp
  - src/test_support/ProcessMemory.h
findings:
  critical: 0
  warning: 1
  info: 3
  total: 4
  necessity: 0
status: issues_found
---

# Phase 6: Code Review Report (incremental re-review)

**Reviewed:** 2026-11-17
**Depth:** standard
**Files Reviewed:** 23
**Status:** issues_found

## Summary

Incremental review of `git diff dc7d0685..HEAD` for the six fix commits 5e1fd627..af81849b, with the surrounding code read where the diff depends on it. All 23 files pass `xcrun clang-format --dry-run -Werror`. Necessity was assessed with `necessity-review.md`; no necessity finding was raised.

What holds up:

- **WR-01 (limit for source scans only).** Every `ImageLoader::load*` caller outside the tests is classified correctly. The three source readers (`LoadFileTask`, `ThumbnailPixmapCache` source branch, test pipelines) use `loadSource`. The six `output/Task.cpp` reads and the thumbnail PNG read use `load`.
- **WR-02 (cap reset).** `stopBatchProcessing()` resets the cap after `m_batchQueue.reset()`. `switchToNewProject()` resets it again for the early-return case. The `else { stopBatchProcessing(); }` branch of `startBatchProcessing()` is also covered, because the queue is non-null there.
- **WR-03 (output dpi).** `output::Filter::outputDpi()` reads through `Settings::getParams`, which takes the mutex. The callback runs after `loadDefaultSettings()` for every page. `StageSequence::outputFilter()` is always non-null.
- **WR-05 (`FootprintSampler`).** Member initialisation order is correct: `m_stop` is initialised before `m_thread` starts. `stop()` is idempotent. A failed `QVERIFY` or `BOOST_REQUIRE` while the sampler is alive joins the thread in the destructor.
- **WR-06 (Save while closing).** `return false` on a failed or cancelled save is correct in both `case SAVE` blocks. The third branch (backup rename) already returned false on failure. The bool-returning slots are still connected through `SIGNAL`/`SLOT` strings, which ignore the return value.

One real gap remains in the WR-04 fix, which I reproduced. The other three items are Info.

## Warnings

### WR-07: WR-04 fix still allocates the full coefficient image for non-interleaved baseline JPEGs

**File:** `src/core/JpegMetadataLoader.cpp:221-228`
**Issue:** The fix skips `jpeg_start_decompress` only when `cinfo->progressive_mode` is set. libjpeg also allocates the whole coefficient image for any file with more than one scan. A baseline file whose scans each carry one component (`comps_in_scan < num_components`) has `has_multiple_scans` set, so `jinit_d_coef_controller` creates the full-image virtual coefficient arrays. A few-hundred-byte forgery of such a file, with a 65500 x 8191 SOF0 (inside `ImageSizeLimit`, 536,500,000 px), therefore costs the same 1.5 GB as the progressive case WR-04 was meant to close. This path runs once per selected file at project creation.

Reproduced with a probe in `build/scratch/jpegprobe/probe.cpp`: a 16x16 baseline JPEG encoded with three one-component scans and its SOF0 patched to 65500 x 8191.

```
progressive=0 multiscan=1 size=65500x8191
footprint growth MB: 1536
```

`test_progressive_jpeg_inside_the_limit_is_not_decompressed_for_metadata` only forges the progressive variant, so it passes while this case stays open. The comment in the code ("only a baseline file goes through it") also states something that is not true for multi-scan baseline files.

**Fix:** Test for multiple scans, which covers both cases. `jpeg_has_multiple_scans()` is public libjpeg API and is valid after `jpeg_read_header`:
```cpp
const bool multiScan = jpeg_has_multiple_scans(cinfo.ptr());  // progressive, or one component per scan
if (!multiScan && !jpeg_start_decompress(cinfo.ptr())) {
  return GENERIC_ERROR;
}
```
Add a `forgedJpeg` variant with non-interleaved baseline scans (65500 x 8191) next to the progressive test, with the same footprint and time bounds. Update the comment to say "multi-scan" instead of "progressive".

## Info

### IN-04: `ImageLoader::load(const ImageId&)` is now unused and is an unlimited reader of source files

**File:** `src/core/ImageLoader.cpp:20-22`, `src/core/ImageLoader.h:25`
**Issue:** After WR-01 no code calls `load(const ImageId&)`. A grep of `ImageLoader::load(` shows only `QString`/`QIODevice` call sites. The overload takes the same argument as `loadSource(const ImageId&)` but applies no size limit. The next person who needs to load an `ImageId` has a one-word choice between safe and unsafe, and `load` is the obvious name. The split makes the unlimited variant the one with the plain name.
**Fix:** Delete the `load(const ImageId&)` overload. Optionally rename the unlimited variants (`loadOwnFile`) so the name carries the trust decision, which is where a mistake would otherwise sit unnoticed.

### IN-05: Unlimited read of the output-folder files accepts a forged file as an own file

**File:** `src/core/filters/output/Task.cpp:261-294` (callers), `src/core/ImageLoader.cpp:32-34`
**Issue:** The output cache, the foreground and background layers, the automask and the speckles files are read with the limit off, on the premise that the application wrote them. The files sit in the output folder, whose location comes from the project file. A project received from someone else, or a shared output folder, can contain a TIFF with a forged header at the expected file name. If its parameters match, `needReprocess` is false and `TiffReader` is asked for the claimed size. Before WR-01 the limit refused this. Now the read ends in `std::bad_alloc` and the out-of-memory dialog, or in a large allocation, instead of a null image. The fix report records the unlimited read as an accepted decision for a one-user fork. This entry only records that it reopens the forged-header case for the output path.
**Fix:** If it is worth closing, give the own-file path a higher ceiling than the source limit instead of none (for example, the budget that `MemoryBudget` already computes from physical memory), so that a legitimate 1200 dpi A3 output passes and a header claiming 100000 x 100000 does not. Otherwise leave it and keep the note in the fix report.

### IN-06: `startBatchProcessing()` dereferences the queue it just reset when the sequence is empty

**File:** `src/app/MainWindowBatch.cpp:166-176`
**Issue:** If `takeForProcessing()` returns null, the `else` branch calls `stopBatchProcessing()`, which sets `m_batchQueue.reset()`. The next statement is `m_batchQueue->selectedPage()`, a null dereference. The code predates this diff, but this phase's change makes `stopBatchProcessing()` the place that does the cleanup, so it is the function a reader checks for the state afterwards. It is reachable only when a loaded project has no pages, so the risk is low.
**Fix:** Return after the stop:
```cpp
} else {
  stopBatchProcessing();
  return;
}
```

---

_Reviewed: 2026-11-17_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
