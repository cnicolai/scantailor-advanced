---
phase: 01
review: 01-REVIEW.md
titles: json
findings:
  - id: WR-01
    severity: warning
    disposition: open
    title: "`macos-asan` test preset cannot gate on sanitizer findings"
  - id: WR-02
    severity: warning
    disposition: open
    title: "`-ffp-contract=off` is not the Apple Clang default, so output may differ from the unmodified build"
  - id: WR-03
    severity: warning
    disposition: open
    title: "`SANITIZE` is injected without validation or quoting"
  - id: WR-04
    severity: warning
    disposition: open
    title: "Boost handling still relies on FindBoost-only variables under CMP0167 NEW"
  - id: IN-01
    severity: info
    disposition: fixed
    title: "`cmake_minimum_required(3.25)` leaves stale docs and non-macOS CI"
  - id: IN-02
    severity: info
    disposition: open
    title: "Presets omit architecture and host-condition guards, and `PORTABLE_VERSION` is untyped"
  - id: IN-03
    severity: info
    disposition: open
    title: "`macos-debug` and `macos-asan` differ from `macos-release` beyond optimisation"
  - id: IN-04
    severity: info
    disposition: open
    title: "Comments cite decision IDs that are not resolvable in the source tree"
open: 7
total: 8
recorded: 2026-10-23T13:34:20.385Z
---

# Phase 01: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-01 | warning | open | - |
| WR-02 | warning | open | - |
| WR-03 | warning | open | - |
| WR-04 | warning | open | - |
| IN-01 | info | fixed | fixed by 03-03 (462f22cd) and 03-07 (cf4da61c): no .github directory and no non-macOS CI any more; the docs say CMake 3.25+ |
| IN-02 | info | open | - |
| IN-03 | info | open | - |
| IN-04 | info | open | - |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
