---
phase: "2"
slug: "golden-baselines"
# status lifecycle: draft (seeded by plan-phase) → validated (set by validate-phase §6)
# audit-milestone §5.5 distinguishes NOT-VALIDATED (draft) from PARTIAL (validated + nyquist_compliant: false) (#2117)
status: draft
nyquist_compliant: false
wave_0_complete: false
created: "2026-10-23"
---

# Phase 2 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Boost.Test 1.92 (dynamic) for all new tests; Qt Test unchanged in `qt_tests` |
| **Config file** | none; registration via `add_test` in each test `CMakeLists.txt`; test presets `macos-release`, `macos-debug`, `macos-asan` in `CMakePresets.json` (`noTestsAction: error`, `outputOnFailure: true`) |
| **Quick run command** | `ctest --preset macos-release -R pipeline_tests --output-on-failure` |
| **Full suite command** | `cmake --build --preset macos-release && ctest --preset macos-release` |
| **Estimated runtime** | ~30 seconds per targeted Release run; a 300 dpi chain is 0.2–0.6 s Release, 0.5–3 s Debug, 1.6–9 s ASan |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build --preset macos-release && ctest --preset macos-release -R "<new_or_changed_test>" --output-on-failure`
- **After every plan wave:** Run `ctest --preset macos-release` and `ctest --preset macos-debug`
- **Before `/gsd-verify-work`:** Both presets green, one `macos-asan` run with findings recorded, `git diff` proof for TEST-04
- **Max feedback latency:** 30 seconds (targeted Release run)

---

## Per-Task Verification Map

Requirement-level map from research; task IDs are assigned by the plans.

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| TBD | TBD | TBD | TEST-01 | — | N/A | unit | `ctest --preset macos-release -R pipeline_tests` (`GeneratorDeterminismTestSuite`) | ❌ W0 | ⬜ pending |
| TBD | TBD | TBD | TEST-02 | — | N/A | unit | `pipeline_tests --run_test=ComparatorSelfTestSuite` | ❌ W0 | ⬜ pending |
| TBD | TBD | TBD | TEST-03 | — | N/A | integration | `pipeline_tests --run_test=GoldenStoreTestSuite` | ❌ W0 | ⬜ pending |
| TBD | TBD | TBD | TEST-04 | — | N/A | script/git | `git diff --stat cde6825 -- src ':!src/**/tests' ':!src/test_support' ':!src/pipeline_tests' ':!src/qt_tests'` | ❌ W0 | ⬜ pending |
| TBD | TBD | TBD | TEST-05 | — | N/A | integration | `pipeline_tests --run_test=FlatDocumentTestSuite` | ❌ W0 | ⬜ pending |
| TBD | TBD | TBD | TEST-06 | — | N/A | integration | `pipeline_tests --run_test=BookSpreadTestSuite` | ❌ W0 | ⬜ pending |
| TBD | TBD | TBD | TEST-07 | — | N/A | integration | `pipeline_tests --run_test=MixedPageTestSuite` | ❌ W0 | ⬜ pending |
| TBD | TBD | TBD | TEST-08 | — | N/A | unit | `ctest --preset macos-release -R imageproc_tests` | partial (`#if 0`) | ⬜ pending |
| TBD | TBD | TBD | TEST-09 | — | N/A | unit | `ctest --preset macos-release -R dewarping_tests` | ❌ W0 | ⬜ pending |
| TBD | TBD | TBD | TEST-10 | — | N/A | integration | `pipeline_tests --run_test=ProjectRoundTripTestSuite` | ❌ W0 | ⬜ pending |
| TBD | TBD | TBD | TEST-11 | — | N/A | system | `ctest --preset macos-release` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/test_support/` library (generator, comparator, golden store, manifest, text golden, environment)
- [ ] `src/pipeline_tests/` executable, global fixture, `HeadlessPipeline`
- [ ] `src/dewarping/tests/` executable
- [ ] Wiring lines in `src/CMakeLists.txt` (inside `if (BUILD_TESTS)`)
- [ ] `ENVIRONMENT` (offscreen, pinned locale, isolated settings) for every new `add_test`, plus `qt_tests`
- [ ] Measurement step 1 (D-12): Release vs Debug spread per image kind on the final fixtures
- [ ] Measurement step 2: ground-truth tolerance sweep (angle, split gap, dewarp ratio) before freezing D-15 values
- [ ] Findings file for D-18/D-19 and a backlog entry for each expected failure

---

## Manual-Only Verifications

All phase behaviors have automated verification.

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
