---
phase: "3"
slug: "qt-6-and-macos-only"
status: verified
# threats_open = count of OPEN threats at or above workflow.security_block_on severity (the blocking gate)
threats_open: 0
asvs_level: 1
created: "2026-10-24"
---

# Phase 3 — Security

> Per-phase security contract: threat register, accepted risks, and audit trail.

---

## Trust Boundaries

| Boundary | Description | Data Crossing |
|----------|-------------|---------------|
| Build and tests ↔ Homebrew / Xcode toolchain | Build tools and test programs read the installed toolchain; the goldens depend on its exact versions | Compiler, Qt, Boost and image libraries (read only; nothing installed or upgraded) |
| Test tools ↔ user settings | `ui_snapshot` and the test suites create `QSettings` stores | Settings files, kept below `build/` instead of `~/.config` and `~/Library` |
| Repository ↔ GitHub | Workflows and the issue helper ran with repository or `gh` credentials | CI and Dependabot runs, issue comments (all removed; nothing pushed) |
| Committed records ↔ local machine | Records and screenshots are produced from command output on this Mac | Local paths and UI images (paths normalised to `~`; screenshots show synthetic UI only) |

---

## Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation | Status |
|-----------|----------|-----------|----------|-------------|------------|--------|
| T-03-01 | Tampering / Information disclosure | `src/app/ui_snapshot/main.cpp` | high | mitigate | `QSettings::setPath` into the output directory before the application object exists, distinct organisation and application names (`scantailor-tests` / `ui_snapshot`), exit 2 for a settings file or output directory outside the allowed locations (`main.cpp:111-158`) | closed |
| T-03-02 | Information disclosure | `records/*.md`, `records/screenshots/` | low | mitigate | Paths normalised; no record contains the home path; screenshots show synthetic UI only | closed |
| T-03-03 | Tampering | Qt version gates in `DistortionModelBuilder.cpp`, `DetectVertContentBounds.cpp` | high | mitigate | `unifdef -DQT_VERSION_MAJOR=6` only (0 additions); both files keep `QRandomGenerator`, no `qsrand`/`qrand`; dewarp goldens unchanged in Release and Debug | closed |
| T-03-04 | Tampering | `CMakeLists.txt` and subdirectory CMake files | medium | mitigate | `ninja -t commands` diff empty for c02 and c03, limited to the D-15 define for c01 (`records/gate-log.md`) | closed |
| T-03-05 | Repudiation | golden manifest provenance (D-15) | low | mitigate | No `TOOLCHAIN DIFFERS` line for `qt_compile`; golden directory unchanged since `a796c2ae` | closed |
| T-03-06 | Elevation of privilege | `.github/`, `scripts/post_issue_followups.py` | low | mitigate | Content checked, then deleted; nothing tracked under `.github`; no automation with repository or `gh` credentials remains | closed |
| T-03-07 | Denial of service | `CMakeLists.txt` | medium | mitigate | Blocks still read by the Mac build kept; configure before commit; gate builds and tests both presets; command diff empty | closed |
| T-03-08 | Tampering | `Utils.cpp`, colour schemes, `ByteOrder.h`, `RelinkablePath.cpp` | medium | mitigate | `unifdef -U` only; the one added line is the macOS side rejoined by clang-format; macOS lines kept; gate passed | closed |
| T-03-09 | Tampering | `RelinkablePath::normalize` backslash handling | low | accept | Pre-existing behaviour kept on purpose (D-03); backlog item 999.8 | closed |
| T-03-10 | Tampering / Information disclosure | `src/app/main.cpp`, `DefaultParamsProfileManager.cpp` | medium | mitigate | Only the never-run portable branch removed; INI format line (`main.cpp:28`) and AppData profile path (`DefaultParamsProfileManager.cpp:19`) unchanged; user files never read or written | closed |
| T-03-11 | Denial of service | `CMakeLists.txt` configure | medium | mitigate | `.desktop.in` `configure_file` and `src/resources/unix/` removed in the same commit (`450ff19f`); gate built both presets | closed |
| T-03-12 | Elevation of privilege | `MainWindow.cpp` batch alarm (`std::system`) | low | accept | Pre-existing; kept until Phase 5 replaces it (D-04, MAC-10) | closed |
| T-03-13 | Tampering | `ApplicationSettings`, `SettingsDialog::commitChanges` | low | mitigate | No reader or writer of the `language` key remains in `src`; nothing deletes or rewrites an existing key (D-07) | closed |
| T-03-14 | Information disclosure | `ui_snapshot` after run | low | mitigate | Output and settings below `build/scratch/ui-after`; PNGs show synthetic UI only | closed |
| T-03-15 | Tampering | Homebrew toolchain behind the goldens | high | mitigate | `brew install` documented, never executed; `brew list --versions` read only; all package versions equal the before-state | closed |
| T-03-16 | Information disclosure | `README.md`, `TESTING.md`, `records/fresh-clone.md` | low | mitigate | Only `~`-relative locations documented; no home path in any of them | closed |
| T-03-17 | Tampering | clone location | low | mitigate | Clone and output in gitignored `build/fresh-clone` and `build/scratch/fresh-clone` | closed |
| T-03-18 | Repudiation | D-14 sanitizer comparison | medium | mitigate | Executables run directly; counts by script from committed raw output; 0 new locations | closed |
| T-03-19 | Information disclosure | `records/test-cases-asan.txt`, `records/leftover-grep.md` | low | mitigate | Paths normalised, colour codes removed; no home path | closed |
| T-03-20 | Tampering | `.claude/CLAUDE.md` profile section | medium | mitigate | `GSD:profile` section byte-identical before and after `e1451ad1`; no stamp or remap | closed |
| T-03-21 | Repudiation | phase verdict | low | mitigate | `records/closing-check.md` cites the record or commit behind every criterion, requirement and decision | closed |
| T-03-SC | Tampering | package installs | low | accept | No package installed, upgraded or removed in any plan | closed |

*Status: open · closed · open — below high threshold (non-blocking)*
*Severity: critical > high > medium > low — only open threats at or above workflow.security_block_on count toward threats_open*
*Disposition: mitigate (implementation required) · accept (documented risk) · transfer (third-party)*

---

## Accepted Risks Log

| Risk ID | Threat Ref | Rationale | Accepted By | Date |
|---------|------------|-----------|-------------|------|
| R-03-01 | T-03-09 | The backslash and drive-letter handling in `RelinkablePath::normalize` runs on the Mac; removing it would change behaviour. Kept by D-03, to be fixed with a test (backlog 999.8). | CONTEXT.md D-03 (user decision) | 2026-10-24 |
| R-03-02 | T-03-12 | The batch-finished alarm calls `std::system` with a fixed `play` command; Phase 5 replaces it with the system beep (MAC-10). | CONTEXT.md D-04 (user decision) | 2026-10-24 |
| R-03-03 | T-03-SC | No package was installed, upgraded or removed, so no supply-chain change entered the build. | Plan threat models 03-01 to 03-09 | 2026-10-24 |

*Accepted risks do not resurface in future audit runs.*

---

## Security Audit Trail

| Audit Date | Threats Total | Closed | Open | Run By |
|------------|---------------|--------|------|--------|
| 2026-10-24 | 22 | 22 | 0 | secure-phase orchestrator (L1 grep-depth; auditor skipped by the short-circuit rule) |

---

## Sign-Off

- [x] All threats have a disposition (mitigate / accept / transfer)
- [x] Accepted risks documented in Accepted Risks Log
- [x] `threats_open: 0` confirmed
- [x] `status: verified` set in frontmatter

**Approval:** verified 2026-10-24
