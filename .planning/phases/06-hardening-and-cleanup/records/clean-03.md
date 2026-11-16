# CLEAN-03: the dead `#if 0` blocks (plan 06-01)

Decision D-13: every `#if 0` opener under `src/` goes, the dead branch goes with it, a live `#else` branch stays as plain code, and a one-line comment names an alternative that the dead branch documented. CMake has no `if(0)` blocks. Planning base `e5319cd9`; the edit is commit `dc138113` (`refactor(06-01): remove the dead #if 0 blocks`), gate label `s01-if0`. Output is normalised: `{repo}` for the repository path.

Before: `git grep -n -E '^[[:space:]]*#[[:space:]]*if[[:space:]]+0' e5319cd9 -- src` finds 8 lines in 7 files. After: the same search on HEAD finds none.

## The eight blocks

| # | File | Original line | Kind | What is left |
|---|------|---------------|------|--------------|
| 1 | `src/core/IncompleteThumbnail.cpp` | 25 | two-branch | the precomputed `m_sCachedPath` outline (the former `#else` body); one comment line names the dropped `addText()` alternative |
| 2 | `src/core/filters/output/DewarpingView.cpp` | 233 | dead-only | one comment line: the curve itself is not drawn because the grid shows it (the opener already carried that remark) |
| 3 | `src/core/filters/page_split/PageLayoutEstimator.cpp` | 660 | two-branch | the `contentBegin` / `contentEnd` loop (the former `#else` body); one comment line says what `gaps` holds |
| 4 | `src/core/filters/page_split/VertLineFinder.cpp` | 55 | two-branch | `GrayImage preprocessed(gray100);` with its existing explanatory comment (the dropped branch called `removeDarkVertBorders`) |
| 5 | `src/core/filters/page_split/VertLineFinder.cpp` | 70 | two-branch | the two `erodeGray` lines with their existing comment (the dropped branch computed real morphological gradients) |
| 6 | `src/imageproc/SavGolFilter.cpp` | 134 | two-branch | the separable two-pass convolution (the former `#else` body); its existing comment "Take advantage of Savitzky-Golay filter being separable." already names the alternative (the dropped branch was the direct 2-D convolution), so no line was added |
| 7 | `src/dewarping/Curve.cpp` | 142 | dead-only | nothing (an old variant of the direction test in `splineHasLoops`) |
| 8 | `src/dewarping/DistortionModelBuilder.cpp` | 481 | dead-only | nothing (an unused `calcReferenceHeight` helper with a TODO) |

No code is kept in comments. The five live `#else` branches (blocks 1, 3, 4, 5, 6) are unchanged apart from the indentation that `clang-format` applies.

## Proof that the compiled code did not change

`pp_equiv.py` (this directory) takes the Release compile command of a file from `build/macos-release/compile_commands.json`, drops the `-o`, `-c` and dependency options and the source path, appends `-E -P -x c++ - -iquote <dir of the file>`, runs it on `git show <base>:<file>` and on the working file (both on stdin, so `__FILE__` and quote-include lookup are identical) and compares the outputs without blank lines and without comments. Equal preprocessed text means equal object code.

Result, `python3 -I .planning/phases/06-hardening-and-cleanup/records/pp_equiv.py e5319cd9 <the seven files>`, exit 0:

```
EQUAL src/core/IncompleteThumbnail.cpp (85157 preprocessed lines)
EQUAL src/core/filters/output/DewarpingView.cpp (98480 preprocessed lines)
EQUAL src/core/filters/page_split/PageLayoutEstimator.cpp (99602 preprocessed lines)
EQUAL src/core/filters/page_split/VertLineFinder.cpp (84987 preprocessed lines)
EQUAL src/imageproc/SavGolFilter.cpp (60576 preprocessed lines)
EQUAL src/dewarping/Curve.cpp (57177 preprocessed lines)
EQUAL src/dewarping/DistortionModelBuilder.cpp (97346 preprocessed lines)
```

Positive control, run before the edit on `src/dewarping/Curve.cpp`:

```
$ python3 -I .planning/phases/06-hardening-and-cleanup/records/pp_equiv.py e5319cd9 src/dewarping/Curve.cpp
EQUAL src/dewarping/Curve.cpp (57177 preprocessed lines)                      (exit 0)

$ printf 'static int ppEquivControl = 1;\n' >> src/dewarping/Curve.cpp      (temporary line)
$ python3 -I .planning/phases/06-hardening-and-cleanup/records/pp_equiv.py e5319cd9 src/dewarping/Curve.cpp
DIFFERENT src/dewarping/Curve.cpp
  base: <end>
  work: static int ppEquivControl = 1;                                         (exit 1)
```

The temporary line was removed again (`git checkout -- src/dewarping/Curve.cpp`; `git diff --quiet` succeeded afterwards). So the script reports an equal result for an unchanged file and a different one for a one-line change.

## Gate

Gate `s01-if0` on commit `dc138113` (`build/scratch/gate/s01-if0/summary.txt`): Release and Debug configure, build and ctest 0, `100% tests passed out of 7` in both, `goldens: unchanged` against a796c2ae, `cmds-macos-release: diff_lines=0`, `cmds-macos-debug: diff_lines=0`, `result: PASS`. Edits only, so an empty command diff is the expected result and was met. The row is in `gate-log.md`.
