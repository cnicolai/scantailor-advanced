# Phase 6: Hardening and Cleanup - Research

**Researched:** 2026-11-16
**Domain:** C++17/Qt 6.12 desktop app hardening on macOS arm64: image-header limits (TIFF/PNG/JPEG), XML project-file round trip, RAM-capped worker pool, OpenGL removal, behaviour-preserving file splits
**Confidence:** HIGH for the measured items (Qt limit, TIFF/JPEG allocation behaviour, QDom behaviour, memory per pixel); MEDIUM for the split proposals (line ranges are measured, final file boundaries are the planner's call)

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Carried forward (not re-asked)
- **D-00a:** Every commit passes the Phase 3 gate (`.planning/phases/03-qt-6-and-macos-only/gate.sh`). The build succeeds, `ctest --preset macos-release` and `ctest --preset macos-debug` pass 7/7, and no golden or manifest file changes. `UPDATE_GOLDENS` is never set. Where a commit intentionally changes compile or link commands, the command diff is expected to be non-empty, and the commit records why. Examples: STRIP-07 drops `Qt6::OpenGL`/`Qt6::OpenGLWidgets`, and the splits add new source files.
- **D-00b:** Claude does the verifying, as in Phases 2 to 5. Live `mac_check` and `retina_check` runs move the pointer and take focus, so they run only after the user says go: the plan stops at a checkpoint first.
- **D-00c:** Values already stored in the user's settings file are left alone (Phase 3 D-07, Phase 5 D-00d). After STRIP-07 a stored `enable_opengl`-style key is no longer read. It is neither deleted nor rewritten. The `settings/batch_processing_threads` value the user set with the slider is kept as stored.
- **D-00d:** The UI is English only. `tr()` stays on user-visible strings, and changed strings need no translation update.
- **D-00e:** The app binary is never launched against the user's real settings or home folder. Harnesses keep their settings under `build/` (`RETINA_CHECK_ROOT` / the `mac_check` HOME redirect).

#### Oversized image rejection (SAFE-01), discussed with the user
- **D-01:** The limit is a **fixed, generous ceiling**, not a share of RAM. It sits well above any real scan: a 1200 dpi A3 colour page is about 277 MP and must load. Examples of the intended size are 1 gigapixel in total, or more than 65,535 px on a side. Research picks the exact numbers and records why. Every machine gives the same result, and the rule is testable with a tiny forged-header file. — **Reversibility:** reversible — one constant.
- **D-02:** The user learns about an oversized file in **both places**, using the existing error display and no new dialog type:
  - **At project creation:** the file list in `ProjectFilesDialog` already reads the header (`ImageMetadataLoader::load`, `src/app/ProjectFilesDialog.cpp:574`). It marks the file as too large and shows its claimed size in pixels.
  - **In the page view:** if a project already holds such a page, the view shows the error through the existing `LoadFileTask` error widget (`src/core/LoadFileTask.cpp:115-135`). The message is a specific "too large" message with the dimensions, not the generic "could not be loaded". No allocation is attempted.
  - The other place that loads source images, `ThumbnailPixmapCache` (`src/core/ThumbnailPixmapCache.cpp:696`), must also refuse without allocating. It shows the usual failed-thumbnail state.
- **D-03:** **One rule covers all formats.** The same limit and the same message apply to TIFF, PNG and JPEG. Qt 6's own `QImageReader` allocation limit (believed to be 256 MB by default; research confirms the value in Qt 6.12) is raised to match our ceiling, through `QImageReader::setAllocationLimit` or a per-reader setting. A large real PNG/JPEG scan that may fail today without a message then loads. One example is a 600 dpi A3 colour page, about 278 MB as RGB32. Files that fail today never produced output, so existing output is unchanged. Research must measure whether such a file really fails on the unmodified code and record it. — **Reversibility:** reversible — a single setting.
- **D-04:** The check runs on header values **before** any pixel buffer is allocated, inside the reader path (`TiffReader::readImage`, `src/core/TiffReader.cpp:279`, and the `extractBinaryOrIndexed8Image` branch; `ImageLoader::load` for Qt formats via `QImageReader::size()`). The metadata path (`TiffReader::readMetadata`, `ImageMetadataLoader`) uses the same limit function, so that project creation and loading agree. A multi-page TIFF rejects only the oversized page. Its other pages load normally.
- **D-05:** Tests (synthetic only): a tiny TIFF whose header claims absurd dimensions (and a PNG equivalent) is rejected with the specific status. The test proves no large allocation happened, for example by running under the ASan preset without an out-of-memory error, or by checking that it returns quickly. A file just under the limit and normal generated pages still load. Goldens are unchanged.

#### Thread cap by RAM (SAFE-03), Claude's decision
- **D-06:** Effective thread count = min(`idealThreadCount`, the user's slider value, the RAM cap). The RAM cap = floor(memory budget ÷ estimated peak memory per page task), with at least 1. The budget is a share of physical RAM (`sysctl hw.memsize`) that research picks, for example half. The per-task estimate scales with the **largest source image in the project**, whose pixel size is already known from `ImageMetadata`. The factor (peak bytes per source pixel for the heaviest stage, output with dewarping and colour) is measured on synthetic pages, not guessed. Small scans therefore keep full parallelism, and only large scans are capped. — **Reversibility:** reversible — contained in `WorkerThreadPool::updateNumberOfThreads`.
- **D-07:** The slider (`src/app/SystemLoadWidget.cpp`) keeps its range and stored value. The cap is applied on top, in `WorkerThreadPool` (`src/core/WorkerThreadPool.cpp:79-90`). The slider's tooltip shows when memory limits the count (for example "4/10, limited to 2 by memory"). The 32-bit branches (`sizeof(void*) <= 4`) in both files are dead on arm64 and may go with this change.
- **D-08:** Proof for success criterion 3:
  - Tests pin the cap arithmetic: given RAM, image size and slider value, the expected count comes out.
  - Claude runs a headless batch of large synthetic pages and records peak resident memory and `vm.swapusage` before and after. That run uses a lot of memory on the user's Mac, so the plan stops at a checkpoint for the user's go first, as for live harnesses.
  - The "Activity Monitor on real scans" check is offered to the user in UAT as optional. It does not block the phase, because real scans are personal material.

#### Special-character names (SAFE-02), Claude's decision
- **D-09:** Phase 2 measured that `&`, `<`, `"`, NFC and NFD names already round-trip byte for byte (`.planning/phases/02-golden-baselines/records/round-trip.md`). There is no expected-failure mark to remove. `QDomElement::text()` already returns unescaped text, so the FIXME at `src/foundation/XmlUnmarshaller.cpp:14` describes a problem that does not exist. Replace it with a short comment that says so.
- **D-10:** Extend `TestProjectRoundTrip` with the cases that XML really does mangle. These include CR/LF and tab in names (attribute-value normalisation turns them into spaces), leading and trailing spaces, characters below U+0020 that XML 1.0 forbids (QDom may write them raw, which gives an unloadable project file), `'`, `>`, emoji (non-BMP) and a backslash. For each case that fails, fix it, provided that project files written for ordinary names stay byte-identical. A case that cannot be fixed that way becomes an `XFAIL_CHECK` with a backlog item and is reported to the user. The backslash case belongs to backlog 999.8 (`RelinkablePath::normalize`). It is tested and marked, not fixed here.

#### Open review findings, Claude's decision
- **D-11:** No review finding is folded in. The Phase 4 warnings WR-01 to WR-03 are already fixed (`04-REVIEW-DISPOSITION.md`). So is Phase 3's WR-01 (`ui_snapshot` compiled the GUI twice; closed by the `app_gui` library in plan 05-01). The "open" notes in `PROJECT.md` Context and `STATE.md` Blockers are out of date and should be corrected at the Phase 6 transition. The remaining info findings, the UBSan reports in `imageproc`, the Margins panel width and the backlog items 999.1 to 999.8 stay deferred (see Deferred).

#### OpenGL removal (STRIP-07), Claude's decision
- **D-12:** Remove `src/core/OpenGLSupport.{h,cpp}`, the OpenGL rows in `src/app/SettingsDialog.ui`/`.cpp` (`enableOpenglCb`, `openglDeviceLabel`), `ApplicationSettings::isOpenGlEnabled`/`setOpenGlEnabled`, the `QOpenGLWidget` viewport branch in `ImageViewBase` and the OpenGL side of `ImageViewBase::sampleImageAtWidgetRect`. Drop `OpenGL` and `OpenGLWidgets` from `find_package` (`CMakeLists.txt:105`) and from `src/core/CMakeLists.txt:127`. `TestHiDpiQt`'s colour-picker test must pass unchanged before and after. A test asserts that the Settings dialog has no OpenGL control. Success criterion 4 is checked by `otool -L` (no `QtOpenGL` frameworks in the app or test binaries).

#### Cleanup and splits, Claude's decision
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

### Deferred Ideas (OUT OF SCOPE)
- Backlog 999.1 to 999.8 (binarizer formats, distortion model lines, residual skew, cutter offset, straightness metric, AUTO split of the mixed page, default-profile layout type, backslash in file names). 999.8 gets a test mark under D-10 but no fix.
- UBSan reports in `imageproc` (`BinaryImage.cpp:1044`, `SlicedHistogram.cpp:49` and others; `.planning/phases/02-golden-baselines/records/sanitizer-findings.md`). Fixing them may touch output code paths, so they need their own golden-guarded step.
- Phase 2, 3 and 5 info review findings (`02-/03-/05-REVIEW-DISPOSITION.md`).
- The Margins options panel is cut at the Filters dock's minimum width (`05-UAT.md` test 3).
- A real drag between a 1x and a 2x display (Phase 4 residual).
- Splitting `ThumbnailSequence.cpp` (1712 lines) and `ContentBoxFinder.cpp` (1331 lines), which are not named by CLEAN-01/02.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SAFE-01 | A TIFF with absurd dimensions is rejected with an error instead of a huge allocation | Measured: forged 13 KB TIFF costs 6.1 GB RSS today; forged 10 KB progressive JPEG costs 12.3 GB in the *metadata* path; Qt rejects PNG/JPEG above 64 MP. Ceiling, check sites, status flow and tests in "SAFE-01" below |
| SAFE-02 | Names with `&`, `<`, `"` or non-ASCII survive save and reload (resolves the FIXME) | Measured QDom matrix: all D-10 cases except control characters U+0001-U+001F (minus tab, LF, CR) already round-trip; those cannot be written by QDom at all. Fix design and test plan in "SAFE-02" |
| SAFE-03 | Worker threads capped by available RAM | Measured peak bytes per pixel (25-33 B per max(source, output) pixel; plan on 40 B), RAM query, budget 50%, where the data lives, cap examples in "SAFE-03" |
| STRIP-07 | OpenGL viewer option, code and Qt modules removed after a colour-picker test | Complete inventory (13 files), picker path analysis, `otool -L` check incl. the system `OpenGL.framework` trap, in "STRIP-07" |
| CLEAN-01 | `OutputGenerator.cpp` split into topic files by pure moves | 82 top-level chunks, helper-to-user map, proposed 7 files, proof recipe incl. pre-split output digests, in "CLEAN-01" |
| CLEAN-02 | `MainWindow.cpp` split (menu/action wiring and batch processing move out) | Function inventory by line, translation-unit split proposal, source-scan test that must follow the move, in "CLEAN-02" |
| CLEAN-03 | Dead `#if 0` blocks removed | 8 blocks in 7 files; **5 of them have a live `#else` branch that must be kept**; preprocessor-equivalence proof, in "CLEAN-03/04" |
| CLEAN-04 | Unused `boost/foreach.hpp` includes removed | 26 includes confirmed, 0 uses, in "CLEAN-03/04" |
| CLEAN-05 | One shared "Save project as" helper | Side-by-side comparison and proposed shape, in "CLEAN-05" |
</phase_requirements>

## Summary

All nine requirements are implementable with the existing stack; **no new package is added**. The research settled every question CONTEXT left open, mostly by measurement on the real toolchain (Qt 6.12.0_2, libtiff 4.7.2, libpng 1.6.59, jpeg-turbo 3.2.1, Apple clang 21, macOS 26.7, 64 GiB / 10 cores). Three findings change the plan compared with CONTEXT: (1) the JPEG *metadata* loader is itself an unbounded allocation site (a 10 KB forged progressive JPEG made `jpeg_start_decompress` take 2.8 s and 12.3 GB RSS), so it needs the check before `jpeg_start_decompress`; (2) Qt's default allocation limit of 256 MB rejects *any* PNG/JPEG above 64 Mi pixels regardless of format (a 70 MB 8-bit grey A3 600 dpi page fails too), which is what D-03 must lift; (3) peak memory of the output stage scales with the larger of *source* and *output* pixel count, and the app default output DPI is 600, so a 300 dpi scan costs about 4x more per source pixel than a 600 dpi scan.

For SAFE-02 the measured truth is narrower than D-10 assumes: Qt 6.12 `QDomDocument` already escapes tab, LF and CR as character references and keeps leading/trailing spaces, `'`, `>`, emoji and backslash intact. The only unrepresentable names contain control characters U+0001-U+0008, U+000B, U+000C, U+000E-U+001F (APFS allows them in file names; QDom writes them raw and the file then fails to parse; `&#x1;` is rejected by the parser too). APFS refuses U+FFFE/U+FFFF in names, so those are unreachable. A flagged escape attribute fixes it without changing ordinary files.

For the splits, a proof strategy stronger than "goldens pass" is available and cheap: the existing pipeline harness is deterministic to the bit on decoded pixels, so a before/after SHA-1 of decoded output images (recorded below for HEAD `e0b632f7`) proves pure moves for the output stage; a chunk-multiset script proves the text moves. One existing test (`TestMacQt::testBatchAlarmUsesBeep`) reads `src/app/MainWindow.cpp` by path and **must be updated in the same commit** that moves `filterResult`.

**Primary recommendation:** Ceiling 536,870,912 px total (2^29) and 65,535 px per side, enforced by one `ImageSizeLimit` helper that `TiffReader`, `JpegMetadataLoader`, `ImageLoader`, `ProjectFilesDialog` and `MainWindow` share, with `QImageReader::setAllocationLimit(2048)` set once inside `ImageLoader`; thread cap = floor(50% of `hw.memsize` / (40 B x largest per-page max(source, output) pixel count)), at least 1; split `MainWindow` into translation units of the same class and `OutputGenerator` into an internal-header plus six topic files, each step gated and proven by digest.

## Architectural Responsibility Map

This is a desktop app, so tiers are the project's layers (CLAUDE.md "Layers").

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Image size limit (constants, `exceeds()`, message text) | `src/core` (new `ImageSizeLimit.{h,cpp}`) | — | Used by readers (core), metadata loaders (core) and two app dialogs; core has no dependency on app |
| TIFF/JPEG/PNG header checks | `src/core` readers (`TiffReader`, `JpegMetadataLoader`, `ImageLoader`) | — | Only place that sees the header before allocation |
| "Too large" display | `src/core` (`LoadFileTask::ErrorResult`) for the page view; `src/app` (`ProjectFilesDialog`, `MainWindow` insert flow) for creation | `ThumbnailPixmapCache` (inherits refusal from `ImageLoader`) | D-02 two places, existing error widgets |
| RAM query and cap arithmetic | `src/core` (new `MemoryBudget.{h,cpp}`, pure functions) | `WorkerThreadPool` applies it; `SystemLoadWidget` shows it | Pure function is unit-testable with injected RAM |
| Largest page estimate | `src/app` (`MainWindow::startBatchProcessing` already iterates every `PageInfo`) | `src/core` provides the pure function | `PageInfo::metadata()` is available in that loop; no new `ProjectPages` API needed |
| Project XML escape for invalid chars | `src/foundation` (`XmlMarshaller`/`XmlUnmarshaller` helper) | `src/core` `ProjectWriter`/`ProjectReader` call it at 3 attribute sites | Format concern, tied to QDom limits |
| OpenGL removal | `src/core` (`ImageViewBase`, `ApplicationSettings`, `OpenGLSupport`), `src/app` (`SettingsDialog`) | CMake | No other tier touches it |
| Shared Save-as flow | `src/app` (new `ProjectSaveFlow.{h,cpp}` in `app_gui`) | — | Both callers (`MainWindow`, `OutOfMemoryDialog`) are in `app_gui` |
| Splits | `src/core/filters/output` (OutputGenerator), `src/app` (MainWindow) | `src/qt_tests` (source scan follows the move) | Pure moves inside their own modules |

## Standard Stack

### Core (all existing; verified installed this session)
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt 6 (Core, Gui, Widgets, Xml, Network, Svg, Test) | 6.12.0_2 `[VERIFIED: /opt/homebrew/opt/qtbase -> ../Cellar/qtbase/6.12.0_2]` | App framework; `QImageReader`, `QDomDocument`, `QThreadPool` | Project stack. OpenGL and OpenGLWidgets leave the list |
| libtiff | 4.7.2 `[VERIFIED: build/macos-release/CMakeCache.txt FIND_PACKAGE_MESSAGE_DETAILS_TIFF "v4.7.2"]` | TIFF read/write | Project stack |
| libpng / jpeg-turbo | 1.6.59 / 3.2.1 `[VERIFIED: ls /opt/homebrew/Cellar/libpng, jpeg-turbo]` | PNG/JPEG metadata loaders | Project stack |
| Boost.Test | 1.92.0_1 `[VERIFIED: ls /opt/homebrew/Cellar/boost]` | `core_tests`, `pipeline_tests` | Project stack |
| Qt Test | in Qt 6.12.0 | `qt_tests` (`TestMacQt`, `TestHiDpiQt`, `TestUxQt`) | Project stack |
| CMake / Ninja | 4.5.0 / 1.13.2 `[VERIFIED: --version]` | Build | Project stack |
| `sysctlbyname("hw.memsize")` | macOS 26.7 libc | Physical RAM | Documented BSD API; no Qt API for total RAM `[ASSUMED: no QSysInfo equivalent in Qt 6.12; not searched]`; this Mac reports `hw.memsize: 68719476736` `[VERIFIED: sysctl hw.memsize]` |

### Supporting
| Tool | Purpose | When to Use |
|------|---------|-------------|
| `xcrun clang-format` 21.0.0 | Mandatory formatting of touched C++ files | Every commit (`xcrun clang-format --dry-run -Werror <file>`) |
| `otool -L`, `c++ -E -P`, `python3 -I` | Link and preprocessor proofs; chunk-multiset script | CLEAN-01/03, STRIP-07 |
| `gate.sh` | Per-commit gate | Every code commit (see "Gate usage") |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| New `TOO_LARGE` value in `ImageMetadataLoader::Status` | Keep `LOADED` and let consumers call `ImageSizeLimit::exceeds(metadata.size())` per page | D-04 wants a multi-page TIFF to reject only the oversized page. A single status for the whole file cannot say which page; per-page checks can. Recommend per-page checks; a status value is optional sugar |
| Class extraction for `MainWindow` batch flow | Same-class translation units (`MainWindowBatch.cpp` etc.) | Slots are connected with `SIGNAL()/SLOT()` strings and tests find actions by object name; moving member definitions between .cpp files keeps all of that intact. A new class changes slot owners and is not a pure move |

**Installation:** none. **Version verification:** all versions above were read from the installed tree or CMake cache this session.

## Package Legitimacy Audit

No external package is added or changed in this phase (Qt OpenGL modules are removed, nothing is installed). Section not applicable.

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| (none) | — | — | — | — | — | — |

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

## Project Constraints (from CLAUDE.md)

From `./.claude/CLAUDE.md` and the user's global `~/.claude/CLAUDE.md` (same authority as locked decisions):
- C++17 only, no C++20; `std::make_shared`/`std::unique_ptr`; avoid raw `new`. Qt/CMake architecture and the six-stage pipeline stay.
- **Output for existing settings must not change.** Test data synthetic only; no personal scans committed.
- clang-format `BasedOnStyle: Chromium`, 2-space indent, `BinPackParameters: false`; run `xcrun clang-format -i <file>` on every touched file; `src/dewarping/TextLineTracer.cpp` keeps two older violations.
- Include guards `SCANTAILOR_<MODULE>_<NAME>_H_`, no `#pragma once`. New files start with the project copyright/licence header. Module headers are included by bare name.
- Failure reporting: parse/convert functions use an out-parameter or status, not exceptions; do not swallow exceptions; user-facing errors via modal dialogs with `tr()`; validate input in the dialog, not deep in the pipeline.
- User-visible strings use `tr()`/`QCoreApplication::translate`; UI English only.
- Keep logging low (about 19 `qDebug/qWarning` calls in `src/`).
- GSD: start work through a GSD command; no direct edits outside a workflow.
- Global rules: walk the user through manual procedures **one step at a time** (checkpoint tasks for `mac_check` and the memory run must present one command at a time with the concrete thing to verify); never access OmniFocus; never access files outside the project directory without asking (standing approval only for `/opt/homebrew` and Xcode/CLT reads); **never install, launch or drive Google Chrome**.
- Memory notes that bind execution: use `xcrun clang-format`; no `/tmp` (scratch is `build/scratch/`); run `ctest` yourself; German locale (`LC_ALL=C` for numeric awk/sort); shell grep is ugrep (repeated `-e`, `-F` for `$ { (`); `gsd_run` unavailable in subagents; live harness runs need the user's go.

## Architecture Patterns

### System Architecture Diagram (data flow touched by this phase)

```
 file on disk ──► [ProjectFilesDialog / MainWindow insert]  ImageMetadataLoader::load(path)
                    │  per page: ImageMetadata(size,dpi)           (Jpeg/Png/Tiff loaders, header only)
                    │       └─► ImageSizeLimit::exceeds(size)? ──yes──► mark "too large (WxH px)", not added
                    ▼ no
              ProjectPages (metadata per image)
                    │
   page selected / thumbnail ────────────────────────────────┐
                    ▼                                         ▼
        LoadFileTask::operator()                 ThumbnailPixmapCache::loadSaveThumbnail
                    │  ImageLoader::load(imageId)  ◄───────────┘
                    │     ├─ TiffReader::readImage ── header W,H ──► ImageSizeLimit::exceeds ──► QImage() (no alloc)
                    │     └─ QImageReader  ── reader.size() ───────► ImageSizeLimit::exceeds ──► QImage() (no alloc)
                    │            (Qt allocation limit raised once to 2048 MB inside ImageLoader)
                    ▼ null image
        ImageSizeLimit::checkFile(imageId)  ── header re-read ──► ErrorResult(too large, WxH)  ── ErrWidget text
                    ▼ image ok
        six-stage task chain ──► output stage peak ≈ 25-33 B x max(source px, output px)

 startBatchProcessing: for each PageInfo ─► P = max(srcPx, srcPx*(outDpi/srcDpi)^2) ─► largest P
        MemoryBudget::threadsForMemory(hw.memsize, P) ─► WorkerThreadPool::setMemoryCap(n)
        effective = min(idealThreadCount, slider, n) ─► QThreadPool::setMaxThreadCount
        hasSpareCapacity() (active < max) throttles the submit loops in startBatchProcessing / filterResult

 Save: saveProjectAsTriggered / OutOfMemoryDialog::saveProjectAs ─► ProjectSaveFlow::saveAs(...)
        ─► native QFileDialog ─► +".ScanTailor" ─► ProjectWriter::write ─► lastDir + RecentProjects
 ProjectWriter: name/path/outputDirectory ─► XmlMarshaller escape (only if XML-1.0-invalid chars) ─► QDom ─► file
```

### Recommended Project Structure (new/changed files)
```
src/core/
├── ImageSizeLimit.{h,cpp}        # constants, exceeds(), checkFile(), message text, one-time Qt limit setup
├── MemoryBudget.{h,cpp}          # physicalMemoryBytes() (sysctl), threadsForMemory(), effectiveThreads()
├── (OpenGLSupport.{h,cpp} removed)
src/core/filters/output/
├── OutputGenerator.cpp           # ctor, process(), Processor ctor/initParams/calcAreas/processImpl (~230 lines)
├── OutputGeneratorProcessor.h    # internal header: class OutputGenerator::Processor definition (moved verbatim)
├── OutputGeneratorPixelOps.{h,cpp}   # shared helpers: reserveBlackAndWhite, fillExcept, BinaryImageXOR, fillMarginsInPlace, applyFillZones*, smoothToGrayscale
├── OutputGeneratorNoDewarp.cpp   # processWithoutDewarping
├── OutputGeneratorDewarp.cpp     # processWithDewarping, dewarper setup, distortion models + their helpers
├── OutputGeneratorBinarize.cpp   # illumination, binarization, thresholds, despeckle, picture zones, segment/posterize
src/app/
├── ProjectSaveFlow.{h,cpp}       # shared Save-as flow (CLEAN-05)
├── MainWindow.cpp                # shrinks; MainWindowActions.cpp, MainWindowBatch.cpp, MainWindowProject.cpp, MainWindowPages.cpp (same class)
src/core/tests/                   # TestImageSizeLimit.cpp, TestMemoryBudget.cpp (Boost, in-memory QBuffer, no files)
```

### Pattern 1: One limit function, per-page, header-only
**What:** `ImageSizeLimit::exceeds(const QSize&)` is the only place that knows the numbers; every consumer calls it with header values *before* allocating.
**When to use:** `TiffReader::readImage` (after `TiffInfo`, before either `QImage` constructor), `JpegMetadataLoader` (between `jpeg_read_header` and `jpeg_start_decompress`), `ImageLoader::load` (after `reader.size()`), `ProjectFilesDialog::FileList::loadNextFile`, `MainWindow::showInsertFileDialog`.
**Example:**
```cpp
// Source: this research; numbers measured below. New file src/core/ImageSizeLimit.h
namespace image_size_limit {
constexpr qint64 kMaxPixels = qint64(1) << 29;  // 536,870,912
constexpr int kMaxSide = 65535;
inline bool exceeds(const QSize& s) {
  // width()/height() are int; a TIFF width of 2^32-1 arrives here as a negative int (TiffInfo reads uint32 into int).
  return s.width() <= 0 || s.height() <= 0 || s.width() > kMaxSide || s.height() > kMaxSide
         || qint64(s.width()) * s.height() > kMaxPixels;
}
}  // namespace image_size_limit
```
Note the sign trap: `TiffReader::TiffInfo` declares `int width;` and calls `TIFFGetField(..., &width)` for a `uint32` tag `[VERIFIED: src/core/TiffReader.cpp:84,121]`, so a header value above 2^31-1 is negative there. Treat `<= 0` as "exceeds" in the reader (a zero-size image is also unusable), but read the width into `uint32_t` first in the new check to report the true claimed size.

### Pattern 2: Thread cap as a pure function plus one setter
**What:** `MemoryBudget::threadsForMemory(qint64 physicalBytes, qint64 pagePixels, double budgetShare = 0.5, qint64 bytesPerPixel = 40)` returns `max(1, floor(share*phys / (bpp*pixels)))`. `WorkerThreadPool` stores the cap (`setMemoryCap(int)`), `updateNumberOfThreads()` applies `min(ideal, slider, cap)`. `SystemLoadWidget::setMemoryCap(int)` only changes the tooltip.
**Why a setter and not a callback:** `updateNumberOfThreads()` runs on every `submitTask`; a project-wide scan per submit is wasteful. `startBatchProcessing()` already loops over every `PageInfo` (it calls `loadDefaultSettings(page)` there), so computing the maximum in the same loop costs nothing.

### Pattern 3: Same-class translation-unit split
**What:** Move member function definitions verbatim into `MainWindowBatch.cpp` etc.; `MainWindow.h` and the `.ui`/moc setup stay unchanged, `SIGNAL()/SLOT()` strings and `QObject` names are untouched. New files join `gui_only_sources` in `src/app/CMakeLists.txt`.

### Anti-Patterns to Avoid
- **Checking after the allocation:** the unmodified reader allocates and fills the whole claimed image (measured below). The check must precede `QImage(w, h, fmt)` and `TIFFReadRGBAImageOriented`.
- **Using `QImageReader` default limit as the policy:** it silently rejects real scans above 64 MP and hides the reason (`qt.gui.imageio` debug line only).
- **Deleting `#if 0 … #endif` wholesale:** five of the eight blocks wrap the *dead* branch of an `#if 0 … #else … #endif`; the `#else` branch is live code (see CLEAN-03).
- **Hashing raw QImage bytes of `Format_Mono` outputs:** padding bits beyond the image width are not deterministic (two identical runs gave different SHA-1 over raw bytes). Convert to `Format_Grayscale8` first.
- **`otool -L | grep -i opengl` as the STRIP-07 check:** `QtGui` and `QtWidgets` themselves link the system `OpenGL.framework` `[VERIFIED: otool -L QtGui, QtWidgets]`, so a case-insensitive grep never goes empty. Grep for `QtOpenGL` only.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Physical RAM | Parsing `vm_stat`/`top` output | `sysctlbyname("hw.memsize", ...)` | One call, 64-bit, no parsing |
| Image header size for PNG/JPEG in `ImageLoader` | A second header parser | `QImageReader::size()` | Measured: returns the true claimed size for PNG without allocating and at any Qt limit |
| Image header size for the metadata path | New parsers | The existing `PngMetadataLoader`/`JpegMetadataLoader`/`TiffReader::readMetadata` | Already read headers; JPEG only needs a reorder |
| XML escaping of tab/LF/CR/`&`/`<`/`"` | A custom escaper | `QDomElement::setAttribute` (Qt 6.12 already emits `&#x9;`, `&#xd;`, `&amp;`, `&lt;`, `&quot;`) | Measured below; only XML-1.0-invalid characters need custom handling |
| Thread throttling | Own semaphore | `QThreadPool::setMaxThreadCount` + existing `hasSpareCapacity()` loops | Both submit loops in `MainWindow` already stop at `activeThreadCount() >= maxThreadCount()` |
| Pure-move proof | Eyeballing a diff | The chunk-multiset script plus pre/post output digests (recipe below) | A 2814-line file cannot be reviewed by eye |

**Key insight:** every hazard here is already handled by a library at the *wrong* layer or with the wrong default (libtiff trusts the header, Qt's reader has an undocumented 256 MB default, QDom cannot write XML-1.0-invalid characters). The fix is a thin policy layer in front, not a new mechanism.

## Runtime State Inventory

This is a removal/refactor phase for OpenGL (STRIP-07) and a move-only phase for the splits, so the five categories are answered explicitly.

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | The user's settings file may hold `settings/enable_opengl` (key built as `ROOT_KEY + '/' + "enable_opengl"` `[VERIFIED: src/core/ApplicationSettings.cpp:32-33,58-59]`) and `settings/batch_processing_threads` `[VERIFIED: src/core/WorkerThreadPool.cpp:87]`. Not inspected (home-folder rule) | **Code edit only.** After STRIP-07 nothing reads `enable_opengl`; it is neither deleted nor rewritten (D-00c). `batch_processing_threads` stays as stored; the RAM cap is applied on top and never written back |
| Live service config | None — verified: no external services in the project (desktop app, local files) | None |
| OS-registered state | None — verified: the app registers no launchd/Task Scheduler/Login items; the embedded `Info.plist` section names the app only | None |
| Secrets/env vars | None for this phase. `QT_IMAGEIO_MAXALLOC` is a Qt env override of the allocation limit `[CITED: doc.qt.io/qt-6/qimagereader.html]`; the app reads no environment variables `[VERIFIED: CLAUDE.md "the app reads none"]` | None; the plan should not rely on the variable |
| Build artifacts / installed packages | `build/<preset>` directories hold binaries linked against `QtOpenGL`/`QtOpenGLWidgets` `[VERIFIED: otool -L on scantailor-advanced, qt_tests, mac_check, retina_check]` and stale untracked `.qm`/`.desktop` files (STATE Blockers) | Rebuild is automatic; `cmake --fresh` only if the stale CMake cache complains about `Qt6::OpenGL`. Stale files are harmless |

## Common Pitfalls

### Pitfall 1: Qt rejects every PNG/JPEG above 64 Mi pixels, whatever its byte size
**What goes wrong:** An A3 600 dpi scan (7016 x 9921 = 69.6 MP) fails to load, even as an 8-bit grey PNG of 70 MB, and a 1200 dpi A4 (139 MP) fails.
**Why it happens:** Measured against Qt 6.12.0 with an unmodified `QImageReader`: default `allocationLimit()` = 256 MB; 8192x8192 (67,108,864 px) passes, 8192x8193 is rejected; mono 14031x19843 (35 MB of data) is rejected. The check evidently counts 4 bytes per pixel.
**How to avoid:** `QImageReader::setAllocationLimit(2048)` once, inside `ImageLoader` (`std::call_once`), so every consumer (app, tests, thumbnails, output reload) gets it. 2048 MB is exactly 4 x 2^29 bytes, i.e. the same ceiling. The setter is static and global; there is no per-reader variant `[VERIFIED: qimagereader.h:110-111 lists only the two static functions]`.
**Warning signs:** `qt.gui.imageio: QImageIOHandler: Rejecting image as it exceeds the current allocation limit of 256 megabytes` on stderr and a null image.

### Pitfall 2: JPEG metadata loading allocates the whole image for progressive files
**What goes wrong:** Adding a file to a project reads its "metadata" and, for a progressive JPEG with a forged 65500x65500 header, takes 2.8 s and 12.3 GB RSS before returning `LOADED` with the absurd size.
**Why it happens:** `JpegMetadataLoader` calls `jpeg_start_decompress` only to detect unsupported compression `[VERIFIED: src/core/JpegMetadataLoader.cpp:195-206]`; for multi-scan files libjpeg realizes coefficient arrays for the whole image there.
**How to avoid:** After `jpeg_read_header`, read `image_width/image_height/density` (valid at that point), run `ImageSizeLimit::exceeds`; if it does, emit the metadata and return `LOADED` *without* calling `jpeg_start_decompress`. Keep the call for sizes inside the limit so "unsupported compression" detection is unchanged.
**Warning signs:** Project creation stalls with high memory on a tiny file.

### Pitfall 3: Forged TIFF header costs the full claimed image today
Measured on the unmodified `TiffReader` (Release, scratch program): 13 KB file claiming 40000x40000 RGB returns a non-null 6.1 GB image after 2.5 s; 10 KB mono file claiming 100000x100000 returns a 1.2 GB image; 60000x60000 8-bit grey returns 3.5 GB. So the reader neither crashes nor errors; it allocates, touches the memory and hands back a fabricated image. See "SAFE-01" for the table.

### Pitfall 4: `int` pixel arithmetic caps the usable ceiling below 2^31
`const int pixelCount = image.width() * image.height();` `[VERIFIED: src/imageproc/Posterizer.cpp:221]`, `const int numDataPoints = src.width() * src.height();` `[VERIFIED: src/imageproc/PolynomialSurface.cpp:34]`, `const int lhsArea = lhs.width() * lhs.height();` `[VERIFIED: src/imageproc/MaxWhitespaceFinder.cpp:16]`. Any pixel ceiling at or above 2^31 would make these overflow; 2^30 is the largest safe power of two and 2^29 keeps RGB32 byte counts below 2^31 as well. [Corrected at plan revision 2: only if the limit is exclusive; exactly 2^29 px take 2^31 bytes as RGB32, one more than INT_MAX, so plan 06-04 accepts fewer than 2^29 px (kMaxPixels = 536870911).]

### Pitfall 5: `TestMacQt::testBatchAlarmUsesBeep` reads `src/app/MainWindow.cpp` by path
It requires `m_batchQueue->allProcessed()` followed by `QApplication::alert(this);`, `m_checkBeepWhenFinished()` and `QApplication::beep();` in that file and forbids shell-command words there `[VERIFIED: src/qt_tests/tst_mac_qt.cpp:1433-1452]`. Moving `filterResult` to another file makes the test fail (`start >= 0` check). The CLEAN-02 commit that moves it must change the test to read the new file (or all `src/app/MainWindow*.cpp`), keeping the control strings.

### Pitfall 6: `#if 0` blocks with a live `#else`
See CLEAN-03; a regex `delete from #if 0 to #endif` destroys live code in `IncompleteThumbnail.cpp`, `SavGolFilter.cpp`, `PageLayoutEstimator.cpp` and twice in `VertLineFinder.cpp`.

### Pitfall 7: Peak memory follows output pixels, and the default output DPI is 600
A 300 dpi source with 600 dpi output measured 106 B per *source* pixel (mixed colour) against 38 B at equal DPI; 300 dpi to 1200 dpi output measured 360 B per source pixel. An estimator on source pixels alone under-estimates by 4x for the common 300 dpi scan.

### Pitfall 8: Gate bookkeeping
`gate.sh check` compares build commands with the reference stored for `HEAD~1`'s input tree. The reference exists only if `snapshot` ran on a commit with the same input tree or a previous `check` ran on it. Run `bash .planning/phases/03-qt-6-and-macos-only/gate.sh snapshot` once on the commit before the first source-changing Phase 6 commit, and always commit before `check` (the gate refuses uncommitted inputs).

### Pitfall 9: `JpegMetadataLoader` baseline vs progressive
Baseline JPEGs with forged dimensions were harmless (19 MB, 0 ms); only progressive ones allocate. The test for JPEG must use a progressive forged file, otherwise it passes on unmodified code and proves nothing (include the negative control).

## Code Examples

### Forge-and-reject test (SAFE-01), in-memory, no files
```cpp
// core_tests (Boost). TiffReader::readImage and ImageLoader::load(QIODevice&, int) accept any seekable QIODevice,
// so QBuffer needs no temporary file and no /tmp.
// Peak-memory guard: mach task_info phys_footprint before/after; unmodified code grows by >= 1.2 GB for the
// 100000x100000 mono forgery (measured), so a 64 MB bound is a real negative control.
BOOST_AUTO_TEST_CASE(test_forged_tiff_is_refused_without_allocating) {
  QByteArray tiff = makeMinimalLittleEndianTiff(/*w=*/100000, /*h=*/100000, /*bps=*/1);  // ~200 bytes, hand-built IFD
  QBuffer dev(&tiff);
  dev.open(QIODevice::ReadOnly);
  const double before = footprintMb();
  const QImage img = TiffReader::readImage(dev, 0);
  BOOST_CHECK(img.isNull());
  BOOST_CHECK_LT(footprintMb() - before, 64.0);
}
```
Source for the numbers: measurement in "SAFE-01" below. `makeMinimalLittleEndianTiff` is a ~30-line helper writing an IFD with tags 256, 257, 258, 259, 262, 273, 277, 278, 279 (libtiff-written forged files read fine, so those tags suffice) `[ASSUMED: hand-built IFD accepted by libtiff; the measured files were written with libtiff itself]`. Alternative: link `TIFF::TIFF` into `core_tests` and forge with libtiff exactly as the scratch program did.

### Memory-cap arithmetic
```cpp
// Source: this research. Pure, testable; physicalBytes injected by the caller (sysctl in production).
inline int threadsForMemory(qint64 physicalBytes, qint64 pagePixels, double share = 0.5, qint64 bytesPerPixel = 40) {
  if (physicalBytes <= 0 || pagePixels <= 0) return 1 << 20;  // "no cap": caller takes min() with the other limits
  const double perTask = double(bytesPerPixel) * double(pagePixels);
  return std::max(1, int(std::floor(share * double(physicalBytes) / perTask)));
}
```

### Shared Save-as flow shape (CLEAN-05)
```cpp
// Source: this research, from MainWindow.cpp:1425-1458 and OutOfMemoryDialog.cpp:50-83 (identical bodies).
class ProjectSaveFlow {
  Q_DECLARE_TR_FUNCTIONS(ProjectSaveFlow)
 public:
  // Native panel; returns the chosen path with ".ScanTailor" appended when missing, or an empty string.
  static QString chooseProjectFile(QWidget* parent, const QString& currentProjectFile);
  static QString withProjectExtension(QString path);          // unit-testable
  // ProjectWriter + QMessageBox::warning(tr("Error"), tr("Error saving the project file!")) on failure.
  static bool writeWithFeedback(QWidget* parent, const QString& file, const std::shared_ptr<ProjectPages>&,
                                const SelectedPage&, const OutputFileNameGenerator&, const StageSequence&);
  // QSettings project/lastDir and RecentProjects::setMostRecent  (unit-testable under build/ settings)
  static void remember(const QString& file);
};
```
The three differences between the two callers are only what happens after success (`updateWindowTitle()` vs `showSaveSuccessScreen()`), which stays in the callers.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `QImageReader` unbounded | Allocation limit 256 MB default, `setAllocationLimit` since Qt 6.0 | Qt 6.0 `[CITED: doc.qt.io/qt-6/qimagereader.html]` | App must raise it deliberately |
| `QDomDocument::setContent(QIODevice*)` returning `bool` | Returns `ParseResult` (explicit `operator bool`); `MainWindow::openProject` still compiles via contextual conversion | Qt 6.5+ `[VERIFIED: compile error "no viable conversion from 'ParseResult' to 'const bool'" in scratch, while `if (!doc.setContent(&file))` compiles in MainWindow.cpp:1499]` | Use `static_cast<bool>` in new code |
| `BOOST_FOREACH` | range-for | C++11 | 26 stale includes only |

**Deprecated/outdated:** `boost/foreach.hpp` (no uses), the 32-bit branches `sizeof(void*) <= 4` in `WorkerThreadPool.cpp:83` and `SystemLoadWidget.cpp:20` and the hidden `only_32bit_*` widgets in `OutOfMemoryDialog.cpp` constructor (dead on arm64; the latter is outside D-07, trivial to remove while that file is open).

---

## SAFE-01 detail

### D-01 ceiling (exact numbers and why)
| Quantity | Value | Evidence |
|----------|-------|----------|
| Reference page that must load | 1200 dpi A3: 14031 x 19843 = 278,417,133 px (about 278 MP); RGB32 = 1.11 GB | `[VERIFIED: scratch: mono 1200 dpi A3 was 14031x19843, sizeInBytes 34,844,308 mono]`; RGB32 bytes = 4 x px |
| **Total pixel ceiling** | **536,870,912 (2^29)**, 1.93x the reference page | choice |
| **Per-side ceiling** | **65,535**, 3.3x the longest reference side (19,843) | matches D-01 example; JPEG's own maximum is 65,500 `[CITED: libjpeg JPEG_MAX_DIMENSION; the forged 65500 header parsed in scratch]` |
| Why not 1 gigapixel | `int` pixel-count arithmetic exists in `imageproc` (Pitfall 4); 2^29 also keeps an RGB32 image under 2^31 bytes, so any `int` byte count in the unaudited code cannot overflow `[ASSUMED: byte-count sites not audited; pixel-count sites verified]` [corrected at plan revision 2: true only for fewer than 2^29 px; plan 06-04 makes the limit exclusive and plan 06-06 audits the byte-count sites] | If the user prefers D-01's 1 GP example, the largest defensible value is 2^30 = 1,073,741,824 (still below 2^31) and the Qt limit becomes 4096 MB |
| Qt limit to set | `QImageReader::setAllocationLimit(2048)` (4 x 2^29 B = 2048 MB; Qt accepts exactly 256 MB = 4 x 2^26 B, as measured at 8192x8192) | measured |
| Rejected examples | 1200 dpi A2 (about 557 MP) and 600 dpi A0 (about 558 MP): above the ceiling by design | arithmetic |

### D-03 measurements (Qt 6.12.0, Release, `build/scratch/phase6/m1`, not committed)
| Test file | Claimed size | `allocationLimit` 256 (default) | 2048 | 0 (off) |
|-----------|--------------|--------------------------------|------|---------|
| RGB32 PNG, 234 KB on disk | 7016x9921 (A3 600 dpi, 278,422,944 B decoded) | **fails** (`Unable to read image data`) | loads, 145 ms | loads, 124 ms |
| RGB32 JPEG, 4.5 MB | 7016x9921 | **fails** | loads, 96 ms | — |
| 8-bit grey PNG | 7016x9921 (69.6 MB decoded) | **fails** | — | — |
| mono PNG | 14031x19843 (34.8 MB) | **fails** | — | — |
| RGB32 PNG | 2480x3508 (A4 300 dpi) | loads | — | — |
| RGB PNG forged IHDR | 8192x8192 / 8192x8193 | passes the limit / **rejected** | — | — |
| RGB PNG forged IHDR | 100000x100000 and 65535x65535 | rejected | rejected | with limit 0 `QImageReader::read` returned an image object of the claimed size (40 GB virtual) before failing on missing data: **limit 0 must not be used** |

Result: **yes, a 278 MB RGB32 PNG/JPEG fails to load on the unmodified code**, and so does any PNG/JPEG above 64 Mi pixels. `QImageReader::size()` returned the true claimed size in all cases without allocating.

### Measured cost of forged headers on the unmodified code
| Forged file (size on disk) | Path | Result today |
|----------------------------|------|--------------|
| TIFF 1-bit 30000x30000 (3.6 KB) | `TiffReader::readImage` | image returned, 116 MB footprint |
| TIFF RGB 12000x12000 (4.8 KB) | `readImage` | image returned, 572 MB RSS |
| TIFF 1-bit 100000x100000 (10 KB) | `readImage` | image returned, **1.2 GB** RSS, 287 ms |
| TIFF 8-bit grey 60000x60000 (9.8 KB) | `readImage` | image returned, **3.5 GB** RSS |
| TIFF RGB 40000x40000 (13 KB) | `readImage` | image returned, **6.1 GB** RSS, 2.5 s |
| TIFF width = height = 2^32-1 | `readImage` | libtiff rejected the directory; null image, no allocation |
| TIFF 2,000,000,000 x 2 mono | `readImage` | accepted, 496 MB RSS |
| JPEG baseline forged 65500x65500 | `JpegMetadataLoader` | 19 MB, 0 ms, size reported |
| JPEG **progressive** forged 65500x65500 (about 1 KB) | `JpegMetadataLoader` | **2.8 s, 12.3 GB RSS** |
All `readMetadata` calls reported `LOADED` with the claimed size (no check there today). Files and programs: `build/scratch/phase6/m2` and `m5` (gitignored).

### D-04 allocation-site inventory and where the check goes
| Site | Line | What allocates | Action |
|------|------|----------------|--------|
| `TiffReader::TiffInfo` | 84, 121 | none, but reads `uint32` tags into `int` | read true `uint32` size for the check |
| `TiffReader::readImage` general path | 279-282 | `QImage(info.width, info.height, RGB32/ARGB32)`; `throw std::bad_alloc()` on null | check **before** line 279 (after `TiffInfo`), return `QImage()` (not an exception: `OutOfMemoryHandler` must not fire) |
| same, tmp buffer | 296 | `TiffBuffer<uint32_t>(info.width * info.height)` (int overflow; only used if `bytesPerLine != 4*width`, which RGB32/ARGB32 never has) | covered by the same check; dead path |
| `extractBinaryOrIndexed8Image` | 383-386 | `QImage(info.width, info.height, format)`; `throw std::bad_alloc()` | covered by the check before the `if (info.mapsToBinaryOrIndexed8())` branch at 275 |
| `readAndUnpackLines` | ~446 | scanline buffer from the header's scanline size | bounded once width <= 65,535 |
| `TIFFReadRGBAImageOriented` | 288 | libtiff internal strip/tile buffers sized from the header | not reached after the check |
| `TiffReader::readMetadata` | 207-231 | none (directory reads only) | no check inside; consumers check each page's size. Per-page: the loop already calls `out()` once per page |
| `JpegMetadataLoader::loadMetadata` | 195-206 | `jpeg_start_decompress` (progressive: whole image) | check between header read and start_decompress |
| `PngMetadataLoader::loadMetadata` | 61-98 | none (`png_read_info` only; libpng's own default user limit is 1,000,000 per side) | consumers check |
| `ImageLoader::load(QIODevice&, int)` | 37-46 | `QImageReader::read` | `reader.size()` check before `read`; if `size()` is invalid, fall through (Qt then fails or applies its limit) |
| `ThumbnailPixmapCache::loadSaveThumbnail` | 691, 696 | two `ImageLoader::load` calls | no change; refusal inherited; returns null → `LOAD_FAILED` (the usual failed-thumbnail state) |
| Other `ImageLoader::load` callers | `LoadFileTask.cpp:58`, `filters/output/Task.cpp:261-294` (reload of cached output and mask files), `HeadlessPipeline.cpp:160,197` | one `QImage` | inherited |
| `ProjectFilesDialog::FileList::loadNextFile` | 565-590 | none | per-page check → new `STATUS_TOO_LARGE`, red text, display text with claimed size, `LOAD_FAILED` return so the existing "Some of the files failed to load" box appears (adjust wording to mention size) |
| `MainWindow::showInsertFileDialog` metadata load | 1970-1982 | none | treat as failed; shows in the existing `LoadFilesStatusDialog` failed list |

**Status flow:** no new exception. `QImage()` (null) is the existing failure contract of `readImage` ("or a null image in case of failure" `[VERIFIED: src/core/TiffReader.h doc comment]`). To tell "too large" from "unreadable", `LoadFileTask::operator()` calls `ImageSizeLimit::checkFile(m_imageId, &claimed)` only when the image came back null (a header re-read, microseconds) and builds `ErrorResult` with a third state. `ErrorResult::updateUI` currently has two messages (file exists / does not exist) `[VERIFIED: src/core/LoadFileTask.cpp:115-135]`; add the third with `Qt::PlainText`.

### D-05 test design (all in `core_tests`, in-memory)
1. Forged TIFF 100000x100000 mono, forged TIFF RGB 40000x40000, forged TIFF with `uint32` dims above 2^31: `readImage` null, footprint growth < 64 MB (negative control: today 1.2-6.1 GB).
2. TIFF at the limit loads; one pixel over, and a side of 65,536, are refused: width 65535 x height 1 (loads), 65536 x 1 (refused), 32768 x 16384 mono (= 2^29, loads, 64 MB), 32768 x 16385 (refused).
3. Multi-page TIFF: page 0 normal, page 1 forged: page 0 loads, page 1 refused (needs a two-IFD forge; hand-built or libtiff).
4. PNG: forged IHDR 100000x100000 via `ImageLoader::load(QIODevice&, 0)` is refused, footprint small; a real 9000x8000 8-bit grey PNG (72 MP, above Qt's old 64 MP limit) loads (D-03 positive).
5. JPEG: **progressive** forged 65500x65500 through `JpegMetadataLoader` returns `LOADED` with the claimed size in under one second and under 64 MB; the same forgery today costs 12.3 GB.
6. `ImageSizeLimit::exceeds` table test (negative, zero, boundary values).
Under `macos-asan` these run without an out-of-memory report; the footprint guard is the real proof, ASan is the second.

## SAFE-02 detail

### D-10 measurements (Qt 6.12.0 `QDomDocument`, write with `QTextStream` and `doc.save(strm, 2)` like `ProjectWriter::write` lines 105-109, read with `setContent(QByteArray)` and with `setContent(QIODevice*)` as `MainWindow::openProject` does)
Attribute value (`<file name="...">`), the only place names appear: `ProjectWriter.cpp:87` (`outputDirectory`), `:120` (directory `path`), `:135` (file `name`); read at `ProjectReader.cpp:29,107,140` `[VERIFIED: grep -n]`. The element-text path (`XmlMarshaller::string`/`XmlUnmarshaller::string`) carries only numbers and the layout-type token `[VERIFIED: grep of marshaller.string uses]`.

| Case | Round trips today? | Written form |
|------|--------------------|--------------|
| `&`, `<`, `>`, `"`, `'`, NFC, NFD, emoji U+1F4C4, backslash | yes (both read paths) | normal |
| Tab, LF, CR, CRLF | **yes** | `&#x9;`, `&#xd;` (Qt escapes them; CONTEXT assumed they become spaces; they do not) |
| Leading, trailing, doubled spaces | yes in attributes | literal |
| U+007F, U+0085, U+00A0, U+2028, `]]>`, U+FEFF | yes | literal |
| **U+0001, U+001F** (all XML-1.0-invalid controls) | **no: written raw, file does not parse** (`Unexpected ''`) | raw byte |
| U+FFFE, U+FFFF | no, same | raw |
| lone surrogate | silently becomes U+FFFD | — |
| Only-spaces element text | lost (text node dropped) | irrelevant for names |
| `&#x1;` / `&#1;` / XML 1.1 declaration | **parser rejects** (`Invalid character reference.`, `Unsupported XML version.`) | cannot be used as a fix |

Filesystem reach (APFS, probed with Python in `build/scratch`): file names with tab, LF, CR, leading/trailing space, `'`, `>`, emoji, U+0001, U+001F, U+007F, backslash and a leading `..` can all be created; **U+FFFE and U+FFFF are refused by APFS (`EILSEQ`)** and lone surrogates cannot occur (UTF-8). So the only realistic failing class is **U+0001-U+0008, U+000B, U+000C, U+000E-U+001F** in a file or directory name (an absurd but legal name; today it makes the project file unloadable with "The project file is broken.").

### Recommended fix (keeps ordinary files byte-identical)
A reversible escape applied **only when a value contains an XML-1.0-invalid character**, announced by an extra attribute so a literal `%` in an ordinary name never changes: e.g. `name="a%01b" nameEscaped="1"`, percent-encoding `%` and every invalid code point as UTF-8 `%XX`. Reader: if `nameEscaped == "1"` percent-decode. Helper pair in `XmlMarshaller`/`XmlUnmarshaller` (foundation, unit-testable), called at the six attribute sites. Ordinary projects get no new attribute, so `TestProjectRoundTrip::test_round_trip_is_byte_identical` and the Phase 2 golden text stay valid. If the planner prefers not to add format surface for a name no one uses: mark `XFAIL_CHECK` with a new backlog item and make `ProjectWriter::write` return `false` (the existing "Error saving the project file!" box) instead of writing an unloadable file. Either is allowed by D-10; the escape is the one that satisfies SAFE-02's wording ("survive saving and reloading").

### Test plan (extend `src/pipeline_tests/TestProjectRoundTrip.cpp`, 465 lines, fixture creates PNGs named from `nameWithMarkup()`…)
- Add a second name set built in the same fixture style: tab, LF, CR, leading/trailing space, `it's`, `a>b`, emoji, U+0001, U+001F, backslash. Assert with the existing helpers (`roundTrip`, `result.readPagePaths` compared as code-unit sequences, byte-identical file1/file2).
- Backslash: **expected to fail** through `RelinkablePath::normalize` (`replace('\\', "/")` `[VERIFIED: src/core/RelinkablePath.cpp:11-12]`) only where that class is applied; the plan must observe first, then wrap in `XFAIL_CHECK(cond, "999.8")` if it fails (ExpectedFailure trip-wire raises an error when it starts passing).
- D-09: replace the FIXME at `src/foundation/XmlUnmarshaller.cpp:14` (`return el.text();  // FIXME: this needs unescaping, but Qt doesn't provide such functionality`) with a comment that `QDomElement::text()` returns unescaped text.
- The existing `ProjectRoundTripTestSuite` has no expected-failure marks to remove (Phase 2 record `round-trip.md`).

## SAFE-03 detail

### Where the thread count lives today
- `WorkerThreadPool::updateNumberOfThreads()` (lines 79-90): `idealThreadCount()`, 32-bit clamp, `QSettings` key `settings/batch_processing_threads` default = ideal, `min`, `setMaxThreadCount`. Called from the constructor and **on every `submitTask`** `[VERIFIED: src/core/WorkerThreadPool.cpp:56,86-89]`.
- `SystemLoadWidget` (constructor lines 14-36): same key, same 32-bit clamp, slider range `1..idealThreadCount`, removes the key when the slider is at the maximum `[VERIFIED: src/app/SystemLoadWidget.cpp:12,41-47]`. Tooltip text is `QString("%1/%2").arg(threads).arg(m_maxThreads)`.
- Concurrency is bounded by `hasSpareCapacity()` = `activeThreadCount() < maxThreadCount()` and the two submit loops in `MainWindow::startBatchProcessing` and `filterResult` `[VERIFIED: MainWindow.cpp:1263-1270,1360-1364]`. Lowering `maxThreadCount` therefore throttles the whole batch with no other change.
- `SystemLoadWidget` lives in `BatchProcessingLowerPanel.ui` (created in `MainWindow::createBatchProcessingWidget`), so it has no project access: push the cap in with `setMemoryCap(int)`.

### Memory model, measured (Release, one page task through all six stages, `build/scratch/phase6/m4`, not committed)
Harness: `HeadlessPipeline::runPipeline` on `test_support` pages, one process per run, `phys_footprint` peak minus the 4 MB start footprint. The harness holds one extra copy of the input and the decoded output, so values are mildly conservative. Generator pages are small (1000x1150 px at 300 dpi), so larger sizes use 600/1200 dpi and, beyond that, `QImage::scaled` (nearest) by 2 (73.6 MP).

| Page kind, colour, dewarp | Source px | Output px | Peak above start | B per source px | B per max(source, output) px |
|---------------------------|-----------|-----------|------------------|-----------------|------------------------------|
| flat, B/W, off, 300 dpi | 1.15 MP | 0.69 MP | 31 MB | 28 | 27 |
| flat, B/W, off, 1200 dpi | 18.4 MP | 11.0 MP | 349 MB | 19.9 | 19.0 |
| flat, grey, off, 1200 dpi | 18.4 MP | 11.0 MP | 322 MB | 18.3 | 17.5 |
| mixed, mixed colour, off, 600 dpi | 4.6 MP | 3.9 MP | 145 MB | **33.1** | 31.5 |
| mixed, mixed colour, off, 1200 dpi | 18.4 MP | 15.4 MP | 551 MB | 31.4 | 30.0 |
| mixed, mixed colour, off, 2400 dpi | 73.6 MP | 61.8 MP | 2178 MB | 31.0 | 29.6 |
| curved, mixed colour, auto dewarp, 600 dpi | 4.6 MP | 3.4 MP | 127 MB | 28.9 | 27.6 |
| curved, mixed colour, auto dewarp, 1200 dpi | 18.4 MP | 13.7 MP | 449 MB | 25.6 | 24.4 |
| curved, mixed colour, auto dewarp, 2400 dpi | 73.6 MP | 54.2 MP | 1733 MB | 24.6 | 23.5 |
| mixed 300 dpi source, **output 600 dpi** | 1.15 MP | 3.9 MP | 117 MB | **106** | 30 |
| mixed 300 dpi source, **output 1200 dpi** | 1.15 MP | 15.4 MP | 395 MB | **361** | 26 |
| curved/auto, 300 dpi source, output 1200 dpi | 1.15 MP | 13.7 MP | 334 MB | 305 | 24 |

Conclusions: (a) per pixel of the **larger of source and output** the peak is 17-32 B and is stable from 1 MP to 74 MP (no superlinear growth); worst measured 33.1; (b) dewarping did not raise the peak above the colour-mixed no-dewarp case on these pages; (c) the factor does **not** scale with source pixels alone. **Recommend 40 B per pixel of max(source px, estimated output px)**, about 1.2x the worst measured value. The measured numbers come from a harness, not from the app (the app binary must not be launched), so the factor is `[ASSUMED]` to cover the app's extra caches until the headless checkpoint confirms it.

`outputPx = sourcePx x (outputDpi / sourceDpi)^2`. Source DPI is in `PageInfo::metadata().dpi()`; per-page output DPI is `output::Settings::getParams(pageId).outputDpi()` `[VERIFIED: src/core/filters/output/Settings.h:42, Params.h:35]`, but `output::Filter::m_settings` is private `[VERIFIED: Filter.h:72]`, so either add a small accessor or take the project's default output DPI from `DefaultParamsProvider`. A cheaper conservative rule that needs neither: `P = max(srcPx, srcPx x (600/srcDpi)^2)` with 600 = the app default; the plan should state which rule it takes. Never take P below the source pixel count.

### Budget share and example caps
Budget = **50% of `hw.memsize`** (the rest is left to the OS, the GUI, the thumbnail cache and compressed memory). On this Mac: 68,719,476,736 B -> budget 34,359,738,368 B.

| Page (output 600 dpi) | P (px) | Per task at 40 B/px | Cap on 64 GiB | Cap on a 16 GiB Mac |
|-----------------------|--------|---------------------|---------------|---------------------|
| A4 300 dpi | 34.8 M (output) | 1.39 GB | 24 -> 10 (ideal) | 6 |
| A4 600 dpi | 34.8 M | 1.39 GB | 24 -> 10 | 6 |
| A3 600 dpi | 69.6 M | 2.78 GB | 12 -> 10 | 3 |
| A4 1200 dpi | 139.2 M | 5.57 GB | 6 | 1 |
| A3 1200 dpi | 278.4 M | 11.14 GB | 3 | 1 |

(Physical figures: 16 GiB budget 8.59 GB.) Small scans keep full parallelism; only large ones are capped, as D-06 asks.

### D-08 proof design
- Unit tests (`core_tests`, new `TestMemoryBudget.cpp`): table above as test vectors, minimum 1, `physicalBytes <= 0`, slider smaller than the cap, ideal smaller than the cap; and a `WorkerThreadPool`/`QThreadPool` check that `maxThreadCount` equals `min(ideal, slider, cap)` after `setMemoryCap` (needs a `maxThreadCount()` accessor or a `static effectiveThreads()`).
- Headless run (checkpoint, user's go): no committed code needed. Launch K copies of a scratch runner in parallel (one process per page task, K = the computed cap for a **simulated** small machine, e.g. pretend 16 GiB and 73.6 MP pages: cap 2, about 4.4 GB real), sample `vm.swapusage` (`sysctl vm.swapusage`) before, during (every few seconds) and after, and record each process's peak footprint. This Mac currently reports `vm.swapusage: total = 0,00M  used = 0,00M  free = 0,00M`. Do not run the uncapped comparison (10 x 2.2 GB of 73.6 MP pages is 22 GB on a machine whose free memory was about 1.5 GB at research time). The simulated machine keeps the run small enough not to disturb the user's session; D-08 still requires the go.
- Optional UAT (user): Activity Monitor on real scans.
- Superseded at plan revision (2026-11-16): the simulated-machine run above does not exercise `startBatchProcessing`'s cap on this 64 GiB Mac. The D-08 run (plan 06-16) instead drives ten 12000 x 13800 synthetic pages through a real `WorkerThreadPool` with `setMemoryCap(MemoryBudget::batchMemoryCap(sequence))` from the real `hw.memsize` (expected cap 5 of 10 threads), using the batch probe of plan 06-09; the uncapped comparison stays forbidden.

## STRIP-07 detail

### Complete inventory (grep `OpenGL`, `QOpenGL`, `opengl`, `OpenGl` over `src/`, top-level CMake, TESTING.md, README.md)
| File | Reference | Action |
|------|-----------|--------|
| `CMakeLists.txt:105` | `find_package(Qt6 6.5 COMPONENTS Core Gui Widgets Xml Network OpenGL Svg OpenGLWidgets Test CONFIG REQUIRED)` | drop `OpenGL`, `OpenGLWidgets` |
| `src/core/CMakeLists.txt:67` | `OpenGLSupport.cpp OpenGLSupport.h` | remove from `common_sources` |
| `src/core/CMakeLists.txt:127` | `PUBLIC Qt6::Core ... Qt6::OpenGL Qt6::Svg Qt6::OpenGLWidgets` | drop both |
| `src/core/OpenGLSupport.{h,cpp}` | `supported()`, `deviceName()`, `QOpenGLContext` | delete |
| `src/core/ImageViewBase.cpp:11,28,158-162` | `#include <QtOpenGLWidgets/QOpenGLWidget>`, `#include "OpenGLSupport.h"`, `if (ApplicationSettings::getInstance().isOpenGlEnabled()) { if (OpenGLSupport::supported()) { setViewport(new QOpenGLWidget()); } }`; comment lines 152-155 mention the OpenGL viewport | delete the branch and the two includes, adjust comment; check that `ApplicationSettings.h` is still needed in this file |
| `src/core/ImageViewBase.h:149` and `.cpp:279-297` | `sampleImageAtWidgetRect` (doc comment: "so it works with OpenGL (issue #89)") | delete the function and declaration; only caller is `ColorPickupInteraction.cpp:74` `[VERIFIED: grep]` |
| `src/core/filters/output/ColorPickupInteraction.cpp:73-76` | `if (ApplicationSettings::getInstance().isOpenGlEnabled()) { image = m_context.imageView().sampleImageAtWidgetRect(rect); }` | delete; the `grab(rect)` path that follows remains the only path; remove the `ApplicationSettings.h` include if unused |
| `src/core/ApplicationSettings.{h,cpp}` | `isOpenGlEnabled`, `setOpenGlEnabled`, `DEFAULT_OPENGL_STATE` (`false`), `OPENGL_STATE_KEY` (`"enable_opengl"`) | delete all four; key stays untouched in the user's file (D-00c) |
| `src/app/SettingsDialog.cpp:14,21-30,83` | `#include "OpenGLSupport.h"`, the `OpenGLSupport::supported()` block, `settings.setOpenGlEnabled(ui.enableOpenglCb->isChecked());` | delete |
| `src/app/SettingsDialog.ui:37-50,87,89` | `enableOpenglCb` ("Accelerate user interface with OpenGL"), `openglDeviceLabel` ("Device: %1"), `<zorder>` entries for both | delete widgets and zorders |
| `build/*/` | `otool -L` shows `QtOpenGLWidgets`, `QtOpenGL` and `/System/Library/Frameworks/OpenGL.framework` on all four executables | see below |
| `.planning/codebase/{STACK,INTEGRATIONS}.md`, `.claude/CLAUDE.md` stack text | mention OpenGL modules "until Phase 6" | update at the Phase 6 transition (CLAUDE.md is generated; edit via `generate-claude-md` only, as in Phase 3) |
No OpenGL mention in `TESTING.md`, `README.md`, `ui_snapshot`, `mac_check`, `retina_check` or any test source `[VERIFIED: grep -l list: only the files above plus OutputGenerator name matches]`.

### Colour-picker path and tests
The picker is `ColorPickupInteraction::takeColor` (`grab(rect)` then `medianColor`); the OpenGL-only branch is guarded by the setting that defaults to `false`, so removing it cannot change the default behaviour. The guard tests exist and need no change: `TestHiDpiQt::testMedianColorRatioInvariant` (line 1329, `medianColor` at ratio 1 and 2 on noise, quadrants, misread input) and `TestHiDpiQt::testColorPickerGrabAtRatio2` (line 1369, real view grab at 2x) `[VERIFIED: src/qt_tests/tst_hidpi_qt.cpp:1329,1369]`. Run them before the removal (record PASS lines) and after.

### New test (D-12)
`qt_tests`: construct `SettingsDialog` (as `testSettingsShortcutOpensSettings` does through `MainWindow`, or directly) and assert `findChildren<QWidget*>()` has no object name containing `opengl` (case-insensitive) and no `QLabel`/`QCheckBox` text containing `OpenGL`, with a control string proving the search finds them (the pattern of `hasLanguageControl` in `src/app/ui_snapshot/main.cpp:80-90`). Add a source scan in the style of `testNoDontUseNativeDialog` (reads all `scanScopeFiles(".cpp"/".h")` and asserts none contains `QOpenGL`, `OpenGLSupport`, `isOpenGlEnabled`) and a CMake scan for `OpenGL` in the two CMake files.

### Link proof (success criterion 4)
After the change: `for b in scantailor-advanced qt_tests mac_check retina_check; do otool -L build/macos-release/$b | grep -e QtOpenGL; done` must print nothing. `-framework OpenGL` (system) **stays**, because `QtGui` and `QtWidgets` link it themselves `[VERIFIED: otool -L]`; the plan's check must not use `grep -i opengl`. Also `grep -c 'QtOpenGL' build/macos-release/build.ninja` -> 0 (the include flags `-isystem .../QtOpenGL` disappear).

## CLEAN-01 detail (OutputGenerator.cpp, 2814 lines)

### Structure (measured; 82 top-level chunks ending at a column-0 `}`)
- Lines 4-72: includes; 74-75 `using namespace imageproc; using namespace dewarping;`; 77 `namespace output {`.
- 78-95: `OutputGenerator` ctor, `outputImageSize`. 97-253: `class OutputGenerator::Processor` **defined in the .cpp** (private nested class forward-declared in `OutputGenerator.h`), members only; every method definition below needs this definition, so it moves to an internal header.
- 254-363: `OutputGenerator::process`, Processor ctor, `process`, `initParams`, `calcAreas`.
- **365-1213: one anonymous namespace** (opened at 365, closed at 1213; a nested one at 565-573 holds `clipContentPolyToRect`) with ~25 helpers: `RaiseAboveBackground`, `CombineInverted`, `reserveBlackAndWhite` (templates and explicit specialisations), `fillExcept`, `BinaryImageXOR`, `fillMarginsInPlace` (4 overloads), `removeAutoPictureZones`, `createPictureZoneFromPoly`, `applyFillZones*` (8 overloads), bit helpers + `findRectAreas` (173 lines), `applyAffineTransform`, `hitMissReplaceAllDirections`, `calcLocalWindowSize`, `movePointToTop/BottomMargin` (2 overloads each), `drawPoint`, `vertBorderSkewAngle`, `smoothToGrayscale`, `from300dpi`, `to300dpi`.
- 1215-1273: `buildEmptyImage`, `updateBlackOnWhite`, `initFilterData`, `processImpl`. 1275-1530 `processWithoutDewarping` (256 lines). 1532-1891 `processWithDewarping` (360 lines). 1893-2813: the remaining Processor methods ending at 2814 `}  // namespace output`.
`calcLocalWindowSize` and `from300dpi` have no callers (single textual occurrence each) `[VERIFIED: grep -c]`; a pure move keeps them (note in the commit, do not delete).

### Helper-to-user map (script over the chunks; verify again in task 1)
| Helper(s) | Used by | Therefore lives in |
|-----------|---------|--------------------|
| `reserveBlackAndWhite`, `smoothToGrayscale`, `fillMarginsInPlace` (+`fillExcept`, `clipContentPolyToRect`), `applyFillZonesInPlace/ToMixedInPlace/ToMask` | both `processWithDewarping` and `processWithoutDewarping` | shared `OutputGeneratorPixelOps.{h,cpp}`; templates and specialisations in the header (explicit specialisations need `inline`) |
| `BinaryImageXOR` | `modifyBinarizationMask` | Binarize file |
| `createPictureZoneFromPoly`, `removeAutoPictureZones`, `findRectAreas` + bit helpers | `processPictureZones` | Binarize file (anonymous namespace) |
| `hitMissReplaceAllDirections` | `morphologicalSmoothInPlace` | Binarize file |
| `to300dpi` | `estimateBinarizationMask` | Binarize file |
| `applyAffineTransform` | `processWithDewarping` | Dewarp file |
| `movePointTo*Margin`, `drawPoint` | `buildMarginalDistortionModel` | Dewarp file |
| `vertBorderSkewAngle` | `buildAutoDistortionModel` | Dewarp file |

### Proposed files
`OutputGenerator.cpp` (about 230 lines), `OutputGeneratorProcessor.h` (class definition, verbatim lines 97-253), `OutputGeneratorPixelOps.{h,cpp}` (365-563, 565-790 minus the Binarize-only ones), `OutputGeneratorNoDewarp.cpp` (1275-1530), `OutputGeneratorDewarp.cpp` (1532-1891, 2038-2107, 2608-2813 and their helpers), `OutputGeneratorBinarize.cpp` (1893-2037, 2108-2607 and their helpers). Add all to `sources` in `src/core/filters/output/CMakeLists.txt`. Each new TU repeats the include set and the two `using namespace` lines it needs. `-ffp-contract=off` is global (`CMakeLists.txt`), so moved code compiles with the same flags; the only change is that functions leaving the anonymous namespace can no longer be inlined across TUs, which cannot change results under `-ffp-contract=off` `[ASSUMED: no flag besides contraction affects FP results here; the digest proof below tests it]`.

### Proof recipe ("pure move")
1. **Text**: a script (Python, `-I`) splits the original and the concatenation of all new files into chunks ending at a column-0 `}` (82 for the original), normalises whitespace, and compares multisets; the expected differences are exactly: removed/added `#include`/`using`/`namespace`/header-guard lines and the anonymous-namespace to header declarations. Any other difference fails.
2. **Behaviour**: output digests of decoded pixels before and after. Harness `build/scratch/phase6/m4` (flags in `flags.sh`, sources `main.cpp`) prints `DIGEST <kind> <dpi> <colour> <dewarp> - <sha1>`. Pre-split reference on HEAD `e0b632f7` (Release, this toolchain, runs repeated: identical):
```
DIGEST flat 300 bw off - d76527cfd84d79f5784c6967e970efb3a5d76c87
DIGEST flat 600 gray off - 1e78a1c0424fd3ef129c9a840a0de3100104d49b
DIGEST mixed 300 mixed off - b7e3a693d73c03ebc79546385c601c537a7cac8b
DIGEST mixed 600 mixed off - 718260ea80a1b8883bf8945ea0e3a23694f7e587
DIGEST curved 300 gray auto - 0476631ce760feb5fb117efd9720e83df0772f8d
DIGEST curved 600 mixed auto - c48ff8a8d7a4be446c75355380ed9bf055675dd7
DIGEST curved 600 bw auto - dd346ae2c91df2c7529f26f9c2ba317d25fb4241
```
(`Format_Mono` outputs are converted to `Format_Grayscale8` before hashing; raw padding bytes differ between runs.) The harness is gitignored; the plan must either copy `main.cpp`/`flags.sh` into `.planning/phases/06-hardening-and-cleanup/records/` first or rebuild from the recipe (compile `src/pipeline_tests/HeadlessPipeline.cpp` with the `pipeline_tests` compile flags from `compile_commands.json`, link the `pipeline_tests` link line without Boost).
3. **Gate** on every commit: Release and Debug ctest 7/7, goldens untouched. The Phase 5 guard `git diff f6cc4457 -- … OutputGenerator.cpp` must be **retired for this file** (the split changes it by design); keep it for `src/imageproc`, `src/dewarping` and the `Task.cpp` files.

## CLEAN-02 detail (MainWindow.cpp, 2319 lines; `MainWindow.h` declares the class)

### Inventory by line (verified list of definitions)
| Group | Functions (line) | Proposed file |
|-------|------------------|---------------|
| Constructor wiring | ctor 109-363: member init and Ui setup (118-205), `addAction`/`connect` block (206-343 `addAction(actionFirstPage)` … `connect(actionQuit…)`), tail (344-362) | extract the 206-343 block into a private `MainWindow::setupActions()` in `MainWindowActions.cpp`, called at the same position (order of connections is kept) |
| Actions and menus | `setupMacKeys` 1628, `updateProjectActions` 1613, `updateZoomActions` 1651, `zoomCurrentView*` 1665-1685, `switchFilter1-6` 1190-1212, `goFirst/Last/Next/Prev…` 927-1002, `execGotoPageDialog` 2272 | `MainWindowActions.cpp` |
| Batch processing | `createBatchProcessingWidget` 487, `startBatchProcessing` 1237, `stopBatchProcessing` 1282, `filterResult` 1310, `isBatchProcessingInProgress` 1688, `createCompositeTask` 2132, `createCompositeCacheDrivenTask` 2176 | `MainWindowBatch.cpp` |
| Project lifecycle | `saveProjectTriggered/AsTriggered` 1414-1458 (replaced by CLEAN-05 call), `newProject*` 1460-1473, `openProject*` 1475-1521, `closeProject*` 1523, 1808-1877, `promptProjectSave` 583, `autoSaveProject` 1032, `saveProjectWithFeedback` 1879 | `MainWindowProject.cpp` |
| Page insertion/removal | `showInsertFileDialog` 1892-2014, `showRemovePagesDialog` 2016, `insertImage` 2037, `removeFromProject` 2053, `eraseOutputFiles` 2104 | `MainWindowPages.cpp` |
| Rest (main area, thumbnails, dialogs, window state) | stays in `MainWindow.cpp` (about 900 lines) | — |
Use same-class translation units (Pattern 3). Moves are verbatim; new files go into `gui_only_sources` (`src/app/CMakeLists.txt`). No `.ui`, header or moc change.

### What guards the split
`qt_tests` builds a real `MainWindow` (`makeShownMainWindow`) and checks menus, roles, shortcuts (every window key), the zone/content-box keys, right-click menus and the source scans `[VERIFIED: tst_mac_qt.cpp function list: testSettingsInFileWithPreferencesRole … testNoDontUseNativeDialog]`; `TestHiDpiQt` and `TestUxQt` cover widgets. Batch behaviour has **no behavioural test** (only the source scan in Pitfall 5); the existing `mac_check` `save_as` row (`mac_check/main.cpp:772`, object name `actionSaveProjectAs`) and the Phase 5 live run cover the actions. If the planner wants a regression test for batch flow, a headless one is not available (needs `MainWindow` + a project); treat batch flow as covered by pure moves plus the digest/gate.

### Final live check (D-00b, D-17)
After the last commit, on the user's go: `build/macos-release/mac_check build/scratch/mac/all --section all -ApplePersistenceIgnoreState YES` (exit 0 pass, 1 fail, 2 usage, 3 not Cocoa). It moves the pointer and takes focus (STATE note). Present it as a checkpoint, one command at a time.

## CLEAN-03/04 detail

### `#if 0` inventory (grep, ugrep-compatible `-e`)
| File:line | Form | Live branch |
|-----------|------|-------------|
| `src/core/filters/output/DewarpingView.cpp:233-243` | `#if 0 // No point in drawing the curve itself…` … `#endif` | none (dead only) |
| `src/dewarping/Curve.cpp:142-148` | `#if 0` … `#endif` | none |
| `src/dewarping/DistortionModelBuilder.cpp:481-491` | `#if 0` … `#endif` | none |
| `src/core/IncompleteThumbnail.cpp:25-49` | `#if 0` … `#else` (line 30) … `#endif  // if 0` | **`#else`: the hand-written path points; keep** |
| `src/imageproc/SavGolFilter.cpp:134-191` | `#if 0` … `#else` (146) … `#endif  // if 0` | **`#else`: the separable filter; keep** |
| `src/core/filters/page_split/PageLayoutEstimator.cpp:660-678` | `#if 0` … `#else` (671) … `#endif` | **`#else`: `contentBegin/contentEnd` gap code; keep** |
| `src/core/filters/page_split/VertLineFinder.cpp:55-68` | `#if 0` … `#else` (60) … `#endif` | **`#else`: `GrayImage preprocessed(gray100);` with its explanatory comment; keep** |
| `src/core/filters/page_split/VertLineFinder.cpp:70-82` | `#if 0` … `#else` (77) … `#endif` | **`#else`: `erodeGray` pair; keep** |
8 blocks, 7 files `[VERIFIED: grep]`; no CMake `if(0)`/`if(FALSE)`. For the five blocks with `#else`, delete the dead branch, the `#if 0`, `#else` and `#endif` lines and **keep** the else-body (re-indent only if clang-format does). Add one-line comments where the dead branch documented an alternative algorithm (D-13): "simple unrolled convolution replaced by the separable pass", "hand-written glyph path replaced by precomputed outline", etc.

**Proof:** preprocess before and after with the real compile flags: `c++ <flags from compile_commands.json> -E -P file.cpp | sed '/^$/d' > …` for each of the 7 files; outputs must be identical modulo whitespace (comments are gone after `-E` unless `-C`). Equal preprocessed text implies equal object code, which is stronger than a diff review. Verify command for the plan: `grep -rn -e '^[[:space:]]*#[[:space:]]*if[[:space:]]*0' src` returns nothing.

### `boost/foreach.hpp`
26 include lines in 26 files (including two headers `src/core/zones/EditableZoneSet.h`, `src/foundation/FastQueue.h` and the test `src/imageproc/tests/TestRastLineFinder.cpp`); no `BOOST_FOREACH`/`foreach (` use outside a comment in `SeedFillGeneric.h:433,435` `[VERIFIED: grep]`. Expected build risk: files that got `<boost/…>` or standard headers transitively through `foreach.hpp`; the compiler names each missing include; add the explicit include in that file (do not restore `foreach.hpp`). Because headers are involved, the full Release+Debug build is the proof. Verify: `grep -rn 'boost/foreach' src` returns nothing.

## CLEAN-05 detail

### Comparison (verbatim bodies read this session)
`MainWindow::saveProjectAsTriggered` (`MainWindow.cpp:1425-1458`) and `OutOfMemoryDialog::saveProjectAs` (`OutOfMemoryDialog.cpp:50-83`) are line-for-line identical in: project directory (`QFileInfo(m_projectFile).absolutePath()` or `QSettings().value("project/lastDir")`), `QFileDialog::getSaveFileName(this, QString(), projectDir, tr("Scan Tailor Projects") + " (*.ScanTailor)")`, empty-path return, `".ScanTailor"` suffix (case-insensitive check), `saveProjectWithFeedback(projectFile)`, then `settings.setValue("project/lastDir", …)` and `RecentProjects rp; rp.read(); rp.setMostRecent(…); rp.write();`. They differ only in the post-success line (`updateWindowTitle()` vs `showSaveSuccessScreen()`). `saveProjectWithFeedback` (`MainWindow.cpp:1879-1887`, `OutOfMemoryDialog.cpp:85-92`) is identical: `ProjectWriter writer(m_pages, m_selectedPage, m_outFileNameGen); if (!writer.write(projectFile, m_stages->filters())) { QMessageBox::warning(this, tr("Error"), tr("Error saving the project file!")); return false; }`.

### Shape and tests
`ProjectSaveFlow` (static functions, see Code Examples) in `src/app/ProjectSaveFlow.{h,cpp}`, added to `gui_only_sources`. The native panel call and the filter text stay character-for-character (Phase 5 MAC-11 scan `testNoDontUseNativeDialog` covers the new file automatically). `MainWindow` keeps `m_projectFile` and `updateWindowTitle()`; `OutOfMemoryDialog` keeps `m_projectFile` and `showSaveSuccessScreen()`. The native panel cannot run offscreen, so test the parts around it: `withProjectExtension` (appends only when missing, case-insensitive), `remember()` against a settings file under `build/<preset>/test-tmp` (qt_tests already redirects settings; assert `project/lastDir` and the recent-project list), and `writeWithFeedback` success on a real `ProjectPages` (the failure path shows a modal box and needs no test). `tr` context moves from `MainWindow`/`OutOfMemoryDialog` to `ProjectSaveFlow` (translations are gone, D-00d). The shortcut stays in `MainWindow.ui` (`actionSaveProjectAs`, `Ctrl+Shift+S`) and is asserted by `testFileMenuTextsAndKeys`.

## Gate usage (`gate.sh`)

```
bash .planning/phases/03-qt-6-and-macos-only/gate.sh snapshot          # once, on the commit before the first src-changing Phase 6 commit
bash .planning/phases/03-qt-6-and-macos-only/gate.sh check <label>     # after every commit that changes src/, CMake, config files
```
- Run from the repository root with a clean `INPUTS` (`src CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in`); it refuses with exit 2 otherwise, and refuses when `UPDATE_GOLDENS` is set `[VERIFIED: gate.sh lines 17-27]`.
- `check` configures, builds and ctests `macos-release` then `macos-debug`, requires `100% tests passed out of 7` in both, compares `src/pipeline_tests/golden` with `a796c2ae`, and diffs the sorted `ninja -t commands` of `scantailor-advanced pipeline_tests core_tests qt_tests dewarping_tests imageproc_tests math_tests foundation_tests` (+`ui_snapshot` when present) against the reference stored for `HEAD~1`'s input-tree hash. Output: `build/scratch/gate/<label>/summary.txt` with `result: PASS` and `goldens: unchanged`, `cmds-<preset>.diff`, `tested-commit.txt`.
- Phase 5 convention to copy: each gated commit gets a row in a phase `records/gate-log.md` (label, commit, subject, both ctest summaries, goldens, diff-line counts, and why a non-empty diff is expected). Expected non-empty diffs in Phase 6: SAFE-01/03 (new `.cpp` and test files), STRIP-07 (QtOpenGL flags and link lines vanish for all targets), CLEAN-01/02/05 (new sources). Empty diffs: CLEAN-03, CLEAN-04, edit-only commits (a header drop can change dependency files only).
- Duration: Release `pipeline_tests` about 42 s, Debug about 206 s `[VERIFIED: round-trip.md]`; budget 5-10 minutes per gate, run it in the background after each commit.
- `mac_check`/`retina_check` are not gated targets: their changes need an explicit build, and live runs need the user's go.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Boost.Test 1.92 (`core_tests`, `pipeline_tests`, `imageproc_tests`…), Qt Test 6.12.0 (`qt_tests`); 7 ctest suites `[VERIFIED: TESTING.md]` |
| Config files | `src/core/tests/CMakeLists.txt` (add new `Test*.cpp` to `sources`), `src/pipeline_tests/CMakeLists.txt` (GLOB, `CONFIGURE_DEPENDS`), `src/qt_tests/CMakeLists.txt`, `CMakePresets.json` |
| Quick run command | `cmake --build --preset macos-release --target core_tests qt_tests && build/macos-release/core_tests --log_level=message && build/macos-release/qt_tests > build/scratch/qt.txt 2>&1` (grep the `PASS   : TestX::name()` lines and `Totals: … 0 failed, 0 skipped`) |
| Full suite command | `ctest --preset macos-release` and `ctest --preset macos-debug` (both `100% tests passed out of 7`); per commit `bash .planning/phases/03-qt-6-and-macos-only/gate.sh check <label>` |
| Sanitizers | `cmake --preset macos-asan && cmake --build --preset macos-asan && ctest --preset macos-asan -V` for the SAFE-01 and SAFE-02 suites (known reports at `BinaryImage.cpp:1044` etc. are recorded, not fixed) |
| Live checks | `mac_check` (user's go), memory headless run (user's go) |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SAFE-01 | forged TIFF/PNG/progressive JPEG refused, small footprint; boundary cases; multi-page; 72 MP PNG loads | unit (`core_tests`, in-memory) | `build/macos-release/core_tests --run_test=ImageSizeLimitTestSuite --log_level=message` | ❌ Wave 0 `src/core/tests/TestImageSizeLimit.cpp` |
| SAFE-01 | view message and file-list marking | Qt Test (`qt_tests`): `LoadFileTask::ErrorResult::updateUI` into a stub `FilterUiInterface`, `ProjectFilesDialog` item status | `build/macos-release/qt_tests` + grep PASS lines | ❌ Wave 0 (new slots in `TestUxQt`) |
| SAFE-02 | extended name set round-trips byte-identical; backslash `XFAIL_CHECK`; control-char escape | pipeline_tests | `build/macos-release/pipeline_tests --run_test=ProjectRoundTripTestSuite --log_level=message` | ✅ file exists, cases ❌ Wave 0 |
| SAFE-02 | escape helper unit tests (ordinary strings unchanged, invalid chars reversible) | unit (`foundation_tests`) | `build/macos-release/foundation_tests --log_level=message` | ❌ Wave 0 |
| SAFE-03 | cap arithmetic vectors; pool count `min(ideal, slider, cap)` | unit (`core_tests`, `qt_tests`) | `core_tests --run_test=MemoryBudgetTestSuite` | ❌ Wave 0 `TestMemoryBudget.cpp` |
| SAFE-03 | no swap growth, peak footprint within budget | headless scratch run, user's go | `sysctl vm.swapusage` before/after + footprints, record in `records/memory-run.md` | manual checkpoint |
| STRIP-07 | Settings dialog has no OpenGL control; sources/CMake free of OpenGL | Qt Test | `qt_tests` PASS line of the new slots | ❌ Wave 0 |
| STRIP-07 | colour-picker unchanged | Qt Test | PASS lines `TestHiDpiQt::testMedianColorRatioInvariant()`, `TestHiDpiQt::testColorPickerGrabAtRatio2()` before and after | ✅ |
| STRIP-07 | no Qt OpenGL linkage | shell | `for b in scantailor-advanced qt_tests mac_check retina_check; do otool -L build/macos-release/$b \| grep -e QtOpenGL; done` prints nothing | n/a |
| CLEAN-01 | pure move: goldens, digests, chunk multiset | gate + script + digest harness | `gate.sh check …`; digest lines equal the table above; chunk script exit 0 | script ❌ Wave 0 |
| CLEAN-02 | menus, shortcuts, source scans unchanged | Qt Test | `qt_tests` all `TestMacQt` PASS lines; updated `testBatchAlarmUsesBeep` | ✅ (one test edited) |
| CLEAN-02 | live menus and keys | `mac_check` | `build/macos-release/mac_check build/scratch/mac/all --section all -ApplePersistenceIgnoreState YES` exit 0 | ✅ user's go |
| CLEAN-03 | no `#if 0`; preprocessed text unchanged | grep + `c++ -E -P` diff | `grep -rn -e '^[[:space:]]*#[[:space:]]*if[[:space:]]*0' src` empty | n/a |
| CLEAN-04 | no foreach includes; build and tests green | grep + gate | `grep -rn 'boost/foreach' src` empty | n/a |
| CLEAN-05 | extension, lastDir, recent list, write | Qt Test | `qt_tests` new `TestMacQt`/`TestUxQt` slots | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** build the touched targets and run them (quick run command) plus `xcrun clang-format --dry-run -Werror` on touched C++ files.
- **Per commit that changes build inputs:** `gate.sh check <label>` (Release+Debug ctest, goldens, command diff), in the background.
- **Phase gate:** both presets green, `git diff a796c2ae -- src/pipeline_tests/golden` empty, all greps above, `otool -L` check, digest table reproduced, ASan run of the new suites, then the user-approved live `mac_check` and memory run, then `/gsd-verify-work`.

### Wave 0 Gaps
- [ ] `src/core/tests/TestImageSizeLimit.cpp` — SAFE-01 (+ `sources` entry in `src/core/tests/CMakeLists.txt`; maybe link `TIFF::TIFF` and `ZLIB::ZLIB` for forging)
- [ ] `src/core/tests/TestMemoryBudget.cpp` — SAFE-03
- [ ] extended cases in `src/pipeline_tests/TestProjectRoundTrip.cpp` — SAFE-02
- [ ] escape-helper tests in `src/foundation/tests/` — SAFE-02
- [ ] new `qt_tests` slots: Settings has no OpenGL, source/CMake scan, `ErrorResult` too-large message, `ProjectSaveFlow` — STRIP-07, SAFE-01, CLEAN-05
- [ ] `records/` scripts: chunk-multiset script, digest harness copy, `gate-log.md` — CLEAN-01
- Framework install: none.

## Security Domain

`security_enforcement` is enabled (config.json, ASVS level 1, block on high). This is a local single-user desktop app with no network or auth surface; the relevant risk is **untrusted file input causing resource exhaustion or data corruption**.

### Applicable ASVS Categories
| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | — (no accounts) |
| V3 Session Management | no | — |
| V4 Access Control | no | — (single user; no new file-system scope) |
| V5 Input Validation | **yes** | Header-value limits before allocation (`ImageSizeLimit`); XML-1.0-invalid characters escaped before writing a project file |
| V6 Cryptography | no | — (no crypto added; `QCryptographicHash` use in the digest harness and thumbnail names is non-security) |
| V12 Files and Resources (resource limits) | **yes** | Fixed ceiling per image, RAM-derived thread cap, no unbounded read of attacker-chosen sizes |

### Known Threat Patterns
| Pattern | STRIP/STRIDE | Standard Mitigation |
|---------|--------------|---------------------|
| Crafted image header claims huge dimensions (decompression/allocation bomb; measured 6.1 GB from a 13 KB TIFF, 12.3 GB from a 1 KB progressive JPEG) | Denial of service | Check header size before any pixel or coefficient allocation; same rule for TIFF, PNG, JPEG; keep Qt's own limit (never `setAllocationLimit(0)`) |
| Integer overflow from unsigned header values read into `int` | Tampering / DoS | Read the `uint32` into a wide type for the check; treat non-positive as invalid |
| Many large pages processed concurrently exhaust RAM, force swap | DoS (self-inflicted) | `MemoryBudget` cap on the pool |
| Project file written with characters XML 1.0 forbids becomes unloadable | Tampering (data loss) | Escape + flag attribute, or refuse to write |
| Untrusted project/image paths (existing) | — | Unchanged this phase; do not add path handling |
Threat refs for plans can be named T-06-01 (header bomb), T-06-02 (JPEG metadata bomb), T-06-03 (RAM exhaustion), T-06-04 (unloadable project).

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake / Ninja | all builds | ✓ | 4.5.0 / 1.13.2 | — |
| Apple clang, `xcrun clang-format` | build, formatting | ✓ | 21.0.0 | — |
| Qt 6 (Homebrew) | all | ✓ | 6.12.0_2 | — |
| Boost.Test, libtiff, libpng, jpeg-turbo | tests, readers | ✓ | 1.92.0_1 / 4.7.2 / 1.6.59 / 3.2.1 | — |
| `python3` | chunk script, forging helpers (use `python3 -I`) | ✓ | 3.15.2 | plain shell and C++ helpers |
| `otool`, `sysctl`, `/usr/bin/time -l` | link proof, RAM, peak memory | ✓ | macOS 26.7.2 | — |
| `timeout` | — | ✗ | — | none needed; no command here needs it |
| Cocoa display session | `mac_check` | needs user presence | — | checkpoint, user's go |
Free memory at research time was low (about 1.5 GB free pages, 64 GiB installed): keep the SAFE-03 run small as described. Missing with no fallback: none.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Qt 6.12 has no total-RAM API, so `sysctlbyname("hw.memsize")` is the way | Standard Stack | Low: sysctl works regardless |
| A2 | RGB32 byte counts above 2^31 would overflow some `int` byte computation (only the pixel-count sites were verified) | SAFE-01 ceiling | Medium: if wrong, 2^30 (1 GP, D-01's example) would also be safe; ceiling is one constant |
| A3 | A hand-built minimal TIFF IFD is accepted by libtiff like the libtiff-written forged files | Code Examples / D-05 | Low: fall back to linking libtiff into `core_tests` |
| A4 | The 40 B per pixel factor also covers the real app's extra memory (caches, view) because the harness over-counts by one input copy and one output copy | SAFE-03 | Medium: caught by the headless checkpoint; factor is one constant |
| A5 | Moving helpers out of the anonymous namespace cannot change floating-point results under `-ffp-contract=off` | CLEAN-01 | Medium: the digest proof tests exactly this; if a digest differs, revert that move |
| A6 | `RelinkablePath::normalize` affects the backslash round trip in the load path (backlog 999.8 says so; not re-measured) | SAFE-02 | Low: the test observes first, then marks |
| A7 | The headless pipeline (not the app binary) is representative of the app's task memory | SAFE-03 | Medium: same as A4 |
| A8 | `jpeg_start_decompress` is only needed to detect unsupported compression, so skipping it for oversized files loses nothing | Pitfall 2 | Low: oversized files are refused anyway |

## Open Questions (RESOLVED)

1. **Which output-DPI rule feeds the estimator? (RESOLVED)**
   - Known: peak follows max(source, output) pixels; default output DPI is 600; per-page output DPI sits in `output::Settings` (private in `output::Filter`).
   - Unclear: whether to add an accessor and use the true per-page value, or use the conservative fixed rule `max(srcPx, srcPx x (600/srcDpi)^2)`.
   - Recommendation: the fixed rule (no new accessor, no coupling); revisit if a user sets a higher output DPI.
   - **RESOLVED:** the fixed rule is taken (`MemoryBudget::pagePixelEstimate`, plan 06-08, decision "Estimator rule"); its limit for output DPI above 600 is recorded in `records/safe-03.md` (plan 06-09) and listed for the user in `records/closing-check.md` (plan 06-16).
2. **Fix or mark the control-character names (SAFE-02)? (RESOLVED)**
   - Known: only U+0001-U+001F (minus tab/LF/CR) fail; APFS allows them; QDom cannot write them.
   - Recommendation: implement the flagged escape (about 40 lines + tests); fallback is `XFAIL_CHECK` + writer refuses.
   - **RESOLVED:** fix, not mark: the flagged escape is implemented in plan 06-07 (decision "Fix, not mark"), with the `XFAIL_CHECK` fallback only if the escape cannot keep ordinary files byte-identical; the outcome is recorded in `records/safe-02.md`.
3. **Ceiling 2^29 or 2^30? (RESOLVED)** D-01 left the exact number to research; 2^29 recommended. If the user wants D-01's 1 GP example literally, use 2^30 and `setAllocationLimit(4096)`; nothing else changes.
   - **RESOLVED (corrected at plan revision 2):** fewer than 2^29 px, written as `kMaxPixels = 2^29 - 1 = 536870911` (with 65535 px per side and `setAllocationLimit(2048)`), is accepted in plan 06-04 (decision "Ceiling"): D-01's 1 GP was an example of the intended size and left the exact number to research. An inclusive 2^29 would not keep RGB32 byte counts below 2^31: exactly 2^29 px take 2^31 bytes, one more than INT_MAX; with the exclusive limit the largest accepted RGB32 image takes at most 2147483644 bytes (`static_assert`), and assumption A2 is closed by the int-arithmetic audit of plan 06-06. 536870911 is still 1.93 times the 1200 dpi A3 reference page. Plan 06-06 records the choice against D-01's 1 GP example (2^30 = 1073741824 px, Qt limit 4096 MB; its RGB32 byte counts would reach 2^32 - 4) in `records/safe-01.md`. The ceiling stays one reversible constant.
4. **Do any consumers rely on `ImageMetadataLoader::LOADED` for oversized files? (RESOLVED)** Only `ProjectFilesDialog` and `MainWindow::showInsertFileDialog` call it; both will check pages themselves.
   - **RESOLVED:** both callers check every page with `ImageSizeLimit::oversizedPagesText` after a `LOADED` status in plan 06-06 (commits `s06-project-files` and `s07-insert-thumbs`); `ImageSizeLimit::claimedSizeIfTooLarge` (plan 06-04) is the only other caller.

## Sources

### Primary (HIGH confidence)
- Local measurement on this Mac (Qt 6.12.0_2, libtiff 4.7.2, jpeg-turbo 3.2.1): `QImageReader` limits and boundaries, forged TIFF and JPEG cost, QDom round-trip matrix, pipeline peak memory, APFS name probes. Scratch programs: `build/scratch/phase6/{m1,m2,m3,m4,m5}` (gitignored).
- Repo files read this session with line references quoted above (`TiffReader.cpp`, `ImageLoader.cpp`, `JpegMetadataLoader.cpp`, `PngMetadataLoader.cpp`, `WorkerThreadPool.cpp`, `SystemLoadWidget.cpp`, `ProjectWriter.cpp`, `ProjectReader.cpp`, `XmlUnmarshaller.cpp`, `ApplicationSettings.cpp`, `ImageViewBase.cpp`, `ColorPickupInteraction.cpp`, `SettingsDialog.{cpp,ui}`, `MainWindow.cpp`, `OutOfMemoryDialog.cpp`, `OutputGenerator.cpp`, `gate.sh`, `TESTING.md`, `tst_mac_qt.cpp`, `TestProjectRoundTrip.cpp`).
- `/opt/homebrew/Cellar/qtbase/6.12.0_2/lib/QtGui.framework/Versions/A/Headers/qimagereader.h:110-111` (`static int allocationLimit(); static void setAllocationLimit(int mbLimit);`) and `strings QtGui` (`QT_IMAGEIO_MAXALLOC`, "Rejecting image as it exceeds the current allocation limit of %i megabytes").

### Secondary (MEDIUM confidence)
- [Qt 6 QImageReader documentation](https://doc.qt.io/qt-6/qimagereader.html): `setAllocationLimit` semantics (0 disables, env override `QT_IMAGEIO_MAXALLOC`, since 6.0, "All functions in this class are reentrant"); the page gives no numeric default, the 256 MB default is from measurement.

### Tertiary (LOW confidence)
- None used.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH, nothing new, versions read from the installed tree.
- SAFE-01/02/03 facts: HIGH, measured on the real toolchain; thresholds (2^29, 40 B, 50%) are decisions with stated margins.
- STRIP-07: HIGH, full grep inventory and `otool` evidence.
- Split proposals (CLEAN-01/02): MEDIUM, line ranges measured, final boundaries are the planner's; helper-to-user map came from a regex script and must be re-run as task 1.
- Pitfalls: HIGH for those measured, MEDIUM for A2/A5.

**Research date:** 2026-11-16
**Valid until:** 2026-12-16 (stable local toolchain; re-measure if Homebrew upgrades Qt or libtiff, and re-record the pre-split digests on any commit before CLEAN-01 starts)
