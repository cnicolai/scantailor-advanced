---
phase: "06"
slug: "hardening-and-cleanup"
# status lifecycle: draft (seeded by plan-phase) → validated (set by validate-phase §6)
# audit-milestone §5.5 distinguishes NOT-VALIDATED (draft) from PARTIAL (validated + nyquist_compliant: false) (#2117)
status: validated
nyquist_compliant: true
wave_0_complete: true
created: "2026-11-16"
---

# Phase 06 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution. Source: `06-RESEARCH.md` § Validation Architecture.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Boost.Test 1.92 (`foundation_tests`, `math_tests`, `imageproc_tests`, `core_tests`, `pipeline_tests`, `dewarping_tests`) and Qt Test 6.12.0 (`qt_tests`); 7 ctest suites |
| **Config file** | `src/core/tests/CMakeLists.txt` (`sources` list), `src/pipeline_tests/CMakeLists.txt` (GLOB, `CONFIGURE_DEPENDS`), `src/qt_tests/CMakeLists.txt`, `CMakePresets.json` |
| **Quick run command** | `cmake --build --preset macos-release --target core_tests qt_tests && build/macos-release/core_tests --log_level=message && build/macos-release/qt_tests > build/scratch/qt.txt 2>&1` (grep `PASS   : TestX::name()` and `Totals: … 0 failed, 0 skipped`) |
| **Full suite command** | `ctest --preset macos-release` and `ctest --preset macos-debug` (both `100% tests passed out of 7`); per commit `bash .planning/phases/03-qt-6-and-macos-only/gate.sh check <label>` |
| **Estimated runtime** | quick ~60 s after an incremental build; full gate several minutes (Release + Debug) |

---

## Sampling Rate

- **After every task commit:** build the touched targets, run the quick command, and `xcrun clang-format --dry-run -Werror` on touched C++ files
- **After every commit that changes build inputs:** `gate.sh check <label>` (Release + Debug ctest, goldens unchanged, compile-command diff)
- **Before `/gsd-verify-work`:** both presets green, goldens unchanged, all greps below empty, `otool -L` check, CLEAN-01 digest table reproduced, ASan run of the new SAFE-01/SAFE-02 suites, then the user-approved live `mac_check` and memory run
- **Max feedback latency:** quick loop ~60 s; gate runs in the background

---

## Per-Task Verification Map

Requirement-level map; the planner binds task IDs in the PLAN files.

| Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| SAFE-01 | T-06-01, T-06-02 | Forged TIFF/PNG/progressive JPEG headers refused before any pixel or coefficient allocation; boundary cases; multi-page TIFF refuses only the oversized page; the limit applies to source scans only (`loadSource`), not to the app's own files (WR-01); a progressive JPEG inside the limit is not decompressed for metadata (WR-04); refusals measured with the sampled footprint (WR-05) | unit (`core_tests`) | `build/macos-release/core_tests --run_test=CoreImageSizeLimitTestSuite --log_level=test_suite` (16 cases, incl. `test_own_files_are_not_limited_but_source_files_are`, `test_progressive_jpeg_inside_the_limit_is_not_decompressed_for_metadata`) | ✅ `src/core/tests/TestImageSizeLimit.cpp` | ✅ green |
| SAFE-01 | T-06-01 | "Too large" message in the page view, file-list marking, insert flow, thumbnail refusal, wide real PNGs | Qt Test (`qt_tests`) | `ctest --preset macos-release -R '^qt_tests$' -V`: PASS `testProjectFilesDialogMarksOversizedFile`, `testOversizedThumbnailFails`, `testLargeGreyPngLoads` and the page-view too-large rows | ✅ | ✅ green |
| SAFE-02 | T-06-04 | Extended name set (control characters, D-10 matrix) round-trips; ordinary files byte-identical and flag-free; backslash round-trips with plain checks (no `XFAIL_CHECK` needed, 999.8 untouched) | `pipeline_tests` | `build/macos-release/pipeline_tests --run_test=ProjectRoundTripTestSuite` (8 cases) and `--run_test=ProjectRoundTripBackslashTestSuite` (1 case) | ✅ | ✅ green |
| SAFE-02 | T-06-04 | Escape helper: ordinary strings unchanged, invalid characters reversible, flag must be exactly `1` | unit (`foundation_tests`) | `build/macos-release/foundation_tests --run_test=FoundationXmlEscapeTestSuite` (7 cases) | ✅ `src/foundation/tests/TestXmlEscape.cpp` | ✅ green |
| SAFE-03 | T-06-03 | Cap arithmetic; pool count = min(ideal, slider, cap); slider tooltip; clamped shared setting reader; page estimate at each page's own output dpi (WR-03) | unit (`core_tests`) + Qt Test | `core_tests --run_test=CoreMemoryBudgetTestSuite` (10 cases, incl. `test_page_pixel_estimate_at_the_output_dpi`, `test_largest_page_pixels_uses_the_output_dpi_of_each_page`); PASS `testWorkerPoolHonoursMemoryCap`, `testStoredThreadCountClamps`, `testSystemLoadToolTipShowsMemoryLimit` | ✅ `src/core/tests/TestMemoryBudget.cpp` | ✅ green |
| STRIP-07 | — | Settings dialog has no OpenGL control; no OpenGL in sources or CMake | Qt Test | PASS `TestMacQt::testSettingsHasNoOpenGlControl()`, `TestMacQt::testNoOpenGlInSourcesOrBuild()` | ✅ | ✅ green |
| STRIP-07 | — | Colour picker unchanged | Qt Test | PASS `TestHiDpiQt::testMedianColorRatioInvariant()`, `TestHiDpiQt::testColorPickerGrabAtRatio2()` | ✅ | ✅ green |
| STRIP-07 | — | No Qt OpenGL linkage | shell | `for b in scantailor-advanced qt_tests mac_check retina_check; do otool -L build/macos-release/$b \| grep -e QtOpenGL; done` prints nothing | n/a | ✅ green |
| CLEAN-01 | — | Pure move: goldens, output digests, chunk multiset | gate + script | gates s17–s19 PASS; seven `DIGEST` lines equal before/after (`records/output-digests.md`); `python3 -I records/move_check.py --base e5319cd9 …` over all six files gives `RESULT: PASS` (79/79) | ✅ `records/move_check.py` | ✅ green |
| CLEAN-02 | — | Menus, shortcuts unchanged | Qt Test + script | all `TestMacQt` PASS lines (`testBatchAlarmUsesBeep` reads `MainWindowBatch.cpp`); move check against 590e751f PASS; gates s20–s22 PASS | ✅ | ✅ green |
| CLEAN-03 | — | No `#if 0`; preprocessed text unchanged | grep + `records/pp_equiv.py` | `grep -rn -e '^[[:space:]]*#[[:space:]]*if[[:space:]]*0' src` empty (7 files at e5319cd9 as positive control) | n/a | ✅ green |
| CLEAN-04 | — | No foreach includes | grep + gate | `grep -rn 'boost/foreach' src` empty (26 files at e5319cd9) | n/a | ✅ green |
| CLEAN-05 | — | One save-as flow: extension, last dir, recent list, write, cancel, save twice; a failed Save while closing keeps the project open (WR-06) | Qt Test | PASS `testProjectExtension`, `testSaveAsFlowFromOutOfMemoryDialog`, `testSaveAsFlowRunsTwice`, `TestMacQt::testFailedSaveWhileClosingKeepsTheProject` | ✅ | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `src/core/tests/TestImageSizeLimit.cpp` — SAFE-01 (+ `sources` entry; link `TIFF::TIFF`/`ZLIB::ZLIB` if forging needs it)
- [x] `src/core/tests/TestMemoryBudget.cpp` — SAFE-03
- [x] Extended cases in `src/pipeline_tests/TestProjectRoundTrip.cpp` — SAFE-02
- [x] Escape-helper tests in `src/foundation/tests/` — SAFE-02
- [x] New `qt_tests` slots: Settings without OpenGL, too-large `ErrorResult` message, shared save-as flow — STRIP-07, SAFE-01, CLEAN-05
- [x] `records/` scripts: chunk-multiset script, digest harness, `gate-log.md` — CLEAN-01

Framework install: none.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| No swap growth during a batch of large pages | SAFE-03 | Uses a lot of RAM on the user's Mac (D-08); runs only on the user's go | Done 2026-11-17 on the user's go: cap 5 of 10, peak 17737 MiB of 32768, swap 0 throughout (`records/memory-run.md`). Original instructions: `sysctl vm.swapusage` before/after plus peak footprint of the headless run; record in `records/memory-run.md` |
| Live menus and shortcuts as in Phase 5 | CLEAN-02 | `mac_check` moves the pointer and takes focus (D-00b) | Done 2026-11-17 on the user's go: both runs `verdict: PASS`, section verdicts equal to Phase 5 (`records/closing-check.md`). Original instructions: `build/macos-release/mac_check build/scratch/mac/all --section all -ApplePersistenceIgnoreState YES` exit 0, on the user's go |
| Batch start sets the memory cap, batch stop and project switch clear it (app wiring of WR-02/WR-03) | SAFE-03 | `MainWindow` members are private and a batch needs a processing queue; pool and widget behaviour with a cap is automated | Done 2026-11-17: `06-UAT.md` test 1, pass |
| Cancelling Save As while closing a never-saved project keeps it open (WR-06, first branch) | CLEAN-05 | Such a project only comes from the New Project wizard, not reachable offscreen; the second branch is automated | Done 2026-11-17: `06-UAT.md` test 2, pass |
| Activity Monitor on real scans | SAFE-03 | Real scans are personal material | Optional UAT item; does not block the phase |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 120 s for the quick loop
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** validated 2026-11-17 (validate-phase audit: 13 rows, 0 gaps; re-audit after the WR-01–WR-06 review fixes: 0 gaps, rows SAFE-01/SAFE-03/CLEAN-05 updated, two UAT-covered wiring checks added to Manual-Only)

## Validation Audit 2026-11-17

| Metric | Count |
|---|---|
| Gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |
