# Closing check of Phase 2 (plan 02-10)

Checked on the tree at the commit below (the last commit before this record; the other records of this plan add only files under `.planning/`). Nothing was fixed in this plan; a failing criterion would carry a FAIL verdict. Output is normalised: `{repo}` for the repository path, `~` for the home directory.

## Commit checked and toolchain

```
$ git rev-parse HEAD
9268cd446d2dd8b25089cbabb6f9b4bc1a7fe04a
exit status: 0
```

short hash: `9268cd44`

```
$ cmake --version | head -1
cmake version 4.4.4
exit status: 0
```

```
$ clang --version | head -1
Apple clang version 21.0.0 (clang-2100.3.34.2)
exit status: 0
```

```
$ brew list --versions qtbase boost jpeg-turbo libpng libtiff
boost 1.92.0
jpeg-turbo 3.2.0
libpng 1.6.59
libtiff 4.7.2
qtbase 6.12.0_1
exit status: 0
```

```
$ sw_vers -productVersion; xcodebuild -version | head -2
26.7
Xcode 27.1
Build version 27A9275
exit status: 0
```

Manifest of the goldens (`golden/flat/MANIFEST.txt`; `golden/book/MANIFEST.txt` and `golden/mixed/MANIFEST.txt` name the same toolchain, only `source_commit` differs):

```
apple_clang = 21.0.0 (clang-2100.3.34.2)
boost = 1_92
build_type = Release
cmake = 4.4.4
ffp_contract = off
libjpeg_turbo = 3.2.0
libpng = 1.6.59
libtiff = LIBTIFF, Version 4.7.2
macos = 26.7.0
qt_compile = 6.12.0
qt_runtime = 6.12.0
source_commit = 24d18b626e9a5c15b3f22667981e31e915c2eeb7
xcode = Xcode 27.1, Build version 27A9275
```

Comparison: cmake 4.4.4, Apple clang 21.0.0, boost 1.92, jpeg-turbo 3.2.0, libpng 1.6.59, libtiff 4.7.2, qtbase 6.12.0 (installed as `6.12.0_1`, the Homebrew revision of the same Qt release), macOS 26.7 and Xcode 27.1 / 27A9275 are the versions in the manifests. No difference to act on; `build_type = Release` and `ffp_contract = off` in the manifests are what the goldens were recorded with.

## TEST-04 proof

Non-test pathspec: `src CMakeLists.txt CMakePresets.json cmake :!src/*/tests/* :!src/qt_tests/* :!src/test_support/* :!src/pipeline_tests/*` (excludes every test path: `src/*/tests`, `src/qt_tests`, `src/test_support`, `src/pipeline_tests`).

(a) Production files that differ from `cde6825` (the last Phase 1 code commit; `src/` is byte-identical to the 1.2.1 release `5eaac18` there):

```
$ git diff --name-only cde6825 -- <non-test pathspec>
src/CMakeLists.txt
exit status: 0
```

The hunk of `src/CMakeLists.txt`:

```
$ git diff cde6825 -- src/CMakeLists.txt
diff --git a/src/CMakeLists.txt b/src/CMakeLists.txt
index ad1a404b..5727fc8e 100644
--- a/src/CMakeLists.txt
+++ b/src/CMakeLists.txt
@@ -9,4 +9,7 @@ add_subdirectory(app)
 
 if (BUILD_TESTS)
   add_subdirectory(qt_tests)
+  add_subdirectory(test_support)
+  add_subdirectory(pipeline_tests)
+  add_subdirectory(dewarping/tests)
 endif()
\ No newline at end of file
exit status: 0
```

Changed lines of the hunk other than the three `add_subdirectory` additions (`test_support`, `pipeline_tests`, `dewarping/tests`): **0**.

(b) Top-level CMake files and `cmake/` (production compile flags) against `cde6825`:

```
$ git diff --quiet cde6825 -- CMakeLists.txt CMakePresets.json cmake

exit status: 0
```

(c) The same non-test diff between `cde6825` and every commit that touched `src/pipeline_tests/golden`:

```

a796c2a  test(02-07): record mixed-page goldens from macos-release
  non-test diff vs cde6825: src/CMakeLists.txt  -> OK

5afce63  test(02-06): record book-spread and dewarp goldens from macos-release
  non-test diff vs cde6825: src/CMakeLists.txt  -> OK

2f6df32  test(02-05): record flat-document goldens from macos-release
  non-test diff vs cde6825: src/CMakeLists.txt  -> OK

```

All golden commits: only `src/CMakeLists.txt` differs, as in (a).

(d) The baseline commit and the history after `cde6825`:

```
$ git log --format=%s cde6825..HEAD | grep -cE '\(0[3-6]-'   # subjects of Phase 3 to 6 plans
0
exit status: 0
```

```
$ git log --oneline cde6825..HEAD | wc -l
      79
exit status: 0
```


Baseline commit: a796c2ae

`a796c2ae` (full hash `a796c2ae16a0a9922b0ff7c54589afd031eb53fb`) is `test(02-07): record mixed-page goldens from macos-release`, the newest commit that touches `src/pipeline_tests/golden`; its non-test source tree equals `cde6825` apart from the three `add_subdirectory` lines (proof (c)). No strip or cleanup commit exists yet: no commit since `cde6825` has the subject of a Phase 3 to 6 plan (count above: 0), so every Phase 3 commit will follow the baseline commit.

What the baseline commit does and does not fix: it fixes the goldens and the production code they were recorded from. The comparator limits (`src/test_support/Tolerances.h`, plan 02-09) and the checks of the later tests (`TestComparatorSelfTest.cpp`, `src/qt_tests`) were committed after it; the test code that Phase 3 runs is the Phase 2 tree at the commit named by `git log -1 --format=%h -- src/pipeline_tests src/test_support src/dewarping/tests src/imageproc/tests src/qt_tests`, and no commit after the baseline touches `src/pipeline_tests/golden`. The goldens do not move without `UPDATE_GOLDENS=1`.

## Goldens

| Suite | Files | Bytes | `du -sk` (KB on disk) |
|---|---|---|---|
| `golden/flat/` | 10 | 2393401 | 2364 |
| `golden/book/` | 16 | 5669493 | 5580 |
| `golden/mixed/` | 12 | 4385102 | 4320 |
| **Total** | **38** | **12447996** (12.4 MB) | **12264** |

Research assumption A5 expected about 15 MB; the goldens are 12.4 MB. Contents: bi-level (1 bit) PNGs for black-and-white output, 8-bit RGB PNGs for grey and colour output, `.txt` goldens for geometry, and per suite a `MANIFEST.txt` (toolchain) and `fixtures.txt` (SHA-256 of the generated inputs).

## Success criteria

Roadmap Phase 2 criteria 1 to 5:

1. **PASS** - The generator is deterministic and uses neither font rendering nor `rand()`: `GeneratorDeterminismTestSuite/test_same_seed_same_pixels` and `test_different_seed_different_pixels` pass, and the fixture fingerprints of flat, spread and mixed pages are in `golden/*/fixtures.txt` and checked by `test_flat_page_unchanged`, `test_book_fixture_fingerprints` and `test_mixed_fixture_fingerprints`. Record: `records/ctest-phase2.md`, `src/pipeline_tests/TestGeneratorDeterminism.cpp`. The grep below finds no `rand(`, `qrand`, `QFont` or `drawText` in `src/test_support` or `src/pipeline_tests`.

```
$ grep -rnE 'rand\(|qrand|QFont|drawText' src/test_support src/pipeline_tests (*.cpp, *.h) | wc -l
       0
exit status: 0
```

2. **PASS** - Deskew angle, content box and binarization on flat documents (`FlatDocumentTestSuite`, `records/scenario-flat.md`), split position and dewarped line straightness on book pages (`BookSpreadTestSuite`, `DewarpPageTestSuite`, `records/scenario-book.md`), picture zones, colour/grey output and despeckling on mixed pages (`MixedPageTestSuite`, `records/scenario-mixed.md`); the revived `TestBinarize` (nine binarizers) and the new `dewarping_tests` (10 cases) run in the same `ctest` (`records/unit-tests.md`, `records/ctest-phase2.md`).
3. **PASS** - A deliberately altered output fails under the per-kind tolerance and leaves `expected.png`, `actual.png` and `diff.png`: `ComparatorSelfTestSuite` (nine cases, `records/tolerance-spread.md`, `records/ctest-phase2.md`). Baselines change only with `UPDATE_GOLDENS=1` (the self-test with the flag set leaves `git status` of the golden directory empty), and each suite directory has a `MANIFEST.txt` with the toolchain versions. Note from plan 02-09: the self-test does not distinguish the frozen limits from the D-12 ceilings; see `02-FINDINGS.md`.
4. **PASS** - The baseline commit `a796c2ae` precedes every strip or cleanup commit, and its non-test source tree matches the Phase 1 code (section 'TEST-04 proof', both proof commands exit 0).
5. **PASS, with one note** - `ProjectRoundTripTestSuite` writes a project with non-default values in every stage and special characters in names, reads it back and writes it again with identical results (`records/round-trip.md`); `ctest --preset macos-release` and `ctest --preset macos-debug` each pass 7 of 7 offscreen with isolated settings (`records/ctest-phase2.md`). Known defects that the unmodified code cannot pass are marked `XFAIL_CHECK` (five findings, `02-FINDINGS.md`). Note: the criterion asks the mark to name the requirement that will fix the defect; none of the roadmap's requirements covers these defects, so each tag says `no requirement` and the defect has a backlog item (999.1 to 999.7) instead.

## Summary for the user

The suite generates synthetic pages with known ground truth (flat documents, book spreads with a curved page, mixed text-and-image pages; 300 and 600 dpi; black-and-white, grey and colour output) and checks the six-stage pipeline on them, plus new dewarping unit tests and a revived binarization test; 325 test cases in 7 test programs.
`ctest --preset macos-release` (about 56 s) and `ctest --preset macos-debug` (about 230 s) each pass 7 of 7, offscreen, with settings and temporary files under `build/`.
38 golden files, 12.4 MB, are in `src/pipeline_tests/golden/` (flat, book, mixed) with a toolchain manifest per suite; they were recorded from production code identical to `cde6825` apart from three CMake lines, and the baseline commit is `a796c2ae`, which every Phase 3 commit follows.
Five checks fail on the unmodified code and are marked as expected failures (`XFAIL_CHECK`); none has a fixing requirement, and each has a backlog item (999.1 to 999.7, `02-FINDINGS.md`):
- F-04-1 (999.1): "global thresholds reject Format_Grayscale8 (std::runtime_error): no requirement"
- F-04-2 (999.2): "model top curve skips the first text line of the curved page (seed 1): no requirement"
- F-05-1 (999.3): "output of the 600 dpi flat page keeps about 0.1 deg residual skew, binarization mismatch 8.3% against 0.5% (F-05-1): no requirement"
- F-06-1 (999.4): "page_split cutter <30.4|55.7> px from the gutter centre at mid-height on the <300|600> dpi spread, bound 2 px (F-06-1): no requirement"
- F-07-1 (999.6): "page_split AUTO cuts the mixed page and keeps only a fragment of its picture, dropping the text: no requirement"
Two more findings carry no mark: F-06-2 (999.5, a limit of the test support) and F-08-1 (999.7, a default-profile layout type missing from a project saved before page_split ran).
The ASan/UBSan run (`records/sanitizer-findings.md`) passes 7 of 7 and reports 12 UBSan and 0 ASan findings at 9 project locations: 2 known from Phase 1, 7 new (Despeckle, InfluenceMap, EstimateBackground); recorded, not fixed.
To re-record a golden after an intended change, run `UPDATE_GOLDENS=1 ctest --preset macos-release`, then commit the changed goldens on their own with the reason in the message.
