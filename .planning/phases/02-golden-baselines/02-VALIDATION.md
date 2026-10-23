---
phase: "2"
slug: "golden-baselines"
# status lifecycle: draft (seeded by plan-phase) → validated (set by validate-phase §6)
# audit-milestone §5.5 distinguishes NOT-VALIDATED (draft) from PARTIAL (validated + nyquist_compliant: false) (#2117)
status: validated
nyquist_compliant: true
wave_0_complete: true
created: "2026-10-23"
validated: "2026-10-23"
---

# Phase 2 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Boost.Test 1.92 (dynamic) for all new tests; Qt Test unchanged in `qt_tests` |
| **Config file** | none; registration via `add_test` in each test `CMakeLists.txt`; test presets `macos-release`, `macos-debug`, `macos-asan` in `CMakePresets.json` (`noTestsAction: error`, `outputOnFailure: true`) |
| **Quick run command** | `build/macos-release/pipeline_tests --run_test=<Suite>` (one suite) or `ctest --preset macos-release -R <test> --output-on-failure` |
| **Full suite command** | `cmake --build --preset macos-release && ctest --preset macos-release` |
| **Estimated runtime** | Release full suite ~56 s (`pipeline_tests` ~42–56 s); Debug ~230 s; ASan ~661 s |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build --preset macos-release && ctest --preset macos-release -R "<new_or_changed_test>" --output-on-failure`
- **After every plan wave:** Run `ctest --preset macos-release` and `ctest --preset macos-debug`
- **Before `/gsd-verify-work`:** Both presets green, one `macos-asan` run with findings recorded, `git diff` proof for TEST-04
- **Max feedback latency:** 30 seconds (one suite in Release)

---

## Per-Task Verification Map

Requirement-level map; each row names the plans whose tasks carry the automated verify. Status from the validation audit on 2026-10-23 (Release suite runs plus the full `ctest` in Release, Debug and ASan).

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 02-03 T1 | 02-03 | 2 | TEST-01 | — | N/A | unit | `pipeline_tests --run_test=GeneratorDeterminismTestSuite` | ✅ | ✅ green |
| 02-02 T1, 02-09 T1–T2 | 02-02, 02-09 | 2, 4 | TEST-02 | — | N/A | unit | `pipeline_tests --run_test=ImageCompareTestSuite` and `--run_test=ComparatorSelfTestSuite` | ✅ | ✅ green |
| 02-02 T2–T3 | 02-02, 02-05–02-07 | 2, 3 | TEST-03 | — | N/A | integration | `pipeline_tests --run_test=GoldenStoreTestSuite`; normal runs leave `src/pipeline_tests/golden` clean | ✅ | ✅ green |
| 02-10 T3 | 02-05–02-07, 02-10 | 3, 5 | TEST-04 | — | N/A | script/git | `git diff cde6825 --name-only -- src` minus test paths lists only `src/CMakeLists.txt`; baseline commit `a796c2ae` (records/closing-check.md) | ✅ | ✅ green |
| 02-01 T2, 02-05 T1–T2 | 02-01, 02-05 | 1, 3 | TEST-05 | — | N/A | integration | `pipeline_tests --run_test=FlatDocumentTestSuite` | ✅ | ✅ green |
| 02-03 T3, 02-06 T1–T3 | 02-03, 02-06 | 2, 3 | TEST-06 | — | N/A | integration | `pipeline_tests --run_test=BookSpreadTestSuite` and `--run_test=DewarpPageTestSuite` | ✅ | ✅ green |
| 02-03 T3, 02-07 T1–T3 | 02-03, 02-07 | 2, 3 | TEST-07 | — | N/A | integration | `pipeline_tests --run_test=MixedPageTestSuite` | ✅ | ✅ green |
| 02-04 T1 | 02-04 | 3 | TEST-08 | — | N/A | unit | `ctest --preset macos-release -R imageproc_tests` (`BinarizeTestSuite`) | ✅ | ✅ green |
| 02-04 T2 | 02-04 | 3 | TEST-09 | — | N/A | unit | `ctest --preset macos-release -R dewarping_tests` | ✅ | ✅ green |
| 02-08 T1–T2 | 02-08 | 3 | TEST-10 | — | N/A | integration | `pipeline_tests --run_test=ProjectRoundTripTestSuite` | ✅ | ✅ green |
| 02-01 T2, 02-09 T3, 02-10 T1 | 02-01, 02-04, 02-09, 02-10 | 1, 3, 4, 5 | TEST-11 | — | N/A | system | `ctest --preset macos-release` and `ctest --preset macos-debug` (7/7 each); `pipeline_tests --run_test=EnvironmentTestSuite` | ✅ | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

Known expected failures (`XFAIL_CHECK`, recorded in `02-FINDINGS.md`, backlog 999.1–999.7) are part of the green runs by design; the trip-wire fails a run if one of them starts passing.

---

## Wave 0 Requirements

- [x] `src/test_support/` library (generator, comparator, golden store, manifest, text golden, environment)
- [x] `src/pipeline_tests/` executable, global fixture, `HeadlessPipeline`
- [x] `src/dewarping/tests/` executable
- [x] Wiring lines in `src/CMakeLists.txt` (inside `if (BUILD_TESTS)`)
- [x] `ENVIRONMENT` (offscreen, pinned locale, isolated settings) for every new `add_test`, plus `qt_tests`
- [x] Measurement step 1 (D-12): Release vs Debug spread per image kind on the final fixtures (records/tolerance-spread.md)
- [x] Measurement step 2: ground-truth tolerance sweep (angle, split gap, dewarp ratio) before freezing D-15 values (records/scenario-*.md)
- [x] Findings file for D-18/D-19 and a backlog entry for each expected failure (02-FINDINGS.md, ROADMAP backlog 999.1–999.7)

---

## Manual-Only Verifications

All phase behaviors have automated verification.

Observation (not a Nyquist gap): the D-13 comparator self-test proves the frozen limits catch real regressions, but its three alterations also fail under the looser D-12 ceilings, so it does not by itself prove the limits are tight. The freeze rests on records/tolerance-spread.md (02-09 SUMMARY).

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 30s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved 2026-10-23

## Validation Audit 2026-10-23

| Metric | Count |
|---|---|
| Gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |
