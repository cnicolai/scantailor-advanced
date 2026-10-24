---
phase: 02-golden-baselines
reviewed: 2026-10-24T00:00:00Z
depth: standard
files_reviewed: 10
files_reviewed_list:
  - src/pipeline_tests/TestBookSpread.cpp
  - src/pipeline_tests/TestComparatorSelfTest.cpp
  - src/pipeline_tests/TestGeneratorDeterminism.cpp
  - src/pipeline_tests/TestGoldenStore.cpp
  - src/pipeline_tests/TestProjectRoundTrip.cpp
  - src/test_support/GoldenStore.cpp
  - src/test_support/PageGenerator.cpp
  - src/test_support/PageGenerator.h
  - src/test_support/TextGolden.cpp
  - src/test_support/TextGolden.h
findings:
  critical: 0
  warning: 0
  info: 4
  total: 4
status: issues_found
---

# Phase 2: Code Review Report (re-review of gap closure 02-11 to 02-14)

**Reviewed:** 2026-10-24
**Depth:** standard
**Files Reviewed:** 10
**Status:** issues_found

## Summary

This is an incremental re-review of the diff `2be71251..HEAD` for the ten files touched by the gap-closure plans. The task was to decide whether each fix closes its finding from the previous review. Result: all six findings (CR-01, WR-01 to WR-05) are closed. No new bug or defect that could cause incorrect behaviour was found; four low-severity quality points remain.

Closure check per finding:

| Finding | Verdict | Basis |
|---|---|---|
| CR-01 glyph write outside the raster | Closed | `dpiSupported(kind, dpi)` now runs `layoutFits` on the rounded geometry, checking the block ink against the page and against its documented frame, and the MIXED picture against the page. `generatePage`, `renderInkMask`, `mapFlatToImage` and `mapImageToFlat` all go through it. `placeGlyph` and `placeSpecks` also throw `std::logic_error` instead of writing out of bounds. |
| WR-01 `UPDATE_GOLDENS` left set | Closed | `ScopedEnvironmentVariable` restores the old value or unsets it in its destructor; `test_golden_directory_untouched` uses it. |
| WR-02 manifest marked handled before write | Closed | `SuiteManifest::refresh` inserts into `m_handled` only after the manifest is on disk (or already identical). A failed write now fails every later golden of the suite. |
| WR-03 vacuous page-split assertion | Closed | `test_every_stage_carries_non_default_values` now reads `layoutType` and `<params>/<pages type>` of every page-split `<image>` in both written files. The attribute names match `Filter::writeImageSettings` and `PageLayout::toXml`. |
| WR-04 dangling reference into a vector | Closed | `spreadPage` uses `std::map<int, GeneratedPage>`, as the sibling caches do. |
| WR-05 unvalidated text serialisation | Closed | `textRoundTrips` checks each entry against `serializeText` and `parseText`. `checkText` runs the check before any file or artifact is touched, in both modes. |

I reproduced the geometry computation of `PageGenerator.cpp` (`scaled`, `centredBlock`, `computeGeometry`, `layoutFits`) in a Python script for every dpi from 75 to 1200. The accepted counts are 1125 (FLAT), 845 (SPREAD), 702 (MIXED) and 697 (CURVED), which equal the literals pinned in `test_dpi_support_rule`. The expectations of `sweepExpectsPage` (FLAT: all sweep dpi values but 90; the other kinds: 75 and 200 only) and the overflow ranges in the test also match. I did not build or run the suite, so the 75 dpi fingerprints in `test_pinned_pages_unchanged` are not verified by me.

## Info

### IN-01: The header names 150 and 225 dpi as refused values, but 400 and 450 dpi are refused too

**File:** `src/test_support/PageGenerator.h:20-22`, `:56-63`; rule pinned in `src/pipeline_tests/TestGeneratorDeterminism.cpp:539-571`
**Issue:** For SPREAD, MIXED and CURVED only 75 <= dpi <= 1200 values for which the 5 base px dot rounds favourably are accepted. By my reproduction 400 dpi (a common archival scan resolution) and 450 dpi are refused as well as 150 and 225; so are 100, 210 to 216 and about a quarter to two fifths of all integers in the range. The header gives "150 and 225 dpi, for one" as the examples and says "75, 300, 600 and 1200 dpi are always supported". `generatePage` signals a refusal with a null image and no exception. A later scenario at 400 dpi therefore gets a null page, and only a test that checks `isNull()` first turns that into a readable failure.
**Fix:** Name the commonly used refused values in the header (150, 225, 400, 450) so nobody plans a sweep around them. If a later phase needs those values, make the layout robust instead of refusing: derive the frame and block sizes from the rounded dot size (`blockWidth = cells * 6 * dot`) rather than rounding each length independently.

### IN-02: The new cache-stability test cannot fail in a normal run

**File:** `src/pipeline_tests/TestBookSpread.cpp:417-426`
**Issue:** `test_spread_page_cache_keeps_references` compares addresses before and after inserting a second page. With the old vector it would only fail if the 300 and 600 dpi entries were inserted inside this test. `test_book_fixture_fingerprints` (lines 410-411) fills both entries earlier in the same suite, and the default order runs it first. The comment says "Run alone"; in the default run the case passes for any container. The real fix, `std::map`, is correct; only the regression test is weak.
**Fix:** Drop the case, or use a dpi nothing else in the suite uses so that both insertions happen inside it (a rejected dpi cannot be used, so for example 450 would need a FLAT page, which `spreadPage` does not build). Alternatively state in the comment that the test only guards against the container type when run alone, and keep it as documentation.

### IN-03: The determinism sweep reuses a truncating `scaled()` helper on non-exact dpi values, and its expected numbers have no regeneration source

**File:** `src/pipeline_tests/TestGeneratorDeterminism.cpp:73-76`, used at `:508`; literals at `:545-571`
**Issue:** The test-local `scaled` truncates (`base * dpi / 300`), while the generator rounds (`(base * dpi + 150) / 300`). The comment says "exact at 300 and 600". The new sweep test uses it for the speck margin at 75 and 200 dpi, where both rules happen to give the same result (5 and 13). A different sweep dpi would silently use a different margin than the generator. The accepted counts (1125, 845, 702, 697) and the overflow ranges come from "a script" that is not in the repository, so a deliberate change of a geometry constant breaks the test with no way to regenerate the numbers.
**Fix:** Use `(base * dpi + 150) / 300` in the test helper, or take the margin from the ground truth. Either commit the reproduction script (for example under `src/pipeline_tests/` or `.planning/phases/02-golden-baselines/records/`) or derive the expected counts in the test by an independent formula.

### IN-04: Carried over from the previous review, unchanged: wrong arithmetic in the noise-floor comment

**File:** `src/pipeline_tests/TestBookSpread.cpp:56-58`
**Issue:** "Measured 0.58 px; twice that, rounded up to 0.5 px" while the constant is 1.5 px. This was IN-01 of the earlier review and was not in the scope of the gap closure.
**Fix:** "twice that, rounded up to the step of 0.5 px: 1.5 px".

---

_Reviewed: 2026-10-24_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
