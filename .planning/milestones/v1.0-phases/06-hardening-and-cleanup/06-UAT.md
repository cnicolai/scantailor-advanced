---
status: complete
phase: 06-hardening-and-cleanup
source: [06-VERIFICATION.md, 06-REVIEW-FIX.md]
started: 2026-11-17T13:21:51Z
updated: 2026-11-17T14:40:57.329Z
---

## Current Test

[testing complete]

## Tests

### 1. Batch start sets the memory limit, batch stop clears it (app wiring of WR-02/WR-03)
expected: During the batch the slider tooltip ends in ", limited to 6 by memory"; after stopping it shows only "n/10"
note: Replaces the earlier real-scan swap check. Cap arithmetic, pool behaviour, tooltip text and a synthetic 165.6 MP run (swap 0) are covered by tests and records/memory-run.md; at the default 600 dpi output for A4 scans the cap is 24 > 10 cores and never binds, so only the MainWindowBatch/MainWindowProject call sites remain unexercised.
result: pass
observed: "10/10, limited to 2 by memory"
evidence: Cap 2 is the correct value for the test files (6000 x 4000 px camera JPEGs at 300 dpi → 24000 x 16000 = 384 MP at 1200 dpi → 32 GiB / (40 B x 384 MP) = 2.2 → 2), not the A4 figure of 6 predicted in the instructions. The stop half cannot be seen in the UI: the batch panel with the slider is replaced by the page view when the batch ends, and the next batch start sets a fresh cap. The reset after stop (MainWindowBatch.cpp stopBatchProcessing, MainWindowProject.cpp switchToNewProject) rests on code reading. Side finding: with these camera shots at the default 600 dpi output the cap is 8 of 10 threads (96 MP per page), so it does bind slightly in real use.

### 2. Cancelling Save while closing a never-saved project keeps it open (WR-06, branch not reachable offscreen)
expected: New project (not yet saved) → File > Close Project → answer Yes to "Save the project?" → Cancel the save dialog → the project stays open, pages still listed
result: pass

## Summary

total: 2
passed: 2
issues: 0
pending: 0
skipped: 0
blocked: 0

## Gaps
