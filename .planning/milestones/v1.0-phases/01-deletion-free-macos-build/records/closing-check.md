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

## Criterion 2

Roadmap wording: the app launches from the build directory, opens a project, takes it through all six stages and writes output files (BUILD-03).

Verdict line of `records/SMOKE-FINDINGS.md`:

```
BUILD-03 verdict: PASS (after re-check of steps 4, 6, 8; first run FAIL, see below)
```

What it covers (plan 01-06, run on commit `015b7a4`, preset `macos-release`, unbundled binary started from Terminal with stderr kept): steps 1 to 11 of the walkthrough (launch, new project with three generated pages, batch run of all six stages, dewarping Auto on one half of the book page, Color / Grayscale on the flat page, Mixed mode with a picture zone, four output TIFFs opened in Preview, save, close and reopen, menus, settings, Command+Q) and the re-check R1 to R5 of steps 4, 6 and 8 after the first run ended in FAIL on two project-file expectations. After the re-check the saved project file parses, holds the colour modes `bw`, `colorOrGray` and `mixed`, dewarping `auto` and one picture zone, and survives close and reopen. No launch fix was needed and no `src/` file was edited. The first run's FAIL and its analysis stay in that record unchanged.

The smoke binary is not rebuilt for this check: the tree checked here (`cde6825`) has the same `src/`, `CMakeLists.txt` and `CMakePresets.json` as the smoke build's `015b7a4` (the commits between them touch only `.planning/`), and the release tree was rebuilt clean from it in criterion 1.

verdict: PASS (BUILD-03 PASS in `SMOKE-FINDINGS.md`; walkthrough steps 1 to 11 plus re-check R1 to R5)

## Criterion 5

Roadmap wording: `git diff 5eaac18` (the 1.2.1 release) outside `.planning/` shows only build and launch fixes and added build settings: no deleted code paths and no change to processing behaviour.

Pathspec: `. ':!.planning' ':!.claude'`. `.claude/` is excluded because `.claude/CLAUDE.md` is tracked and already differed from `5eaac18` before Phase 1 started (GSD project instructions, no code; research Pitfall 3, T-01-17). It is checked separately below.

```
$ git diff --stat 5eaac18 -- . ':!.planning' ':!.claude'
 CMakeLists.txt    | 24 ++++++++++++++-
 CMakePresets.json | 90 +++++++++++++++++++++++++++++++++++++++++++++++++++++++
 2 files changed, 113 insertions(+), 1 deletion(-)
$ git diff --name-status 5eaac18 -- . ':!.planning' ':!.claude'
M	CMakeLists.txt
A	CMakePresets.json
$ git diff --numstat 5eaac18 -- src CMakeLists.txt
23	1	CMakeLists.txt
```

Every listed path is `CMakeLists.txt` or `CMakePresets.json`. `src/` has no line in the numstat: no source file differs from `5eaac18`. The numstat has 23 added and 1 removed line in `CMakeLists.txt` and nothing else; the one removed line is line 1, `cmake_minimum_required(VERSION 3.9 FATAL_ERROR)`, replaced by `VERSION 3.25` (fix-log row 1). The full diff is in `records/src-diff.txt` (143 lines, contains the `VERSION 3.9` to `VERSION 3.25` hunk). Removed lines in the whole diff, listed by `grep -E '^-[^-]'`: exactly one, the line above.

Per-hunk list (`git diff -U0 5eaac18 -- . ':!.planning' ':!.claude' | grep '^@@'` gives four hunks):

| File | Hunk header | Class | Fix-log row |
|------|-------------|-------|-------------|
| CMakeLists.txt | `@@ -1 +1 @@` | modified (line 1 replaced by its 3.25 form) | 1 |
| CMakeLists.txt | `@@ -16,0 +17,4 @@` | added only (CMP0167 policy block) | 1 |
| CMakeLists.txt | `@@ -59,0 +64,18 @@` | added only (Clang warning and `-ffp-contract=off` block, `SANITIZE` block) | 3, 4, 5 |
| CMakePresets.json | `@@ -0,0 +1,90 @@` | added only (new file) | 2, 6 |

No hunk is `REMOVED ONLY`. No Qt 5, Windows or Linux branch is removed: no line of any `if (...)` branch, `find_package(Qt5 ...)` fallback, `WIN32` or `UNIX` block was deleted (the single removed line is the CMake minimum version).

`.claude/` checked separately:

```
$ git diff --stat 5eaac18 -- .claude
 .claude/CLAUDE.md | 332 ++++++++++++++++++++++++++++++++++++++++++++++++++++++
 1 file changed, 332 insertions(+)
$ git log --format='%h %s' 5eaac18..HEAD -- .claude
c03e873 docs: create roadmap (6 phases)
```

Only `.claude/CLAUDE.md` differs, and it was added by the roadmap commit before the first Phase 1 plan ran.

Commits since `5eaac18` that touch files outside `.planning/` and `.claude/`:

```
$ git log --format='%h %s' 5eaac18..HEAD -- . ':!.planning' ':!.claude'
11bc573 build: add debug and ASan presets
8670057 build(cmake): add SANITIZE option for ASan/UBSan builds
606bea0 build(cmake): freeze -ffp-contract=off for C and C++ (D-03)
e35b498 build(cmake): enable Clang warnings (D-08)
d7f2ca2 build: add macOS CMake presets
b2d5214 build(cmake): require CMake 3.25 and use BoostConfig (CMP0167)
```

Every commit listed has a row in `records/fix-log.md` (checked by script: `all-commits-logged`; the table with the commit-to-row mapping is in the fix-log's `## Closing cross-check (plan 01-07)`). No row had to be added in this plan.

Processing behaviour. `Affects processing?` values in `fix-log.md`:

| Row | Change | Affects processing? |
|-----|--------|---------------------|
| 1 | CMake minimum 3.25 and CMP0167 | No: `flags-compare.txt` shows no flag changed by this edit |
| 2 | `CMakePresets.json` (release) | No: no source file touched |
| 3 | Clang warnings `-Wall -Wextra -Werror=return-type -Wno-unused` | No: warnings only |
| 4 | `-ffp-contract=off` | Yes, by design (D-03); the user's decision, see below |
| 5 | `SANITIZE` option | No: off unless a preset sets it |
| 6 | debug and ASan presets | No: separate build directories |

Flag differences from `records/flags-compare.txt` (untouched configure against `build/macos-release`): five tokens added (`-Wall`, `-Wextra`, `-Werror=return-type`, `-Wno-unused`, `-ffp-contract=off`), none removed or changed; `-std=gnu++17`, `-O3`, `-DNDEBUG`, every `-D`, `-I` and `-isystem` token are identical, so the CMake 3.25 minimum caused no CMP0128 or include-path change. Four of the five are warning flags with no effect on generated code; `-ffp-contract=off` is the one token that changes generated code. It is the intended frozen setting of D-03, taken by the user in the Phase 1 discussion (`01-CONTEXT.md`), with a stated reversibility of "costly", and no earlier Mac output exists to compare against. Row 4 is therefore the user's decision rather than an unrequested change, and no row is a code edit. The disposition `approved by user` is not written in the row itself; the reference is D-03.

verdict: PASS (only `CMakeLists.txt` and `CMakePresets.json` differ outside `.planning/` and `.claude/`; four hunks, none remove-only, one replaced line (the CMake minimum); `src/` identical to `5eaac18`; all six commits have fix-log rows; the one generated-code change is the user-decided `-ffp-contract=off`)

## Summary of verdicts

| Criterion | Requirements | Verdict |
|-----------|--------------|---------|
| 1 build, arm64, warnings, one image library each | BUILD-01, BUILD-02, BUILD-05 | PASS |
| 2 launch and six stages end to end | BUILD-03 | PASS |
| 3 ctest on the final tree, first results tracked | BUILD-04 | PASS |
| 4 `-ffp-contract=off` and sanitizer option with findings | BUILD-06, BUILD-07 | PASS |
| 5 diff against `5eaac18` | (phase goal) | PASS |

Known baseline facts reproduced, not fixed: `imageproc_tests` fails `ScaleTestSuite/test_random_image` (`TestScale.cpp:68`, `:70`) in Release (this run), and per the earlier records in Debug and ASan; UBSan reports at `src/imageproc/BinaryImage.cpp:1044:37` and `src/imageproc/SlicedHistogram.cpp:49:47`. One difference from the earlier records: Homebrew `qtbase` is now `6.12.0_1` (was `6.12.0`); no test result or library check changed with it.

Phase 1 verdict: PASS

Phase 2 baseline commit: cde6825 (the commit checked; its `src/`, `CMakeLists.txt` and `CMakePresets.json` equal those of `11bc573`, the last commit that touches code or build files, and every later commit touches only `.planning/`, so Phase 2's goldens come from this source tree)
