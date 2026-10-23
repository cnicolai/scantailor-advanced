---
phase: 02-golden-baselines
reviewed: 2026-10-23T00:00:00Z
depth: standard
files_reviewed: 43
files_reviewed_list:
  - src/CMakeLists.txt
  - src/dewarping/tests/CMakeLists.txt
  - src/dewarping/tests/main.cpp
  - src/dewarping/tests/TestCylindricalDewarper.cpp
  - src/dewarping/tests/TestDistortionModel.cpp
  - src/dewarping/tests/TestTextLineTracer.cpp
  - src/imageproc/tests/CMakeLists.txt
  - src/imageproc/tests/TestBinarize.cpp
  - src/imageproc/tests/TestScale.cpp
  - src/pipeline_tests/CMakeLists.txt
  - src/pipeline_tests/HeadlessPipeline.cpp
  - src/pipeline_tests/HeadlessPipeline.h
  - src/pipeline_tests/main.cpp
  - src/pipeline_tests/TestBookSpread.cpp
  - src/pipeline_tests/TestComparatorSelfTest.cpp
  - src/pipeline_tests/TestEnvironment.cpp
  - src/pipeline_tests/TestFlatDocument.cpp
  - src/pipeline_tests/TestGeneratorDeterminism.cpp
  - src/pipeline_tests/TestGoldenStore.cpp
  - src/pipeline_tests/TestImageCompare.cpp
  - src/pipeline_tests/TestMixedPage.cpp
  - src/pipeline_tests/TestPipelineSmoke.cpp
  - src/pipeline_tests/TestProjectRoundTrip.cpp
  - src/pipeline_tests/TestTruthMetrics.cpp
  - src/qt_tests/CMakeLists.txt
  - src/qt_tests/main.cpp
  - src/test_support/CMakeLists.txt
  - src/test_support/ExpectedFailure.h
  - src/test_support/GoldenChecks.h
  - src/test_support/GoldenStore.cpp
  - src/test_support/GoldenStore.h
  - src/test_support/ImageCompare.cpp
  - src/test_support/ImageCompare.h
  - src/test_support/Manifest.cpp
  - src/test_support/Manifest.h
  - src/test_support/PageGenerator.cpp
  - src/test_support/PageGenerator.h
  - src/test_support/Rng.h
  - src/test_support/TextGolden.cpp
  - src/test_support/TextGolden.h
  - src/test_support/Tolerances.h
  - src/test_support/TruthMetrics.cpp
  - src/test_support/TruthMetrics.h
findings:
  critical: 1
  warning: 5
  info: 7
  total: 13
status: issues_found
---

# Phase 2: Code Review Report

**Reviewed:** 2026-10-23
**Depth:** standard
**Files Reviewed:** 43
**Status:** issues_found

## Summary

The phase adds a synthetic page generator, an image and text comparator, a golden store with an update mode, a headless six-stage pipeline driver and the scenario suites on top of them. The comparator, the golden store and the truth metrics are carefully built: names are validated before they become paths, writes under the golden root go through `QSaveFile`, the only recursive delete re-checks its root, and the self-tests prove the update mode cannot fire from the comparator suite. Production code is untouched except for the three `add_subdirectory` lines, as intended.

The one serious defect is in `PageGenerator`: for part of the documented dpi range (75..1200) the glyph raster is written outside its buffer. No current test uses an affected dpi (all use 75, 300 and 600), so the suite is green, but the contract in `PageGenerator.h` is wrong and the first scenario at a common scan dpi (150 for a spread) will silently produce a wrapped, wrong page. The warnings concern test-isolation hygiene (an environment variable left set on failure), a manifest bookkeeping gap in the store, one vacuous assertion in the round-trip suite, a dangling-reference hazard in a page cache, and unvalidated text-golden serialisation.

Known recorded expected failures (`XFAIL_CHECK` for F-04-1, F-04-2, F-05-1, F-06-1 and the AUTO layout case) were treated as intentional and are not reported.

## Critical Issues

### CR-01: `generatePage` / `renderInkMask` write outside the raster for 44 of the documented dpi values

**File:** `src/test_support/PageGenerator.cpp:377-400` (write at line 394), geometry in `computeGeometry` `:213-261`, validity gate `:146-148`, `:557`, `:649`
**Issue:** `scaled()` rounds every length to the nearest pixel independently, but the glyph dot (`scaled(5, dpi)`) is multiplied by up to 125 cells and 117 line dots afterwards, so the rounding error of the dot is amplified 100-fold while the block origin and the page size are rounded only once. `dpiSupported()` accepts every dpi in 75..1200, `placeGlyph` writes `layout.raster[layout.index(x, y)]` with no bounds check, and `Layout::index` is `y * width + x`. I reproduced the geometry in a script for all dpi 75..1200 and found overshoots:

- dpi 90..97 (and 98..104 for SPREAD only): the block's bottom edge lies 1 to 22 px below the page, so rows beyond `height` are written: a heap buffer overflow of up to `dot * width` bytes (for example MIXED at 90 dpi: 22 rows).
- dpi 95..104 and 150..157 for SPREAD, 95..97 for CURVED: the block's right edge lies 1 to 24 px beyond `width`; the glyph pixels wrap into the next row (wrong page, no crash except on the last row).

150 dpi is a very common scan resolution and is exactly the kind of value a later phase adds to a sweep. The existing `test_invalid_dpi_gives_null_page` only probes 75 as the lowest valid dpi, and `test_format_size_and_dpi` only 300 and 600, so nothing catches this. The header promises "Anything else gives a null image", i.e. every dpi inside the range is valid.
**Fix:** Either narrow the contract to the dpi values that are verified, or make the layout robust. The cheapest safe change is to reject block geometry that does not fit, and to guard the write:

```cpp
// in placeGlyph, before the inner loops
if (x0 < 0 || y0 < 0 || x0 + dot > layout.g.width || y0 + dot > layout.g.height) {
  throw std::runtime_error("test_support: text block does not fit the page at this dpi");
}
```

and document in `PageGenerator.h` (and in `dpiSupported`) that only dpi values for which the block fits are supported, or restrict `dpiSupported` to `dpi % 75 == 0` (exact integer scaling, which also removes the rounding differences between `scaled()` and the truncating `scaled()` copy in `TestGeneratorDeterminism.cpp:72`). Add a test that generates every kind at a sweep of dpi values (for example 75, 90, 100, 150, 200, 450, 1200) and expects either a valid page or the documented rejection.

## Warnings

### WR-01: `test_golden_directory_untouched` leaves `UPDATE_GOLDENS=1` set when an assertion throws

**File:** `src/pipeline_tests/TestComparatorSelfTest.cpp:473-500`
**Issue:** The case sets `UPDATE_GOLDENS=1` at line 477 and restores it at lines 493-497. Between the two, `requireFailure` (line 482), `compareWithGolden` and `loadGolden` (line 485) can reach `BOOST_REQUIRE_MESSAGE` (`requireMetricFails` line 157, `loadGolden` line 123), which throws and skips the restore. The process then keeps update mode on for every later suite that has not yet created `pipelineGoldenStore()` (the store reads the variable once, lazily, in `GoldenChecks.h:42-46`). In the default run `BookSpread` creates the store first, so the leak is harmless there, but with `--run_test=ComparatorSelfTestSuite,FlatDocumentTestSuite` or any reordering, a failing self-test would turn the next suite into one that overwrites goldens. The test also guards the one invariant (golden files never written by a failing comparison) that must not be undermined by the test itself.
**Fix:** Restore the variable with an RAII guard:

```cpp
struct ScopedEnv {
  ScopedEnv(const char* name, const char* value) : m_name(name), m_had(qEnvironmentVariableIsSet(name)), m_old(qgetenv(name)) {
    qputenv(name, value);
  }
  ~ScopedEnv() {
    if (m_had) { qputenv(m_name, m_old); } else { qunsetenv(m_name); }
  }
  const char* m_name; bool m_had; QByteArray m_old;
};
// BOOST_AUTO_TEST_CASE(...) { const ScopedEnv env("UPDATE_GOLDENS", "1"); ... }
```

### WR-02: A failed manifest write is reported once and then silently skipped for the rest of the suite

**File:** `src/test_support/GoldenStore.cpp:190-202`
**Issue:** `SuiteManifest::refresh` calls `m_handled.insert(m_suite)` before it tries to write. If `saveAtomically` fails (read-only file, full disk), the first golden of the suite fails with the error, but `m_suite` is already marked as handled. Every following golden in the same suite then gets `refresh() == true` and is recorded as a normal pass. The result of an update run is a suite of fresh goldens without a `MANIFEST.txt` (or with a stale one) and only one failing check that is easy to overlook, which defeats the toolchain-difference warning (D-09) on the next normal run.
**Fix:** Mark the suite handled only after success:

```cpp
bool refresh(QString* error) {
  if (m_handled.count(m_suite)) {
    return true;
  }
  ... // build data
  if (!(readBytes(m_path, &readable) == data && readable) && !saveAtomically(m_path, data, error)) {
    return false;
  }
  m_handled.insert(m_suite);
  return true;
}
```

### WR-03: `test_every_stage_carries_non_default_values` is vacuous for the page-split stage

**File:** `src/pipeline_tests/TestProjectRoundTrip.cpp:326-339` (setup at `:277-279`)
**Issue:** The test asserts that each stage's XML differs between the non-default and the default round trip. For `page-split`, `roundTrip` only calls `runPageSplit` when `nonDefault` is true (line 277). The default project therefore has no `<image>` elements for the stage at all (the comment at lines 230-233 says exactly that), so the two serialisations differ by presence regardless of whether `SINGLE_PAGE_UNCUT` is written. A bug that dropped the layout type from the project file would still pass. The check proves "the stage ran", not "the non-default value is carried".
**Fix:** Run the stage in both round trips (call `runPageSplit` unconditionally; with the default layout type the stage needs content detection, so run it with `AUTO` on the 64x64 fixture only if that works, otherwise compare the `type` attribute explicitly), or assert the value directly: find the `<image>`/`layout` element of the non-default project and check that its type attribute equals `SINGLE_PAGE_UNCUT`, as `test_numeric_values_parse_back_exactly` does for the other stages.

### WR-04: `spreadPage` returns a reference into a `std::vector` that grows

**File:** `src/pipeline_tests/TestBookSpread.cpp:98-107`
**Issue:** The cache is `std::vector<std::pair<int, GeneratedPage>>` and the function returns `cache.back().second` / `entry.second` by const reference. The second `emplace_back` (dpi 600 after dpi 300) can reallocate and invalidate every reference handed out earlier. Today every caller copies (`m.page = spreadPage(...)`, line 187) or uses the value inside one full expression (lines 410-411), so nothing dangles, but the signature invites a `const auto& page = spreadPage(300); ... spreadPage(600); use(page)` bug. The sibling caches in `TestFlatDocument.cpp:79`, `TestMixedPage.cpp:107` and `TestGeneratorDeterminism.cpp:79` use `std::map`, which has stable references.
**Fix:** Use `static std::map<int, test_support::GeneratedPage> cache;` like the other suites.

### WR-05: Text goldens are written without checking that they parse back

**File:** `src/test_support/GoldenStore.cpp:341, 367`; `src/test_support/TextGolden.cpp:51-60`
**Issue:** `serializeText` writes `key = value\n` and the preconditions "keys must not contain ' = ' or line breaks, values no line breaks" (`TextGolden.h:15`) are comments only. A value or key that violates them produces a file that `parseText` rejects or mis-splits (a key containing " = " is cut at the first separator; a value with a newline creates a bogus line). In update mode `checkText` records such a file, reports `written`, and the next normal run fails with "cannot be parsed", far from the cause. This matters because values are produced by test code from pipeline state, for example any future string value taken from a stage.
**Fix:** In `checkText`, before saving (and before the artifact write), verify `bool ok; parseText(serializeText(actual), &ok) == actual && ok` and fail with a message naming the offending key; or make `serializeText` assert the preconditions.

## Info

### IN-01: Stale and misleading constants comment in the book suite

**File:** `src/pipeline_tests/TestBookSpread.cpp:55-57`
**Issue:** The comment says "Measured 0.58 px; twice that, rounded up to 0.5 px" but the value is 1.5 px (twice 0.58 is 1.16, rounded up to the 0.5 step is 1.5). A reader will take the comment for the bound.
**Fix:** "twice that, rounded up to the step of 0.5 px: 1.5 px".

### IN-02: `areaAverageReference` can read one row/column past the source if sizes change

**File:** `src/imageproc/tests/TestScale.cpp:68-76`
**Issue:** The loops run to `ceil(bottom)` / `ceil(right)` where `bottom = (dy + 1) * (src.height() / dst.height())` in floating point. For ratios that are not exactly representable the product can land one ulp above `src.height()`, `ceil` then yields `height + 1` and `src.constScanLine(sy)` reads outside the image. The two sizes used today (50 and 80 from 100) are exact, so it is latent. The block of commented-out `checkScale` calls (lines 104-108, including 55x145 and 145x55, which are not exact) is the case that would trigger it if re-enabled.
**Fix:** Clamp: `for (int sy = int(top); sy < std::min(src.height(), int(std::ceil(bottom))); ++sy)` and the same for `sx`; delete the commented-out calls or turn them into a documented skipped case.

### IN-03: The NFD file name depends on raw combining bytes in the source file

**File:** `src/pipeline_tests/TestProjectRoundTrip.cpp:71-73`
**Issue:** `u"Seite_ä_NFD.png"` is meant to hold U+0061 U+0308; the bytes in the file are `61 cc 88`, so it works today, but any editor, formatter or `git` normalisation that composes the character turns the NFD name into the NFC name and the test then fails with a confusing collision (`nameNfc() != nameNfd()` at line 353 would flag it, but only after the fixture wrote one file over the other).
**Fix:** Spell both forms with escapes: `u"Seite_ä_NFC.png"` and `u"Seite_ä_NFD.png"`.

### IN-04: Nearly identical sanity and reporting code in three scenario files

**File:** `src/pipeline_tests/TestFlatDocument.cpp:136-179`, `src/pipeline_tests/TestBookSpread.cpp:120-166`, `src/pipeline_tests/TestMixedPage.cpp:339-396`; also `reportMetric`, `num`, `SanityResult` in all three
**Issue:** `flatSanity` and `subPageSanity` are the same 45 lines (size, ink, orientation, line count, margin position) with different expectations, and `SanityResult` re-declares the `{ok, reason}` pair that `test_support::SanityCheck` and `test_support::Sanity` already provide. Constants `kMarginSlackMm`, `kSanityWidthSlack`, `kMinInkFractionOfOutput` are repeated. A fix to one copy (for example the margin slack) will miss the others.
**Fix:** Move the shared checks into `test_support` (for example `TruthMetrics`) with the per-scenario numbers as parameters, and use `SanityCheck` directly.

### IN-05: Test code carries plan and decision identifiers that mean nothing outside `.planning`

**File:** `src/pipeline_tests/HeadlessPipeline.h:72-75`, `src/test_support/Tolerances.h:7-10`, `src/pipeline_tests/TestGeneratorDeterminism.cpp:392`, many `D-xx` / `plan 02-0x` references in the scenario files
**Issue:** "This file is frozen after plan 02-03 ... wave-3 scenario plans ... must not edit" is process state, not a property of the code, and goes stale as soon as the phase is archived. Several comments (`D-05`, `D-15`, `plan 02-01`) cannot be resolved without the planning directory. CLAUDE.md asks for comments on non-obvious intent, not restated process.
**Fix:** Keep the technical reason (what the file guarantees, what the bound is derived from) and drop the plan identifiers, or reference `.planning/phases/02-golden-baselines/records/*.md` by path once.

### IN-06: `ExpectedFailure.h` is included but unused

**File:** `src/dewarping/tests/TestCylindricalDewarper.cpp:5`
**Issue:** The file uses no `XFAIL_CHECK`; the include only adds a dependency on `test_support`'s Boost-aware header. `src/dewarping/tests/main.cpp:4-6` also keeps a `HAVE_CONFIG_H` / `config.h` include that no longer does anything.
**Fix:** Remove both.

### IN-07: The test sources need Qt 6.5 and Boost headers, but the build still advertises a Qt 5 fallback

**File:** `src/CMakeLists.txt:10-15`, `src/test_support/PageGenerator.cpp:693` (`QByteArrayView`), `src/pipeline_tests/TestProjectRoundTrip.cpp:209-211` (`QDomDocument::setContent` returning `ParseResult`, Qt 6.5)
**Issue:** The top-level `CMakeLists.txt` still falls back to Qt 5.9 when Qt 6 is missing, but `BUILD_TESTS` (default ON) now unconditionally adds targets that do not compile against Qt 5 and against Qt 6.0-6.4. The project constraint is Qt 6 on macOS, so this is a robustness point only, but the failure mode is a compile error far from the cause.
**Fix:** Guard the three `add_subdirectory` lines with `if (BUILD_TESTS AND Qt6_FOUND)` (or a `Qt6_VERSION VERSION_GREATER_EQUAL 6.5` check) and print a clear `message(STATUS ...)` otherwise.

---

_Reviewed: 2026-10-23_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
