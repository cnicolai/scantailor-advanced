---
status: resolved
phase: 02-golden-baselines
source: [02-VERIFICATION.md]
started: 2026-10-23T22:45:00Z
updated: 2026-10-24T13:48:38.528Z
---

## Current Test

[testing complete]

## Tests

### 1. Decide whether roadmap criterion 5 is met by the wording 'no requirement' in every XFAIL tag

expected: Criterion 5 asks that each expected failure 'names the requirement that will fix it'. No requirement in Phases 3 to 6 covers F-04-1, F-04-2, F-05-1, F-06-1, F-07-1, so all five tags say 'no requirement' and each has backlog item 999.x. Accept this as meeting the criterion (then add an override), or ask for the criterion to be reworded.
result: pass
note: "User delegated the decision ('you decide'). Accepted: the intent of the criterion holds (no red run, traceable through finding code to backlog 999.1, 999.2, 999.3, 999.4, 999.6, trips when fixed). Override recorded in 02-VERIFICATION.md."

### 2. Accept or reject the plan 02-07 despeckle-sanity deviation (D7)

expected: The plan's sanity condition (90% of specks present with despeckling off) cannot hold; the test counts the specks inside the content box (15/15 at 300 dpi, 11/11 at 600 dpi). It is open in .planning/WINDOWS.md as a deviation for the user.
result: pass
note: "User accepted. .planning/WINDOWS.md entry 5 waived with reason."

### 3. Triage 02-REVIEW.md CR-01 (generator heap overflow at unused dpi values) and WR-03 (vacuous page-split check)

expected: Decide whether to fix now (narrow dpiSupported to values where the text block fits, or add a bounds check in placeGlyph; assert the page-split layout value directly in the round-trip test) or carry as backlog. All 13 review items are 'open' in 02-REVIEW-DISPOSITION.md.
result: issue
reported: "you decide"
severity: major
note: "User delegated the triage. Decision: fix CR-01 and WR-01 to WR-05 now (test code only, no golden changes), defer IN-01 to IN-07."

## Summary

total: 3
passed: 2
issues: 0
pending: 0
skipped: 0
blocked: 0

## Gaps

- gap_id: G-02-3
  truth: "The Phase 2 test code has no open critical or warning review finding: the page generator never writes outside its raster at any dpi it accepts (CR-01), golden-update mode is restored when a self-test fails (WR-01), a failed manifest write fails every later golden of the suite (WR-02), the round trip asserts the page-split layout value itself (WR-03), the spread-page cache hands out stable references (WR-04), and text goldens are checked to parse back before they are written (WR-05)"
  status: resolved
  resolved_by: "gap closure plans 02-12 to 02-14; 02-VERIFICATION.md re-verification passed 9/9"
  reason: "User delegated the triage ('you decide'); decision: fix CR-01 and WR-01 to WR-05 in this phase, defer IN-01 to IN-07"
  severity: major
  test: 3
  root_cause: "Six independent defects in Phase 2 test code, diagnosed in 02-REVIEW.md (src unchanged since the review, commit 2be71251). CR-01: scaled() in PageGenerator.cpp rounds every length to the nearest pixel on its own ((base * dpi + 150) / 300, base unit 1/300 in); the glyph dot (kDot = 5) is then multiplied by up to 125 cells and 117 line dots, so a dot rounded UP (e.g. 3 instead of 2.5 at 150 dpi) grows the text block by up to 20% while page size and block origin are rounded once; dpiSupported() accepts every dpi in 75..1200 and placeGlyph writes layout.raster[y * width + x] without a bounds check, so rows past height (heap overflow, e.g. MIXED at 90 dpi: 22 rows) or columns past width (wrap into the next row) are written. WR-01: the self-test sets UPDATE_GOLDENS=1 and restores it after BOOST_REQUIRE calls that can throw. WR-02: SuiteManifest::refresh marks the suite handled before the write succeeds. WR-03: the default round trip never runs page_split, so the page-split XML differs by presence alone. WR-04: spreadPage returns a reference into a std::vector cache that can reallocate. WR-05: serializeText preconditions are comments only and nothing checks that a text golden parses back."
  artifacts:
    - path: "src/test_support/PageGenerator.cpp"
      issue: "CR-01: placeGlyph write without bounds check (lines 377-400, write at 394); geometry in computeGeometry (213-261) not checked against page size; dpiSupported (146-148) accepts dpi values where the block does not fit; callers at 557, 649, 699, 711"
    - path: "src/pipeline_tests/TestComparatorSelfTest.cpp"
      issue: "WR-01: test_golden_directory_untouched (473-500) leaks UPDATE_GOLDENS=1 when an assertion throws"
    - path: "src/test_support/GoldenStore.cpp"
      issue: "WR-02: SuiteManifest::refresh (190-202) inserts m_suite into m_handled before saveAtomically; WR-05: checkText (341, 367) saves text goldens without a parse-back check"
    - path: "src/test_support/TextGolden.cpp"
      issue: "WR-05: serializeText (51-60) does not enforce the key/value preconditions stated in TextGolden.h:15"
    - path: "src/pipeline_tests/TestProjectRoundTrip.cpp"
      issue: "WR-03: test_every_stage_carries_non_default_values (326-339) is vacuous for page-split because roundTrip runs runPageSplit only when nonDefault (277-279)"
    - path: "src/pipeline_tests/TestBookSpread.cpp"
      issue: "WR-04: spreadPage (98-107) returns a const reference into a growing std::vector<std::pair<int, GeneratedPage>>"
  missing:
    - "CR-01: reject any dpi whose geometry does not fit (block, picture and specks inside the page) so generatePage/renderInkMask return the documented null image, and guard the raster write; do NOT use the review's 'dpi % 75 == 0' hint: 150 and 225 dpi round the dot up and still overflow, since scaling is only exact where 5 * dpi is a multiple of 300"
    - "CR-01: add a dpi sweep test over every kind (for example 75, 90, 100, 150, 200, 450, 1200) expecting a valid page or the documented null image, and run it under the ASan preset"
    - "CR-01 constraint: page geometry and pixels at 75, 300 and 600 dpi stay byte-identical; golden/*/fixtures.txt fingerprints and every golden must still pass unchanged (no UPDATE_GOLDENS run)"
    - "WR-01: restore UPDATE_GOLDENS through an RAII guard so a throwing assertion cannot leave update mode on"
    - "WR-02: mark the suite handled only after the manifest write succeeds"
    - "WR-03: assert the page-split layout type of the non-default project directly (SINGLE_PAGE_UNCUT), as test_numeric_values_parse_back_exactly does for the other stages"
    - "WR-04: use a node-stable container (std::map<int, GeneratedPage>, as the other suites do) for the spread cache"
    - "WR-05: before saving a text golden, check parseText(serializeText(actual)) == actual and fail naming the offending key"
    - "Full ctest (macos-release and macos-debug) passes 7/7 afterwards; clang-format clean on every touched file"
  debug_session: ".planning/phases/02-golden-baselines/02-REVIEW.md"
