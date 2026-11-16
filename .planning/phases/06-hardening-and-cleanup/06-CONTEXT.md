# Phase 6: Hardening and Cleanup - Context

**Gathered:** 2026-11-16
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 6 fixes three known safety defects, removes the OpenGL viewer and cleans up the riskiest code, so that later feature work is safe. Output stays unchanged.

- **SAFE-01:** an image whose header claims absurd dimensions is rejected with an error instead of a huge allocation.
- **SAFE-02:** names containing `&`, `<`, `"` or non-ASCII characters survive saving and reloading a project. The `XmlUnmarshaller` FIXME is resolved.
- **SAFE-03:** the worker-thread count is capped by RAM.
- **STRIP-07:** the OpenGL viewer option, its code and the Qt OpenGL modules are removed.
- **CLEAN-01:** `OutputGenerator.cpp` is split by pure moves.
- **CLEAN-02:** `MainWindow.cpp` is split.
- **CLEAN-03:** dead `#if 0` blocks are removed.
- **CLEAN-04:** unused `boost/foreach.hpp` includes are removed.
- **CLEAN-05:** one shared "Save project as" flow replaces the duplicated one.

New features, behaviour changes beyond these items, and the 999.x backlog items are outside this phase.

</domain>

<decisions>
## Implementation Decisions

### Carried forward (not re-asked)
- **D-00a:** Every commit passes the Phase 3 gate (`.planning/phases/03-qt-6-and-macos-only/gate.sh`). The build succeeds, `ctest --preset macos-release` and `ctest --preset macos-debug` pass 7/7, and no golden or manifest file changes. `UPDATE_GOLDENS` is never set. Where a commit intentionally changes compile or link commands, the command diff is expected to be non-empty, and the commit records why. Examples: STRIP-07 drops `Qt6::OpenGL`/`Qt6::OpenGLWidgets`, and the splits add new source files.
- **D-00b:** Claude does the verifying, as in Phases 2 to 5. Live `mac_check` and `retina_check` runs move the pointer and take focus, so they run only after the user says go: the plan stops at a checkpoint first.
- **D-00c:** Values already stored in the user's settings file are left alone (Phase 3 D-07, Phase 5 D-00d). After STRIP-07 a stored `enable_opengl`-style key is no longer read. It is neither deleted nor rewritten. The `settings/batch_processing_threads` value the user set with the slider is kept as stored.
- **D-00d:** The UI is English only. `tr()` stays on user-visible strings, and changed strings need no translation update.
- **D-00e:** The app binary is never launched against the user's real settings or home folder. Harnesses keep their settings under `build/` (`RETINA_CHECK_ROOT` / the `mac_check` HOME redirect).

### Oversized image rejection (SAFE-01), discussed with the user
- **D-01:** The limit is a **fixed, generous ceiling**, not a share of RAM. It sits well above any real scan: a 1200 dpi A3 colour page is about 277 MP and must load. Examples of the intended size are 1 gigapixel in total, or more than 65,535 px on a side. Research picks the exact numbers and records why. Every machine gives the same result, and the rule is testable with a tiny forged-header file. — **Reversibility:** reversible — one constant.
- **D-02:** The user learns about an oversized file in **both places**, using the existing error display and no new dialog type:
  - **At project creation:** the file list in `ProjectFilesDialog` already reads the header (`ImageMetadataLoader::load`, `src/app/ProjectFilesDialog.cpp:574`). It marks the file as too large and shows its claimed size in pixels.
  - **In the page view:** if a project already holds such a page, the view shows the error through the existing `LoadFileTask` error widget (`src/core/LoadFileTask.cpp:115-135`). The message is a specific "too large" message with the dimensions, not the generic "could not be loaded". No allocation is attempted.
  - The other place that loads source images, `ThumbnailPixmapCache` (`src/core/ThumbnailPixmapCache.cpp:696`), must also refuse without allocating. It shows the usual failed-thumbnail state.
- **D-03:** **One rule covers all formats.** The same limit and the same message apply to TIFF, PNG and JPEG. Qt 6's own `QImageReader` allocation limit (believed to be 256 MB by default; research confirms the value in Qt 6.12) is raised to match our ceiling, through `QImageReader::setAllocationLimit` or a per-reader setting. A large real PNG/JPEG scan that may fail today without a message then loads. One example is a 600 dpi A3 colour page, about 278 MB as RGB32. Files that fail today never produced output, so existing output is unchanged. Research must measure whether such a file really fails on the unmodified code and record it. — **Reversibility:** reversible — a single setting.
- **D-04:** The check runs on header values **before** any pixel buffer is allocated, inside the reader path (`TiffReader::readImage`, `src/core/TiffReader.cpp:279`, and the `extractBinaryOrIndexed8Image` branch; `ImageLoader::load` for Qt formats via `QImageReader::size()`). The metadata path (`TiffReader::readMetadata`, `ImageMetadataLoader`) uses the same limit function, so that project creation and loading agree. A multi-page TIFF rejects only the oversized page. Its other pages load normally.
- **D-05:** Tests (synthetic only): a tiny TIFF whose header claims absurd dimensions (and a PNG equivalent) is rejected with the specific status. The test proves no large allocation happened, for example by running under the ASan preset without an out-of-memory error, or by checking that it returns quickly. A file just under the limit and normal generated pages still load. Goldens are unchanged.

### Thread cap by RAM (SAFE-03), Claude's decision
- **D-06:** Effective thread count = min(`idealThreadCount`, the user's slider value, the RAM cap). The RAM cap = floor(memory budget ÷ estimated peak memory per page task), with at least 1. The budget is a share of physical RAM (`sysctl hw.memsize`) that research picks, for example half. The per-task estimate scales with the **largest source image in the project**, whose pixel size is already known from `ImageMetadata`. The factor (peak bytes per source pixel for the heaviest stage, output with dewarping and colour) is measured on synthetic pages, not guessed. Small scans therefore keep full parallelism, and only large scans are capped. — **Reversibility:** reversible — contained in `WorkerThreadPool::updateNumberOfThreads`.
- **D-07:** The slider (`src/app/SystemLoadWidget.cpp`) keeps its range and stored value. The cap is applied on top, in `WorkerThreadPool` (`src/core/WorkerThreadPool.cpp:79-90`). The slider's tooltip shows when memory limits the count (for example "4/10, limited to 2 by memory"). The 32-bit branches (`sizeof(void*) <= 4`) in both files are dead on arm64 and may go with this change.
- **D-08:** Proof for success criterion 3:
  - Tests pin the cap arithmetic: given RAM, image size and slider value, the expected count comes out.
  - Claude runs a headless batch of large synthetic pages and records peak resident memory and `vm.swapusage` before and after. That run uses a lot of memory on the user's Mac, so the plan stops at a checkpoint for the user's go first, as for live harnesses.
  - The "Activity Monitor on real scans" check is offered to the user in UAT as optional. It does not block the phase, because real scans are personal material.

### Special-character names (SAFE-02), Claude's decision
- **D-09:** Phase 2 measured that `&`, `<`, `"`, NFC and NFD names already round-trip byte for byte (`.planning/phases/02-golden-baselines/records/round-trip.md`). There is no expected-failure mark to remove. `QDomElement::text()` already returns unescaped text, so the FIXME at `src/foundation/XmlUnmarshaller.cpp:14` describes a problem that does not exist. Replace it with a short comment that says so.
- **D-10:** Extend `TestProjectRoundTrip` with the cases that XML really does mangle. These include CR/LF and tab in names (attribute-value normalisation turns them into spaces), leading and trailing spaces, characters below U+0020 that XML 1.0 forbids (QDom may write them raw, which gives an unloadable project file), `'`, `>`, emoji (non-BMP) and a backslash. For each case that fails, fix it, provided that project files written for ordinary names stay byte-identical. A case that cannot be fixed that way becomes an `XFAIL_CHECK` with a backlog item and is reported to the user. The backslash case belongs to backlog 999.8 (`RelinkablePath::normalize`). It is tested and marked, not fixed here.

### Open review findings, Claude's decision
- **D-11:** No review finding is folded in. The Phase 4 warnings WR-01 to WR-03 are already fixed (`04-REVIEW-DISPOSITION.md`). So is Phase 3's WR-01 (`ui_snapshot` compiled the GUI twice; closed by the `app_gui` library in plan 05-01). The "open" notes in `PROJECT.md` Context and `STATE.md` Blockers are out of date and should be corrected at the Phase 6 transition. The remaining info findings, the UBSan reports in `imageproc`, the Margins panel width and the backlog items 999.1 to 999.8 stay deferred (see Deferred).

### OpenGL removal (STRIP-07), Claude's decision
- **D-12:** Remove `src/core/OpenGLSupport.{h,cpp}`, the OpenGL rows in `src/app/SettingsDialog.ui`/`.cpp` (`enableOpenglCb`, `openglDeviceLabel`), `ApplicationSettings::isOpenGlEnabled`/`setOpenGlEnabled`, the `QOpenGLWidget` viewport branch in `ImageViewBase` and the OpenGL side of `ImageViewBase::sampleImageAtWidgetRect`. Drop `OpenGL` and `OpenGLWidgets` from `find_package` (`CMakeLists.txt:105`) and from `src/core/CMakeLists.txt:127`. `TestHiDpiQt`'s colour-picker test must pass unchanged before and after. A test asserts that the Settings dialog has no OpenGL control. Success criterion 4 is checked by `otool -L` (no `QtOpenGL` frameworks in the app or test binaries).

### Cleanup and splits, Claude's decision
- **D-13:** CLEAN-03: remove every `#if 0 … #endif` block (seven files today: `IncompleteThumbnail.cpp`, `output/DewarpingView.cpp`, `page_split/PageLayoutEstimator.cpp`, `page_split/VertLineFinder.cpp`, `imageproc/SavGolFilter.cpp`, `dewarping/Curve.cpp`, `dewarping/DistortionModelBuilder.cpp`). The preprocessor already drops them, so the object code stays the same. A search for `#if 0` under `src/` must come back empty. If a block documents an algorithm, keep a one-line comment saying what it was. Do not keep the code.
- **D-14:** CLEAN-04: remove all 26 `#include <boost/foreach.hpp>` lines. `BOOST_FOREACH` has no uses left. A search must come back empty.
- **D-15:** CLEAN-05: one shared "Save project as" flow replaces `MainWindow::saveProjectAsTriggered` (`src/app/MainWindow.cpp:1425`) and `OutOfMemoryDialog::saveProjectAs` (`src/app/OutOfMemoryDialog.cpp:50`), including the duplicated `saveProjectWithFeedback`. The native panel stays (Phase 5 MAC-11), and the ⇧⌘S shortcut and the dialog filter text stay as they are. Do CLEAN-05 before CLEAN-02, so that the split moves the shared call and not two copies.
- **D-16:** CLEAN-01: `OutputGenerator.cpp` is split into topic files in `src/core/filters/output/` **by pure moves**: no renamed functions, no changed signatures, no reordered arithmetic. File-local helpers keep internal linkage, either in an anonymous namespace inside the new file or as a narrow internal header. Proof: goldens byte-identical, and the moved function bodies match the originals in a diff that ignores the move. The compile flags must not change for the moved code (`-ffp-contract=off` stays global).
- **D-17:** CLEAN-02: `MainWindow.cpp` sheds its menu and action wiring and its batch-processing flow into separate files (new classes or translation units in `src/app/`, compiled into `app_gui`). Behaviour stays identical. The offscreen `TestMacQt`/qt_tests are the gate for every commit. At the end of the phase, one live `mac_check` run (on the user's go, D-00b) confirms that the Phase 5 menus and shortcuts behave exactly as before.

### Claude's Discretion
- Exact pixel ceiling and memory budget share (D-01, D-06), with the measurements recorded under `.planning/phases/06-hardening-and-cleanup/records/`.
- Exact wording of the "too large" messages and the slider tooltip.
- File boundaries of the `OutputGenerator` and `MainWindow` splits, and the names of the new files and classes.
- Plan and commit split. Suggested order: CLEAN-03 and CLEAN-04 (trivial and mechanical), SAFE-01, SAFE-02, SAFE-03, STRIP-07, CLEAN-05, then CLEAN-01 and CLEAN-02 last, so that the splits move already-fixed code.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Scope and requirements
- `.planning/ROADMAP.md` § Phase 6: goal and the five success criteria
- `.planning/REQUIREMENTS.md`: SAFE-01..03, STRIP-07, CLEAN-01..05
- `.planning/PROJECT.md`: constraints (output unchanged, synthetic test data only) and the Key Decisions table

### Gate and earlier phase records
- `.planning/phases/03-qt-6-and-macos-only/gate.sh`: per-commit gate (Release and Debug ctest, goldens, build-command diff)
- `.planning/phases/02-golden-baselines/records/round-trip.md`: SAFE-02 premise not reproduced; how the round-trip test is built
- `.planning/phases/02-golden-baselines/records/access-approval.md`: standing approval for build-tool reads under `/opt/homebrew` and Xcode/CLT through Phase 6
- `.planning/phases/04-retina-rendering/04-CONTEXT.md` D-00d and the colour-picker decision: the test STRIP-07 relies on
- `.planning/phases/04-retina-rendering/04-REVIEW-DISPOSITION.md`: WR-01..03 fixed (D-11)
- `.planning/phases/05-mac-menus-and-shortcuts/05-CONTEXT.md`: menu and shortcut behaviour that CLEAN-02 must keep; `mac_check` HOME redirect
- `.planning/phases/05-mac-menus-and-shortcuts/05-REVIEW-DISPOSITION.md`: open info findings (deferred)
- `TESTING.md`: test commands and the manual `xcrun clang-format` step

### Codebase maps
- `.planning/codebase/CONCERNS.md`: unbounded TIFF dimensions, the `XmlUnmarshaller` FIXME, oversized files, dead code
- `.planning/codebase/ARCHITECTURE.md`, `.planning/codebase/CONVENTIONS.md`

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `ImageMetadataLoader` / `TiffReader::readMetadata`: header-only size read, already used by project creation. This is the natural place for the shared size-limit check.
- `LoadFileTask` error widget (`ErrWidget` subclass of `ErrorWidget`): existing in-view error display with a relink button. A "too large" message fits here.
- `src/test_support/` page generator and `ExpectedFailure.h` (`XFAIL_CHECK`): synthetic large pages for the SAFE-03 measurement and marks for unfixable SAFE-02 cases.
- `TestProjectRoundTrip.cpp` (`src/pipeline_tests/`): extend it for D-10.
- `TestHiDpiQt` colour-picker test and `TestMacQt` (`src/qt_tests/`): the STRIP-07 and CLEAN-02 regression guards.
- `build/macos-release/mac_check`: live Cocoa check for menus and shortcuts (user's go only).

### Established Patterns
- Parse and convert functions report failure through an out-parameter or status, not by throwing (`ImageMetadataLoader::Status`). The limit check should add a status value (for example `TOO_LARGE`) rather than throw.
- `TiffReader::readImage` throws `std::bad_alloc` when `QImage` allocation fails, and `OutOfMemoryHandler` catches it. The new check must stop the oversized case before that point.
- The thread count is read from `QSettings` key `settings/batch_processing_threads` in two places (`WorkerThreadPool`, `SystemLoadWidget`). Keep them consistent, ideally through one shared function.

### Integration Points
- `src/core/TiffReader.cpp` (`readImage`, `readMetadata`), `src/core/ImageLoader.cpp`, `src/core/ImageMetadataLoader.*`, `src/app/ProjectFilesDialog.cpp:574`, `src/app/MainWindow.cpp:1970` (another metadata load call)
- `src/core/WorkerThreadPool.cpp:79-90`, `src/app/SystemLoadWidget.cpp`
- `src/foundation/XmlUnmarshaller.cpp`, `src/foundation/XmlMarshaller.cpp`, `src/core/ProjectWriter.cpp`, `src/core/ProjectReader.cpp`
- `src/core/ImageViewBase.{h,cpp}`, `src/core/OpenGLSupport.*`, `src/app/SettingsDialog.*`, `src/core/ApplicationSettings.*`, `CMakeLists.txt:105`, `src/core/CMakeLists.txt:67,127`
- `src/app/MainWindow.cpp` (2319 lines), `src/app/OutOfMemoryDialog.cpp`, `src/core/filters/output/OutputGenerator.cpp` (2814 lines)

</code_context>

<specifics>
## Specific Ideas

- The user wants a size limit that real work never reaches. A 1200 dpi A3 colour scan (about 277 MP) is the reference point for "generous".
- The user preferred one consistent rule for all image formats over a TIFF-only fix, including lifting Qt's lower PNG/JPEG cap.

</specifics>

<deferred>
## Deferred Ideas

- Backlog 999.1 to 999.8 (binarizer formats, distortion model lines, residual skew, cutter offset, straightness metric, AUTO split of the mixed page, default-profile layout type, backslash in file names). 999.8 gets a test mark under D-10 but no fix.
- UBSan reports in `imageproc` (`BinaryImage.cpp:1044`, `SlicedHistogram.cpp:49` and others; `.planning/phases/02-golden-baselines/records/sanitizer-findings.md`). Fixing them may touch output code paths, so they need their own golden-guarded step.
- Phase 2, 3 and 5 info review findings (`02-/03-/05-REVIEW-DISPOSITION.md`).
- The Margins options panel is cut at the Filters dock's minimum width (`05-UAT.md` test 3).
- A real drag between a 1x and a 2x display (Phase 4 residual).
- Splitting `ThumbnailSequence.cpp` (1712 lines) and `ContentBoxFinder.cpp` (1331 lines), which are not named by CLEAN-01/02.

</deferred>

---

*Phase: 06-hardening-and-cleanup*
*Context gathered: 2026-11-16*
