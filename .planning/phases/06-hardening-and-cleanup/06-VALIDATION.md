---
phase: "06"
slug: "hardening-and-cleanup"
# status lifecycle: draft (seeded by plan-phase) → validated (set by validate-phase §6)
# audit-milestone §5.5 distinguishes NOT-VALIDATED (draft) from PARTIAL (validated + nyquist_compliant: false) (#2117)
status: draft
nyquist_compliant: false
wave_0_complete: false
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
| SAFE-01 | T-06-01, T-06-02 | Forged TIFF/PNG/progressive JPEG headers refused before any pixel or coefficient allocation; boundary cases; multi-page TIFF refuses only the oversized page | unit (`core_tests`) | `build/macos-release/core_tests --run_test=ImageSizeLimitTestSuite --log_level=message` | ❌ W0 `src/core/tests/TestImageSizeLimit.cpp` | ⬜ pending |
| SAFE-01 | T-06-01 | "Too large" message in the page view and file-list marking | Qt Test (`qt_tests`) | `build/macos-release/qt_tests` + PASS-line grep | ❌ W0 new slots | ⬜ pending |
| SAFE-02 | T-06-04 | Extended name set round-trips; ordinary files byte-identical; backslash `XFAIL_CHECK` (999.8) | `pipeline_tests` | `build/macos-release/pipeline_tests --run_test=ProjectRoundTripTestSuite --log_level=message` | ✅ file, ❌ W0 cases | ⬜ pending |
| SAFE-02 | T-06-04 | Escape helper: ordinary strings unchanged, invalid characters reversible | unit (`foundation_tests`) | `build/macos-release/foundation_tests --log_level=message` | ❌ W0 | ⬜ pending |
| SAFE-03 | T-06-03 | Cap arithmetic; pool count = min(ideal, slider, cap) | unit (`core_tests`) | `build/macos-release/core_tests --run_test=MemoryBudgetTestSuite` | ❌ W0 `src/core/tests/TestMemoryBudget.cpp` | ⬜ pending |
| STRIP-07 | — | Settings dialog has no OpenGL control | Qt Test | `qt_tests` PASS line of the new slot | ❌ W0 | ⬜ pending |
| STRIP-07 | — | Colour picker unchanged | Qt Test | PASS `TestHiDpiQt::testMedianColorRatioInvariant()`, `TestHiDpiQt::testColorPickerGrabAtRatio2()` | ✅ | ⬜ pending |
| STRIP-07 | — | No Qt OpenGL linkage | shell | `for b in scantailor-advanced qt_tests mac_check retina_check; do otool -L build/macos-release/$b \| grep -e QtOpenGL; done` prints nothing | n/a | ⬜ pending |
| CLEAN-01 | — | Pure move: goldens, output digests, chunk multiset | gate + script | `gate.sh check …`; digest table equal; chunk script exit 0 | ❌ W0 script | ⬜ pending |
| CLEAN-02 | — | Menus, shortcuts unchanged | Qt Test | all `TestMacQt` PASS lines (with `testBatchAlarmUsesBeep` updated for the new file) | ✅ | ⬜ pending |
| CLEAN-03 | — | No `#if 0`; preprocessed text unchanged | grep + `c++ -E -P` diff | `grep -rn -e '^[[:space:]]*#[[:space:]]*if[[:space:]]*0' src` empty | n/a | ⬜ pending |
| CLEAN-04 | — | No foreach includes | grep + gate | `grep -rn 'boost/foreach' src` empty | n/a | ⬜ pending |
| CLEAN-05 | — | One save-as flow: extension, last dir, recent list, write | Qt Test | new `qt_tests` slots | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/core/tests/TestImageSizeLimit.cpp` — SAFE-01 (+ `sources` entry; link `TIFF::TIFF`/`ZLIB::ZLIB` if forging needs it)
- [ ] `src/core/tests/TestMemoryBudget.cpp` — SAFE-03
- [ ] Extended cases in `src/pipeline_tests/TestProjectRoundTrip.cpp` — SAFE-02
- [ ] Escape-helper tests in `src/foundation/tests/` — SAFE-02
- [ ] New `qt_tests` slots: Settings without OpenGL, too-large `ErrorResult` message, shared save-as flow — STRIP-07, SAFE-01, CLEAN-05
- [ ] `records/` scripts: chunk-multiset script, digest harness, `gate-log.md` — CLEAN-01

Framework install: none.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| No swap growth during a batch of large pages | SAFE-03 | Uses a lot of RAM on the user's Mac (D-08); runs only on the user's go | `sysctl vm.swapusage` before/after plus peak footprint of the headless run; record in `records/memory-run.md` |
| Live menus and shortcuts as in Phase 5 | CLEAN-02 | `mac_check` moves the pointer and takes focus (D-00b) | `build/macos-release/mac_check build/scratch/mac/all --section all -ApplePersistenceIgnoreState YES` exit 0, on the user's go |
| Activity Monitor on real scans | SAFE-03 | Real scans are personal material | Optional UAT item; does not block the phase |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 120 s for the quick loop
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
