---
phase: "4"
slug: "retina-rendering"
# status lifecycle: draft (seeded by plan-phase) → validated (set by validate-phase §6)
# audit-milestone §5.5 distinguishes NOT-VALIDATED (draft) from PARTIAL (validated + nyquist_compliant: false) (#2117)
status: validated
nyquist_compliant: true
wave_0_complete: true
created: "2026-10-25"
---

# Phase 4 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Qt Test (Qt 6.12.0) in the existing `qt_tests` executable; Boost.Test for the other six suites |
| **Config file** | `src/qt_tests/CMakeLists.txt` (sources, `add_test(NAME qt_tests …)` environment), `src/qt_tests/main.cpp` (two-screen offscreen platform `OneX`/`TwoX`, then `QTest::qExec` per test class) |
| **Quick run command** | `ctest --preset macos-release -R qt_tests` |
| **Full suite command** | `ctest --preset macos-release` and `ctest --preset macos-debug` (expect `100% tests passed out of 7`) |
| **Live check** | `build/macos-release/retina_check <dir under build/> --section all` on the Cocoa display (not a ctest suite) |
| **Estimated runtime** | ~2 seconds quick; full Release + Debug run about 2 minutes |

---

## Sampling Rate

- **After every task commit:** Run `ctest --preset macos-release -R qt_tests` and `xcrun clang-format --dry-run -Werror` on touched C++ files
- **After every plan wave:** Run `bash .planning/phases/03-qt-6-and-macos-only/gate.sh check <label>` on a committed tree (Release and Debug ctest, golden comparison with `a796c2ae`, build-command diff)
- **Before `/gsd-verify-work`:** Full suite must be green in Release and Debug, goldens untouched, live Studio Display run recorded
- **Max feedback latency:** ~10 seconds for the quick run including an incremental build

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 04-01-01 | 01 | 1 | HIDPI-02, HIDPI-05, TESTX-03 | see 04-01 threat model | N/A | Qt Test offscreen 2x (`testHqPixmapAtDeviceResolution`, `testHqPixmapAtOneXUnchanged`) + gate r01 | `ctest --preset macos-release -R qt_tests` | ✅ | ✅ green |
| 04-01-02 | 01 | 1 | HIDPI-02, HIDPI-05 | see 04-01 threat model | N/A | Qt Test (`testHelpersMatchLegacyAtRatio1`, `testHelpersAtFractionalRatio`, `testHqGrabMatchesDeviceReference`, `testHqRebuildsOnScreenChange`, `testRepeatedRatioEventKeepsHqPixmap`, `testSmoothingSwitchPhysicalSize`) + gate r02 | `ctest --preset macos-release -R qt_tests` | ✅ | ✅ green |
| 04-02-01 | 02 | 2 | HIDPI-01, HIDPI-02 | see 04-02 threat model | harness writes only below `build/` | harness build + path-guard check + gate r03 | `ctest --preset macos-release -N` (7 suites) and `retina_check build/../retina-outside` exits 2 | ✅ | ✅ green |
| 04-02-02 | 02 | 2 | HIDPI-01, HIDPI-02 | see 04-02 threat model | N/A | live Cocoa run, recorded | `retina_check … --section page` → `verdict: PASS` (`records/hidpi-01.md`, `records/live-page-view.md`) | ✅ | ✅ green |
| 04-03-01 | 03 | 3 | HIDPI-03 | see 04-03 threat model | other thumbnail files never touched | Qt Test (`testThumbnailEffectiveSize`, `testThumbnailFilesKeyedByRatio`, `testOldThumbnailLeftInPlace`, `testStaleLoadDiscarded`) + gate r04 | `ctest --preset macos-release -R qt_tests` | ✅ | ✅ green |
| 04-03-02 | 03 | 3 | HIDPI-03 | see 04-03 threat model | N/A | Qt Test (`testThumbnailQualityDefault`) + gate r05 | `ctest --preset macos-release -R qt_tests` | ✅ | ✅ green |
| 04-04-01 | 04 | 4 | HIDPI-03, HIDPI-05 | see 04-04 threat model | N/A | Qt Test (`testThumbnailPaintAtRatio`) + gate r06 | `ctest --preset macos-release -R qt_tests` | ✅ | ✅ green |
| 04-04-02 | 04 | 4 | HIDPI-03, HIDPI-05 | see 04-04 threat model | N/A | Qt Test two-screen (`testThumbnailsFollowScreenChange`) + gate r07 | `ctest --preset macos-release -R qt_tests` | ✅ | ✅ green |
| 04-05-01 | 05 | 5 | HIDPI-04, HIDPI-05 | see 04-05 threat model | N/A | Qt Test (`testZoneMaskAtDeviceResolution`, `testZoneMaskOverlayConfined`, `testZoneMaskRebuildsOnScreenChange`) + gate r08 | `ctest --preset macos-release -R qt_tests` | ✅ | ✅ green |
| 04-05-02 | 05 | 5 | HIDPI-04, HIDPI-05 | see 04-05 threat model | N/A | Qt Test (`testStageAnimationFrames`, `testStageAnimationFollowsScreenChange`) + gate r09 | `ctest --preset macos-release -R qt_tests` | ✅ | ✅ green |
| 04-06-01 | 06 | 6 | HIDPI-04 | see 04-06 threat model | N/A | Qt Test (`testIconPixmapLogicalGeometry`, `testPageBadgeAtDeviceResolution`) + gate r10 | `ctest --preset macos-release -R qt_tests` | ✅ | ✅ green |
| 04-06-02 | 06 | 6 | HIDPI-06 | see 04-06 threat model | N/A | Qt Test (`testMedianColorRatioInvariant`, `testColorPickerGrabAtRatio2`) + gate r11 | `ctest --preset macos-release -R qt_tests` | ✅ | ✅ green |
| 04-07-01 | 07 | 7 | HIDPI-03, HIDPI-04, HIDPI-06 | see 04-07 threat model | harness writes only below `build/` | harness sections build + gate r12 | `ctest --preset macos-release` (7 suites) | ✅ | ✅ green |
| 04-07-02 | 07 | 7 | HIDPI-03, HIDPI-04, HIDPI-06 | see 04-07 threat model | records hold synthetic crops only | live Cocoa run, recorded | `retina_check … --section all` → `verdict: PASS` (`records/live-overlays.md`) | ✅ | ✅ green |
| 04-08-01 | 08 | 8 | all | see 04-08 threat model | N/A | docs + gate r13 | `bash .planning/phases/03-qt-6-and-macos-only/gate.sh check r13-docs` | ✅ | ✅ green |
| 04-08-02 | 08 | 8 | all | see 04-08 threat model | no output code or golden change | static diffs, ASan/UBSan `qt_tests`, closing record | `git diff a796c2ae -- src/pipeline_tests/golden` empty; `records/closing-check.md` ends `Phase 4 verdict: PASS` | ✅ | ✅ green |
| Output unchanged | all | all | — | — | — | Boost.Test goldens | `ctest --preset macos-release` and `ctest --preset macos-debug` (100% of 7 at `c3c389f1`) | ✅ | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `src/qt_tests/tst_hidpi_qt.{h,cpp}` — 24 HiDPI tests in `TestHiDpiQt`, registered in `src/qt_tests/main.cpp` and `src/qt_tests/CMakeLists.txt`
- [x] `src/qt_tests/main.cpp` — two-screen offscreen config (`QT_QPA_PLATFORM=offscreen:configfile=<json>`, `QT_SCREEN_SCALE_FACTORS="OneX=1;TwoX=2"`)
- [x] `src/core/HiDpi.{h,cpp}` — pure DPR helpers, listed in `common_sources` of `src/core/CMakeLists.txt`
- [x] Diagnostic test accessors on `ImageViewBase`, `PictureZoneEditor` and `StageListView`
- [x] `.planning/phases/04-retina-rendering/records/` for the live verdicts

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| App binary DPR is 2.0 | HIDPI-01 | Needs the real Retina display and Cocoa | Done 2026-10-25: `retina_check` on the Studio Display, recorded in `records/hidpi-01.md` (ratio 2, no `Info.plist` needed) |
| Live sharpness at fit/100%/200%, thumbnails, masks, badges, handles, stage animation, colour picker | HIDPI-02, HIDPI-03, HIDPI-04, HIDPI-06 | Needs the real display and Cocoa | Done 2026-10-25: `retina_check --section all`, recorded in `records/live-page-view.md` and `records/live-overlays.md` (all PASS) |
| Real display drag between Retina and non-Retina | HIDPI-05 | Only one display attached; the offscreen two-screen tests cover the code path | When a 1x display is attached: follow the re-check procedure in `records/closing-check.md` |
| Perceptual "as sharp as Preview" | HIDPI-02 | Visual judgement, not a pixel measure | Open a page at fit, 100% and 200% next to Preview |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 10s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved 2026-10-25

## Validation Audit 2026-10-25

| Metric | Count |
|---|---|
| Gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |
