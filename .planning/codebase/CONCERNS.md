---
last_mapped_commit: 5eaac1884cdcabb6514bd632114f688631bd8dbc
last_mapped_at: 2026-10-18
---
# Codebase Concerns

**Analysis Date:** 2026-10-18

## Tech Debt

**Oversized UI and processing files:**
- Issue: A few translation units carry many responsibilities and are hard to change safely. Largest files: `src/core/filters/output/OutputGenerator.cpp` (2814 lines), `src/app/MainWindow.cpp` (2245 lines), `src/app/ThumbnailSequence.cpp` (1712 lines), `src/core/filters/select_content/ContentBoxFinder.cpp` (1331 lines), `src/dewarping/TopBottomEdgeTracer.cpp` (1197 lines), `src/app/DefaultParamsDialog.cpp` (1117 lines).
- Files: `src/core/filters/output/OutputGenerator.cpp`, `src/app/MainWindow.cpp`, `src/app/ThumbnailSequence.cpp`
- Impact: Changes to one feature touch long files with many interacting members; review and regression risk is high.
- Fix approach: Split by responsibility (e.g. colour/binarization paths and dewarping paths out of `OutputGenerator.cpp`; menu/action wiring and batch processing out of `MainWindow.cpp`) before adding new features to these files.

**Duplicated logic between main window and out-of-memory dialog:**
- Issue: The "save project as" flow is copied, with an explicit `XXX` marker.
- Files: `src/app/MainWindow.cpp` (around line 1417, `saveProjectAsTriggered`), `src/app/OutOfMemoryDialog.cpp` (around line 51)
- Impact: Fixes to one copy (directory selection, file-extension handling, last-dir persistence) can silently miss the other.
- Fix approach: Move the shared function into a common helper (for example in `src/core/` or `src/app/`) and call it from both sites.

**Unfinished features and open TODOs:**
- Files and notes:
  - `src/core/LoadFileTask.cpp` (line ~83): "do something about DPIs when we have the ability"; DPI handling on load is incomplete.
  - `src/app/MainWindow.cpp` (line ~1869): adding individual pages from a multi-page TIFF that is already partly in the project is "not supported right now".
  - `src/imageproc/Binarize.cpp` (line ~194): integral images could be released earlier; memory is held longer than needed.
  - `src/dewarping/DistortionModelBuilder.cpp` (lines ~448, ~483, ~503): TODOs for a hinted search signature, a penalty for non-horizontal curves, and a dewarping approach that needs revisiting; one block is disabled with `#if 0`.
  - `src/core/ImageViewBase.cpp` (line ~684): a "hack" forces an immediate viewport update.
- Impact: Known gaps that users can hit (multi-page TIFF insertion) and code paths that behave in ways the authors flagged as temporary.
- Fix approach: Track each as an issue; for the TIFF case, make the restriction visible in the UI dialog instead of silently skipping pages.

**Dead code kept behind `#if 0`:**
- Files: `src/core/filters/output/DewarpingView.cpp` (~238), `src/core/filters/page_split/PageLayoutEstimator.cpp` (~660), `src/core/filters/page_split/VertLineFinder.cpp` (~55, ~70), `src/core/IncompleteThumbnail.cpp` (~25), `src/imageproc/tests/TestBinarize.cpp` (~18), `src/imageproc/SavGolFilter.cpp` (~134), `src/dewarping/Curve.cpp` (~150), `src/dewarping/DistortionModelBuilder.cpp` (~500)
- Impact: Nine disabled blocks (`#if 0`) make the real logic harder to read and drift out of date as the surrounding code changes. `TestBinarize.cpp` disabled means binarization has less test coverage than the file name suggests.
- Fix approach: Delete blocks that are superseded; for the test file, either restore it or document why it is off.

**Unescaping FIXME in XML reader:**
- Issue: `XmlUnmarshaller::string` returns `el.text()` with a `FIXME` saying the value "needs unescaping".
- Files: `src/foundation/XmlUnmarshaller.cpp` (line 14)
- Impact: Unclear. `QDomDocument` already decodes standard XML entities during parsing, so the FIXME may be stale. If the project file stores text that is escaped twice, round-trips would corrupt strings. Verify with a project file containing `&`, `<`, and `"` before changing the code.
- Fix approach: Add a round-trip test in `src/foundation/tests/` for special characters; remove the FIXME if the test passes.

**Inconsistent use of "XXX" placeholder strings in code:**
- Files: `src/core/filters/output/OutputGenerator.cpp` (lines ~2225-2293) use `"XXX"`, `"X ?"`, `"XX?"` as ASCII bitmap glyph strings for a debug or rendering table. Real markers are mixed in with these, which makes grep-based debt tracking unreliable.
- Fix approach: Use a clearly named constant for the glyph patterns, and search for real markers as `TODO|FIXME`.

## Known Bugs

**Batch completion alarm runs a shell command from settings:**
- Symptoms: When batch processing finishes and the "beep when finished" option is on, the configured command is executed through the shell.
- Files: `src/app/MainWindow.cpp` (lines ~1327-1339)
- Trigger: `QSettings` key `main_window/external_alarm_cmd` is non-empty. On Unix the default is `play /usr/share/sounds/freedesktop/stereo/bell.oga`.
- Workaround: Clear the setting to use `QApplication::beep()`.
- Note: Command execution through `std::system` runs whatever string is stored in the settings file. This is a local-user setting, not remote input, but it should be documented as "runs a shell command".

**Platform-dependent startup fallback:**
- Symptoms: On Wayland sessions the app forces the `xcb` Qt platform plugin unless overridden.
- Files: `src/app/main.cpp` (lines ~22-24)
- Trigger: `XDG_SESSION_TYPE=wayland` with `QT_QPA_PLATFORM` unset and `SCANTAILOR_NO_XCB_FALLBACK` unset.
- Workaround: Set `SCANTAILOR_NO_XCB_FALLBACK=1` to keep native Wayland.

## Security Considerations

**Shell execution of a configurable command:**
- Risk: `std::system` in `src/app/MainWindow.cpp` (~1337) passes a settings string directly to the shell. Anyone who can write the user's settings file can run arbitrary commands at batch end.
- Files: `src/app/MainWindow.cpp`
- Current mitigation: Only the local user's settings are read; no network input reaches this path.
- Recommendations: Prefer `QProcess` with an argument list (program plus arguments split by the UI), so no shell interpretation happens. At minimum, show the command in the settings UI.

**Project file loading from untrusted input:**
- Risk: Project files (`.ScanTailor` XML) and `DefaultParams` XML are parsed with `QDomDocument` and read field by field through `XmlUnmarshaller`. Malformed numeric or enum values are converted with `toInt()`/`toDouble()` defaults, and no schema validation is done. Crafted files can produce out-of-range geometry that later reaches image routines.
- Files: `src/foundation/XmlUnmarshaller.cpp`, `src/core/ProjectReader.cpp`, `src/core/DefaultParams.cpp`
- Current mitigation: Qt parser; numeric conversions default to 0 on failure.
- Recommendations: Clamp sizes and dimensions read from project files (for example image width/height and DPI) before allocating buffers.

**Raw image dimensions in TIFF reading:**
- Risk: `src/core/TiffReader.cpp` allocates `QImage` buffers from TIFF header dimensions (`TIFFGetField` for width and height, lines ~121-126 and ~281); the code throws `std::bad_alloc` on failure but does not bound the size first.
- Files: `src/core/TiffReader.cpp`
- Current mitigation: Allocation failure throws; `assert(image.bytesPerLine() % 4 == 0)` at line ~304 is compiled out in Release builds.
- Recommendations: Check width times height and bytes-per-line against a sane maximum before allocating, and turn the `assert` into a runtime error.

## Performance Bottlenecks

**Heavy image processing on the GUI-adjacent thread pool:**
- Problem: Large operations (morphology, seed fill, distance maps, binarization) run on full-resolution scanned pages.
- Files: `src/imageproc/Morphology.cpp` (1114 lines), `src/imageproc/SeedFillGeneric.h` (479 lines), `src/imageproc/SEDM.cpp`, `src/imageproc/Binarize.cpp`
- Cause: Full-resolution `BinaryImage` and `GrayImage` buffers; integral images are kept alive longer than necessary (see `src/imageproc/Binarize.cpp` line ~194).
- Improvement path: Release intermediate buffers as soon as they are consumed; profile the output stage before optimising it, since `OutputGenerator.cpp` is the most complex stage.

**Thumbnail cache:**
- Problem: Thumbnails are generated and cached for every page in the project.
- Files: `src/core/ThumbnailPixmapCache.cpp` (894 lines), `src/app/ThumbnailSequence.cpp` (1712 lines)
- Cause: Cache sizing and eviction live in long files; large projects keep many pixmaps.
- Improvement path: Document the cache limits and evict by memory size, not only by count.

## Fragile Areas

**Output generation pipeline:**
- Files: `src/core/filters/output/OutputGenerator.cpp`, `src/core/filters/output/Task.cpp`, `src/core/filters/output/OptionsWidget.cpp` (1062 lines)
- Why fragile: One generator combines picture/fill zones, dewarping, background colour, despeckling, and colour segmentation. Changes in one option change the output of others.
- Safe modification: Make small changes with a before/after comparison on a fixed set of sample pages; run `core_tests` and `imageproc_tests`.
- Test coverage: `src/core/filters/output/` has no dedicated test file in the repository; the generator is only exercised manually.

**Deskew overlay geometry:**
- Files: `src/core/filters/deskew/ImageView.cpp`, `src/core/filters/deskew/OptionsWidget.cpp`, `src/core/DefaultParams.cpp`
- Why fragile: Recent commit `d710384` (issue #145) changed the grid, arc, and handle positions to the content area "respecting visible scroll bars", and changed the default oblique mode through `DefaultParams` XML. The same area was touched by `c3831d0` (formatting) and `91f5ba1` (merge), so regressions are plausible.
- Safe modification: Test in windowed and maximised mode, with and without scroll bars, and with the default and saved oblique modes.
- Test coverage: `src/core/tests/TestDeskewParams.cpp` covers parameters only, not the overlay geometry.

**Threading and wait/notify code:**
- Files: `src/app/RelinkingModel.cpp` (uses `m_cond.wait(&m_mutex)` near line 376 and `wait()` at line 330), `src/core/BackgroundExecutor.cpp` (line ~90), `src/core/ThumbnailPixmapCache.cpp` (line ~327)
- Why fragile: Hand-written condition-variable and wait logic; shutdown order matters and is not documented.
- Safe modification: Keep wait points paired with their signals; test cancel during batch processing and during relinking.

**Ownership by raw pointers and `.get()`/`.release()`:**
- Files: about 114 `new` expressions in `src/**/*.cpp` and about 148 `.get()`/`.release()` calls. Examples: `src/core/filters/output/OutputGenerator.cpp`, `src/core/filters/output/DespeckleView.cpp`.
- Why fragile: Ownership is split between `std::unique_ptr`, `std::shared_ptr`, Qt parent-child, and raw `new`; mixing them is easy to get wrong.
- Safe modification: Use `std::make_unique` and pass Qt-owned widgets by parent; do not add new raw `new` for objects that have a clear single owner.

**Casts:**
- Files: 28 uses of `const_cast` or `reinterpret_cast` in `src/`. The TIFF reader uses C casts for libtiff buffers (`src/core/TiffReader.cpp` lines ~62, ~290).
- Safe modification: Keep casts in the lowest-level file that talks to the C API; do not spread them into the UI code.

## Scaling Limits

**Memory per page at high resolution:**
- Current capacity: Not measured in the repository; pages are held as full-resolution `QImage` and `BinaryImage` objects.
- Limit: Very large scans (high DPI, large paper) multiply memory per open page. The out-of-memory path exists (`src/app/OutOfMemoryDialog.cpp`), which indicates the limit is known.
- Scaling path: Process pages one at a time through the batch queue, keep only thumbnails in memory, and reload originals from disk when needed.

**Project file size:**
- Current capacity: Project state is held in memory as a whole and written as XML (`src/core/ProjectWriter.cpp`, `src/core/ProjectReader.cpp`).
- Limit: Large projects (thousands of pages) make load and save slower; XML DOM parsing loads the full file.
- Scaling path: Consider streaming read or splitting per-page state into separate files.

## Dependencies at Risk

**Qt 5 and Qt 6 both supported:**
- Risk: `CMakeLists.txt` (lines ~222-229) first looks for Qt 6, then falls back to Qt 5. CI in `.github/workflows/ci.yml` and `.github/workflows/cmake.yml` installs Qt 5 only (`qtbase5-dev`). Qt 6 code paths are therefore not exercised in CI.
- Impact: Qt 6 build breakage can go unnoticed; Qt 5 end-of-life also affects distributions.
- Migration plan: Add a Qt 6 job to CI; track Qt 5 removal as a milestone.

**libtiff and libpng/libjpeg system libraries:**
- Risk: Found through `find_package(TIFF REQUIRED)`, `PNG`, `JPEG`, and `ZLIB` in `CMakeLists.txt` (lines ~137-161). Version drift on distributions is handled only by the README note about glibc (`README.md`, AppImage section).
- Impact: Build failures or different TIFF behaviour (for example `TIFFReadRGBAImageOriented` results in `src/core/TiffReader.cpp`) across distributions.
- Migration plan: Pin minimum versions in CMake and add them to the CI install list.

## Missing Critical Features

**Automated test gating in CI:**
- Problem: `.github/workflows/ci.yml` runs `ctest -C Release --output-on-failure || true`, so failing tests never fail the build.
- Blocks: Regressions in `core_tests`, `imageproc_tests`, `math_tests`, `foundation_tests`, and `qt_tests` are reported but do not stop merges.

**Regression coverage for GUI workflows:**
- Problem: `src/qt_tests` (6 files) covers only a small part of the UI; the filters (`select_content`, `page_layout`, `output`, `deskew`) have no interaction tests.
- Blocks: Safe refactoring of `MainWindow.cpp` and the `OptionsWidget` files.

## Test Coverage Gaps

**Output generation:**
- What's not tested: `OutputGenerator` end-to-end, colour segmentation, dewarping output, and despeckle output.
- Files: `src/core/filters/output/OutputGenerator.cpp`, `src/core/filters/output/Task.cpp`
- Risk: Output regressions are visible only to users on real scans.
- Priority: High

**Binarization:**
- What's not tested: The file `src/imageproc/tests/TestBinarize.cpp` is wrapped in `#if 0` (line ~18).
- Files: `src/imageproc/Binarize.cpp`, `src/imageproc/tests/TestBinarize.cpp`
- Risk: Binarization changes (threshold, Sauvola, Otsu variants) are not checked by unit tests.
- Priority: High

**Dewarping:**
- What's not tested: Distortion model builder and edge tracers (`src/dewarping/DistortionModelBuilder.cpp` 661 lines, `src/dewarping/TopBottomEdgeTracer.cpp` 1197 lines, `src/dewarping/TextLineTracer.cpp`) have no dedicated test files in `src/dewarping/`.
- Risk: The disabled blocks and open TODOs in `DistortionModelBuilder.cpp` may hide behaviour changes.
- Priority: Medium

**Project file round-trip with special characters:**
- What's not tested: XML escaping for names, paths, and free text in project files (see the FIXME in `src/foundation/XmlUnmarshaller.cpp`).
- Files: `src/foundation/XmlUnmarshaller.cpp`, `src/core/ProjectReader.cpp`, `src/core/ProjectWriter.cpp`
- Risk: Silent corruption of file names or labels after save and reload.
- Priority: Medium

---

*Concerns audit: 2026-10-18*
