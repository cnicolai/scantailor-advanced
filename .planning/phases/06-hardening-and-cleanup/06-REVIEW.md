---
phase: 06-hardening-and-cleanup
reviewed: 2026-11-17T00:00:00Z
depth: standard
files_reviewed: 97
files_reviewed_list:
  - CMakeLists.txt
  - src/app/CMakeLists.txt
  - src/app/FixDpiDialog.cpp
  - src/app/MainWindow.cpp
  - src/app/MainWindow.h
  - src/app/MainWindowActions.cpp
  - src/app/MainWindowBatch.cpp
  - src/app/MainWindowPages.cpp
  - src/app/MainWindowProject.cpp
  - src/app/OutOfMemoryDialog.cpp
  - src/app/OutOfMemoryDialog.h
  - src/app/ProjectFilesDialog.cpp
  - src/app/ProjectSaveFlow.cpp
  - src/app/ProjectSaveFlow.h
  - src/app/RelinkingModel.cpp
  - src/app/SettingsDialog.cpp
  - src/app/SettingsDialog.ui
  - src/app/SystemLoadWidget.cpp
  - src/app/SystemLoadWidget.h
  - src/app/ThumbnailSequence.cpp
  - src/core/ApplicationSettings.cpp
  - src/core/ApplicationSettings.h
  - src/core/CMakeLists.txt
  - src/core/FileNameDisambiguator.cpp
  - src/core/ImageLoader.cpp
  - src/core/ImageSizeLimit.cpp
  - src/core/ImageSizeLimit.h
  - src/core/ImageViewBase.cpp
  - src/core/ImageViewBase.h
  - src/core/IncompleteThumbnail.cpp
  - src/core/JpegMetadataLoader.cpp
  - src/core/LoadFileTask.cpp
  - src/core/MemoryBudget.cpp
  - src/core/MemoryBudget.h
  - src/core/PngMetadataLoader.cpp
  - src/core/ProjectPages.cpp
  - src/core/ProjectReader.cpp
  - src/core/ProjectWriter.cpp
  - src/core/ThumbnailPixmapCache.cpp
  - src/core/TiffReader.cpp
  - src/core/WorkerThreadPool.cpp
  - src/core/WorkerThreadPool.h
  - src/core/filters/output/CMakeLists.txt
  - src/core/filters/output/ColorPickupInteraction.cpp
  - src/core/filters/output/DewarpingView.cpp
  - src/core/filters/output/OutputGenerator.cpp
  - src/core/filters/output/OutputGeneratorBinarize.cpp
  - src/core/filters/output/OutputGeneratorDewarp.cpp
  - src/core/filters/output/OutputGeneratorNoDewarp.cpp
  - src/core/filters/output/OutputGeneratorPixelOps.h
  - src/core/filters/output/OutputGeneratorProcessor.h
  - src/core/filters/page_layout/Settings.cpp
  - src/core/filters/page_split/ImageView.cpp
  - src/core/filters/page_split/PageLayoutEstimator.cpp
  - src/core/filters/page_split/VertLineFinder.cpp
  - src/core/tests/CMakeLists.txt
  - src/core/tests/TestImageSizeLimit.cpp
  - src/core/tests/TestMemoryBudget.cpp
  - src/core/zones/EditableZoneSet.h
  - src/core/zones/SerializableSpline.cpp
  - src/dewarping/Curve.cpp
  - src/dewarping/CylindricalSurfaceDewarper.cpp
  - src/dewarping/DetectVertContentBounds.cpp
  - src/dewarping/DistortionModelBuilder.cpp
  - src/dewarping/TextLineRefiner.cpp
  - src/dewarping/TextLineTracer.cpp
  - src/dewarping/TopBottomEdgeTracer.cpp
  - src/dewarping/TowardsLineTracer.cpp
  - src/foundation/FastQueue.h
  - src/foundation/XmlMarshaller.cpp
  - src/foundation/XmlMarshaller.h
  - src/foundation/XmlUnmarshaller.cpp
  - src/foundation/XmlUnmarshaller.h
  - src/foundation/tests/CMakeLists.txt
  - src/foundation/tests/TestXmlEscape.cpp
  - src/imageproc/PolygonRasterizer.cpp
  - src/imageproc/RastLineFinder.cpp
  - src/imageproc/SavGolFilter.cpp
  - src/imageproc/tests/TestRastLineFinder.cpp
  - src/math/XSpline.cpp
  - src/math/spfit/ConstraintSet.cpp
  - src/math/spfit/Optimizer.cpp
  - src/math/spfit/SplineFitter.cpp
  - src/pipeline_tests/TestOutputDigestProbe.cpp
  - src/pipeline_tests/TestPeakMemoryProbe.cpp
  - src/pipeline_tests/TestProjectRoundTrip.cpp
  - src/qt_tests/CMakeLists.txt
  - src/qt_tests/tst_core_qt.cpp
  - src/qt_tests/tst_core_qt.h
  - src/qt_tests/tst_mac_qt.cpp
  - src/qt_tests/tst_mac_qt.h
  - src/qt_tests/tst_ux_qt.cpp
  - src/qt_tests/tst_ux_qt.h
  - src/test_support/CMakeLists.txt
  - src/test_support/ForgedImages.cpp
  - src/test_support/ForgedImages.h
  - src/test_support/ProcessMemory.h
findings:
  critical: 0
  warning: 6
  info: 3
  total: 9
  necessity: 0
status: issues_found
---

# Phase 6: Code Review Report

**Reviewed:** 2026-11-17
**Depth:** standard
**Files Reviewed:** 97
**Status:** issues_found

## Summary

I read the new SAFE-01/02/03 code (ImageSizeLimit, the TIFF/PNG/JPEG/ImageLoader changes, MemoryBudget, WorkerThreadPool, SystemLoadWidget, XmlMarshaller/Unmarshaller, ProjectWriter/Reader, ProjectSaveFlow), the STRIP-07 OpenGL removal, the move boundaries of the two splits, and the diffs of the CLEAN-03/04 files against `e26b3da9`. I did not build or run anything. `xcrun clang-format --dry-run` over the in-scope C++ files reports only the two known `TextLineTracer.cpp` violations.

No blocker was found. The size-limit arithmetic is sound: `4 * (2^29 - 1)` fits an int, negative header sides count as too large, and the TIFF check runs before the first pixel buffer. The XML escape round-trips (percent-encoding of UTF-8 under a flag attribute, ordinary values unchanged). The split headers carry declarations only and the anonymous-namespace helpers live in a header that no inline code of another header uses, so I found no ODR problem. The OpenGL removal is complete (no residual reference in `src`, CMake or the `.ui`). The deleted `#if 0` blocks and `boost/foreach` includes are behaviour-neutral.

The six warnings are all about the edges of the new limits and caps: the limit also applies to files the app wrote itself, the memory cap lives longer than the batch it was computed for, and the cap's cost model ignores the one setting that drives output size. One pre-existing data-loss path (WR-06) is listed because the CLEAN-05 refactor touched the same flow.

## Warnings

### WR-01: The size limit also refuses the app's own output, automask and speckles files

**File:** `src/core/ImageLoader.cpp:58-62` (caller: `src/core/filters/output/Task.cpp:261-294`)
**Issue:** The `ImageSizeLimit::exceeds()` check sits in `ImageLoader::load(QIODevice&, int)`, which every caller uses, not only the one that loads a user's source image. `output::Task` reloads the cached output TIFF, the foreground/background layers, the automask and the speckles file through the same function. The limit is applied to source images only on input; output generation has no limit. A source inside the limit can produce an output above it, for example an A3 scan of 14031 x 19843 px (278 MP) at output DPI 2400 gives about 1.1 GP. The writer writes it, the reload then returns a null image, `needReprocess = outImg.isNull()` becomes true, and the page is regenerated on every visit and every batch run, with the cache never hitting. The same happens to the automask and speckles files, which have the output's size. No error is shown.
**Fix:** Limit the loader only where the input is untrusted. Either give `ImageLoader::load` a flag (or add `loadSource(const ImageId&)`) that applies `ImageSizeLimit`, and have `LoadFileTask` and the thumbnail cache use it, or refuse oversized output at generation time with a visible error. Add a test that writes and reloads an output file above 2^29 px, or one that pins the chosen behaviour.

### WR-02: The memory cap outlives the batch and the project it was computed for

**File:** `src/app/MainWindowBatch.cpp:145-149`, `src/core/WorkerThreadPool.cpp:81-85`
**Issue:** `setMemoryCap()` is called only in `startBatchProcessing()` (the one call site in `src/`). Nothing resets it to `MemoryBudget::kNoCap` when the batch stops, when the project is closed or when another project is opened. After one batch with a very large page (cap 1), the shared pool stays at one thread for all later interactive work and for any later batch until the next `startBatchProcessing()` recomputes it. The slider tooltip keeps "limited to 1 by memory" while a different project is open. The effect is lost parallelism rather than a crash.
**Fix:** In `stopBatchProcessing()` (and in `switchToNewProject()`), call `m_workerThreadPool->setMemoryCap(MemoryBudget::kNoCap)` and `m_systemLoadWidget->setMemoryCap(MemoryBudget::kNoCap)`.

### WR-03: The cap assumes 600 dpi output, but the output dpi is a per-page setting

**File:** `src/core/MemoryBudget.cpp:33-40`, `src/core/MemoryBudget.h:26-27`
**Issue:** `pagePixelEstimate()` derives the output size from the fixed `kAssumedOutputDpi = 600` and the source dpi. The output stage stores a per-page output dpi (`output::Settings::setDpi`, used by `TestPeakMemoryProbe.cpp` to run at the source dpi), and the batch applies the user's default parameters to every page before the cap is computed. A user who sets 1200 or 2400 dpi output on a 300 dpi A4 scan gets an output 4 to 8 times larger than the estimate, so `threadsForMemory()` allows several times too many pages in flight. The 33.1 B/px figure and the D-08 run were measured at output dpi equal to source dpi, so the cap protects exactly that case.
**Fix:** In `startBatchProcessing()`, read each page's output dpi from `m_stages->outputFilter()` after `loadDefaultSettings(page)` and pass it into the estimate (for example an optional `outputDpi` argument), keeping 600 only as the fallback. If this is deliberately out of scope, document the assumption in the tooltip or in `records/` so a user with high output dpi knows the cap does not cover them.

### WR-04: JPEG metadata load still runs `jpeg_start_decompress` for every in-limit file

**File:** `src/core/JpegMetadataLoader.cpp:214-225`
**Issue:** The change moves `jpeg_start_decompress()` behind the limit check, so a forged header is no longer expanded. A progressive JPEG just inside the limit (for example 65535 x 8191, about 2^29 px, a few KB on disk) still goes through `jpeg_start_decompress`, which allocates the whole coefficient image. The test comment gives 12.3 GB for 65500 x 65500, which scales to roughly 1.5 GB for 2^29 px. libjpeg-turbo's default memory manager keeps these arrays in RAM. The project file dialog and the insert flow load metadata for every selected file, so a folder with several such files costs the allocation each time. Metadata (size and dpi) does not need decompression: `jpeg_read_header` has already filled `image_width`, `image_height` and the density fields. This is a judgment call on how far SAFE-01 was meant to go, but the stated goal is bounded allocation from a header.
**Fix:** Drop the `jpeg_start_decompress` call from `loadMetadata`, or run it only for baseline files (`!cinfo->progressive_mode`), where it allocates nothing proportional to the image. If it is kept for the "unsupported compression type" check, bound it, for example skip it for progressive files above 64 MP.

### WR-05: The peak-footprint assertions can pass vacuously

**File:** `src/core/tests/TestImageSizeLimit.cpp:62-77, 128-147`, `src/qt_tests/tst_ux_qt.cpp` (`requestThumbnail`, `testOversizedThumbnailFails`)
**Issue:** The refusal tests assert `peakAfter - peakBefore < 64 MB`, where both values come from `ledger_phys_footprint_peak`, a process-wide high-water mark that never falls. If anything earlier in the same process (other suites of `core_tests` or `qt_tests`, an earlier test case) pushed the peak above what a regressed refusal would allocate, the difference is 0 and the test passes while the code allocates freely. The comment on the suite orders the refusal cases first, but that ordering holds only inside this suite, not across the binary, and after the first test case in each binary every later refusal case is only checked against the higher peak left by earlier ones. The elapsed-time bound (1 s) is the only independent check.
**Fix:** Measure the current footprint (`physFootprintBytes()`, not the peak) with a sampler thread during the call, or run each refusal case in a fresh process (a `--run_test` per case, or a small helper executable). At minimum, assert that the peak before the call is below a stated ceiling so a vacuous pass fails loudly: `BOOST_REQUIRE_LT(peakBefore, kFootprintBoundBytes + baseline)`.

### WR-06: Cancelling or failing "Save" while closing a project still closes it (pre-existing, moved)

**File:** `src/app/MainWindowProject.cpp:263-275, 318-323` (`closeProjectInteractive`)
**Issue:** Marked pre-existing: identical in `e26b3da9:src/app/MainWindow.cpp:1813-1825`. For a project without a file, `case SAVE: saveProjectTriggered();` falls through to `closeProjectWithoutSaving()` whatever the outcome. If the user answers "Yes", then cancels the Save As panel, or the write fails (the error box was shown), the project is discarded and all page settings are lost. CLEAN-05 made `ProjectSaveFlow::saveAs` return the saved path or an empty string, so the information needed to fix this exists now. The same pattern appears in the backup-write-failed branch.
**Fix:** Make `saveProjectTriggered()` return `bool` (true when the file was written) and use it:
```cpp
case SAVE:
  if (!saveProjectTriggered()) {
    return false;  // stay in the project
  }
  break;
```

## Info

### IN-01: Every split translation unit repeats the full include list of the original file

**File:** `src/app/MainWindowActions.cpp:1-90`, `MainWindowBatch.cpp`, `MainWindowPages.cpp`, `MainWindowProject.cpp`; `src/core/filters/output/OutputGeneratorProcessor.h:1-80`, `OutputGeneratorPixelOps.h:1-80`, `OutputGeneratorBinarize.cpp`, `OutputGeneratorDewarp.cpp`, `OutputGeneratorNoDewarp.cpp`, `OutputGenerator.cpp`
**Issue:** The four MainWindow files carry the same 83 includes (for example `ImageSizeLimit.h` and `MemoryBudget.h` are used in one file each; `ui_AboutDialog.h`, `QResource` and the filter `Task.h` headers appear in files that never use them). The output split repeats about 70 includes and the two `using namespace` lines in the private header `OutputGeneratorProcessor.h`, again in `OutputGeneratorPixelOps.h`, and again in each `.cpp`. This is harmless for correctness (the move is verbatim) but it hides real dependencies, slows each of the eight translation units and makes later changes to the headers noisy.
**Fix:** Trim each file to the headers it uses. For the output split, keep the heavy imageproc includes in the `.cpp` files that need them and let `OutputGeneratorProcessor.h` hold only what its declarations need.

### IN-02: A 16-bit PNG inside the limit may be refused with the generic "could not be loaded" message

**File:** `src/core/ImageSizeLimit.h:34-35`, `src/core/ImageLoader.cpp:38`
**Issue:** The Qt allocation limit is set to 2048 MB, sized for 4 bytes per pixel (the static_assert says so). Qt decodes 16-bit RGB PNGs to a 64-bit-per-pixel format, so a 16-bit RGB PNG above about 268 MP is inside `ImageSizeLimit` but above Qt's allocation limit; the reader returns null, and `LoadFileTask` reports "could not be loaded", because `claimedSizeIfTooLarge()` says false. I read this from the Qt behaviour I know and did not run it, so treat it as likely rather than shown. It affects only very large 16-bit scans.
**Fix:** Either accept it and document it in `ImageSizeLimit.h`, or raise `kQtAllocationLimitMb` to 4096 and keep the claimed-size check as the real guard.

### IN-03: Comments and test names say "W x H" where the code prints "W × H"

**File:** `src/core/ImageSizeLimit.h:57`, `src/app/ProjectFilesDialog.cpp:35`
**Issue:** The doc comments describe `"too large: W x H px"`, while `sizeText()` produces `"W × H px"` (U+00D7), as `TestImageSizeLimit::test_texts` confirms.
**Fix:** Change the comments to use the multiplication sign.

---

_Reviewed: 2026-11-17_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
