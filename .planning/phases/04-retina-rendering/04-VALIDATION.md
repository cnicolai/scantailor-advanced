---
phase: "4"
slug: "retina-rendering"
# status lifecycle: draft (seeded by plan-phase) → validated (set by validate-phase §6)
# audit-milestone §5.5 distinguishes NOT-VALIDATED (draft) from PARTIAL (validated + nyquist_compliant: false) (#2117)
status: draft
nyquist_compliant: false
wave_0_complete: false
created: "2026-10-25"
---

# Phase 4 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Qt Test (Qt 6.12.0) in the existing `qt_tests` executable; Boost.Test for the other six suites |
| **Config file** | `src/qt_tests/CMakeLists.txt` (sources, `add_test(NAME qt_tests …)` environment), `src/qt_tests/main.cpp` (sets the environment, then `QTest::qExec` per test class) |
| **Quick run command** | `ctest --preset macos-release -R qt_tests` |
| **Full suite command** | `ctest --preset macos-release` and `ctest --preset macos-debug` (expect `100% tests passed out of 7`) |
| **Estimated runtime** | ~2 seconds quick; full Release + Debug run several minutes |

---

## Sampling Rate

- **After every task commit:** Run `ctest --preset macos-release -R qt_tests` and `xcrun clang-format --dry-run -Werror` on touched C++ files
- **After every plan wave:** Run `bash .planning/phases/03-qt-6-and-macos-only/gate.sh check <label>` on a committed tree (Release and Debug ctest, golden comparison with `a796c2ae`, build-command diff)
- **Before `/gsd-verify-work`:** Full suite must be green in Release and Debug, goldens untouched, live Studio Display run recorded
- **Max feedback latency:** ~10 seconds for the quick run including an incremental build

---

## Per-Task Verification Map

Filled in from the PLAN.md files once planning is complete. Requirement → test intent from research:

| Requirement | Behavior | Test Type | Automated Command | File Exists | Status |
|-------------|----------|-----------|-------------------|-------------|--------|
| HIDPI-01 | Build-directory binary reports DPR 2.0 on the Studio Display | manual-on-Mac (Claude-run harness, D-02) | harness run, recorded in `records/` | ❌ W0 | ⬜ pending |
| HIDPI-02 | HQ pixmap at device resolution at 2×, unchanged at 1×; 1-device-pixel edge stays one device pixel in `grab()`; pure DPR helpers | Qt Test, offscreen 2× screen | `ctest --preset macos-release -R qt_tests` | ❌ W0 | ⬜ pending |
| HIDPI-03 | Thumbnails written at Quality × DPR, cache keyed and flushed by DPR; `ThumbnailBase::paint` builds a DPR-2 temp pixmap; default Quality 250 | Qt Test | `ctest --preset macos-release -R qt_tests` | ❌ W0 | ⬜ pending |
| HIDPI-04 | Zone mask, stage-list frames and icon pixmaps carry the view ratio and logical geometry | Qt Test | `ctest --preset macos-release -R qt_tests` | ❌ W0 | ⬜ pending |
| HIDPI-05 | Moving the view 1×→2×→1× rebuilds HQ pixmap, thumbnails and stage frames | Qt Test, two-screen offscreen | `ctest --preset macos-release -R qt_tests` | ❌ W0 | ⬜ pending |
| HIDPI-06 | `medianColor` equal at 15×15/DPR 1 and 30×30/DPR 2; real 2× `grab` returns the patch colour | Qt Test | `ctest --preset macos-release -R qt_tests` | ❌ W0 | ⬜ pending |
| Output unchanged | No golden, manifest or output byte changes | Boost.Test goldens | `ctest --preset macos-release` and `ctest --preset macos-debug` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/qt_tests/tst_hidpi_qt.{h,cpp}` — HIDPI tests; registered in `src/qt_tests/main.cpp` and `src/qt_tests/CMakeLists.txt`
- [ ] `src/qt_tests/main.cpp` — two-screen offscreen config (`QT_QPA_PLATFORM=offscreen:configfile=<json>`, `QT_SCREEN_SCALE_FACTORS="OneX=1;TwoX=2"`)
- [ ] `src/core/HiDpi.{h,cpp}` — pure DPR helpers, listed in `common_sources` of `src/core/CMakeLists.txt`
- [ ] Diagnostic test accessors on `ImageViewBase` and `PictureZoneEditor`
- [ ] `.planning/phases/04-retina-rendering/records/` for the live verdicts

No framework install needed (Qt Test is already linked).

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| App binary DPR is 2.0 | HIDPI-01 | Needs the real Retina display | Claude runs the harness once on the Studio Display and records the value |
| Live sharpness at fit/100%/200%, thumbnails, masks, badges, handles, stage animation | HIDPI-02, HIDPI-03, HIDPI-04 | Needs the real display and Cocoa | Claude-run harness plus `grab()`, verdicts recorded in `records/` |
| Real display drag between Retina and non-Retina | HIDPI-05 | Only one display attached; simulated offscreen | Simulated two-screen test covers the code path; live drag recorded if a second display is available |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 10s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
