---
phase: 06-hardening-and-cleanup
plan: 06
subsystem: image-loading
tags: [cpp, safety, image-size-limit, project-files-dialog, insert-flow, thumbnails, sanitizers, int-audit, qt-test, boost-test, gate]

requires:
  - phase: 06-hardening-and-cleanup
    provides: "ImageSizeLimit, ForgedImages, ProcessMemory.h, the TIFF/PNG/JPEG checks and the page-view test of plans 06-04 and 06-05"
provides:
  - "ImageSizeLimit::oversizedPagesText: the one text for a file whose pages are all known from their headers"
  - "Project file dialog: STATUS_TOO_LARGE, red list entry with the claimed size, existing warning box plus one limit line, dialog not accepted"
  - "Insert-files flow: an oversized file is listed in the existing failed-files dialog with its size and is not added"
  - "Thumbnail test: an oversized source gives LOAD_FAILED, no thumbnail file, peak growth below 64 MiB"
  - "records/safe-01.md: ceiling, why fewer than 2^29 px, D-03 and forged-header measurements, check sites, int-arithmetic audit (closes A2), tests, sanitizer run, gate rows"
affects: [06-16]

actuals:
  tokens: 9990
  tasks: 2
  commits: 5

plan_head_before: 0e10bd83a5232d442fc9d07b7a40461b9503be78
plan_head_after: 6cb418b1ba1d6c98132299aa63ae79ad5fe90f85

tech-stack:
  added: []
  patterns:
    - "One limit function for creation and loading: dialog, insert flow and readers all call ImageSizeLimit::exceeds (directly or through oversizedPagesText)"
    - "A file is admitted or refused as a whole at creation; inside an existing project only the oversized page is refused"
    - "Every refusal test is proven to discriminate with an uncommitted mutation run, restored from a byte copy"

key-files:
  created:
    - .planning/phases/06-hardening-and-cleanup/records/safe-01.md
  modified:
    - src/core/ImageSizeLimit.h
    - src/core/ImageSizeLimit.cpp
    - src/core/tests/TestImageSizeLimit.cpp
    - src/app/ProjectFilesDialog.cpp
    - src/app/MainWindow.cpp
    - src/qt_tests/tst_ux_qt.h
    - src/qt_tests/tst_ux_qt.cpp
    - .planning/phases/06-hardening-and-cleanup/records/gate-log.md

key-decisions:
  - "A file with one oversized page is refused as a whole in the project file dialog and the insert flow (the model cannot leave out a page); the text names the first oversized page"
  - "The existing 'Some of the files failed to load' text stays byte-identical; the limit line is appended only when a too-large file is in the list"
  - "The ceiling stays 536870911 px (fewer than 2^29), not D-01's 1 gigapixel example: the int audit found no overflow, so 2^30 would be possible later, but it needs the Qt limit at 4096 MB and RGB32 byte counts up to 2^32 - 4"
  - "Seven (d) products (squared lengths, w x w x h comparisons, histogram and kernel sizes) are listed, not changed: DetectVertContentBounds.cpp:138 can overflow above 46340 px per side, ContentBoxFinder.cpp:60/66 likewise, ProjectPages.cpp:208 multiplies a header dpi"

requirements-completed: []

coverage:
  - id: D1
    description: "The project file dialog lists a forged 100000 x 100000 TIFF as 'forged.tif (too large: 100000 × 100000 px)' and a two-page TIFF with an oversized second page as 'two-pages.tif (page 2 too large: ...)' in red, a normal PNG in green, shows the existing box with the added limit line and does not accept; a control folder with one normal PNG accepts without a box"
    requirement: SAFE-01
    verification:
      - kind: automated_ui
        ref: "src/qt_tests/tst_ux_qt.cpp#TestUxQt::testProjectFilesDialogMarksOversizedFile"
        status: pass
    human_judgment: false
  - id: D2
    description: "ImageSizeLimit::oversizedPagesText returns empty for no or small pages, 'too large: ...' for one oversized page and 'page N too large: ...' for the first oversized page of several"
    requirement: SAFE-01
    verification:
      - kind: unit
        ref: "src/core/tests/TestImageSizeLimit.cpp#test_oversized_pages_text"
        status: pass
    human_judgment: false
  - id: D3
    description: "A thumbnail request for a forged 100000 x 100000 TIFF ends LOAD_FAILED, writes no thumbnail file and grows the peak footprint by less than 64 MiB; a normal PNG gives LOADED and one file"
    requirement: SAFE-01
    verification:
      - kind: automated_ui
        ref: "src/qt_tests/tst_ux_qt.cpp#TestUxQt::testOversizedThumbnailFails"
        status: pass
    human_judgment: false
  - id: D4
    description: "MainWindow::showInsertFileDialog lists an oversized file in the existing failed-files dialog with the too-large text and does not add it"
    requirement: SAFE-01
    verification:
      - kind: other
        ref: "git grep -cF oversizedPagesText src/app/MainWindow.cpp prints 1; the shared function is covered by test_oversized_pages_text and the dialog test (the native file panel cannot be driven offscreen)"
        status: pass
    human_judgment: true
    rationale: "The insert flow itself is not driven by any test (native file panel); a human can confirm it by inserting a forged file into a project, which needs the app and the user's settings"
  - id: D5
    description: "core_tests and qt_tests pass under macos-asan with no AddressSanitizer report and no out-of-memory error; the only runtime error lines are the known BinaryImage.cpp:1044 report"
    requirement: SAFE-01
    verification:
      - kind: other
        ref: "ctest --preset macos-asan -R '^(core_tests|qt_tests)$' -V -> 100% tests passed out of 2, 0 AddressSanitizer, 2 runtime error at BinaryImage.cpp:1044:37"
        status: pass
    human_judgment: false
  - id: D6
    description: "Int-arithmetic audit (A2): 47 candidate lines classified; no pixel, element or byte count (35 lines) exceeds 2147483647 for an accepted source image; seven non-size products listed"
    requirement: SAFE-01
    verification:
      - kind: other
        ref: "build/scratch/int-audit/scan.py and classify.py; table in records/safe-01.md; positive controls Posterizer.cpp:221 and PolynomialSurface.cpp:34 present"
        status: pass
    human_judgment: false
  - id: D7
    description: "Both code commits pass the gate: 100% tests passed out of 7 in Release and Debug, goldens unchanged, empty command diff"
    requirement: SAFE-01
    verification:
      - kind: other
        ref: "gate.sh check s06-project-files (tested 752a869b) and s07-insert-thumbs (tested aeeb0419) -> result: PASS"
        status: pass
    human_judgment: false

duration: 20min
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 06: SAFE-01, project creation, insert flow, thumbnails and the record Summary

**The project file dialog and the insert flow now name an oversized file with its claimed size and refuse it through the one `ImageSizeLimit` rule, a thumbnail request for one fails without allocating, the SAFE-01 tests pass under ASan/UBSan, and an audit of 47 `int` products closes assumption A2 (no pixel, element or byte count of an accepted source image exceeds 2147483647); the decisions, measurements and evidence are in `records/safe-01.md`.**

## Performance

- **Duration:** about 20 min of clock time (two gate runs of about 7 min each in the background, one sanitizer build)
- **Completed:** 2026-11-17
- **Tasks:** 2 (tracer, auto/tdd)
- **Files:** 1 created (record), 8 modified (6 under `src/`, 2 records)

## Accomplishments

- `ImageSizeLimit::oversizedPagesText(const std::vector<ImageMetadata>&)`: empty for no oversized page, `too large: W × H px` for one page, `page N too large: ...` (first oversized page, 1-based) for several.
- `ProjectFilesDialog`: `Item::STATUS_TOO_LARGE` with `setTooLargeText`/`tooLargeText`; `loadNextFile` checks every page after a `LOADED` status, keeps the metadata out of the item and returns `LOAD_FAILED`, so the existing failure flow runs; display text `name (too large: ...)` in the same red as a failed file; failed and too-large items sort first; the existing box text is unchanged and `Files marked "too large" exceed the image size limit of 65535 px per side and 537 megapixels in total.` is appended only when such a file is listed.
- `MainWindow::showInsertFileDialog`: an oversized file goes to `failedFiles` as `path (too large: ...)`, so the existing `LoadFilesStatusDialog` names it.
- Tests: `test_oversized_pages_text` (core, 14 cases in the suite now), `testProjectFilesDialogMarksOversizedFile` (real dialog, forged, two-page and normal files, plus a control folder), `testOversizedThumbnailFails`. `qt_tests`: 28 passed, 0 failed in both result blocks.
- Sanitizers: `core_tests` and `qt_tests` pass under `macos-asan`; 0 `ERROR: AddressSanitizer`, 0 out-of-memory, 2 `runtime error:` lines, both at `src/imageproc/BinaryImage.cpp:1044:37` (known Phase 1 location, recorded, not fixed).
- Audit: `build/scratch/int-audit/scan.py` found 47 candidate lines (positive controls present); classified 30 pixel or element counts, 2 byte counts, 3 row offsets (the largest line maximum is 537067488; the RGB32 reference of 2147483644 bytes fits in `int` too), 7 non-size products (d) and 5 not reachable (e). The A2 verdict and the three (d) observations are in the record.
- Record `records/safe-01.md`: ceiling and why fewer than 2^29 px, the D-03 table (a 278 MB RGB32 PNG or JPEG fails on the unmodified code), the forged-header cost table, the check at every site, the multi-page decision, the audit table, the test list with three mutation runs, the sanitizer table and the gate rows.
- Gates: `s06-project-files` (tested `752a869b`) and `s07-insert-thumbs` (tested `aeeb0419`): Release and Debug `100% tests passed out of 7`, goldens unchanged, command diff 0 / 0. The output-path guard of convention 13 prints nothing; every touched C++ file passes `clang-format --dry-run -Werror`.

## Task Commits

1. **Task 1 (tracer): the project file dialog marks an oversized file red with its size and does not accept** - `752a869b` (fix), gated as `s06-project-files`; gate record `3b6380a9`
2. **Task 2: the insert flow follows the same rule, thumbnails fail without allocating** - `aeeb0419` (fix), gated as `s07-insert-thumbs`; gate record `05b21cf1`; sanitizer run, audit and record: `6cb418b1` (docs)

## Mutation checks (not committed)

- `STATUS_TOO_LARGE` branch in `loadNextFile` disabled (`if (false && ...)`): `testProjectFilesDialogMarksOversizedFile` fails at `QTRY_VERIFY(!closer.texts().isEmpty())` (forged files accepted, no box). Restored from `build/scratch/PFD.cpp.orig`; `git diff` empty afterwards and the test passed again.
- `TiffReader::readImage` refusal disabled (`|| (false && ...)`): `testOversizedThumbnailFails` fails (`status` 0 = `LOADED`, expected `LOAD_FAILED`). In the full `qt_tests` run the earlier page-view test crashes first (SIGSEGV after libtiff reads the forged strip), so the thumbnail slot was run alone (`./qt_tests testOversizedThumbnailFails`). Restored from `build/scratch/TiffReader.cpp.orig`; `git diff --quiet src/core/TiffReader.cpp` succeeded.

## TDD gate note

Task 2 is marked `tdd="true"`, but the code under test (the shared function and the dialog flow) landed in Task 1 and phase convention 6 forbids a failing-test commit, so there is no RED/GREEN pair; the RED evidence is the mutation runs above.

## Deviations from Plan

None - plan executed as written. Three small notes, none a deviation from the plan's outcome:

- `qt_tests` was run from `build/macos-release` with the ctest environment (`QT_QPA_PLATFORM=offscreen`, `LC_ALL=en_US.UTF-8`, `TMPDIR=build/macos-release/test-tmp`) instead of the bare `build/macos-release/qt_tests` of the plan's verify line, so settings and temporary files stay in the build tree (the gate runs it the same way through ctest).
- The scan found 47 lines, not the 60 the plan mentions from planning time; the plan's scan wording ("a product of two terms") was applied literally and the wider cross-check (208 lines) was read as well, see the record.
- `ctest ... --output-on-failure` hides reports of passing tests, so the sanitizer reports were read from a second `ctest -V` run (TESTING.md says the same).

## Issues Encountered

None open.

## Known Stubs

None.

## Threat Flags

None. T-06-09 (oversized file admitted into a project) is mitigated at both creation places and tested; T-06-10 (thumbnail loading of oversized files) is mitigated by the single `ImageLoader::load` path (`git grep QImageReader -- src/core/ThumbnailPixmapCache.cpp` prints nothing) and tested with status, file and footprint checks. Recorded observations (not new surface): three (d) products can overflow `int` for extreme pages or a forged header dpi (`DetectVertContentBounds.cpp:138`, `ContentBoxFinder.cpp:60` and `:66`, `ProjectPages.cpp:208`); none measures memory, none changed here.

## Next Phase Readiness

SAFE-01 stays unmarked in REQUIREMENTS.md until plan 06-16 has finished (`requirements.ready-ids` holds it). Plan 06-07 (SAFE-02, XML name escaping) is next. The (d) observations in `records/safe-01.md` are candidates for a later hardening step, not blockers.

## Self-Check: PASSED

- Created and modified files exist: `records/safe-01.md`, `src/core/ImageSizeLimit.{h,cpp}`, `src/app/ProjectFilesDialog.cpp`, `src/app/MainWindow.cpp`, `src/qt_tests/tst_ux_qt.{h,cpp}`, `src/core/tests/TestImageSizeLimit.cpp` (pass).
- Commits `752a869b`, `3b6380a9`, `aeeb0419`, `05b21cf1`, `6cb418b1` are ancestors of HEAD; `git rev-list --count` from the recorded base gives 5 (pass).
- Gate summaries `s06-project-files` and `s07-insert-thumbs`: `result: PASS`, `goldens: unchanged`, tested commit equals the commit with the plan's subject; both gate-log rows exist (pass).
- Acceptance criteria re-run: `STATUS_TOO_LARGE` and `oversizedPagesText` in `ProjectFilesDialog.cpp`, `oversizedPagesText` in `MainWindow.cpp`, the original box sentence still present once, no `QImageReader` in `ThumbnailPixmapCache.cpp`, `core-s06.txt` holds `No errors detected` and `test_oversized_pages_text`, `qt-s07.txt` holds both new PASS lines and every `Totals:` line says `0 failed, 0 skipped`, `asan-s07-ctest.txt` holds `100% tests passed` and no AddressSanitizer line, `candidates.txt` holds both positive controls, `safe-01.md` holds 536870911, 2147483644, 1073741824, D-01, 2048, Posterizer.cpp, A2 and `unmodified` and no home path (pass).
- Output-path guard prints nothing; clang-format dry-run clean on all touched C++ files (pass).
