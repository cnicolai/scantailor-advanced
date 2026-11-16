---
phase: 06-hardening-and-cleanup
plan: 04
subsystem: image-loading
tags: [cpp, tiff, safety, image-size-limit, boost-test, qt-test, gate]

requires:
  - phase: 06-hardening-and-cleanup
    provides: "gate-log.md, output-path guard and the gate conventions (plan 06-01)"
provides:
  - "ImageSizeLimit: the one place for the image size limit (fewer than 2^29 px, at most 65535 px per side) and its texts"
  - "TiffReader::readImage returns a null image before any allocation for an oversized header"
  - "LoadFileTask error widget with a third state: 'The following file is too large to load' with the claimed size and the limit"
  - "test_support: ForgedImages (in-memory forged TIFFs) and the header-only ProcessMemory.h footprint readers"
  - "CoreImageSizeLimitTestSuite (ten TIFF cases) and TestUxQt::testOversizedImageShowsTooLargeInPageView"
affects: [06-05, 06-06]

actuals:
  tokens: 7900
  tasks: 2
  commits: 4

plan_head_before: 903294afba403e8ac885ea4934c3bff677942d16
plan_head_after: 387820ce706957b932a81179db2d7df2756a6d41

tech-stack:
  added: []
  patterns:
    - "Header values are checked against ImageSizeLimit::exceeds before any pixel buffer exists; a null image is the failure contract, never std::bad_alloc"
    - "No-allocation proof: mach ledger_phys_footprint_peak before and after the call, bound 64 MiB, with a mutation run on the unmodified behaviour"
    - "Forged test images are written with libtiff into a QByteArray (TIFFClientOpen) and read back through QBuffer; nothing touches the disk in core_tests"

key-files:
  created:
    - src/core/ImageSizeLimit.h
    - src/core/ImageSizeLimit.cpp
    - src/test_support/ForgedImages.h
    - src/test_support/ForgedImages.cpp
    - src/test_support/ProcessMemory.h
    - src/core/tests/TestImageSizeLimit.cpp
  modified:
    - src/core/CMakeLists.txt
    - src/core/TiffReader.cpp
    - src/core/LoadFileTask.cpp
    - src/core/tests/CMakeLists.txt
    - src/qt_tests/CMakeLists.txt
    - src/qt_tests/tst_ux_qt.h
    - src/qt_tests/tst_ux_qt.cpp
    - .planning/phases/06-hardening-and-cleanup/records/gate-log.md

key-decisions:
  - "kMaxPixels = 2^29 - 1 (inclusive maximum), kMaxSide = 65535, kQtAllocationLimitMb = 2048, pinned by two static_asserts in ImageSizeLimit.h (RGB32 bytes of the largest accepted image, 2147483644, fit in int; 2048 is the smallest whole MB holding them)"
  - "Forged header-only pages are built with TIFFWriteRawStrip into one strip (RowsPerStrip = claimed height) instead of one scanline per strip, so a very wide forgery needs no scanline buffer of the claimed width"
  - "Every reader call in the tests gets a fresh QBuffer: TiffReader closes the device it is given"

requirements-completed: []

coverage:
  - id: D1
    description: "A TIFF whose header claims 100000 x 100000 px, opened as a page, shows 'too large to load' with the path, '100000 x 100000 px' and '65535 px per side and 537 megapixels in total'; an unreadable file still shows the generic text"
    requirement: SAFE-01
    verification:
      - kind: test
        ref: "src/qt_tests/tst_ux_qt.cpp#TestUxQt::testOversizedImageShowsTooLargeInPageView (rows tiff, garbage)"
        status: pass
    human_judgment: false
  - id: D2
    description: "TiffReader::readImage returns a null image without allocating (peak footprint growth below 64 MiB, under 1000 ms) for forged mono 100000 x 100000, RGB 40000 x 40000, mono 2000000000 x 2 and header-only RGB 32768 x 16384 (exactly 2^29 px)"
    requirement: SAFE-01
    verification:
      - kind: test
        ref: "src/core/tests/TestImageSizeLimit.cpp#test_forged_mono_tiff_is_refused_without_allocating, test_forged_rgb_tiff_is_refused_without_allocating, test_forged_wide_tiff_is_refused_without_allocating, test_rgb_tiff_at_the_ceiling_is_refused_without_allocating"
        status: pass
    human_judgment: false
  - id: D3
    description: "The limit boundaries: 65535 x 1 and 32767 x 16384 load, 65536 x 1 and exactly 2^29 px are refused, the largest accepted rectangle 55245 x 9718 has an RGB32 byte count of 2147483640 that fits in int, A3 at 1200 dpi is inside and A2 outside"
    requirement: SAFE-01
    verification:
      - kind: test
        ref: "src/core/tests/TestImageSizeLimit.cpp#test_exceeds_table, test_texts, test_tiff_side_boundary, test_tiff_total_boundary, test_largest_accepted_size_fits_int_bytes"
        status: pass
    human_judgment: false
  - id: D4
    description: "In a two-page TIFF whose second page claims an oversized header, page 0 loads and page 1 is refused (TiffReader and ImageLoader)"
    requirement: SAFE-01
    verification:
      - kind: test
        ref: "src/core/tests/TestImageSizeLimit.cpp#test_multi_page_tiff_refuses_only_the_oversized_page"
        status: pass
    human_judgment: false
  - id: D5
    description: "Both commits pass the gate: 100% tests passed out of 7 in Release and Debug, goldens unchanged"
    requirement: SAFE-01
    verification:
      - kind: other
        ref: "gate.sh check s03-tiff-limit (tested commit dc8e8091) and s04-tiff-limit-tests (tested commit b4cca806) -> result: PASS"
        status: pass
    human_judgment: false

duration: 20min
completed: 2026-11-16
status: complete
---

# Phase 6 Plan 04: SAFE-01, TIFF reader and page view Summary

**An oversized TIFF header is now refused before any allocation (null image, no `std::bad_alloc`), and the page view says the file is too large with its claimed size and the limit (fewer than 2^29 px, at most 65535 px per side); the boundaries, the multi-page rule and the "no allocation" proof are pinned by ten core_tests cases and a page-view Qt test.**

## Performance

- **Duration:** about 20 min (two full gate runs of about 6 min each)
- **Completed:** 2026-11-16
- **Tasks:** 2 (tracer, auto/tdd)
- **Files:** 6 created, 8 modified (13 under `src/`, 659 lines added)

## Accomplishments

- `ImageSizeLimit` (`src/core/`): `exceeds`, `sizeText`, `limitText`, `tooLargeText`, `claimedSizeIfTooLarge`; constants `kMaxPixels = 2^29 - 1`, `kMaxSide = 65535`, `kQtAllocationLimitMb = 2048` (applied in plan 06-05); two `static_assert`s. All functions are stateless, so parallel loaders share no state.
- `TiffReader::readImage` checks the page header right after `currentPageMetadata`, before either `QImage` constructor, and returns `QImage()` for a side `<= 0` or a size over the limit. Both allocation sites follow this one check.
- `LoadFileTask`: for a null image it re-reads the header (`claimedSizeIfTooLarge`) and `ErrorResult` shows "The following file is too large to load: path, Its image size is 100000 × 100000 px. The limit is 65535 px per side and 537 megapixels in total." The two existing messages are unchanged.
- `test_support`: `forgedTiff` (libtiff over an in-memory stream) and header-only `ProcessMemory.h` (`physFootprintBytes`, `peakPhysFootprintBytes`).
- Qt test `testOversizedImageShowsTooLargeInPageView` (rows `tiff`, `garbage`) runs a real `LoadFileTask` and reads the `textLabel` of the resulting error widget through a file-local `RecordingUi`.
- Gates: `s03-tiff-limit` (dc8e8091) and `s04-tiff-limit-tests` (b4cca806): Release and Debug `100% tests passed out of 7`, goldens unchanged. The output-path guard of convention 13 prints nothing; every touched C++ file passes `clang-format --dry-run -Werror`.

## Task Commits

1. **Task 1 (tracer): a forged TIFF opened as a page shows "too large"** - `dc8e8091` (fix), gated as `s03-tiff-limit`; gate record `478008f4`
2. **Task 2: pin the TIFF limit boundaries, the multi-page rule and the no-allocation proof** - `b4cca806` (test), gated as `s04-tiff-limit-tests`; gate record `387820ce`

## Mutation check (Task 2, not committed)

With the refusal in `TiffReader::readImage` disabled (`if (false && (...))`), `test_forged_mono_tiff_is_refused_without_allocating` fails on both checks: `image.isNull()` has failed and `peakAfter - peakBefore < kFootprintBoundBytes` has failed `[1250526096 >= 67108864]` (1.25 GB, the figure RESEARCH measured on the unmodified reader). The file was restored with a byte copy, `git diff` was empty, and the suite then passed again (10 cases, no errors). The RGB case was not run mutated, as the plan says.

## TDD gate note

Task 2 is marked `tdd="true"`, but its code under test (the reader check) already shipped in Task 1 and the phase convention 6 forbids a failing-test commit (every commit must pass the gate). So there is no RED/GREEN commit pair; the RED evidence is the mutation run above, in which the target test failed on the planned assertions for the planned reason (a non-null 1.25 GB image).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Test helper made the refusal tests vacuous**
- **Found during:** Task 2
- **Issue:** The first draft read the metadata through a `QBuffer` and then called `TiffReader::readImage` on the same buffer. `TiffReader` (like the metadata loader) closes the device it is given (libtiff close procedure), so `readImage` returned null because the device was unreadable, and the two refusal tests would have passed with or without the limit. The multi-page test exposed it: page 0 came back null.
- **Fix:** Every reader call now gets a fresh `QBuffer` over the bytes (`openBuffer`, `pageSizes`, `readPage`, `loadPage`), as the application opens a fresh file for each read. The mutation run above proves the refusal tests now discriminate.
- **Files modified:** `src/core/tests/TestImageSizeLimit.cpp`
- **Commit:** `b4cca806` (found before the commit)

**2. [Rule 3 - Blocking issue] Forged header-only pages use one raw strip, not scanline strips**
- **Found during:** Task 1
- **Issue:** The plan describes RowsPerStrip 1 with only the first scanline written. For the 2000000000 x 2 page this needs a 250 MB scanline buffer in the test process, and because the peak footprint is a high-water mark, forging it would raise the baseline above what an unguarded reader allocates (496 MB) and hide the very defect the test looks for.
- **Fix:** `fullData == false` pages set RowsPerStrip to the claimed height and write 16 raw bytes with `TIFFWriteRawStrip`: one strip, no scanline buffer, a few hundred bytes per page (two-page file: 382 bytes), whatever the claimed size. `fullData == true` pages write every scanline as planned (RowsPerStrip 64).
- **Files modified:** `src/test_support/ForgedImages.cpp`
- **Commit:** `dc8e8091`

**Total deviations:** 2 auto-fixed (1 bug in my own draft test helper, 1 blocking design issue). **Impact:** none on scope; the tests are stronger than planned.

## Notes for the gate log and next plans

- Both command diffs also contain the compile lines of the existing `qt_tests` (nine) and `core_tests` (twelve) objects, which gained `-I src/test_support` through the new `test_support` link dependency. This is the expected side effect of the plan's "add `test_support` to `target_link_libraries`" and is recorded in the gate-log rows; no application target changed.
- libtiff prints `TIFFFillStrip: Likely invalid strip byte count` warnings on stderr only in the mutated run (the reader tried to decode a forged strip); the committed code never reaches the decoder for these pages.

## Issues Encountered

None open.

## Known Stubs

None.

## Threat Flags

| Flag | File | Description |
|------|------|-------------|
| threat_flag: transient-header-reread | src/core/ImageSizeLimit.cpp | `claimedSizeIfTooLarge` re-reads the file header through `ImageMetadataLoader` after any null image. For a forged progressive JPEG, Qt already refuses the image, and `JpegMetadataLoader` then still costs the full image (RESEARCH measured 12.3 GB) until plan 06-05 adds the size check between `jpeg_read_header` and `jpeg_start_decompress`. The same loader is already called at project creation, so this adds no new exposure beyond the window before plan 06-05. |

T-06-01, T-06-07 and T-06-28 (the threat register of this plan) are mitigated for TIFF: header values are checked before either `QImage` constructor, negative sides count as too large, `sizeText` prints unsigned values, the total limit is exclusive with a `static_assert` on the RGB32 byte count, and the 2^29 px RGB header is refused without allocating.

## Next Phase Readiness

Plan 06-05 applies the same limit to PNG and JPEG (`ImageLoader`, `JpegMetadataLoader`) and sets `QImageReader::setAllocationLimit(ImageSizeLimit::kQtAllocationLimitMb)`; it extends `ForgedImages` with `forgedPng` and `forgedJpeg` and the page-view test with more rows. SAFE-01 stays unmarked in REQUIREMENTS.md until plans 06-05, 06-06 and 06-16 have finished.

## Self-Check: PASSED

- Created files exist: `src/core/ImageSizeLimit.h`, `src/core/ImageSizeLimit.cpp`, `src/test_support/ForgedImages.h`, `src/test_support/ForgedImages.cpp`, `src/test_support/ProcessMemory.h`, `src/core/tests/TestImageSizeLimit.cpp` (pass); no `src/test_support/ProcessMemory.cpp` (pass).
- Commits `dc8e8091`, `478008f4`, `b4cca806`, `387820ce` are ancestors of HEAD; `git rev-list --count` from the recorded base gives 4 (pass).
- Gate summaries `s03-tiff-limit` and `s04-tiff-limit-tests`: `result: PASS`, `goldens: unchanged`, tested commit equals the commit with the plan's subject (pass); both gate-log rows exist.
- Acceptance criteria re-run: `ImageSizeLimit::exceeds(metadata.size())` in `TiffReader.cpp`, `claimedSizeIfTooLarge` in `LoadFileTask.cpp`, `kMaxPixels = (qint64(1) << 29) - 1` and the `static_assert` in the header, the original `could not be loaded` text still present once (pass); `qt-s03.txt` holds both PASS lines and every `Totals:` line says `0 failed, 0 skipped` (pass); `core-s04.txt` holds `No errors detected` and ten "Leaving test case" lines (pass); output-path guard prints nothing, clang-format dry-run clean on all touched files (pass).
