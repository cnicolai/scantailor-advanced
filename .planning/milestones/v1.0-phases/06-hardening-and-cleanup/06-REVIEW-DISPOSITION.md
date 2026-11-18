---
phase: 06
review: 06-REVIEW.md
titles: json
findings:
  - id: WR-07
    severity: warning
    disposition: fixed
    title: "WR-04 fix still allocates the full coefficient image for non-interleaved baseline JPEGs"
  - id: IN-04
    severity: info
    disposition: fixed
    title: "`ImageLoader::load(const ImageId&)` is now unused and is an unlimited reader of source files"
  - id: IN-05
    severity: info
    disposition: fixed
    title: "Unlimited read of the output-folder files accepts a forged file as an own file"
  - id: IN-06
    severity: info
    disposition: fixed
    title: "`startBatchProcessing()` dereferences the queue it just reset when the sequence is empty"
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
    disposition: fixed
    title: "Every split translation unit repeats the full include list of the original file"
  - id: IN-02
    severity: info
    disposition: fixed
    title: "A 16-bit PNG inside the limit may be refused with the generic \"could not be loaded\" message"
  - id: IN-03
    severity: info
    disposition: fixed
    title: "Comments and test names say \"W x H\" where the code prints \"W × H\""
open: 0
total: 13
recorded: 2026-11-17T14:57:05.443Z
---

# Phase 06: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-07 | warning | fixed | 06-REVIEW-FIX.md |
| IN-04 | info | fixed | fixed by 06.1-06 (afcaa7ee): the unused overload is deleted and the own-file readers are named loadOwnFile (D-12) |
| IN-05 | info | fixed | fixed by 06.1-06 (d9a32fd9): own files are read with a fixed ceiling below 2^30 px and 131070 px per side, checked on the header (D-11); a forged header at the cached output name makes the page regenerate (PipelineSmokeTestSuite, 26451961) and is refused within 64 MB and 1 s at the own-file loader (CoreImageSizeLimitTestSuite); a header inside the ceiling is an accepted residual |
| IN-06 | info | fixed | fixed by 06.1-07 (73b538a5): early return after stopBatchProcessing() in startBatchProcessing(); offscreen test removes the only page through the thumbnail context menu and starts a batch (a project file without pages is refused by the reader); it crashed on the null queue before the fix |
| WR-01 | warning | fixed | 06-REVIEW-FIX.md (not in the current review) |
| WR-02 | warning | fixed | 06-REVIEW-FIX.md (not in the current review) |
| WR-03 | warning | fixed | 06-REVIEW-FIX.md (not in the current review) |
| WR-04 | warning | fixed | 06-REVIEW-FIX.md (not in the current review) |
| WR-05 | warning | fixed | 06-REVIEW-FIX.md (not in the current review) |
| WR-06 | warning | fixed | 06-REVIEW-FIX.md (not in the current review) |
| IN-01 | info | fixed | fixed by 06.1-08 (13dc112f): the eleven split files keep only the includes they name or need, by Release and Debug syntax checks; OutputGenerator object code proven identical by disassembly (records/include-prune.md) |
| IN-02 | info | fixed | fixed by 06.1-06 (d9a32fd9): the 16-bit PNG case of the Qt allocation limit is documented as accepted in ImageSizeLimit.h (the reviewer's first option); not measured |
| IN-03 | info | fixed | fixed by 06.1-06 (d9a32fd9, afcaa7ee): both comments use the multiplication sign |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
