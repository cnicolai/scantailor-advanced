---
phase: 06
review: 06-REVIEW.md
titles: json
findings:
  - id: WR-01
    severity: warning
    disposition: fixed
    title: "The size limit also refuses the app's own output, automask and speckles files"
  - id: WR-02
    severity: warning
    disposition: fixed
    title: "The memory cap outlives the batch and the project it was computed for"
  - id: WR-03
    severity: warning
    disposition: fixed
    title: "The cap assumes 600 dpi output, but the output dpi is a per-page setting"
  - id: WR-04
    severity: warning
    disposition: fixed
    title: "JPEG metadata load still runs `jpeg_start_decompress` for every in-limit file"
  - id: WR-05
    severity: warning
    disposition: fixed
    title: "The peak-footprint assertions can pass vacuously"
  - id: WR-06
    severity: warning
    disposition: fixed
    title: "Cancelling or failing \"Save\" while closing a project still closes it (pre-existing, moved)"
  - id: IN-01
    severity: info
    disposition: open
    title: "Every split translation unit repeats the full include list of the original file"
  - id: IN-02
    severity: info
    disposition: open
    title: "A 16-bit PNG inside the limit may be refused with the generic \"could not be loaded\" message"
  - id: IN-03
    severity: info
    disposition: open
    title: "Comments and test names say \"W x H\" where the code prints \"W × H\""
open: 3
total: 9
recorded: 2026-11-17T14:07:59.646Z
---

# Phase 06: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-01 | warning | fixed | 06-REVIEW-FIX.md |
| WR-02 | warning | fixed | 06-REVIEW-FIX.md |
| WR-03 | warning | fixed | 06-REVIEW-FIX.md |
| WR-04 | warning | fixed | 06-REVIEW-FIX.md |
| WR-05 | warning | fixed | 06-REVIEW-FIX.md |
| WR-06 | warning | fixed | 06-REVIEW-FIX.md |
| IN-01 | info | open | - |
| IN-02 | info | open | - |
| IN-03 | info | open | - |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
