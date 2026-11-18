---
phase: 06-hardening-and-cleanup
verified: 2026-11-17T16:00:00Z
status: passed
score: 5/5 must-haves verified
covered_files:
  - ".planning/phases/06-hardening-and-cleanup/06-01-PLAN.md"
  - ".planning/phases/06-hardening-and-cleanup/06-01-SUMMARY.md"
  - ".planning/phases/06-hardening-and-cleanup/06-02-PLAN.md"
  - ".planning/phases/06-hardening-and-cleanup/06-02-SUMMARY.md"
  - ".planning/phases/06-hardening-and-cleanup/06-03-PLAN.md"
  - ".planning/phases/06-hardening-and-cleanup/06-03-SUMMARY.md"
  - ".planning/phases/06-hardening-and-cleanup/06-04-PLAN.md"
  - ".planning/phases/06-hardening-and-cleanup/06-04-SUMMARY.md"
  - ".planning/phases/06-hardening-and-cleanup/06-05-PLAN.md"
  - ".planning/phases/06-hardening-and-cleanup/06-05-SUMMARY.md"
  - ".planning/phases/06-hardening-and-cleanup/06-06-PLAN.md"
  - ".planning/phases/06-hardening-and-cleanup/06-06-SUMMARY.md"
  - ".planning/phases/06-hardening-and-cleanup/06-07-PLAN.md"
  - ".planning/phases/06-hardening-and-cleanup/06-07-SUMMARY.md"
  - ".planning/phases/06-hardening-and-cleanup/06-08-PLAN.md"
  - ".planning/phases/06-hardening-and-cleanup/06-08-SUMMARY.md"
  - ".planning/phases/06-hardening-and-cleanup/06-09-PLAN.md"
  - ".planning/phases/06-hardening-and-cleanup/06-09-SUMMARY.md"
  - ".planning/phases/06-hardening-and-cleanup/06-10-PLAN.md"
  - ".planning/phases/06-hardening-and-cleanup/06-10-SUMMARY.md"
  - ".planning/phases/06-hardening-and-cleanup/06-11-PLAN.md"
  - ".planning/phases/06-hardening-and-cleanup/06-11-SUMMARY.md"
  - ".planning/phases/06-hardening-and-cleanup/06-12-PLAN.md"
  - ".planning/phases/06-hardening-and-cleanup/06-12-SUMMARY.md"
  - ".planning/phases/06-hardening-and-cleanup/06-13-PLAN.md"
  - ".planning/phases/06-hardening-and-cleanup/06-13-SUMMARY.md"
  - ".planning/phases/06-hardening-and-cleanup/06-14-PLAN.md"
  - ".planning/phases/06-hardening-and-cleanup/06-14-SUMMARY.md"
  - ".planning/phases/06-hardening-and-cleanup/06-15-PLAN.md"
  - ".planning/phases/06-hardening-and-cleanup/06-15-SUMMARY.md"
  - ".planning/phases/06-hardening-and-cleanup/06-16-PLAN.md"
  - ".planning/phases/06-hardening-and-cleanup/06-16-SUMMARY.md"
  - "CMakeLists.txt"
  - "src/app/MainWindow.cpp"
  - "src/app/MainWindow.h"
  - "src/app/MainWindowBatch.cpp"
  - "src/app/MainWindowProject.cpp"
  - "src/app/ProjectSaveFlow.cpp"
  - "src/app/SystemLoadWidget.cpp"
  - "src/core/ImageLoader.cpp"
  - "src/core/ImageLoader.h"
  - "src/core/ImageSizeLimit.cpp"
  - "src/core/JpegMetadataLoader.cpp"
  - "src/core/LoadFileTask.cpp"
  - "src/core/MemoryBudget.cpp"
  - "src/core/MemoryBudget.h"
  - "src/core/ProjectReader.cpp"
  - "src/core/ProjectWriter.cpp"
  - "src/core/ThumbnailPixmapCache.cpp"
  - "src/core/TiffReader.cpp"
  - "src/core/WorkerThreadPool.cpp"
  - "src/core/filters/output/Filter.cpp"
  - "src/core/filters/output/Filter.h"
  - "src/core/filters/output/OutputGenerator.cpp"
  - "src/core/filters/output/OutputGeneratorBinarize.cpp"
  - "src/core/filters/output/OutputGeneratorDewarp.cpp"
  - "src/core/filters/output/OutputGeneratorNoDewarp.cpp"
  - "src/core/tests/TestImageSizeLimit.cpp"
  - "src/core/tests/TestMemoryBudget.cpp"
  - "src/foundation/XmlMarshaller.cpp"
  - "src/foundation/XmlUnmarshaller.cpp"
  - "src/pipeline_tests/TestProjectRoundTrip.cpp"
  - "src/qt_tests/tst_mac_qt.cpp"
  - "src/qt_tests/tst_ux_qt.cpp"
  - "src/test_support/ForgedImages.cpp"
covered_digest: "v4:sha256:0dddfdd0cd0da9ce83d5b6edaec3fd5a1337c356ef02b626b7354651fc28c10b"
behavior_unverified: 0
overrides_applied: 0
re_verification:
  previous_status: human_needed
  previous_score: 4/5
  gaps_closed:
    - "SC 3 behaviour on the real MainWindow batch path (previously PRESENT_BEHAVIOR_UNVERIFIED): 06-UAT.md test 1 observed the memory cap live in the app (tooltip '10/10, limited to 2 by memory')"
  gaps_remaining: []
  regressions: []
advisory:
  - finding: "Three XXX markers appear as added lines in the Phase 6 diff (MainWindowPages.cpp:146 and two morphology pattern strings in OutputGeneratorBinarize.cpp)"
    category: other
    reason: "All three are pre-existing upstream text moved verbatim by the pure-move splits (same lines at base e5319cd9 in MainWindow.cpp:1942 and OutputGenerator.cpp:2225/2295). No new debt; the one real FIXME (XmlUnmarshaller) was resolved."
    evidence_status: "git show e5319cd9 shows the same lines"
  - finding: "The reset of the memory cap after a batch stops or a project is switched (WR-02) is verified by code reading only"
    category: other
    reason: "stopBatchProcessing and switchToNewProject call setMemoryCap(kNoCap) (MainWindowBatch.cpp:193, MainWindowProject.cpp:99). No test drives MainWindow through a batch stop, and the batch panel with the slider is replaced when the batch ends, so the UI cannot show it (06-UAT.md test 1 note). It is not a ROADMAP success criterion and no plan must-have asserts it; the worst case of a stale cap is fewer threads, never more memory."
    evidence_status: "code read; pool and widget cap-lifting covered by testWorkerPoolHonoursMemoryCap and testSystemLoadToolTipShowsMemoryLimit"
  - finding: "Open Info items IN-01 to IN-06 of the code review (06-REVIEW-DISPOSITION.md)"
    category: other
    reason: "Info severity, recorded and open; IN-04 (ImageLoader::load(const ImageId&) now unused and an unlimited reader) and IN-06 (startBatchProcessing with an empty sequence) are the only ones with a code consequence, neither on a success-criterion path."
    evidence_status: "none provided; not blocking"
---

# Phase 6: Hardening and Cleanup Verification Report

**Phase Goal:** Known safety defects are fixed, the OpenGL viewer is gone and the riskiest oversized files are split, so later feature work is safe. Output stays unchanged.
**Verified:** 2026-11-17
**Status:** passed
**Re-verification:** Yes. The earlier report (human_needed, 4/5) predates the review fixes WR-01 to WR-07 (`5e1fd627` to `af81849b`, `5bf8fe1e`) and the UAT. Every truth was re-checked against the current tree, not carried over.

## Goal Achievement

### Observable Truths (ROADMAP success criteria)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | A TIFF whose header claims absurd dimensions shows an error instead of a huge allocation or crash, and a test covers it | VERIFIED | `ImageSizeLimit::exceeds` is called from `TiffReader.cpp:277`, `ImageLoader.cpp:77` (Qt formats), `JpegMetadataLoader.cpp:214` and `LoadFileTask.cpp:69`; `static_assert`s pin the int-overflow reasoning (`ImageSizeLimit.h:68`). After WR-01 the limit applies to source scans only (`loadSource`, `enforceSizeLimit`), so the app's own output files are no longer refused; `Task.cpp` output reads still use `ImageLoader::load`. WR-07: `JpegMetadataLoader.cpp:222` skips `jpeg_start_decompress` when `jpeg_has_multiple_scans`. `core_tests --run_test=CoreImageSizeLimitTestSuite` passes (fresh, 17 cases incl. the forged TIFF/PNG/JPEG and the footprint bound). `TestUxQt::testOversizedImageShowsTooLargeInPageView` is inside the green `qt_tests`. |
| 2 | Names with `&`, `<`, `"`, non-ASCII save and reload intact; round-trip test passes without expected failures | VERIFIED | `ProjectWriter.cpp:88,122,137` write via `XmlMarshaller::setEscapedAttribute`; `ProjectReader.cpp:29,107,140` read via `XmlUnmarshaller::escapedAttribute`. `TestProjectRoundTrip.cpp` has 0 `XFAIL`/`expected_failures`; fixtures `a&b <c> "d" 'e'.png`, `in & <dir> "q" 'a'`, `out & <x> "y"` plus NFC/NFD and 200-byte names; cases for control characters, unusual names, backslash. `ProjectRoundTrip*` and `FoundationXmlEscapeTestSuite` pass fresh. No FIXME/TBD remains in `src`. |
| 3 | Batch-processing large scans runs with a worker-thread count capped by the Mac's RAM; no swap growth | VERIFIED | Wiring: `MainWindowBatch.cpp:148-151` computes `MemoryBudget::batchMemoryCap(sequence, outputDpiOf)` (WR-03: per-page output dpi through `Filter::outputDpi`) and sets it on the pool and slider before the first `submitTask` (line 160ff); `WorkerThreadPool.cpp:93` applies `min(ideal, slider, cap)`. Behaviour: `CoreMemoryBudgetTestSuite` (fresh pass), `testWorkerPoolHonoursMemoryCap`, `testSystemLoadToolTipShowsMemoryLimit`. Recorded D-08 run (`records/memory-run.md`): ten 165.6 MP pages through `LoadFileTask` on `WorkerThreadPool`, cap 5 of ideal 10, peak 17737 MiB of a 32768 MiB budget, swap 0 in all samples and before/after. The earlier gap (real `MainWindow` path never exercised) is closed by `06-UAT.md` test 1: the user ran the app on real camera JPEGs and saw "10/10, limited to 2 by memory" during the batch. Not checked live: swap in Activity Monitor on that real run was not recorded; the swap evidence is the synthetic run on the production pool. See advisory for the stop-reset. |
| 4 | OpenGL option gone from Settings, build no longer links the Qt OpenGL modules, colour-picker test passes | VERIFIED | `CMakeLists.txt:105`: `find_package(Qt6 6.5 COMPONENTS Core Gui Widgets Xml Network Svg Test ...)`; `git grep -i opengl` over every `CMakeLists.txt` and `cmake/`: empty. `git grep -il opengl -- src`: only `tst_mac_qt.cpp/.h` (the absence tests). `otool -L` of `scantailor-advanced`, `qt_tests`, `core_tests`: QtOpenGL 0, QtGui 1 (positive control). `git grep` for `enable_opengl`, `OpenGLSupport`, `QOpenGL` in `src`: only the forbidden-term list inside `testNoOpenGlInSourcesOrBuild`. `qt_tests` green, which includes `testSettingsHasNoOpenGlControl`, `testNoOpenGlInSourcesOrBuild` and the colour-picker tests. |
| 5 | Files split, one Save-as flow, no `#if 0` / `boost/foreach`; full ctest passes with baselines unchanged; Phase 5 menus behave as before | VERIFIED | `OutputGenerator.cpp` 268 lines plus `OutputGeneratorNoDewarp/Dewarp/Binarize.cpp` and two headers; `MainWindow.cpp` 1237 lines plus `MainWindowActions/Batch/Project/Pages.cpp`. `git grep` for `^\s*#\s*if\s+0` (control: `#if` found in `src/app/*.h`) and `boost/foreach` in `src`: empty. `QFileDialog::getSaveFileName` only at `ProjectSaveFlow.cpp:34`, reached from `MainWindowProject.cpp:235` and `OutOfMemoryDialog.cpp:49`. Fresh `cmake --build` (no work) and `ctest --preset macos-release`: 100% passed, 7 of 7 (`build/scratch/verify6b/ctest-release.txt`). `git diff a796c2ae HEAD -- src/pipeline_tests/golden` empty. The output digest probe re-run now matches the pre-split digests line for line for all 7 pages (`build/scratch/verify6b/digests.txt` vs `build/scratch/closing6/digests.txt`), so the review fixes did not change output either. Files changed under `src/imageproc` and `src/dewarping` are exactly the 12 on `records/output-path-allowlist.txt`. Menus: recorded live `mac_check` PASS (both invocations, `records/closing-check.md`) plus offscreen `TestMacQt` in the green run; not re-run (instructed). The fixes after that run touched `MainWindowBatch/Project.cpp` only in the cap reset and the close-on-failed-save path (WR-06, UAT test 2 passed), not menu or shortcut wiring. |

**Score:** 5/5 truths verified, 0 present-but-behaviour-unverified.

### Required Artifacts

`verify.artifacts` over all 16 plans: every plan `all_passed: true` (artifacts exist, substantive).

| Artifact | Status | Details |
|----------|--------|---------|
| `src/core/ImageSizeLimit.{h,cpp}` | VERIFIED | Used by TiffReader, ImageLoader, JpegMetadataLoader, LoadFileTask, ProjectFilesDialog, MainWindowPages |
| `src/core/MemoryBudget.{h,cpp}` | VERIFIED | Used by MainWindowBatch, WorkerThreadPool, SystemLoadWidget |
| Escape API in `XmlMarshaller` / `XmlUnmarshaller` | VERIFIED | Used by ProjectWriter / ProjectReader |
| `src/app/ProjectSaveFlow.{h,cpp}` | VERIFIED | Sole save-dialog caller |
| `OutputGenerator*` split files | VERIFIED | Built; exercised by pipeline goldens and digests |
| `MainWindow{Batch,Actions,Project,Pages}.cpp` | VERIFIED | Built into `scantailor-advanced` |

### Key Link Verification

`verify.key-links`: all plans verified except two, both false negatives checked by hand.

| From | To | Status | Details |
|------|----|--------|---------|
| `MainWindow::startBatchProcessing` | `MemoryBudget::batchMemoryCap` then pool and slider | WIRED | `MainWindowBatch.cpp:148-151`, live-observed in UAT |
| `MainWindow::stopBatchProcessing` / `switchToNewProject` | cap reset to `kNoCap` | WIRED (code read) | `MainWindowBatch.cpp:193`, `MainWindowProject.cpp:99` |
| `ProjectWriter` / `ProjectReader` | escape helpers | WIRED | lines above |
| `MainWindow` (06-12 link) | `ProjectSaveFlow::saveAs` | WIRED | tool says not found in `MainWindow.cpp` because 06-15 moved the call to `MainWindowProject.cpp:235` |
| `OutOfMemoryDialog` | `ProjectSaveFlow` | WIRED | `OutOfMemoryDialog.cpp:41,49` |
| `CMakeLists.txt` (06-11 link) | no OpenGL components | WIRED | tool could not evaluate the plan's pattern (`find_package(Qt6` is not valid RE2); checked by hand, line 105 |
| Loaders | `ImageSizeLimit` | WIRED | see truth 1 |

### Data-Flow Trace (Level 4)

| Artifact | Data | Source | Real data | Status |
|----------|------|--------|-----------|--------|
| Batch cap | `largestPagePixels` | `PageInfo::metadata()` and `Filter::outputDpi` from the output `Settings` (default 600 if absent) | yes | FLOWING (UAT cap 2 matched the hand calculation of 384 MP) |
| Too-large message | claimed size | header read via `ImageSizeLimit::claimedSizeIfTooLarge` | yes | FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Full suite | `ctest --preset macos-release` | 100% passed, 7 of 7 (68 s) | PASS |
| Size limit | `core_tests --run_test=CoreImageSizeLimitTestSuite` | no errors | PASS |
| Memory budget | `core_tests --run_test=CoreMemoryBudgetTestSuite` | no errors | PASS |
| Round trip | `pipeline_tests --run_test='ProjectRoundTrip*'` | no errors | PASS |
| Escape helpers | `foundation_tests --run_test=FoundationXmlEscapeTestSuite` | no errors | PASS |
| Output unchanged | `pipeline_tests --run_test=OutputDigestProbeTestSuite` | 7 digests equal the pre-split set | PASS |
| No Qt OpenGL linking | `otool -L` on three binaries | QtOpenGL 0, QtGui 1 | PASS |

### Probe Execution

SKIPPED: the phase declares no `scripts/*/tests/probe-*.sh`. Its probes are Boost cases (digest probe, peak-memory probe) run above or recorded.

Not re-run (instructed or recorded): `mac_check`, `retina_check`, memory probe (live harnesses), Debug and ASan ctest (the session's Debug run was reported 100% by the caller and `records/closing-check.md` shows Debug and ASan green at the last pre-fix source; I did not run Debug myself).

### Requirements Coverage

All nine IDs are declared in plan frontmatter (06-16 declares all nine) and mapped to Phase 6 in `REQUIREMENTS.md`, which marks all nine complete. No orphaned requirements.

| Requirement | Source Plans | Status | Evidence |
|-------------|--------------|--------|----------|
| SAFE-01 | 06-04, 06-05, 06-06, 06-16 | SATISFIED | Truth 1 |
| SAFE-02 | 06-07, 06-16 | SATISFIED | Truth 2 |
| SAFE-03 | 06-08, 06-09, 06-16 | SATISFIED | Truth 3 |
| STRIP-07 | 06-10, 06-11, 06-16 | SATISFIED | Truth 4 |
| CLEAN-01 | 06-13, 06-14, 06-16 | SATISFIED | Truth 5; goldens and 7 digests unchanged, re-run now |
| CLEAN-02 | 06-15, 06-16 | SATISFIED | Truth 5; sizes, build, `TestMacQt`, recorded live `mac_check` |
| CLEAN-03 | 06-01, 06-16 | SATISFIED | no `#if 0` in `src` |
| CLEAN-04 | 06-02, 06-03, 06-16 | SATISFIED | no `boost/foreach` in `src` |
| CLEAN-05 | 06-12, 06-16 | SATISFIED | single `getSaveFileName` caller; `testSaveAsFlowFromOutOfMemoryDialog` in green `qt_tests` |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `MainWindowPages.cpp` | 146 | `XXX:` upstream note, moved verbatim | Info | Not new debt (advisory) |
| `OutputGeneratorBinarize.cpp` | 2 sites | `"XXX"` morphology pattern strings, not comments | Info | Pattern data, not a marker |

Added-line scan of `git diff e5319cd9 HEAD -- src`: no new TBD, FIXME, TODO or HACK. Review ledger: WR-01 to WR-07 fixed, IN-01 to IN-06 open and advisory. `06-SECURITY.md`: `threats_open: 0`; `06-VALIDATION.md`: nyquist compliant, 0 gaps (as reported, not re-derived).

### Human Verification Required

None. The two items of the previous report were run: `06-UAT.md` complete, 2 passed, 0 issues.

### Gaps Summary

No gaps. Phase goal achieved: the size limit, XML escaping and RAM cap are in the code, wired, tested and (for the cap) observed live; the OpenGL option and modules are gone; the two oversized files are split by pure moves with goldens and output digests unchanged; one save-as flow remains. Two points are recorded as non-blocking advisories: the cap reset after a stopped batch rests on code reading, and swap growth was measured on the synthetic production-pool run rather than on the user's real-scan run.

---

_Verified: 2026-11-17_
_Verifier: Claude (gsd-verifier)_
