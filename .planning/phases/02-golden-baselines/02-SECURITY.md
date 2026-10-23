---
phase: "2"
slug: "golden-baselines"
status: verified
# threats_open = count of OPEN threats at or above workflow.security_block_on severity (the blocking gate)
threats_open: 0
asvs_level: 1
created: "2026-10-23"
---

# Phase 2 — Security

> Per-phase security contract: threat register, accepted risks, and audit trail.

Register source: the `<threat_model>` blocks of plans 02-01 to 02-10 (authored at plan time) plus the `## Threat Flags` sections of the ten SUMMARYs (all "None"). Verification depth: ASVS level 1 (evidence located in committed source, records, golden manifests and git history), block threshold `high`. Paths: `records/` = `.planning/phases/02-golden-baselines/records/`.

---

## Trust Boundaries

| Boundary | Description | Data Crossing |
|----------|-------------|---------------|
| Test process → user's home folder | A Qt application object reads and writes QSettings and standard paths of the real user unless redirected | settings (private) |
| Test process → file system outside the project | Temporary files default to the system temp folder | temporary files |
| Build tools and test programs → /opt/homebrew, Xcode / CLT, macOS | Reads outside the project that the global rule gates (approved for Phase 2 only) | headers, libraries, plug-ins |
| Test wiring → production build | CMake edits could change production targets | build configuration |
| Normal test runs → tracked goldens | A comparison run could silently rewrite the baselines it checks against | golden images and text (integrity) |
| Test records → git history | Committed logs could carry home-directory paths | file paths (private) |

---

## Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation | Status |
|-----------|----------|-----------|----------|-------------|------------|--------|
| T-02-01 | Information disclosure | reads outside the project by build tools and test programs | medium | mitigate | `records/access-approval.md` starts `Decision: approve` (scope 02-01 to 02-10), committed first (`dccdcda`) before any build command | closed |
| T-02-02 | Information disclosure | `pipeline_tests` global fixture, `DefaultParamsProvider` | medium | mitigate | `src/pipeline_tests/main.cpp` calls `QSettings::setPath` under `build/<preset>/test-tmp` and checks `QSettings().fileName()`; `EnvironmentTestSuite` passes | closed |
| T-02-03 | Tampering | temporary files of `HeadlessPipeline` and the fixture | low | mitigate | `TMPDIR` in `set_tests_properties` (`src/pipeline_tests/CMakeLists.txt`) and in the fixture | closed |
| T-02-04 | Tampering | `src/CMakeLists.txt` and production targets | medium | mitigate | Non-test diff since `cde6825` is `src/CMakeLists.txt` only, and every changed line is an `add_subdirectory` line | closed |
| T-02-05 | Tampering | `GoldenStore` normal runs | medium | mitigate | `UPDATE_GOLDENS` gate in `GoldenStore.cpp`; `test_normal_run_never_writes`; `git status --porcelain -- src/pipeline_tests/golden` empty after every run | closed |
| T-02-06 | Tampering | interrupted update | low | mitigate | `QSaveFile` in `GoldenStore.cpp`; `test_update_leaves_no_temporary_files` | closed |
| T-02-07 | Repudiation | golden provenance | low | mitigate | `MANIFEST.txt` in `golden/flat`, `golden/book`, `golden/mixed` with toolchain, `build_type` and `source_commit` | closed |
| T-02-08 | Tampering | suite/name path components | low | mitigate | `^[a-z0-9_]+$` name check in `GoldenStore.cpp`; `test_invalid_name_rejected`; `clearArtifacts` re-checks the artifact root before its recursive delete | closed |
| T-02-09 | Information disclosure | test inputs | low | mitigate | Inputs generated in memory; the only tracked images under `src/` outside resources are the goldens | closed |
| T-02-10 | Tampering | generator determinism | low | mitigate | No `rand()`, `<random>`, `QRandomGenerator` or libm trigonometry in `PageGenerator.cpp` / `Rng.h`; `GeneratorDeterminismTestSuite` passes | closed |
| T-02-11 | Repudiation | fixture design vs. defect findings | low | mitigate | `test_flat_page_unchanged` pins the flat page fingerprints | closed |
| T-02-12 | Tampering | `TestScale.cpp` reference change | low | mitigate | Investigation commit `9e35594` precedes the change `184fe85`; new reference is an exact area average (`records/unit-tests.md`) | closed |
| T-02-13 | Repudiation | binarizer and dewarper defects | low | mitigate | F-04-1 and F-04-2 in `02-FINDINGS.md`, backlog 999.1–999.2 | closed |
| T-02-14 | Tampering | `src/CMakeLists.txt`, module CMake files | low | mitigate | `src/dewarping/CMakeLists.txt` and `src/imageproc/CMakeLists.txt` equal `cde6825` | closed |
| T-02-15 | Tampering | flat goldens | medium | mitigate | `build_type = Release` in `golden/flat/MANIFEST.txt`; golden commit `2f6df32` touches only `golden/flat/` | closed |
| T-02-16 | Repudiation | tolerance choices | low | mitigate | `## Tolerance decisions` in `records/scenario-flat.md` (and the book/mixed records) with measured worst cases | closed |
| T-02-17 | Tampering | book goldens | medium | mitigate | `build_type = Release` in `golden/book/MANIFEST.txt`; golden commit `5afce63` touches only `golden/book/` | closed |
| T-02-18 | Repudiation | split and dewarp shortfalls | low | mitigate | F-06-1 (`XFAIL_CHECK`) and F-06-2 in `02-FINDINGS.md`; hard checks stay strict | closed |
| T-02-19 | Tampering | mixed goldens | medium | mitigate | `build_type = Release` in `golden/mixed/MANIFEST.txt`; golden commit `a796c2a` touches only `golden/mixed/` | closed |
| T-02-20 | Repudiation | AUTO-layout mis-cut and other mixed-page shortfalls | low | mitigate | F-07-1 in `02-FINDINGS.md`; `XFAIL_CHECK` in `TestMixedPage.cpp` | closed |
| T-02-21 | Tampering | files with special-character names | low | mitigate | Created inside `QTemporaryDir` in `TestProjectRoundTrip.cpp` under the redirected temp root | closed |
| T-02-22 | Tampering | `DefaultParamsProvider` state | low | mitigate | Fixture in `TestProjectRoundTrip.cpp` saves params and profile (lines 155–156), restores them (line 179) and checks the profile name (line 180); random-order run in `records/ctest-phase2.md` | closed |
| T-02-23 | Tampering | frozen limits | medium | mitigate | One constants file `src/test_support/Tolerances.h`; freezing rule and measurement in `records/tolerance-spread.md` | closed |
| T-02-24 | Tampering | self-test and goldens | medium | mitigate | `test_golden_directory_untouched` in `TestComparatorSelfTest.cpp`; golden tree clean after `UPDATE_GOLDENS=1` self-test runs | closed |
| T-02-25 | Information disclosure | `qt_tests` settings | medium | mitigate | `src/qt_tests/main.cpp` calls `QSettings::setPath` and returns 2 if the settings file is elsewhere | closed |
| T-02-26 | Information disclosure | `records/test-cases-asan.txt`, `closing-check.md` | low | mitigate | `$HOME` occurs 0 times in both files (paths normalised to `{repo}`, `~`) | closed |
| T-02-27 | Repudiation | TEST-04 claim | medium | mitigate | `records/closing-check.md` records the git proof and names baseline commit `a796c2ae` | closed |
| T-02-28 | Tampering | `.planning/ROADMAP.md` | low | mitigate | Additions only, under `## Backlog` (999.1–999.7) | closed |
| T-02-SC | Tampering | package installs | low | accept | No package installed in this phase; clang-format used from the existing Xcode toolchain (`xcrun`) | closed |

*Status: open · closed · open — below high threshold (non-blocking)*
*Severity: critical > high > medium > low — only open threats at or above workflow.security_block_on count toward threats_open*
*Disposition: mitigate (implementation required) · accept (documented risk) · transfer (third-party)*

---

## Accepted Risks Log

| Risk ID | Threat Ref | Rationale | Accepted By | Date |
|---------|------------|-----------|-------------|------|
| AR-02-01 | T-02-SC | No package was installed in Phase 2, so there is no supply-chain surface to mitigate; the Xcode-bundled clang-format was used instead of installing one | plan-time register (plans 02-01 to 02-10) | 2026-10-23 |

---

## Security Audit Trail

| Audit Date | Threats Total | Closed | Open | Run By |
|------------|---------------|--------|------|--------|
| 2026-10-23 | 29 | 29 | 0 | secure-phase orchestrator (L1 evidence checks, no auditor needed) |

---

## Sign-Off

- [x] All threats have a disposition (mitigate / accept / transfer)
- [x] Accepted risks documented in Accepted Risks Log
- [x] `threats_open: 0` confirmed
- [x] `status: verified` set in frontmatter

**Approval:** verified 2026-10-23
