# Review fixes for gap G-02-3 (02-REVIEW.md CR-01, WR-01 to WR-05)

Evidence recorded by plan 02-12 for CR-01. Paths are normalised: the checkout root is `{repo}`.

## CR-01: page generator dpi range

### Rule

`test_support::dpiSupported(kind, dpi)` is true only for 75 <= dpi <= 1200 where, with the geometry computed exactly as `computeGeometry` does (every length rounded to whole pixels), each text block's nominal ink box lies inside the page and inside its documented frame (SPREAD 640 x 860 per block, MIXED 420 x 850, CURVED 700 x 800 centred on the page; FLAT documents no frame), and the MIXED picture lies inside the page. The first condition keeps the glyph writes inside the raster; the frames keep the ground truth honest (the SPREAD gutter band starts where the left frame ends, MIXED speck clearance is measured from the MIXED frame).

A divisibility rule such as "multiples of 75" was rejected: `scaled()` is `(base * dpi + 150) / 300` and the glyph dot is 5 base px, so the dot is exact only where 5 * dpi is a multiple of 300 (every multiple of 60). At 150 dpi the dot is 3 px instead of 2.5 and at 225 dpi 4 px instead of 3.75; the dot error is multiplied by up to 125 cells and 117 line dots while the page size, block origins and frames are rounded once. 150 and 225 dpi are multiples of 75 and still overflow, so the rule is computed from the scaled geometry.

`generatePage`, `renderInkMask`, `mapFlatToImage` and `mapImageToFlat` all refuse a rejected dpi (null image with `truth.dpi` 0, null mask, `std::invalid_argument`). `placeGlyph` and `placeSpecks` additionally throw `std::logic_error` before any write outside the raster (backstop, unreachable for an accepted dpi).

### Accepted dpi counts (pinned by `test_dpi_support_rule`)

Of the 1126 values 75..1200: FLAT 1125, SPREAD 845, MIXED 702, CURVED 697. Accepted for every kind: 75 and every multiple of 60 from 120 to 1200 (300, 600 and 1200 among them). Rejected where the old generator's block crossed the page edge: FLAT 90; SPREAD 90..104 and 150..157; MIXED 90..96; CURVED 90..97. 150 and 225 are rejected for SPREAD, MIXED and CURVED. 1200 dpi is checked through `dpiSupported` only, not rendered.

### Sweep table (pinned by `test_dpi_sweep_gives_valid_or_null_page`)

| dpi | FLAT | SPREAD | MIXED | CURVED |
|-----|------|--------|-------|--------|
| 75 | page | page | page | page |
| 90 | null | null | null | null |
| 100 | page | null | null | null |
| 150 | page | null | null | null |
| 200 | page | page | page | page |
| 225 | page | null | null | null |
| 450 | page | null | null | null |

"page" means a valid page with every truth box inside the page, a non-null Grayscale8 ink mask of the page size and maps that do not throw; "null" means a null image, `truth.dpi` 0, a null mask for the spec and for the 300 dpi spec at that target dpi, and maps that throw `std::invalid_argument`. The suite output matched this table on the first run of the implementation.

### Pinned fingerprints

Recorded from the unmodified generator (seed 1, skew 1.5 / 1.0 / 0.5 / 0.5 for FLAT / SPREAD / MIXED / CURVED) in commit `8c47d66e` (`test(02-12): pin generator fingerprints at 75 dpi and of the curved page at 600 dpi`, touches only `src/pipeline_tests/TestGeneratorDeterminism.cpp`). The generator fix is commit `2a4ad510`; `8c47d66e` precedes it (checked with `git merge-base --is-ancestor`).

| Page | SHA-256 of the RGB32 scanlines |
|------|--------------------------------|
| FLAT 75 dpi | 6a55a26ae3e6c61d7e2843ed8776252777520a4f7a0a0ef9fa7a2403d3128676 |
| SPREAD 75 dpi | be9fd06ab9bcd1f484e618390a334491c4de029cf450f04e22308a9be9ba084a |
| MIXED 75 dpi | 69e7489f2618efdaceafc1c753fefda28439a71337813d518bb6021fffaa0458 |
| CURVED 75 dpi | cddf2d048f0f61cb10d9283730f0c4e9d8138fc6bd41fdea0a7133d9f5485942 |
| CURVED 600 dpi | d7e6fb18871c11675c4909a2e93275102bac3a8b37768dd82fd86ca771c70096 |

### Which case pins each kind and dpi

| Kind | 75 dpi | 300 dpi | 600 dpi |
|------|--------|---------|---------|
| FLAT | `GeneratorDeterminismTestSuite/test_pinned_pages_unchanged` | `test_flat_page_unchanged`; `FlatDocumentTestSuite/test_flat_fixture_fingerprints` (`flat_300_skew_*`) | `test_flat_page_unchanged`; `test_flat_fixture_fingerprints` (`flat_600_skew_*`) |
| SPREAD | `test_pinned_pages_unchanged` | `BookSpreadTestSuite/test_book_fixture_fingerprints` (`spread_300`) | `test_book_fixture_fingerprints` (`spread_600`) |
| MIXED | `test_pinned_pages_unchanged` | `MixedPageTestSuite/test_mixed_fixture_fingerprints` (`mixed_300`) | `test_mixed_fixture_fingerprints` (`mixed_600`) |
| CURVED | `test_pinned_pages_unchanged` | `test_book_fixture_fingerprints` (`curved_300`) | `test_pinned_pages_unchanged` |

Named pin run on the final tree, `build/macos-release/pipeline_tests --run_test=GeneratorDeterminismTestSuite/test_pinned_pages_unchanged --run_test=GeneratorDeterminismTestSuite/test_flat_page_unchanged --run_test=FlatDocumentTestSuite/test_flat_fixture_fingerprints --run_test=BookSpreadTestSuite/test_book_fixture_fingerprints --run_test=MixedPageTestSuite/test_mixed_fixture_fingerprints --log_level=test_suite` (log `build/scratch/cr01-pins.txt`): exit status 0, `*** No errors detected`, all five cases entered and left. `git diff --quiet a796c2ae -- src/pipeline_tests/golden/flat/fixtures.txt src/pipeline_tests/golden/book/fixtures.txt src/pipeline_tests/golden/mixed/fixtures.txt` exits 0: no golden file changed.

### RED: the sweep against the unmodified generator, under ASan

Command (macos-asan build of the tree with the sweep and the unmodified generator; the sweep was never run in the Release or Debug build):

```
ASAN_OPTIONS=detect_leaks=0:halt_on_error=0 UBSAN_OPTIONS=print_stacktrace=1 \
  build/macos-asan/pipeline_tests \
  --run_test=GeneratorDeterminismTestSuite/test_dpi_sweep_gives_valid_or_null_page --log_level=test_suite
```

Exit status 201. First report (log `build/scratch/cr01-asan-red.txt`; the sweep starts with FLAT and FLAT overflows only at 90 dpi, so this is FLAT 90 dpi):

```
==81564==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x633000035c8b at pc 0x00010385f72c bp 0x00016cf9bf20 sp 0x00016cf9bf18
WRITE of size 1 at 0x633000035c8b thread T0
    #0 in test_support::(anonymous namespace)::placeGlyph(...) PageGenerator.cpp:394
    #1 in test_support::(anonymous namespace)::placeTextBlock(...) PageGenerator.cpp:420
    #2 in test_support::(anonymous namespace)::buildLayout(test_support::PageKind, int, unsigned long long) PageGenerator.cpp:482
    #3 in test_support::generatePage(test_support::PageSpec const&) PageGenerator.cpp:562
    #4 in (anonymous namespace)::cachedPage(test_support::PageSpec const&) TestGeneratorDeterminism.cpp:84
    #5 in (anonymous namespace)::sweepPage(test_support::PageSpec const&) TestGeneratorDeterminism.cpp:115
    #6 in GeneratorDeterminismTestSuite::test_dpi_sweep_gives_valid_or_null_page::test_method() TestGeneratorDeterminism.cpp:464
SUMMARY: AddressSanitizer: heap-buffer-overflow PageGenerator.cpp:394 in ...placeGlyph(...)
```

The log holds 6 `AddressSanitizer` lines; after the first error the process continued into a second report, `AddressSanitizer: SEGV` at the same line.

### GREEN: the whole generator suite on the final tree, under ASan

Same environment, `build/macos-asan/pipeline_tests --run_test=GeneratorDeterminismTestSuite --log_level=test_suite` (log `build/scratch/cr01-asan-green.txt`): exit status 0, `*** No errors detected`, 0 lines with `AddressSanitizer`, 0 lines with `runtime error:`. All 13 cases of the suite ran, including `test_pinned_pages_unchanged`, `test_dpi_sweep_gives_valid_or_null_page` and `test_dpi_support_rule`.

### Release

`build/macos-release/pipeline_tests --run_test=GeneratorDeterminismTestSuite --log_level=test_suite`: exit status 0, `*** No errors detected`.

`ctest --preset macos-release -R 'pipeline_tests|imageproc_tests|dewarping_tests' --output-on-failure`: `100% tests passed out of 3` (imageproc_tests, pipeline_tests 54.39 s, dewarping_tests 0.54 s).

### Commits

| Commit | Content |
|--------|---------|
| `8c47d66e` | pin commit, test file only |
| `2a4ad510` | `dpiSupported(PageKind, int)`, `layoutFits`, raster guards, maps on the same rule, the sweep |
| `eb62abd5` | `test_dpi_support_rule`, map checks in the sweep, header contract |
