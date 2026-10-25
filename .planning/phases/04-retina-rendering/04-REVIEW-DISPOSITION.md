---
phase: 04
review: 04-REVIEW.md
titles: json
findings:
  - id: WR-01
    severity: warning
    disposition: open
    title: "`removeLoadedItemsLocked()` resets `m_endOfLoadedItems` to `end()` while non-LOADED items remain, and a display change now reaches it"
  - id: WR-02
    severity: warning
    disposition: open
    title: "`recreateThumbnail()` refreshes only the thumbnail file of the current ratio, and drops its in-memory invalidation when the generation changed"
  - id: WR-03
    severity: warning
    disposition: open
    title: "`ImageViewBase::event()` reads the viewport's ratio inside the scroll area's own `DevicePixelRatioChange`, which the tests show can lag behind"
  - id: IN-01
    severity: info
    disposition: open
    title: "Test-only accessors added to production classes"
  - id: IN-02
    severity: info
    disposition: open
    title: "Settings tooltip hard-codes a display model and a default"
  - id: IN-03
    severity: info
    disposition: open
    title: "ctest registration still sets `QT_QPA_PLATFORM=offscreen`, which `main()` immediately overrides"
  - id: IN-04
    severity: info
    disposition: open
    title: "Planning reference inside a source comment"
  - id: IN-05
    severity: info
    disposition: open
    title: "`retina_check` output-path guard is lexical"
open: 8
total: 8
recorded: 2026-10-25T15:57:44.154Z
---

# Phase 04: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-01 | warning | open | - |
| WR-02 | warning | open | - |
| WR-03 | warning | open | - |
| IN-01 | info | open | - |
| IN-02 | info | open | - |
| IN-03 | info | open | - |
| IN-04 | info | open | - |
| IN-05 | info | open | - |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
