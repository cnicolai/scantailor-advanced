---
phase: 04
review: 04-REVIEW.md
titles: json
findings:
  - id: WR-01
    severity: warning
    disposition: fixed
    title: "`removeLoadedItemsLocked()` resets `m_endOfLoadedItems` to `end()` while non-LOADED items remain, and a display change now reaches it"
  - id: WR-02
    severity: warning
    disposition: fixed
    title: "`recreateThumbnail()` refreshes only the thumbnail file of the current ratio, and drops its in-memory invalidation when the generation changed"
  - id: WR-03
    severity: warning
    disposition: fixed
    title: "`ImageViewBase::event()` reads the viewport's ratio inside the scroll area's own `DevicePixelRatioChange`, which the tests show can lag behind"
  - id: IN-01
    severity: info
    disposition: fixed
    title: "Test-only accessors added to production classes"
  - id: IN-02
    severity: info
    disposition: fixed
    title: "Settings tooltip hard-codes a display model and a default"
  - id: IN-03
    severity: info
    disposition: fixed
    title: "ctest registration still sets `QT_QPA_PLATFORM=offscreen`, which `main()` immediately overrides"
  - id: IN-04
    severity: info
    disposition: open
    title: "Planning reference inside a source comment"
  - id: IN-05
    severity: info
    disposition: fixed
    title: "`retina_check` output-path guard is lexical"
open: 1
total: 8
recorded: 2026-10-25T17:05:15.963Z
---

# Phase 04: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-01 | warning | fixed | 04-REVIEW-FIX.md |
| WR-02 | warning | fixed | 04-REVIEW-FIX.md |
| WR-03 | warning | fixed | 04-REVIEW-FIX.md |
| IN-01 | info | fixed | fixed by 06.1-05 (076021ab): one diagnostics wording on the three headers; no friend accessor |
| IN-02 | info | fixed | fixed by 06.1-05 (076021ab): the tooltips name no display model and no default number |
| IN-03 | info | fixed | fixed by 06.1-05 (46e8a86b): platform from main() only, scale variables cleared; a run with QT_SCALE_FACTOR=2 in the shell passes |
| IN-04 | info | open | - |
| IN-05 | info | fixed | fixed by 06.1-07 (acc893f0): retina_check calls the shared harness::outputDirIsStrictlyInside before any mkpath and before its thumbs-run removeRecursively; guarded by the shared unit tests and a source scan, not run live (D-09) |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
