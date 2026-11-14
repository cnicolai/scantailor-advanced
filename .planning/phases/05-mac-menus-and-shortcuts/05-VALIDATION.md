---
phase: "05"
slug: "mac-menus-and-shortcuts"
# status lifecycle: draft (seeded by plan-phase) → validated (set by validate-phase §6)
# audit-milestone §5.5 distinguishes NOT-VALIDATED (draft) from PARTIAL (validated + nyquist_compliant: false) (#2117)
status: draft
nyquist_compliant: false
wave_0_complete: false
created: "2026-11-14"
---

# Phase 05 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
> Seeded from `05-RESEARCH.md` § Validation Architecture.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Qt Test 6.12.0 inside the existing `qt_tests` executable (new class `TestMacQt`), plus a live harness `mac_check` (not a ctest suite) |
| **Config file** | `src/qt_tests/CMakeLists.txt`, `src/qt_tests/main.cpp` |
| **Quick run command** | `cmake --build --preset macos-release && ctest --preset macos-release -R qt_tests` |
| **Full suite command** | `ctest --preset macos-release` and `ctest --preset macos-debug` (7 of 7); per commit `bash .planning/phases/03-qt-6-and-macos-only/gate.sh check <label>` |
| **Live check** | `build/macos-release/mac_check build/scratch/mac/all --section all -ApplePersistenceIgnoreState YES` (exit 0 pass, 1 fail, 2 usage or path outside `build/`, 3 platform not Cocoa) |
| **Estimated runtime** | ~60 seconds for `qt_tests`; full gate several minutes |

---

## Sampling Rate

- **After every task commit:** Run the quick run command and `xcrun clang-format --dry-run -Werror` on touched C++ files
- **After every plan wave:** Run the full suite command (Release and Debug) through `gate.sh check <label>`
- **Before `/gsd-verify-work`:** Full suite must be green, goldens unchanged (`git diff a796c2ae -- src/pipeline_tests/golden` empty), harness records complete
- **Max feedback latency:** 120 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| (filled from the PLAN.md files once they exist) | | | | | | | | | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/app/CMakeLists.txt` — `app_gui` static library; `scantailor-advanced` and `ui_snapshot` link it (closes review finding WR-01)
- [ ] `src/qt_tests/tst_mac_qt.{h,cpp}` — `TestMacQt` registered in `src/qt_tests/main.cpp` and `CMakeLists.txt`, with the string-scan helpers
- [ ] `src/app/mac_check/` — live harness (Objective-C++ probe, path guard like `retina_check`)
- [ ] `.planning/phases/05-mac-menus-and-shortcuts/records/` — verdict records (menubar, dialogs, keys, zone, context, panels, gate-log)
- [ ] `TESTING.md` — "Live Mac check" section; `qt_tests` row lists the new tests

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| The system beep is audible when a batch run finishes | MAC-10 | Sound output cannot be observed in-process | Covered by the source scan; the user hears it in normal use |
| A two-finger trackpad click opens the context menu | MAC-08 | At NSEvent level a two-finger click and a right-click are the same `rightMouseDown`; in-process injection cannot tell them apart | Harness proves `rightMouseDown` and Control-click; record states the indistinguishability |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 120s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
