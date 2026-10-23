---
status: testing
phase: 02-golden-baselines
source: [02-VERIFICATION.md]
started: 2026-10-23T22:45:00Z
updated: 2026-10-23T22:45:00Z
---

## Current Test

number: 1
name: Decide whether roadmap criterion 5 is met by the wording 'no requirement' in every XFAIL tag
expected: |
  Criterion 5 asks that each expected failure 'names the requirement that will fix it'. No requirement in Phases 3 to 6 covers F-04-1, F-04-2, F-05-1, F-06-1, F-07-1, so all five tags say 'no requirement' and each has backlog item 999.x. Accept this as meeting the criterion (then add an override), or ask for the criterion to be reworded.
awaiting: user response

## Tests

### 1. Decide whether roadmap criterion 5 is met by the wording 'no requirement' in every XFAIL tag
expected: Criterion 5 asks that each expected failure 'names the requirement that will fix it'. No requirement in Phases 3 to 6 covers F-04-1, F-04-2, F-05-1, F-06-1, F-07-1, so all five tags say 'no requirement' and each has backlog item 999.x. Accept this as meeting the criterion (then add an override), or ask for the criterion to be reworded.
result: [pending]

### 2. Accept or reject the plan 02-07 despeckle-sanity deviation (D7)
expected: The plan's sanity condition (90% of specks present with despeckling off) cannot hold; the test counts the specks inside the content box (15/15 at 300 dpi, 11/11 at 600 dpi). It is open in .planning/WINDOWS.md as a deviation for the user.
result: [pending]

### 3. Triage 02-REVIEW.md CR-01 (generator heap overflow at unused dpi values) and WR-03 (vacuous page-split check)
expected: Decide whether to fix now (narrow dpiSupported to values where the text block fits, or add a bounds check in placeGlyph; assert the page-split layout value directly in the round-trip test) or carry as backlog. All 13 review items are 'open' in 02-REVIEW-DISPOSITION.md.
result: [pending]

## Summary

total: 3
passed: 0
issues: 0
pending: 3
skipped: 0
blocked: 0

## Gaps
