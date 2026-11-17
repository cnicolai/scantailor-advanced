---
phase: 06-hardening-and-cleanup
verified: 2026-11-17T15:00:00Z
status: human_needed
score: 4/5 must-haves verified
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
  - "src/app/MainWindowBatch.cpp"
  - "src/app/MainWindowProject.cpp"
  - "src/app/ProjectSaveFlow.cpp"
  - "src/core/ImageSizeLimit.cpp"
  - "src/core/MemoryBudget.cpp"
  - "src/core/ProjectReader.cpp"
  - "src/core/ProjectWriter.cpp"
  - "src/core/WorkerThreadPool.cpp"
  - "src/core/filters/output/OutputGenerator.cpp"
  - "src/core/filters/output/OutputGeneratorBinarize.cpp"
  - "src/core/filters/output/OutputGeneratorDewarp.cpp"
  - "src/core/filters/output/OutputGeneratorNoDewarp.cpp"
  - "src/foundation/XmlMarshaller.cpp"
  - "src/foundation/XmlUnmarshaller.cpp"
covered_digest: "v4:sha256:8c9854a52abff3f03a36b2251e11610fba1fe87f3090341964eae63da6ce3587"
behavior_unverified: 1
behavior_unverified_items:
  - truth: "Batch-processing a project of large scans runs with a worker-thread count capped by RAM and Activity Monitor shows no growth in swap (ROADMAP SC 3)"
    test: "In the built app, open a project of several large real scans, start batch processing, and watch Activity Monitor > Memory > Swap Used before and during the run"
    expected: "Swap Used does not grow; the thread-count slider tooltip says 'limited to n by memory' when the cap binds"
    why_human: "The cap arithmetic, pool behaviour and tooltip are tested, and the D-08 run measured swap 0 through WorkerThreadPool with synthetic 165.6 MP pages and a stand-in for the MainWindow shell. The real MainWindow::startBatchProcessing ordering (cap set before the first submitTask) is only verified by reading the code, and no real scans were used; the app binary is never launched by Claude."
human_verification:
  - test: "Real-scan batch run with Activity Monitor open (optional check named in records/closing-check.md 'For the user')"
    expected: "Swap Used does not grow during the batch; tooltip shows the memory limit for pages above about 85 MP"
    why_human: "Needs the app launched on real scans; Claude must not launch the app (D-00e)"
advisory:
  - finding: "Three XXX comments appear as added lines in the Phase 6 diff (MainWindowPages.cpp:146 and two morphology pattern strings 'XXX' in OutputGeneratorBinarize.cpp)"
    category: other
    reason: "All three are pre-existing upstream text moved verbatim by the pure-move splits; none is new debt. The one real FIXME (XmlUnmarshaller) was resolved. Strict reading of the debt-marker gate would flag MainWindowPages.cpp:146 (an upstream 'not supported right now' note), so it is reported, not blocking."
    evidence_status: "git diff e5319cd9 shows the same lines in MainWindow.cpp / OutputGenerator.cpp at the base"
---

# Phase 6: Hardening and Cleanup Verification Report

**Phase Goal:** Known safety defects are fixed, the OpenGL viewer is gone and the riskiest oversized files are split, so later feature work is safe. Output stays unchanged.
**Verified:** 2026-11-17
**Status:** human_needed
**Re-verification:** No, initial verification

## Goal Achievement

### Observable Truths (ROADMAP success criteria)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | A TIFF with absurd header dimensions shows an error instead of a huge allocation or crash, and a test covers it | VERIFIED | `ImageSizeLimit::exceeds` is called in `TiffReader.cpp:276`, `ImageLoader.cpp:61`, `JpegMetadataLoader.cpp:214`, `LoadFileTask.cpp:69`. `CoreImageSizeLimitTestSuite` exists and passes fresh (`core_tests --run_test=CoreImageSizeLimitTestSuite`: no errors). `TestUxQt::testOversizedImageShowsTooLargeInPageView` passes for tiff, png, png-wide, jpeg, garbage. Static_asserts pin the int-overflow reasoning. |
| 2 | Names with `&`, `<`, `"`, non-ASCII save and reload intact; round-trip test passes without expected failures | VERIFIED | `ProjectWriter.cpp:88,122,137` write through `XmlMarshaller::setEscapedAttribute`; `ProjectReader.cpp:29,107,140` read through `XmlUnmarshaller::escapedAttribute`. The `-FIXME` in XmlUnmarshaller is gone. `TestProjectRoundTrip.cpp` has 0 `XFAIL_CHECK`. `ProjectRoundTrip*` suites and `FoundationXmlEscapeTestSuite` (7 cases) pass fresh. Control-character, unusual-name and backslash cases exist. |
| 3 | Batch run uses a worker-thread count capped by RAM; no swap growth | PRESENT_BEHAVIOR_UNVERIFIED | Wired: `MainWindowBatch.cpp:147-149` computes `MemoryBudget::batchMemoryCap(sequence)` and sets it on the pool and slider widget before the first `submitTask`; `WorkerThreadPool.cpp:93` applies `min(ideal, slider, cap)`. `CoreMemoryBudgetTestSuite` and `testWorkerPoolHonoursMemoryCap` pass fresh. Recorded D-08 run (`records/memory-run.md`): cap 5 of ideal 10, peak 17737 MiB, swap 0. Not behaviourally proven in the real app path or on real scans, see Human Verification. |
| 4 | OpenGL option gone from Settings, build no longer links Qt OpenGL modules, colour-picker test passes | VERIFIED | `CMakeLists.txt:105` lists no OpenGL component. `otool -L` of `scantailor-advanced`, `qt_tests`, `core_tests`: QtOpenGL 0, QtGui 1 (positive control). `git grep -il opengl -- src` lists only the two test files that assert absence. `testSettingsHasNoOpenGlControl`, `testNoOpenGlInSourcesOrBuild`, `testMedianColorRatioInvariant`, `testColorPickerGrabAtRatio2` all pass fresh. No `enable_opengl` reference remains in `src`, so a stored value is neither read nor rewritten. |
| 5 | Files split, one Save-as flow, no `#if 0`/`boost/foreach`; full ctest passes with baselines unchanged; Phase 5 menus behave as before | VERIFIED | `OutputGenerator.cpp` 268 lines plus five split files; `MainWindow.cpp` 1237 lines plus four split files. `git grep` for `#if 0` and `boost/foreach` in `src`: empty. `QFileDialog::getSaveFileName` only in `ProjectSaveFlow.cpp:34`; used from `MainWindowProject.cpp` and `OutOfMemoryDialog`. Fresh `ctest --preset macos-release`: 100% passed, 7 of 7 (`build/scratch/verify6/ctest-release.txt`). `git diff a796c2ae -- src/pipeline_tests/golden` is empty. All 12 changed files under output paths are on `records/output-path-allowlist.txt`; the `src/imageproc`/`src/dewarping` diff is only dead-code and foreach-include removal. Live `mac_check` PASS for both invocations is recorded and was not re-run (per instruction). Menu behaviour on the Cocoa display therefore rests on the recorded run plus offscreen `TestMacQt` (passes). |

**Score:** 4/5 truths verified (1 present, wired, behaviour not exercised on the real path)

### Required Artifacts

| Artifact | Status | Details |
|----------|--------|---------|
| `src/core/ImageSizeLimit.{h,cpp}` | VERIFIED | Substantive, used by four loaders |
| `src/core/MemoryBudget.{h,cpp}` | VERIFIED | Used by `MainWindowBatch.cpp`, `WorkerThreadPool.cpp`, `SystemLoadWidget.cpp` |
| `src/foundation/XmlMarshaller/XmlUnmarshaller` escape API | VERIFIED | Used by `ProjectWriter`/`ProjectReader` |
| `src/app/ProjectSaveFlow.{h,cpp}` | VERIFIED | Sole save dialog caller |
| `OutputGenerator*` split files | VERIFIED | Built and exercised by pipeline goldens |
| `MainWindow{Batch,Actions,Project,Pages}.cpp` | VERIFIED | In `src/app/CMakeLists.txt`, linked into the binary |

### Key Link Verification

| From | To | Status |
|------|----|--------|
| `MainWindow::startBatchProcessing` | `MemoryBudget::batchMemoryCap` -> pool and slider | WIRED (code read) |
| `ProjectWriter`/`ProjectReader` | escape helpers | WIRED |
| `MainWindow`, `OutOfMemoryDialog` | `ProjectSaveFlow` | WIRED |
| `TiffReader`/`ImageLoader`/`Jpeg`/`LoadFileTask` | `ImageSizeLimit` | WIRED |

### Requirements Coverage

All nine IDs are declared in plan frontmatter (06-16 declares all nine) and mapped to Phase 6 in REQUIREMENTS.md, which marks all nine complete. No orphaned requirements.

| Requirement | Source Plans | Status | Evidence |
|-------------|--------------|--------|----------|
| SAFE-01 | 06-04, 06-05, 06-06 | SATISFIED | Truth 1 |
| SAFE-02 | 06-07 | SATISFIED | Truth 2 |
| SAFE-03 | 06-08, 06-09, 06-16 | SATISFIED in code and tests; real-scan swap check pending (human) | Truth 3 |
| STRIP-07 | 06-10, 06-11 | SATISFIED | Truth 4 |
| CLEAN-01 | 06-13, 06-14 | SATISFIED | goldens unchanged, 7 output digests equal per `records/closing-check.md` (digest probe not re-run; goldens and pipeline tests re-run green) |
| CLEAN-02 | 06-15, 06-16 | SATISFIED | Split sizes, build, `TestMacQt` pass; live mac_check recorded |
| CLEAN-03 | 06-01 | SATISFIED | no `#if 0` in `src` |
| CLEAN-04 | 06-02, 06-03 | SATISFIED | no `boost/foreach` in `src` |
| CLEAN-05 | 06-12 | SATISFIED | single `getSaveFileName` caller, tests pass |

### Anti-Patterns Found

| File | Pattern | Severity | Impact |
|------|---------|----------|--------|
| `MainWindowPages.cpp:146` | `XXX` upstream note, moved verbatim | Info | Pre-existing, not new debt (see advisory) |
| Review findings WR-01 to WR-06, IN-01 to IN-03 | open, advisory (0 critical) | Warning | WR-02 (cap outlives batch) and WR-03 (cap assumes 600 dpi output) are documented limits of the cap; WR-06 is pre-existing behaviour moved. None negates a must-have. |

Other scans: no new FIXME/TBD in the Phase 6 diff; the old FIXME was removed. `06-SECURITY.md` `threats_open: 0`.

### Behavioral Spot-Checks

| Behavior | Command | Result |
|----------|---------|--------|
| Full suite | `ctest --preset macos-release` | 100% of 7 passed |
| Size limit suite | `core_tests --run_test=CoreImageSizeLimitTestSuite` | no errors |
| Round trip | `pipeline_tests --run_test='ProjectRoundTrip*'` | no errors |
| Named qt tests (each in its own class) | `qt_tests <names>` | all pass in their class (the "function not found" lines come from passing names to classes that do not own them, not from test failures) |
| No Qt OpenGL linking | `otool -L` | 0 |

Not re-run: Debug and ASan ctest (closing record shows both 100%; source unchanged since `a9d17d1c`), digest probe, live `mac_check`, memory probes (instructed).

### Human Verification Required

#### 1. Real-scan batch swap check (optional, from the closing record)

**Test:** Open a project of several large real scans in the built app, start batch processing, and watch Activity Monitor, tab Memory, "Swap Used".
**Expected:** Swap Used does not grow; the thread slider tooltip reads "limited to n by memory" for pages above about 85 MP.
**Why human:** Needs the app launched on real scans, which Claude does not do. All code, cap arithmetic and a synthetic 165.6 MP batch through the pool (swap 0) are already verified.

### Gaps Summary

No gaps. No goal-blocking defect found. The one open item is the ROADMAP wording of criterion 3 ("Activity Monitor shows no growth in swap") which has only synthetic, stand-in evidence; this is the reason for `human_needed` rather than `passed`. If you accept the synthetic memory run as sufficient, record it as an override and the phase is `passed`. Housekeeping: ROADMAP.md still shows Phase 6 as unchecked at line 20 and "In Progress" in the progress table although 16/16 plans are done; update it at phase completion.

---

_Verified: 2026-11-17_
_Verifier: Claude (gsd-verifier)_
