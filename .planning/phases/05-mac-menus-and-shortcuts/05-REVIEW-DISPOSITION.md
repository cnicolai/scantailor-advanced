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
    disposition: fixed
    title: "`TestMacQt::testRightClickOpensZoneMenu` has a dead wait and a stale comment"
  - id: IN-02
    severity: info
    disposition: skipped
    title: "`mac_check` verdicts ignore the US-layout alias keys it measures"
  - id: IN-03
    severity: info
    disposition: skipped
    title: "Verdicts hard-require the German layout, TESTING.md says \"current keyboard layout\""
  - id: IN-04
    severity: info
    disposition: skipped
    title: "`mac_check` open-panel row waits a fixed 800 ms instead of polling"
  - id: IN-05
    severity: info
    disposition: fixed
    title: "Output-directory guard is lexical and allows the build root itself"
  - id: IN-06
    severity: info
    disposition: fixed
    title: "Test brittleness and duplication"
open: 0
total: 8
recorded: 2026-11-15T17:18:11.673Z
---

# Phase 05: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-01 | warning | fixed | 05-REVIEW-FIX.md |
| WR-02 | warning | fixed | 05-REVIEW-FIX.md |
| IN-01 | info | fixed | fixed by 06.1-05 (c2e8f81a): polled menu close plus a Backspace effect check instead of a fixed wait |
| IN-02 | info | skipped | skipped: needs a live run; verdicts are correct on the one target machine (D-09) |
| IN-03 | info | skipped | skipped: needs a live run; verdicts are correct on the one target machine (D-09) |
| IN-04 | info | skipped | skipped: needs a live run; verdicts are correct on the one target machine (D-09) |
| IN-05 | info | fixed | fixed by 06.1-07 (acc893f0): mac_check calls the shared harness::outputDirIsStrictlyInside before any mkpath; isBelow() for the home redirect is a different, intentionally permissive check and stays; guarded by the shared unit tests and a source scan, not run live (D-09) |
| IN-06 | info | fixed | fixed by 06.1-05 (c2e8f81a): named scan floors with their derivation, whole-identifier system( marker, truthful zoom comment; skipped: autogen path (single-config only, latent) and helper duplication (mac_check does not link test_support, needs a live run) |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
