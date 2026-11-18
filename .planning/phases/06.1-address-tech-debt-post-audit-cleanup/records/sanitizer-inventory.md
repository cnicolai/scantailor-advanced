# Sanitizer inventory: collect-mode run of all seven suites on the unfixed tree (plan 06.1-11)

A fresh run of the `macos-asan` build in collect mode, taken before any fix of this plan, to find every location the strict preset would stop at. The Phase 2 record is `.planning/phases/02-golden-baselines/records/sanitizer-findings.md`; a location is `known` when that record lists it, `new` otherwise.

## Method

- Tested commit: `ce3e6760f24dc449058e44ebf5f85869a0eafff5` (clean tree; only docs commits after the last source change).
- Rebuild: `cmake --preset macos-asan && cmake --build --preset macos-asan` (exit 0), so the objects match the tested tree.
- Run: `ctest --preset macos-asan -V > build/scratch/asan-inventory.txt 2>&1`. At this commit the test preset `macos-asan` is still the collect preset: `ASAN_OPTIONS=detect_leaks=0:halt_on_error=0`, `UBSAN_OPTIONS=print_stacktrace=1`, no suppression file.
- Result of the run: `100% tests passed out of 7` (exit 0), `Total Test time (real) = 746.39 sec`. Per test: foundation_tests 0.14 s, math_tests 0.06 s, imageproc_tests 20.22 s, core_tests 1.05 s, qt_tests 16.56 s, pipeline_tests 695.84 s, dewarping_tests 12.51 s.
- Extraction: `grep -c 'runtime error:'` prints 13 and `grep -c AddressSanitizer` prints 0; a small `python3 -I` script under `build/scratch/` groups the 13 lines by source location and report text and prints the first frames of each stack. UBSan prints each location once per process, so a count is the number of executables that reached the location.

## Findings in project code

| Report | Location | Count | Executables | Known? |
|--------|----------|-------|-------------|--------|
| member access within null pointer of type 'SharedData' | `{repo}/src/imageproc/BinaryImage.cpp:1044:37` | 5 | imageproc_tests, core_tests, qt_tests, pipeline_tests, dewarping_tests | known (Phase 1 and Phase 2 record; qt_tests also reaches it now) |
| left shift of negative value -1 | `{repo}/src/imageproc/SlicedHistogram.cpp:49:47` | 1 | imageproc_tests | known (Phase 1 and Phase 2 record) |
| left shift of negative value -1 | `{repo}/src/core/Despeckle.cpp:368:46` | 1 | pipeline_tests | known (Phase 2 record) |
| left shift of negative value -125 | `{repo}/src/core/Despeckle.cpp:392:48` | 1 | pipeline_tests | known (Phase 2 record) |
| left shift of negative value -60 | `{repo}/src/core/Despeckle.cpp:418:48` | 1 | pipeline_tests | known (Phase 2 record) |
| left shift of negative value -1 | `{repo}/src/core/Despeckle.cpp:446:46` | 1 | pipeline_tests | known (Phase 2 record) |
| left shift of negative value -1 | `{repo}/src/imageproc/InfluenceMap.cpp:129:37` | 1 | pipeline_tests | known (Phase 2 record) |
| left shift of negative value -1 | `{repo}/src/imageproc/InfluenceMap.cpp:130:37` | 1 | pipeline_tests | known (Phase 2 record) |
| shift exponent -431 is negative | `{repo}/src/core/EstimateBackground.cpp:243:46` | 1 | pipeline_tests | known (Phase 2 record) |

Top frames: `BinaryImage::SharedData::operator new` (from `SharedData::create`); `SlicedHistogram::processHorizontalLines`; `voronoi` in `Despeckle.cpp` (reached through `Despeckle::despeckle` from `ContentBoxFinder::findContentBox`); `InfluenceMap::init` (from `ContentBoxFinder::segmentGarbage`); `estimateBackground` (from `OutputGenerator::Processor::normalizeIlluminationGray`). Every top frame lies in project code.

The Despeckle tests reach four of the ten `vec.x` shift sites (lines 368, 392, 418 and 446); the other six have the same shape and are fixed with them.

## Findings in third-party code

None. No report has a top frame in Qt, libtiff, libpng, libjpeg or Boost, so no suppression file is created.

## Totals

- UBSan reports: 13 (9 distinct project locations); ASan reports: 0.
- Known: 9; new: 0. The fresh run finds nothing beyond the Phase 2 record.
- Per executable (UBSan): imageproc_tests 2, core_tests 1, qt_tests 1, pipeline_tests 8, dewarping_tests 1, foundation_tests 0, math_tests 0.
