---
phase: 05
review: 05-REVIEW.md
titles: json
findings:
  - id: WR-01
    severity: warning
    disposition: fixed
    title: "Zoom actions keep a stale enabled state when the Output tab or its view changes"
  - id: WR-02
    severity: warning
    disposition: fixed
    title: "`mac_check` keys section does not verify that the production slots were disconnected"
  - id: IN-01
    severity: info
    disposition: open
    title: "`TestMacQt::testRightClickOpensZoneMenu` has a dead wait and a stale comment"
  - id: IN-02
    severity: info
    disposition: open
    title: "`mac_check` verdicts ignore the US-layout alias keys it measures"
  - id: IN-03
    severity: info
    disposition: open
    title: "Verdicts hard-require the German layout, TESTING.md says \"current keyboard layout\""
  - id: IN-04
    severity: info
    disposition: open
    title: "`mac_check` open-panel row waits a fixed 800 ms instead of polling"
  - id: IN-05
    severity: info
    disposition: open
    title: "Output-directory guard is lexical and allows the build root itself"
  - id: IN-06
    severity: info
    disposition: open
    title: "Test brittleness and duplication"
open: 6
total: 8
recorded: 2026-11-15T17:18:11.673Z
---

# Phase 05: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-01 | warning | fixed | 05-REVIEW-FIX.md |
| WR-02 | warning | fixed | 05-REVIEW-FIX.md |
| IN-01 | info | open | - |
| IN-02 | info | open | - |
| IN-03 | info | open | - |
| IN-04 | info | open | - |
| IN-05 | info | open | - |
| IN-06 | info | open | - |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
