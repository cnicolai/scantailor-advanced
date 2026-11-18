---
phase: 01
review: 01-REVIEW.md
titles: json
findings:
  - id: WR-01
    severity: warning
    disposition: fixed
    title: "`macos-asan` test preset cannot gate on sanitizer findings"
  - id: WR-02
    severity: warning
    disposition: fixed
    title: "`-ffp-contract=off` is not the Apple Clang default, so output may differ from the unmodified build"
  - id: WR-03
    severity: warning
    disposition: fixed
    title: "`SANITIZE` is injected without validation or quoting"
  - id: WR-04
    severity: warning
    disposition: deferred
    title: "Boost handling still relies on FindBoost-only variables under CMP0167 NEW"
  - id: IN-01
    severity: info
    disposition: fixed
    title: "`cmake_minimum_required(3.25)` leaves stale docs and non-macOS CI"
  - id: IN-02
    severity: info
    disposition: fixed
    title: "Presets omit architecture and host-condition guards, and `PORTABLE_VERSION` is untyped"
  - id: IN-03
    severity: info
    disposition: fixed
    title: "`macos-debug` and `macos-asan` differ from `macos-release` beyond optimisation"
  - id: IN-04
    severity: info
    disposition: fixed
    title: "Comments cite decision IDs that are not resolvable in the source tree"
open: 0
total: 8
recorded: 2026-10-23T13:34:20.385Z
---

# Phase 01: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-01 | warning | fixed | fixed by 06.1-11 (dec6a64c): macos-asan halts on the first ASan or UBSan report (halt_on_error=1), so a report in a passing test fails it; macos-asan-collect keeps the collect run; the known reports are fixed in code (751f75b2); full strict run 7/7 in .planning/phases/06.1-address-tech-debt-post-audit-cleanup/records/asan-strict-run.md |
| WR-02 | warning | fixed | fixed by 06.1-11 and 06.1-12: comment narrowed to Clang (dec6a64c); one-off -ffp-contract=on measurement against every golden in .planning/phases/06.1-address-tech-debt-post-audit-cleanup/records/fp-contract-measurement.md; default unchanged |
| WR-03 | warning | fixed | fixed by 06.1-11 (dec6a64c): SANITIZE is checked against address, undefined, thread at configure time and passed as one quoted argument; default and asan commands unchanged |
| WR-04 | warning | deferred | deferred: hygiene only — Homebrew's Boost config sets Boost_INCLUDE_DIRS today (RESEARCH probe), deferred to keep the pre-milestone cleanup small; user decision 2026-11-17 |
| IN-01 | info | fixed | fixed by 03-03 (462f22cd) and 03-07 (cf4da61c): no .github directory and no non-macOS CI any more; the docs say CMake 3.25+ |
| IN-02 | info | fixed | fixed by 06.1-11 (dec6a64c): Darwin host condition and typed booleans; PORTABLE_VERSION gone since 03-05; CMAKE_OSX_ARCHITECTURES=arm64 added, command-neutral by trial |
| IN-03 | info | fixed | fixed by 06.1-02 (1f6c2af9): TESTING.md states the Debug-only differences and that goldens come from macos-release; DEVELOPER_VERSION stays on for Debug |
| IN-04 | info | fixed | fixed by 06.1-11 (dec6a64c): the Clang block comments state their reasons; the C compiler id half skipped, Apple Clang compiles both languages |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
