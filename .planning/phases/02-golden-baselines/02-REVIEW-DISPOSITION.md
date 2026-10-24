---
phase: 02
review: 02-REVIEW.md
titles: json
findings:
  - id: IN-01
    severity: info
    disposition: deferred
    title: "The header names 150 and 225 dpi as refused values, but 400 and 450 dpi are refused too"
  - id: IN-02
    severity: info
    disposition: deferred
    title: "The new cache-stability test cannot fail in a normal run"
  - id: IN-03
    severity: info
    disposition: deferred
    title: "The determinism sweep reuses a truncating `scaled()` helper on non-exact dpi values, and its expected numbers have no regeneration source"
  - id: IN-04
    severity: info
    disposition: deferred
    title: "Carried over from the previous review, unchanged: wrong arithmetic in the noise-floor comment"
  - id: CR-01
    severity: critical
    disposition: fixed
    title: "`generatePage` / `renderInkMask` write outside the raster for 44 of the documented dpi values"
  - id: WR-01
    severity: warning
    disposition: fixed
    title: "`test_golden_directory_untouched` leaves `UPDATE_GOLDENS=1` set when an assertion throws"
  - id: WR-02
    severity: warning
    disposition: fixed
    title: "A failed manifest write is reported once and then silently skipped for the rest of the suite"
  - id: WR-03
    severity: warning
    disposition: fixed
    title: "`test_every_stage_carries_non_default_values` is vacuous for the page-split stage"
  - id: WR-04
    severity: warning
    disposition: fixed
    title: "`spreadPage` returns a reference into a `std::vector` that grows"
  - id: WR-05
    severity: warning
    disposition: fixed
    title: "Text goldens are written without checking that they parse back"
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
open: 0
total: 13
recorded: 2026-10-24T15:40:32.710Z
---

# Phase 02: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| IN-01 | info | deferred | re-review 4db72234: info only, deferred (no effect on results); decided by Claude on the user's instruction to decide, 2026-10-24 |
| IN-02 | info | deferred | re-review 4db72234: info only, deferred (no effect on results); decided by Claude on the user's instruction to decide, 2026-10-24 |
| IN-03 | info | deferred | re-review 4db72234: info only, deferred (no effect on results); decided by Claude on the user's instruction to decide, 2026-10-24 |
| IN-04 | info | deferred | re-review 4db72234: info only, deferred (no effect on results); decided by Claude on the user's instruction to decide, 2026-10-24 |
| CR-01 | critical | fixed | plan 02-12 (G-02-3) (not in the current review) |
| WR-01 | warning | fixed | plan 02-13 (G-02-3) (not in the current review) |
| WR-02 | warning | fixed | plan 02-13 (G-02-3) (not in the current review) |
| WR-03 | warning | fixed | plan 02-14 (G-02-3) (not in the current review) |
| WR-04 | warning | fixed | plan 02-14 (G-02-3) (not in the current review) |
| WR-05 | warning | fixed | plan 02-13 (G-02-3) (not in the current review) |
| IN-05 | info | deferred | UAT 02 test 3: info only, deferred (no effect on results) (not in the current review) |
| IN-06 | info | deferred | UAT 02 test 3: info only, deferred (no effect on results) (not in the current review) |
| IN-07 | info | deferred | UAT 02 test 3: info only, deferred (no effect on results) (not in the current review) |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
