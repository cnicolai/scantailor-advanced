# Closing check of Phase 1 (plan 01-07)

Checked on the final tree from fresh CMake configurations of all three presets and a clean rebuild of `macos-release`. Nothing was fixed in this plan; a failing criterion would carry a FAIL verdict. Paths are normalised to `{repo}`, `~` and `{scratch}`.

## Commit checked and toolchain

```
$ git rev-parse HEAD
cde682504f6a76179bdd3b5a5138df60d8cb0780
short hash: cde6825
$ cmake --version | head -1
cmake version 4.4.4
$ clang --version | head -1
Apple clang version 21.0.0 (clang-2100.3.34.2)
$ brew list --versions qtbase boost jpeg-turbo libpng libtiff
boost 1.92.0
jpeg-turbo 3.2.0
libpng 1.6.59
libtiff 4.7.2
qtbase 6.12.0_1
```

Comparison with `records/toolchain.txt` (`## After install`): cmake, clang, boost, jpeg-turbo, libpng and libtiff are the same versions. One difference: `qtbase` was recorded as `6.12.0` and is now `6.12.0_1` (a Homebrew revision of the same upstream release; the Qt frameworks in the `otool -L` output below still report 6.12.0). Recorded, not acted on.

Source state: `src/` is byte-identical to `5eaac18` (criterion 5 below). The last commit that touches `src`, `CMakeLists.txt` or `CMakePresets.json` is `11bc573`; every later commit touches only `.planning/` and `.claude/`.

Fresh trees: `cmake --preset <name> --fresh` for `macos-release`, `macos-debug` and `macos-asan` (each `exit status: 0`, `Build files have been written`), then `cmake --build --preset macos-release --clean-first`.

## Criterion 1

Roadmap wording: the `macos-release` preset configures and builds an arm64 binary against Homebrew Qt 6, Boost, jpeg-turbo, libpng and libtiff, with `-Wall -Wextra` active, and exactly one libjpeg, libpng and libtiff (BUILD-01, BUILD-02, BUILD-05).

Build:

```
$ cmake --preset macos-release --fresh
-- Configuring done (2.1s)
-- Generating done (0.3s)
exit status: 0
$ cmake --build --preset macos-release --clean-first
[456/456] Linking CXX executable scantailor-advanced
exit status: 0
```

Searching the build output for `error:` and `FAILED` finds 0 lines of each. The only linker message is `ld: warning: ignoring duplicate libraries` (as in `binary-check.txt` and `fix-log.md`).

Architecture:

```
$ file build/macos-release/scantailor-advanced
build/macos-release/scantailor-advanced: Mach-O 64-bit executable arm64
$ lipo -archs build/macos-release/scantailor-advanced
arm64
```

Image libraries in the load commands (`grep -c` on `otool -L`, one count per library):

```
$ otool -L build/macos-release/scantailor-advanced | grep -c libjpeg
libjpeg 1
$ otool -L build/macos-release/scantailor-advanced | grep -c libpng
libpng 1
$ otool -L build/macos-release/scantailor-advanced | grep -c libtiff
libtiff 1
$ otool -L build/macos-release/scantailor-advanced | grep -E 'libjpeg|libpng|libtiff'
	/opt/homebrew/opt/libtiff/lib/libtiff.6.dylib (compatibility version 10.0.0, current version 10.0.0)
	/opt/homebrew/opt/libpng/lib/libpng16.16.dylib (compatibility version 76.0.0, current version 76.0.0)
	/opt/homebrew/opt/jpeg-turbo/lib/libjpeg.8.dylib (compatibility version 8.0.0, current version 8.3.2)
```

Image libraries loaded at runtime (`DYLD_PRINT_LIBRARIES=1 build/macos-release/core_tests --list_content`, exit status 0, counted with `grep -c /<name>`; transitive loads included):

```
libjpeg 1
libpng 1
libtiff 1
dyld[...]: <4664EC98-4EDB-3DBC-BDA1-88CD43926F22> /opt/homebrew/Cellar/libtiff/4.7.2/lib/libtiff.6.dylib
dyld[...]: <7477C063-3048-3006-B32F-6A0C0451FCE2> /opt/homebrew/Cellar/libpng/1.6.59/lib/libpng16.16.dylib
dyld[...]: <8A556CB1-F63C-35A2-9417-3657A66AB758> /opt/homebrew/Cellar/jpeg-turbo/3.2.0/lib/libjpeg.8.3.2.dylib
```

Warning flags on the release tree (`jq` over `build/macos-release/compile_commands.json`):

```
entries: 408
-Wall missing: 0
-Wextra missing: 0
```

Warnings seen in this build (recorded, not fixed, D-08): 1125 `-Wunused-parameter`, 7 `-Winconsistent-missing-override`, 6 `-Wdeprecated-pragma`, 1 `-Wreorder-ctor`, 1 `-Wdeprecated-declarations`.

Comparison with `records/binary-check.txt` (first build, commit `1985dcc`) and `records/fix-log.md` (`## Warnings recorded, not fixed`): same architecture result, same three load-command lines and library versions, the same three dyld images with the same UUIDs, and the same warning counts. No difference.

verdict: PASS (arm64; libjpeg 1, libpng 1, libtiff 1 in the load commands and at runtime; `-Wall` and `-Wextra` on 408 of 408 release compile commands)

## Criterion 3

Roadmap wording: `ctest` runs all five test programs on the Mac and the initial pass/fail result is recorded (BUILD-04).

```
$ ctest --preset macos-release
1/5 Test #1: foundation_tests .................   Passed    0.35 sec
2/5 Test #2: math_tests .......................   Passed    0.35 sec
3/5 Test #3: imageproc_tests ..................***Failed    0.51 sec
4/5 Test #4: core_tests .......................   Passed    0.03 sec
5/5 Test #5: qt_tests .........................   Passed    0.54 sec

80% tests passed, 1 tests failed out of 5

The following tests FAILED:
	  3 - imageproc_tests (Failed)
ctest exit: 8
```

The failing assertions in `imageproc_tests`:

```
{repo}/src/imageproc/tests/TestScale.cpp:68: error: in "ScaleTestSuite/test_random_image": check checkScale(img, QSize(50, 50)) has failed
{repo}/src/imageproc/tests/TestScale.cpp:70: error: in "ScaleTestSuite/test_random_image": check checkScale(img, QSize(80, 80)) has failed
*** 2 failures are detected in the test module "Master Test Suite"
```

Case counts from running each executable directly (`--report_level=short`; `qt_tests` prints Qt Test totals):

| Test | Cases passed / total | Assertions |
|------|----------------------|------------|
| core_tests | 55 / 55 | 379 of 379 |
| foundation_tests | 22 / 22 | 51 of 51 |
| math_tests | 32 / 32 | 3104 of 3104 |
| imageproc_tests | 93 / 94 | 192 of 194 |
| qt_tests | 8 + 7 passed, 0 failed | - |

The first build's per-test results are tracked in git:

```
$ git ls-files .planning/phases/01-deletion-free-macos-build/records/ctest-release.txt .planning/phases/01-deletion-free-macos-build/records/ctest-debug.txt .planning/phases/01-deletion-free-macos-build/records/ctest-summary.md
.planning/phases/01-deletion-free-macos-build/records/ctest-debug.txt
.planning/phases/01-deletion-free-macos-build/records/ctest-release.txt
.planning/phases/01-deletion-free-macos-build/records/ctest-summary.md
```

Tests whose result differs from the first build (`records/ctest-summary.md`, Release column): No differences. Every ctest status is the same (4 passed, `imageproc_tests` failed with exit status 201 in the first build; this run reports `Failed` for the same test), the case counts are the same (55, 22, 32, 93 of 94, 8 + 7), and the failing case is the same `ScaleTestSuite/test_random_image` at `TestScale.cpp:68` and `:70`. The failure is the known baseline from plans 01-03 and 01-04 (Release, Debug and ASan builds); it is recorded, not fixed.

verdict: PASS (ctest runs all five tests on the final tree, the initial result is recorded and tracked in git, no result differs from the first build; the one failing test is a recorded baseline, not a gap in the criterion)

## Criterion 4

Roadmap wording: `-ffp-contract` is fixed in the build and recorded, and a CMake option builds with ASan and UBSan and the initial findings are recorded (BUILD-06, BUILD-07).

`-ffp-contract=off`, `-Wall` and `-Wextra` in every compile command of all three trees, after the fresh configures (`jq`, entries and entries lacking any of the three):

```
macos-release entries=408 missing=0
macos-debug entries=408 missing=0
macos-asan entries=408 missing=0
```

Sanitizer flags per tree (entries containing `-fsanitize=`): `macos-release` 0, `macos-debug` 0, `macos-asan` 408. The sanitizer switch is off unless the preset sets it.

Setting line of `records/fp-contract.md`:

```
3:**Setting:** -ffp-contract=off for C and C++
```

The sanitizer switch in `CMakeLists.txt`:

```
$ grep -n 'set(SANITIZE ""' CMakeLists.txt
72:set(SANITIZE "" CACHE STRING "Value for -fsanitize=, e.g. address,undefined. Empty disables.")
```

`## Totals` of `records/sanitizer-findings.md` (from the ASan + UBSan run of plan 01-04):

```
- Total ASan reports: 0
- Total UBSan reports: 3
- Distinct project locations: 2
- Per executable (UBSan / ASan):
  - `core_tests`: 1 / 0
  - `foundation_tests`: 0 / 0
  - `math_tests`: 0 / 0
  - `imageproc_tests`: 2 / 0
  - `qt_tests (default)`: 0 / 0
  - `qt_tests (offscreen)`: 0 / 0
```

The two project locations are `src/imageproc/BinaryImage.cpp:1044:37` (member access within null pointer) and `src/imageproc/SlicedHistogram.cpp:49:47` (left shift of negative value), matching the baseline facts of plan 01-04. The sanitizer run itself was not repeated here (the `macos-asan` tree was configured fresh, not rebuilt, as the plan specifies); the findings are the tracked record.

```
$ git ls-files .planning/phases/01-deletion-free-macos-build/records/sanitizer-findings.md
.planning/phases/01-deletion-free-macos-build/records/sanitizer-findings.md
```

verdict: PASS (`-ffp-contract=off` on 408 of 408 commands in each of the three presets and recorded in `fp-contract.md`; `SANITIZE` option present and the ASan/UBSan findings recorded and tracked)
