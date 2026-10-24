---
phase: 02
review: 02-REVIEW.md
titles: json
findings:
  - id: CR-01
    severity: critical
    disposition: open
    title: "`generatePage` / `renderInkMask` write outside the raster for 44 of the documented dpi values"
  - id: WR-01
    severity: warning
    disposition: open
    title: "`test_golden_directory_untouched` leaves `UPDATE_GOLDENS=1` set when an assertion throws"
  - id: WR-02
    severity: warning
    disposition: open
    title: "A failed manifest write is reported once and then silently skipped for the rest of the suite"
  - id: WR-03
    severity: warning
    disposition: open
    title: "`test_every_stage_carries_non_default_values` is vacuous for the page-split stage"
  - id: WR-04
    severity: warning
    disposition: open
    title: "`spreadPage` returns a reference into a `std::vector` that grows"
  - id: WR-05
    severity: warning
    disposition: open
    title: "Text goldens are written without checking that they parse back"
  - id: IN-01
    severity: info
    disposition: deferred
    title: "Stale and misleading constants comment in the book suite"
  - id: IN-02
    severity: info
    disposition: deferred
    title: "`areaAverageReference` can read one row/column past the source if sizes change"
  - id: IN-03
    severity: info
    disposition: deferred
    title: "The NFD file name depends on raw combining bytes in the source file"
  - id: IN-04
    severity: info
    disposition: deferred
    title: "Nearly identical sanity and reporting code in three scenario files"
  - id: IN-05
    severity: info
    disposition: deferred
    title: "Test code carries plan and decision identifiers that mean nothing outside `.planning`"
  - id: IN-06
    severity: info
    disposition: deferred
    title: "`ExpectedFailure.h` is included but unused"
  - id: IN-07
    severity: info
    disposition: deferred
    title: "The test sources need Qt 6.5 and Boost headers, but the build still advertises a Qt 5 fallback"
open: 6
total: 13
recorded: 2026-10-23T21:27:23.648Z
---

# Phase 02: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| CR-01 | critical | open | - |
| WR-01 | warning | open | - |
| WR-02 | warning | open | - |
| WR-03 | warning | open | - |
| WR-04 | warning | open | - |
| WR-05 | warning | open | - |
| IN-01 | info | deferred | UAT 02 test 3: info only, deferred (no effect on results) |
| IN-02 | info | deferred | UAT 02 test 3: info only, deferred (no effect on results) |
| IN-03 | info | deferred | UAT 02 test 3: info only, deferred (no effect on results) |
| IN-04 | info | deferred | UAT 02 test 3: info only, deferred (no effect on results) |
| IN-05 | info | deferred | UAT 02 test 3: info only, deferred (no effect on results) |
| IN-06 | info | deferred | UAT 02 test 3: info only, deferred (no effect on results) |
| IN-07 | info | deferred | UAT 02 test 3: info only, deferred (no effect on results) |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
