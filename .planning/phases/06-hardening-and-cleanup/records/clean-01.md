# CLEAN-01: OutputGenerator.cpp split

Decision D-16: `src/core/filters/output/OutputGenerator.cpp` (2814 lines at the planning base `e5319cd9`) is split into topic files by pure moves, in two steps. Every step is proven by `records/move_check.py` (chunk multiset), by the decoded-pixel digests of `records/output-digests.md` and by the gate (Release and Debug ctest 7 of 7, goldens unchanged). Output is normalised: `{repo}` for the repository path, `~` for the home folder.

## Step 1 (plan 06-13)

Checked on 2026-11-17. Commits: probe `f899b433` (gate `s17-digest-probe`), split `ff354702` (gate `s18-split-nodewarp`).

### The three new files

| File | Lines | Holds |
|------|-------|-------|
| `OutputGeneratorProcessor.h` | 239 | the `class OutputGenerator::Processor { ... };` definition, verbatim (guard `SCANTAILOR_OUTPUT_OUTPUTGENERATORPROCESSOR_H_`; the original include block and the two `using namespace` lines, so the header stands on its own) |
| `OutputGeneratorPixelOps.h` | 443 | the helpers that both dewarping paths need, verbatim and in original order, inside an anonymous namespace (`clipContentPolyToRect` stays in its nested anonymous namespace): `reserveBlackAndWhite` (6 definitions), `fillExcept` (2), `clipContentPolyToRect`, `fillMarginsInPlace` (4), `applyFillZonesInPlace` (3, with `using MapPointFunc`), `applyFillZonesToMixedInPlace` (2), `applyFillZonesToMask` (2), `smoothToGrayscale` |
| `OutputGeneratorNoDewarp.cpp` | 336 | `OutputGenerator::Processor::processWithoutDewarping`, verbatim; it needs no helper of its own, so it has no anonymous namespace |

`OutputGenerator.cpp` went from 2814 to 2041 lines. It includes the two headers. The three files were added to `sources` in `src/core/filters/output/CMakeLists.txt`. `PixelOps.h` is header-only with internal linkage: every translation unit that includes it gets its own copy of the helpers it calls, exactly as the single file had, and `-Wno-unused` (global) silences the helpers a unit does not call. Nothing in the three files has static storage besides functions, so the step adds no shared state (parallel page processing on the worker threads behaves as before).

### Helper-to-user map (re-derived, transitive closure)

Future files: `OutputGenerator.cpp` (OG), `OutputGeneratorNoDewarp.cpp` (NoDewarp), `OutputGeneratorDewarp.cpp` (Dewarp), `OutputGeneratorBinarize.cpp` (Binarize). The map comes from a scratch script that cuts the file into its 79 chunks and, for every helper, lists the chunks of each future file that mention its name, then adds helpers used by helpers (`fillMarginsInPlace` uses `clipContentPolyToRect` and `fillExcept`; `findRectAreas` uses `countConsecutiveZeroBitsTrailing` and `findPositionOfTheHighestBitSet`, which use the two `MultiplyDeBruijnBitPosition` tables; the `applyFillZones*` functions use `MapPointFunc`). OG itself calls no helper.

| Helper | Referenced by (with helper-to-helper calls) | Target file |
|--------|---------------------------------------------|-------------|
| `reserveBlackAndWhite` (6) | NoDewarp, Dewarp | `OutputGeneratorPixelOps.h` (moved in step 1) |
| `fillExcept` (2) | NoDewarp and Dewarp (through `fillMarginsInPlace`), Binarize (directly) | `OutputGeneratorPixelOps.h` (moved in step 1) |
| `clipContentPolyToRect` | NoDewarp, Dewarp (through `fillMarginsInPlace`) | `OutputGeneratorPixelOps.h` (moved in step 1) |
| `fillMarginsInPlace` (4) | NoDewarp, Dewarp | `OutputGeneratorPixelOps.h` (moved in step 1) |
| `MapPointFunc`, `applyFillZonesInPlace` (3) | NoDewarp, Dewarp | `OutputGeneratorPixelOps.h` (moved in step 1) |
| `applyFillZonesToMixedInPlace` (2) | NoDewarp, Dewarp | `OutputGeneratorPixelOps.h` (moved in step 1) |
| `applyFillZonesToMask` (2) | NoDewarp, Dewarp | `OutputGeneratorPixelOps.h` (moved in step 1) |
| `smoothToGrayscale` | NoDewarp, Dewarp | `OutputGeneratorPixelOps.h` (moved in step 1) |
| `RaiseAboveBackground`, `CombineInverted` | Binarize | `OutputGeneratorBinarize.cpp` (anonymous namespace) |
| `BinaryImageXOR` | Binarize | `OutputGeneratorBinarize.cpp` |
| `removeAutoPictureZones`, `createPictureZoneFromPoly` | Binarize | `OutputGeneratorBinarize.cpp` |
| `MultiplyDeBruijnBitPosition`, `MultiplyDeBruijnBitPosition2`, `countConsecutiveZeroBitsTrailing`, `findPositionOfTheHighestBitSet` | Binarize (through `findRectAreas`) | `OutputGeneratorBinarize.cpp` |
| `findRectAreas` | Binarize | `OutputGeneratorBinarize.cpp` |
| `hitMissReplaceAllDirections` | Binarize | `OutputGeneratorBinarize.cpp` |
| `to300dpi` | Binarize | `OutputGeneratorBinarize.cpp` |
| `calcLocalWindowSize`, `from300dpi` | nothing (no caller anywhere) | `OutputGeneratorBinarize.cpp`, next to `to300dpi`; kept, not deleted (D-16) |
| `applyAffineTransform` (2) | Dewarp | `OutputGeneratorDewarp.cpp` (anonymous namespace) |
| `movePointToTopMargin` (2), `movePointToBottomMargin` (2), `drawPoint` | Dewarp | `OutputGeneratorDewarp.cpp` |
| `vertBorderSkewAngle` | Dewarp | `OutputGeneratorDewarp.cpp` |

Placement plan for plan 06-14: the Dewarp-only and Binarize-only helpers (still in the anonymous namespace of `OutputGenerator.cpp`, chunks 9, 10, 19, 25, 26, 35 to 47, 49 and 50 of the chunk index) move with their users; after that `OutputGenerator.cpp` keeps no anonymous namespace. Binarize also calls `fillExcept`, so `OutputGeneratorBinarize.cpp` includes `OutputGeneratorPixelOps.h` too. The member functions still in `OutputGenerator.cpp` after step 1 are `processWithDewarping` and the Processor methods from `normalizeIlluminationGray` on; the Dewarp group is `processWithDewarping`, `setupTrivialDistortionModel`, `createDewarper`, `dewarp`, `buildAutoDistortionModel` and `buildMarginalDistortionModel` (chunks 56, 60, 61, 62, 78, 79), and the Binarize group is the methods from `normalizeIlluminationGray` to `modifyBinarizationMask` and from `detectPictures` to `transformToWorkingCs` (chunks 57 to 59 and 63 to 77).

The two helpers without a caller are `calcLocalWindowSize` and `from300dpi` (one textual occurrence each in the original file).

### Move check

Command (the original is read from git at `e5319cd9`, the four new files from the working tree):

```
$ python3 -I .planning/phases/06-hardening-and-cleanup/records/move_check.py --base e5319cd9 --orig src/core/filters/output/OutputGenerator.cpp --new src/core/filters/output/OutputGenerator.cpp src/core/filters/output/OutputGeneratorProcessor.h src/core/filters/output/OutputGeneratorPixelOps.h src/core/filters/output/OutputGeneratorNoDewarp.cpp
chunks: orig=79 new=79
RESULT: PASS
```

Only include, include-guard, `using namespace`, `namespace` and copyright lines differ (the plan counted 82 chunks including the three namespace closers; the script drops those, which leaves 79). `xcrun clang-format -i` was run on the four files and changed no moved line (the check above ran after formatting).

Positive controls, run before the script was used:

```
# unchanged file against itself
$ python3 -I .../move_check.py --base e5319cd9 --orig src/core/filters/output/OutputGenerator.cpp --new src/core/filters/output/OutputGenerator.cpp
chunks: orig=79 new=79
RESULT: PASS

# a copy under build/scratch with one character changed in a function body (line 82, QPoint(0, 0) became QPoint(0, 1))
$ python3 -I .../move_check.py --base e5319cd9 --orig src/core/filters/output/OutputGenerator.cpp --new build/scratch/OutputGenerator-mutated.cpp
chunks: orig=79 new=79
LOST  x1: OutputGenerator::OutputGenerator(const ImageTransformation& xform, const QPolygonF& contentRectPhys)
ADDED x1: OutputGenerator::OutputGenerator(const ImageTransformation& xform, const QPolygonF& contentRectPhys)
RESULT: FAIL   (exit 1)
```

### Digests

The probe is `pipeline_tests --run_test=OutputDigestProbeTestSuite --log_level=message` (disabled by default). Full lines are in `records/output-digests.md`.

| Page (kind dpi colour dewarping) | Research (e0b632f7) | s17 (before) | s18 (after) |
|----------------------------------|---------------------|--------------|-------------|
| flat 300 bw off | d76527cf | d76527cf | d76527cf |
| flat 600 gray off | 1e78a1c0 | 1e78a1c0 | 1e78a1c0 |
| mixed 300 mixed off | b7e3a693 | b7e3a693 | b7e3a693 |
| mixed 600 mixed off | 718260ea | 718260ea | 718260ea |
| curved 300 gray auto | 0476631c | 0476631c | 0476631c |
| curved 600 mixed auto | c48ff8a8 | c48ff8a8 | c48ff8a8 |
| curved 600 bw auto | dd346ae2 | dd346ae2 | dd346ae2 |

(first eight hex digits of the SHA-1.) Verdict: all three columns are equal; the first split step changes no decoded output pixel.

### Compile flags

`-ffp-contract=off` is the global compile option of the top-level `CMakeLists.txt`. In `build/macos-release/compile_commands.json` both `OutputGenerator.cpp` and `OutputGeneratorNoDewarp.cpp` carry it (check: two output-generator translation units found, both with the flag). The s18 command diff shows the same flag on the new compile line. No flag was added, removed or made specific to a file.

### Gate rows

| Label | Commit | Release ctest | Debug ctest | Goldens | Command diff (release / debug) |
|-------|--------|---------------|-------------|---------|--------------------------------|
| s17-digest-probe | f899b433 | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 3 / 3: the probe's compile line and the `pipeline_tests` link line (one object added) |
| s18-split-nodewarp | ff354702 | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 3 / 3: the `OutputGeneratorNoDewarp.cpp` compile line and the `liboutput.a` archive line (one member added, none removed, no automoc change) |

Output-path guard of convention 13: prints nothing (`OutputGenerator*` is outside it, the other guarded paths are untouched).
