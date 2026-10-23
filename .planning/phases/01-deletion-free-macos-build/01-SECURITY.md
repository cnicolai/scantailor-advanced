---
phase: "1"
slug: "deletion-free-macos-build"
status: verified
# threats_open = count of OPEN threats at or above workflow.security_block_on severity (the blocking gate)
threats_open: 0
asvs_level: 1
created: "2026-10-23"
---

# Phase 1 — Security

> Per-phase security contract: threat register, accepted risks, and audit trail.

Register source: the `<threat_model>` blocks of plans 01-01 to 01-07 (authored at plan time) plus the `## Threat Flags` sections of the seven SUMMARYs (all "None"). Verification depth: ASVS level 1 (evidence located in committed records, presets and git), block threshold `high`. Paths: `records/` = `.planning/phases/01-deletion-free-macos-build/records/`.

---

## Trust Boundaries

| Boundary | Description | Data Crossing |
|----------|-------------|---------------|
| Homebrew taps → this Mac | Bottles and formulae downloaded from the network become the compiler-visible dependencies | packages (integrity) |
| Homebrew → toolchain | A replacement dependency (another Boost, clang-format) would come from the network | packages (integrity) |
| CMake package discovery → link line | `find_package` decides which copy of libjpeg/libpng/libtiff/Boost the binary uses | library paths |
| Dynamic loader → process | Which libjpeg/libpng/libtiff images are actually loaded at runtime | loaded images |
| Compiler diagnostics → source edits | Compiler-driven fixes could reach processing code the user relies on for real scanning output | source (output fidelity) |
| Project directory → user's home (`~/.config`, `~/Library`) | Any access needs the user's explicit yes (global rule, D-02) | file names, settings (private) |
| App runtime → user's home | The app writes its INI and profiles outside the project (D-01/D-02) | settings (private) |
| Settings file → shell | `main_window/external_alarm_cmd` from the INI is passed to `std::system` when a batch finishes with "Beep when finished" on | command string |
| Test inputs → image-processing code | Memory-safety defects in input handling show up here first | synthetic images |
| Sanitizer options → findings record | Options can hide reports | diagnostics |
| Scratchpad → repository | Synthetic pages must not leak into git | images |
| Command output / diagnostics → committed records | Captured logs and stderr may carry absolute home paths or user data | paths (private) |
| Git history → phase verdict | The closing record is what later phases trust as "unmodified except build fixes" | diff, commit list |

---

## Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation | Status |
|-----------|----------|-----------|----------|-------------|------------|--------|
| T-01-SC | Tampering | Homebrew installs (plan 01-01; clang-format option in 01-02; no installs in 01-03 to 01-07) | high | mitigate | Legitimacy gate `brew info --json=v2` (tap homebrew/core, not deprecated, not disabled) in `records/toolchain.txt`; clang-format never installed (`records/fix-log.md`: "Formatting decision: not needed (no src/ edits)"); 01-07 compares closing versions with `toolchain.txt` (`records/closing-check.md`) | closed |
| T-01-01 | Tampering | `brew install` moving unrelated formulae | medium | mitigate | `records/toolchain.txt` `## Before install` / `## After install`: jpeg-turbo 3.2.0, libpng 1.6.59, libtiff 4.7.2 unchanged. The `HOMEBREW_NO_AUTO_UPDATE=1` setting itself is not captured in any record; the outcome it protects is verified | closed |
| T-01-02 | Tampering | `find_package(JPEG/PNG/TIFF)` discovery | medium | mitigate | `CMAKE_PREFIX_PATH=/opt/homebrew` and `CMAKE_FIND_FRAMEWORK` in `CMakePresets.json`; `jpeg` formula absent; one copy each proven in `records/binary-check.txt` and re-checked in `records/closing-check.md` | closed |
| T-01-03 | Information disclosure | records under `records/` | low | mitigate | No `/Users/` or `$HOME` string in any committed record (grep, 2026-10-23) | closed |
| T-01-04 | Tampering | compile fixes in processing code | high | mitigate | No compile fix was needed: `git diff 5eaac18 -- src` is empty (`records/src-diff.txt`, `records/closing-check.md` criterion 5) | closed |
| T-01-05 | Tampering | Boost replacement | medium | mitigate | Only Boost 1.92 in `records/toolchain.txt`; no other Boost installed or built | closed |
| T-01-06 | Tampering | two copies of an image library | medium | mitigate | `otool -L` and `DYLD_PRINT_LIBRARIES` counts exactly 1 per library (`records/binary-check.txt`; re-checked in `records/closing-check.md`) | closed |
| T-01-07 | Repudiation | records of the starting state | low | mitigate | Captured output with commands; `tested commit:` headers and source-commit list in `records/ctest-release.txt` and `records/ctest-debug.txt` | closed |
| T-01-08 | Denial of service | memory-safety / UB defects in existing processing code | medium | accept | Recorded with locations in `records/sanitizer-findings.md` (UBSan at `src/imageproc/BinaryImage.cpp:1044:37`, `src/imageproc/SlicedHistogram.cpp:49:47`); fixes owned by Phase 6 (SAFE-01 to SAFE-03) | closed (accepted) |
| T-01-09 | Repudiation | sanitizer options and suppressions | low | mitigate | `## Method` in `records/sanitizer-findings.md`: only `detect_leaks=0:halt_on_error=0`, `print_stacktrace=1`; no suppression files | closed |
| T-01-10 | Information disclosure | listing `~/.config`, `~/Library/Preferences`, `~/Library/Application Support` | high | mitigate | Blocking-human D-02 question answered `list-corrected` before any listing; names/sizes/dates only, `~`-normalised (`records/prior-state.md`) | closed |
| T-01-11 | Tampering | user's prior ScanTailor settings | medium | mitigate | `No ScanTailor entries found.` (`records/prior-state.md`); nothing moved or deleted | closed |
| T-01-12 | Information disclosure | DPR probe | medium | mitigate | `probe constructs QSettings: no` (`records/probe-output.txt`) | closed |
| T-01-13 | Information disclosure | test images | medium | mitigate | Pages generated in the scratchpad only; no image or `.ScanTailor` file tracked (`git ls-files`, 2026-10-23) | closed |
| T-01-14 | Elevation of privilege | `std::system` alarm at `src/app/MainWindow.cpp:1337` | medium | accept | Observed only (`records/SMOKE-FINDINGS.md`: `sh: play: command not found`); value comes from the user's own INI; Phase 5 (MAC-10) replaces it | closed (accepted) |
| T-01-15 | Tampering | launch fixes | high | mitigate | `## Launch fixes: None.` (`records/SMOKE-FINDINGS.md`); `src/` unchanged since `5eaac18` | closed |
| T-01-16 | Information disclosure | `smoke-stderr.txt`, crash reports | medium | mitigate | `records/smoke-stderr.txt` normalised (no `/Users/`); no crash report was read | closed |
| T-01-17 | Repudiation | criterion 5 pathspec excluding `.claude/` | low | mitigate | `.claude/` diff recorded separately and lists only `.claude/CLAUDE.md` (`records/closing-check.md`) | closed |
| T-01-18 | Tampering | hidden deletion of a code path | medium | mitigate | Full diff in `records/src-diff.txt`; per-hunk classification, no remove-only hunk (`records/closing-check.md` criterion 5) | closed |
| T-01-19 | Information disclosure | build-tool reads outside the project | medium | mitigate | `Decision: approve` in `records/access-approval.md`; precondition on the first task of plans 01-02 to 01-07; Claude's own outside access asked separately (D-02, the Desktop project file in 01-06) | closed |

*Status: open · closed · open — below high threshold (non-blocking)*
*Severity: critical > high > medium > low — only open threats at or above workflow.security_block_on count toward threats_open*
*Disposition: mitigate (implementation required) · accept (documented risk) · transfer (third-party)*

---

## Accepted Risks Log

| Risk ID | Threat Ref | Rationale | Accepted By | Date |
|---------|------------|-----------|-------------|------|
| AR-01-01 | T-01-08 | Phase 1 records sanitizer findings only (success criterion 4). Inputs are the user's own scans, there is no network boundary, and Phase 6 (SAFE-01 to SAFE-03) owns the fixes with the recorded locations | plan 01-04 threat model (disposition `accept`), carried out as planned | 2026-10-23 |
| AR-01-02 | T-01-14 | The alarm command comes from the user's own INI; anyone who can write it already runs code as the user. Observed only in Phase 1 (D-06); Phase 5 (MAC-10) replaces it with the system beep. The user stated the sound is not important | plan 01-06 threat model (disposition `accept`), carried out as planned | 2026-10-23 |

*Accepted risks do not resurface in future audit runs.*

---

## Audit Notes

- Homebrew `qtbase` moved from `6.12.0` (`records/toolchain.txt`, 2026-10-18) to the formula revision `6.12.0_1` by the closing check (2026-10-23). No Phase 1 plan after 01-01 installs anything, and the Qt frameworks still report 6.12.0; no test, library or flag result changed. Not a threat; noted because Phase 2 baselines are pinned to this toolchain.

---

## Security Audit Trail

| Audit Date | Threats Total | Closed | Open | Run By |
|------------|---------------|--------|------|--------|
| 2026-10-23 | 20 | 20 | 0 | secure-phase (orchestrator, ASVS L1 short-circuit) |

---

## Sign-Off

- [x] All threats have a disposition (mitigate / accept / transfer)
- [x] Accepted risks documented in Accepted Risks Log
- [x] `threats_open: 0` confirmed
- [x] `status: verified` set in frontmatter

**Approval:** verified 2026-10-23
