---
status: testing
phase: 06-hardening-and-cleanup
source: [06-VERIFICATION.md]
started: 2026-11-17T13:21:51Z
updated: 2026-11-17T13:21:51Z
---

## Current Test

number: 1
name: Batch of large real scans stays out of swap
expected: |
  In the built app, open a project of several large real scans, start batch processing,
  and watch Activity Monitor > Memory > Swap Used before and during the run.
  Swap Used does not grow during the batch; the thread-count slider tooltip shows
  "limited to n by memory" when the cap binds (pages above about 85 MP).
awaiting: user response

## Tests

### 1. Batch of large real scans stays out of swap
expected: Swap Used does not grow during the batch; tooltip shows the memory limit for pages above about 85 MP
result: [pending]

## Summary

total: 1
passed: 0
issues: 0
pending: 1
skipped: 0
blocked: 0

## Gaps
