---
phase: 06-hardening-and-cleanup
plan: 05
subsystem: image-loading
tags: [cpp, png, jpeg, safety, image-size-limit, qt-reader-limit, boost-test, qt-test, gate]

requires:
  - phase: 06-hardening-and-cleanup
    provides: "ImageSizeLimit, ForgedImages (forgedTiff), ProcessMemory.h and the page-view test of plan 06-04"
provides:
  - "ImageLoader refuses a claimed size over ImageSizeLimit before QImageReader::read, and raises Qt's reader limit once to 2048 MB (std::call_once, never 0)"
  - "JpegMetadataLoader reports the claimed size of an oversized file before jpeg_start_decompress"
  - "PngMetadataLoader reports the claimed size of any PNG (libpng's per-side user limit lifted), so PNG, JPEG and TIFF give one verdict"
  - "test_support: forgedPng and forgedJpeg (baseline or progressive)"
  - "Three JPEG metadata cases in CoreImageSizeLimitTestSuite, four TestCoreQt slots, three page-view rows (png, png-wide, jpeg)"
affects: [06-06]

actuals:
  tokens: 5000
  tasks: 1
  commits: 2

plan_head_before: 5c0a834f613d8f1b2c4f36295fd4d14e9a57a67a
plan_head_after: d36da206b78e5b59dea0216b8226aaf51c349f72

tech-stack:
  added: []
  patterns:
    - "One size rule for every format: the claimed size is checked with ImageSizeLimit::exceeds before any pixel buffer exists"
    - "Qt reader limit set once through std::call_once inside ImageLoader::load, so the thumbnail thread and the worker threads share one limit"
    - "Header-only metadata loaders report the claimed size and leave the verdict to the one consumer check (claimedSizeIfTooLarge)"

key-files:
  created: []
  modified:
    - src/core/ImageLoader.cpp
    - src/core/JpegMetadataLoader.cpp
    - src/core/PngMetadataLoader.cpp
    - src/test_support/ForgedImages.h
    - src/test_support/ForgedImages.cpp
    - src/test_support/CMakeLists.txt
    - src/core/tests/TestImageSizeLimit.cpp
    - src/qt_tests/tst_core_qt.h
    - src/qt_tests/tst_core_qt.cpp
    - src/qt_tests/tst_ux_qt.cpp
    - .planning/phases/06-hardening-and-cleanup/records/gate-log.md

key-decisions:
  - "The ImageLoader size check is pinned by a real 65536 x 1 grey PNG (one pixel over the side limit, 64 KB), not by the forged 100000 x 100000 PNG: Qt's own raised limit (2048 MB) already refuses that one, so it cannot tell whether the check exists"
  - "PngMetadataLoader gets no early return of its own; it lifts libpng's user limit (png_set_user_limits to PNG_UINT_31_MAX) before png_read_info and reports LOADED with the claimed size, like the TIFF loader"
  - "The JPEG metadata size check sits between jpeg_read_header and jpeg_start_decompress; size and dpi are computed right after the header assert"

requirements-completed: []

coverage:
  - id: D1
    description: "A forged 100000 x 100000 PNG, a forged 2000000 x 1 PNG and a forged progressive 65500 x 65500 JPEG, opened as pages, show 'too large to load' with the claimed size; a TIFF and an unreadable file behave as before"
    requirement: SAFE-01
    verification:
      - kind: test
        ref: "src/qt_tests/tst_ux_qt.cpp#TestUxQt::testOversizedImageShowsTooLargeInPageView (rows png, png-wide, jpeg, tiff, garbage)"
        status: pass
    human_judgment: false
  - id: D2
    description: "ImageLoader::load of a forged 100000 x 100000 PNG and a forged progressive 65500 x 65500 JPEG returns null in under 1000 ms with under 64 MiB peak growth"
    requirement: SAFE-01
    verification:
      - kind: test
        ref: "src/qt_tests/tst_core_qt.cpp#TestCoreQt::testForgedPngIsRefused, testForgedJpegIsRefusedAndLargeJpegLoads"
        status: pass
    human_judgment: false
  - id: D3
    description: "A 9000 x 8000 grey PNG and a 9000 x 8000 grey JPEG (72 MP) load; the loader leaves Qt's limit at 2048; the control read under 256 fails; 65535 x 1 loads while 65536 x 1 and 1 x 65536 are refused"
    requirement: SAFE-01
    verification:
      - kind: test
        ref: "src/qt_tests/tst_core_qt.cpp#TestCoreQt::testLargeGreyPngLoads, testForgedJpegIsRefusedAndLargeJpegLoads"
        status: pass
    human_judgment: false
  - id: D4
    description: "JpegMetadataLoader reports the claimed 65500 x 65500 of a forged progressive JPEG in under 1000 ms with under 64 MiB peak growth; a baseline forgery reports the claimed size; a 64 x 64 progressive JPEG reports 64 x 64"
    requirement: SAFE-01
    verification:
      - kind: test
        ref: "src/core/tests/TestImageSizeLimit.cpp#test_forged_progressive_jpeg_metadata_is_fast_and_small, test_forged_baseline_jpeg_metadata_reports_claimed_size, test_small_progressive_jpeg_metadata_is_unchanged"
        status: pass
    human_judgment: false
  - id: D5
    description: "PngMetadataLoader reports LOADED with the claimed size for forgedPng(100000, 100000) and forgedPng(2000000, 1)"
    requirement: SAFE-01
    verification:
      - kind: test
        ref: "src/qt_tests/tst_core_qt.cpp#TestCoreQt::testForgedPngMetadataReportsClaimedSize"
        status: pass
    human_judgment: false
  - id: D6
    description: "The commit passes the gate: 100% tests passed out of 7 in Release and Debug, goldens unchanged"
    requirement: SAFE-01
    verification:
      - kind: other
        ref: "gate.sh check s05-qt-formats (tested commit 1fb688fc) -> result: PASS"
        status: pass
    human_judgment: false

duration: 25min
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 05: SAFE-01, PNG and JPEG follow the one limit Summary

**PNG and JPEG now obey the same size rule and show the same "too large" text as TIFF: `ImageLoader` checks the claimed size before `QImageReader::read` and raises Qt's reader limit once to 2048 MB (never 0), `JpegMetadataLoader` stops before decompressing an oversized progressive file (a 1 KB forgery no longer costs 12.3 GB), and `PngMetadataLoader` reports the claimed size of any PNG instead of failing above 1000000 px per side.**

## Performance

- **Duration:** about 25 min (one gate run of about 7 min, four mutation builds)
- **Completed:** 2026-11-17
- **Tasks:** 1 (tracer)
- **Files:** 10 under `src/` modified (328 lines added, 6 removed), no file created

## Accomplishments

- `ImageLoader::load`: `std::call_once` sets `QImageReader::setAllocationLimit(ImageSizeLimit::kQtAllocationLimitMb)` once for all threads; after the reader is set up, a valid `reader.size()` that `ImageSizeLimit::exceeds` returns a null image. An invalid size (format cannot tell) falls through to `read` as before.
- `JpegMetadataLoader::loadMetadata`: size and dpi are taken right after the header assert; an oversized file is reported as `LOADED` with its claimed size without calling `jpeg_start_decompress`. Files inside the limit take the old path with the old `GENERIC_ERROR` branch.
- `PngMetadataLoader::loadMetadata`: `png_set_user_limits(png.handle(), PNG_UINT_31_MAX, PNG_UINT_31_MAX)` before `png_read_info`. Nothing else in the loader changed.
- `test_support`: `forgedPng(width, height)` (1 x 1 PNG with the IHDR size patched and its CRC recomputed with zlib) and `forgedJpeg(width, height, progressive)` (libjpeg into memory, SOF0 or SOF2 patched by walking the marker segments). `ZLIB::ZLIB` added to the private link libraries.
- Tests: three JPEG metadata cases in `CoreImageSizeLimitTestSuite` (13 cases now), four `TestCoreQt` slots, rows `png`, `png-wide` and `jpeg` in the page-view test. `qt_tests`: 28 passed, 0 failed in both result blocks; `core_tests` suite: no errors.
- Gate `s05-qt-formats` (tested commit `1fb688fc`): Release and Debug `100% tests passed out of 7`, goldens unchanged. The output-path guard of convention 13 prints nothing; every touched C++ file passes `clang-format --dry-run -Werror`.

## Task Commits

1. **Task 1 (tracer): one rule for PNG and JPEG** - `1fb688fc` (fix), gated as `s05-qt-formats`; gate record `d36da206`

## Observe first: the unmodified PNG metadata loader

With the tests in place and `PngMetadataLoader` unchanged (JPEG and `ImageLoader` already patched), `forgedPng(2000000, 1)` gave `ImageMetadataLoader::GENERIC_ERROR` (status 3) in `testForgedPngMetadataReportsClaimedSize`, and the `png-wide` page-view row showed "The following file could not be loaded" instead of the too-large text. `forgedPng(100000, 100000)` already reported LOADED (below libpng's side limit) and its `png` row passed. With `png_set_user_limits` in place both report LOADED with the claimed size and the `png-wide` row shows `2000000 × 1 px`.

## Mutation checks (not committed)

- **ImageLoader size check disabled** (`if (false && ...)`): the plan's forged 100000 x 100000 PNG test and the page-view rows still passed, because Qt's raised limit (2048 MB) refuses that allocation itself. So the plan's test could not tell whether the check exists. I added a real file case to `testLargeGreyPngLoads` (65535 x 1 loads, 65536 x 1 and 1 x 65536 grey PNGs, which Qt would load, are refused); with the check disabled that slot fails (`QCOMPARE(!result.isNull(), !exceeds)` at the 65536 case), with it enabled it passes. I also tried a forged 40000 x 20000 PNG (800 Mpx, 800 MB for Qt): it did not discriminate either (Qt returns null quickly and the untouched allocation does not grow the footprint), so it was not kept.
- **Qt limit not raised** (`setAllocationLimit` replaced by a no-op): `testLargeGreyPngLoads` and `testForgedJpegIsRefusedAndLargeJpegLoads` fail on `!loaded.isNull()` for the 72 MP images. The control read under 256 passes (fails to read) in the unmodified code.
- **JPEG metadata check disabled** (`if (false && ...)`): the forged JPEG was reduced to 12000 x 12000 progressive for this run only, because the real 65500 x 65500 forgery costs 12.3 GB on an unpatched path. The footprint check failed with `[432406888 >= 67108864]` (432 MB), proving the check and the 64 MiB bound discriminate; the other two failures in that run were the test's own claimed-size and `exceeds` checks for the reduced size.
- **PNG limit not lifted**: shown by the observe-first run above.

All edited files were restored from byte copies; `git diff` against the committed tree was empty and the suites passed again before the commit.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The plan's PNG refusal test did not discriminate**
- **Found during:** Task 1, mutation run
- **Issue:** `testForgedPngIsRefused` with a 100000 x 100000 PNG (and the `png` and `jpeg` page-view rows) pass with the `ImageLoader` size check removed, because Qt's own 2048 MB limit refuses the claimed allocation. A forged 3-component JPEG cannot be told apart either: any claimed size above the limit needs at least 2^31 bytes as RGB32.
- **Fix:** `testLargeGreyPngLoads` also loads a real 65535 x 1, 65536 x 1 and 1 x 65536 grey PNG and compares the result with `ImageSizeLimit::exceeds`; this fails when the check is removed. The forged refusal tests stay as the no-allocation and speed proof.
- **Files modified:** `src/qt_tests/tst_core_qt.cpp`
- **Commit:** `1fb688fc`

**Total deviations:** 1 auto-fixed (a test that did not discriminate). **Impact:** none on scope; one more case in an existing slot.

## Notes for the gate log and next plans

- The command diff is 4 lines per preset: only the link lines of `dewarping_tests` and `imageproc_tests`, where `libz.tbd` and `libjpeg.dylib` swap places because `test_support` now links `ZLIB::ZLIB` privately. No compile line changed (edit-only commit otherwise), no application target changed. Recorded in the gate-log row.
- `git grep -n 'setAllocationLimit' -- src/core` matches two lines, not one: the call in `ImageLoader.cpp` and the comment on `kQtAllocationLimitMb` in `ImageSizeLimit.h` written in plan 06-04. `setAllocationLimit(0)` appears nowhere under `src`.
- Plan 06-06 (project file dialog and insert flow) can rely on all three metadata loaders reporting the claimed size of an oversized file, so `ImageSizeLimit::exceeds` on the reported size is the one check.

## Issues Encountered

None open.

## Known Stubs

None.

## Threat Flags

None. T-06-26 and T-06-02 are mitigated (the size check precedes `QImageReader::read` and `jpeg_start_decompress`, with peak-footprint and time tests); T-06-31 is accepted as planned (`png_read_info` allocates nothing in proportion to the claimed size).

## Next Phase Readiness

Plan 06-06 builds on the loaders' common verdict (project file dialog and insert flow). SAFE-01 stays unmarked in REQUIREMENTS.md until plans 06-06 and 06-16 have finished.

## Self-Check: PASSED

- Modified files exist and carry the changes: `std::call_once` and `kQtAllocationLimitMb` in `ImageLoader.cpp`; `ImageSizeLimit::exceeds` (line 214) before `jpeg_start_decompress` (line 221) in `JpegMetadataLoader.cpp`; `png_set_user_limits` once in `PngMetadataLoader.cpp`; `forgedPng` and `forgedJpeg` in `ForgedImages.{h,cpp}` (pass).
- Commits `1fb688fc` and `d36da206` are ancestors of HEAD; `git rev-list --count` from the recorded base gives 2 (pass).
- Gate summary `s05-qt-formats`: `result: PASS`, `goldens: unchanged`, tested commit equals the commit with the plan's subject; gate-log row exists (pass).
- Verify commands re-run: `core-s05.txt` holds `No errors detected` and the three JPEG "Leaving test case" lines; `qt-s05.txt` holds the eight PASS lines and every `Totals:` line says `0 failed, 0 skipped` (pass); output-path guard prints nothing; clang-format dry-run clean on all nine touched C++ files (pass).
