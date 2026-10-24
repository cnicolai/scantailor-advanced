---
phase: "3"
slug: "qt-6-and-macos-only"
# status lifecycle: draft (seeded by plan-phase) → validated (set by validate-phase §6)
# audit-milestone §5.5 distinguishes NOT-VALIDATED (draft) from PARTIAL (validated + nyquist_compliant: false) (#2117)
status: draft
nyquist_compliant: false
wave_0_complete: false
created: "2026-10-24"
---

# Phase 3 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Boost.Test (6 executables) and Qt Test (`qt_tests`), run through CTest presets |
| **Config file** | `CMakePresets.json` (`macos-release`, `macos-debug`, `macos-asan`) and the per-directory `CMakeLists.txt` files |
| **Quick run command** | `cmake --build --preset macos-release && ctest --preset macos-release` |
| **Full suite command** | the quick command, then `cmake --preset macos-debug && cmake --build --preset macos-debug && ctest --preset macos-debug`; both must report 7/7 with `git status --porcelain -- src/pipeline_tests/golden` empty |
| **Estimated runtime** | ~120 seconds for Release (build about 60 s, ctest about 59 s); ~330 seconds for Debug (build about 58 s, ctest 230–270 s); worst-case full gate about 7–8 minutes |

---

## Sampling Rate

- **After every task commit:** Run the quick command for C++-only edits. For any CMake edit, also diff `ninja -t commands` for the 8 targets against the baseline lists in `build/scratch/`.
- **After every strip commit (D-11):** Run the full suite command, the goldens-unchanged check (`git diff --quiet a796c2ae -- src/pipeline_tests/golden`) and the leftover greps from RESEARCH.md § Validation Architecture.
- **Before `/gsd-verify-work`:** Full suite green, plus the fresh clone (D-13), the `macos-asan` comparison (D-14), the offscreen screenshots (D-12) and every leftover grep empty.
- **Max feedback latency:** 480 seconds (one full Release and Debug gate)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| *Filled by the planner from the PLAN.md `<verify>` blocks; see RESEARCH.md § Validation Architecture for the requirement-to-command map.* | | | | | | | | | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/app/ui_snapshot/main.cpp` and its `BUILD_TESTS` block in `src/app/CMakeLists.txt`: offscreen grabs of the main window and the Settings dialog, plus the `--assert-no-language` check (SC4, D-12). Not registered with ctest, so the 7/7 gate stays as it is.
- [ ] `build/scratch/cmds-*-baseline.txt`: baseline `ninja -t commands` lists for Release and Debug, taken on the unmodified tree before the first strip commit.
- [ ] "Before" artefacts: `nm -u` translator count (5), `strings` hits for `scantailor_*.qm` (1), `build.ninja` translation references (40), `languageBox` in `ui_SettingsDialog.h` (4), baseline PNGs.

*No test framework install is needed.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| — | — | — | — |

*All phase behaviors have automated verification. Per D-12, the executor views the offscreen PNGs itself and records the verdict; the user does no GUI step.*

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 480s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
