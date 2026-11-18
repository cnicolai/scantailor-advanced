# fp-contract measurement: what `-ffp-contract=on` does to the committed goldens

One-off measurement for the Phase 1 finding WR-02. Nothing in the tracked tree changed: the default flag stays `-ffp-contract=off`, no golden was written, and `UPDATE_GOLDENS` was unset for every run.

- Tested commit: `0a4fe7c776db6793d883ea37c510c033e3fc6be0` (final source tree of the phase; only `.planning/` commits follow).
- Compiler: `Apple clang version 21.0.0 (clang-2100.4.1.1)`; Release build, Ninja, `-DCMAKE_PREFIX_PATH=/opt/homebrew`.

## Method

1. A copy of the committed tree: `git archive -o build/scratch/fp-src.tar HEAD`, unpacked into `build/scratch/fp-src` (the main tree was never edited).
2. Two changes in the copy only: in `CMakeLists.txt` the value `-ffp-contract=off` became `-ffp-contract=on`; `GoldenStore::checkImage` and `GoldenStore::checkText` in `src/test_support/GoldenStore.cpp` print one `FPDIFF <suite> <name> <kind> ...` line to stderr per golden. Images are compared on `canonicalImage` of both sides: bilevel goldens count the differing pixels (maximum difference 1), colour goldens count pixels with any channel difference and give the largest channel difference. Text goldens compare every key as a string; for differing numeric values the absolute delta is reported.
3. Configure and build: `cmake -S build/scratch/fp-src -B build/scratch/fp-on -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/homebrew -DBUILD_TESTS=ON -DCMAKE_FIND_FRAMEWORK=LAST -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`, then `cmake --build build/scratch/fp-on --target pipeline_tests`. `compile_commands.json` of that tree holds `-ffp-contract=on` on 473 lines and `-ffp-contract=off` on none.
4. Run, only the four scenario suites (the comparator self-test and the golden-store test use perturbed images or throwaway stores and would give false rows): `env -u UPDATE_GOLDENS build/scratch/fp-on/pipeline_tests --run_test=FlatDocumentTestSuite,BookSpreadTestSuite,DewarpPageTestSuite,MixedPageTestSuite --log_level=message`.
5. Afterwards: `git status --porcelain -- src CMakeLists.txt CMakePresets.json cmake` printed nothing in the main tree, and `diff -r -q src/pipeline_tests/golden build/scratch/fp-src/src/pipeline_tests/golden` printed nothing.

The run prints `TOOLCHAIN DIFFERS` warnings (the manifest key `ffp_contract` is `on` instead of `off`, and the manifests were recorded with an older Apple Clang build, cmake and libjpeg-turbo). Compare-only mode warns and does not fail on them.

## Result

18 image goldens and 17 text goldens (the three `fixtures.txt` included) were compared, 35 `FPDIFF` lines, one per golden.

**One golden differs: `book/curved_300_dewarp` (image).** 426 of its 1872 x 1824 = 3,414,528 pixels (0.0125 percent) differ, and the largest difference is 1 (the image is bilevel, so 426 pixels flip). Every other image golden is pixel-identical, and every text golden (including the numeric stage values of the dewarp case) is value-identical.

The stock metrics all pass in this run, so no scenario case fails with the flag on: `*** No errors detected`, exit 0, and 457 `GOLDEN-METRIC` lines all say `result=pass`. For the dewarp image the comparator's `edge_tolerant_xor_fraction` is 0 (limit 0.0001), because the 426 flipped pixels sit on glyph edges.

### Control with the default flag

To show that the 426 pixels come from the flag and not from the newer toolchain than the one that recorded the goldens, the same instrumented copy was built with the flag left at `-ffp-contract=off` (`build/scratch/fp-src-off`, `build/scratch/fp-off`; 473 `off` lines and no `on` line in its `compile_commands.json`) and run the same way. All 35 goldens are identical there (`differing=0` for every image, `differing_keys=0` for every text golden, exit 0).

### Per golden

| Suite | Golden | Kind | With -ffp-contract=on | Stock metrics of that run |
|-------|--------|------|-----------------------|---------------------------|
| book | curved_300_dewarp | image | 426 pixels differ, largest difference 1 | 4 metrics pass |
| book | curved_300_dewarp | text | identical | 8 metrics pass |
| book | fixtures | text | identical | 3 metrics pass |
| book | spread_300_bw | text | identical | 41 metrics pass |
| book | spread_300_bw_left | image | identical | 4 metrics pass |
| book | spread_300_bw_right | image | identical | 4 metrics pass |
| book | spread_300_color | text | identical | 41 metrics pass |
| book | spread_300_color_left | image | identical | 4 metrics pass |
| book | spread_300_color_right | image | identical | 4 metrics pass |
| book | spread_600_bw | text | identical | 41 metrics pass |
| book | spread_600_bw_left | image | identical | 4 metrics pass |
| book | spread_600_bw_right | image | identical | 4 metrics pass |
| book | spread_600_color | text | identical | 41 metrics pass |
| book | spread_600_color_left | image | identical | 4 metrics pass |
| book | spread_600_color_right | image | identical | 4 metrics pass |
| flat | fixtures | text | identical | 18 metrics pass |
| flat | flat_300_bw | image | identical | 4 metrics pass |
| flat | flat_300_bw | text | identical | 20 metrics pass |
| flat | flat_300_color | image | identical | 4 metrics pass |
| flat | flat_300_color | text | identical | 20 metrics pass |
| flat | flat_600_bw | image | identical | 4 metrics pass |
| flat | flat_600_bw | text | identical | 20 metrics pass |
| flat | flat_600_color | image | identical | 4 metrics pass |
| flat | flat_600_color | text | identical | 20 metrics pass |
| mixed | fixtures | text | identical | 2 metrics pass |
| mixed | mixed_300_bw | image | identical | 4 metrics pass |
| mixed | mixed_300_bw | text | identical | 22 metrics pass |
| mixed | mixed_300_color | image | identical | 4 metrics pass |
| mixed | mixed_300_color | text | identical | 22 metrics pass |
| mixed | mixed_300_mixed | image | identical | 4 metrics pass |
| mixed | mixed_300_mixed | text | identical | 22 metrics pass |
| mixed | mixed_600_bw | image | identical | 4 metrics pass |
| mixed | mixed_600_bw | text | identical | 22 metrics pass |
| mixed | mixed_600_color | image | identical | 4 metrics pass |
| mixed | mixed_600_color | text | identical | 22 metrics pass |

## Reading

- With `-ffp-contract=on` (Apple Clang's own default), 34 of 35 goldens stay byte-identical in decoded pixels and values; the dewarping case changes by 426 bilevel pixels. The comparator would still accept that output.
- The default stays `-ffp-contract=off`. This is information for the user; changing the default would be its own phase with a golden update.
