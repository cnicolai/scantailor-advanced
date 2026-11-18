# Full strict sanitizer run on the final tree of plan 06.1-11

tested-commit: dec6a64c3b46a764fa1fe303dec419bdfe38f735
input-key: 644c473c208a3fff65f273a63a627b5135394f44

The key is computed like `input_key` in `.planning/phases/03-qt-6-and-macos-only/gate.sh`: `git ls-tree -r <commit> -- src CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in | git hash-object --stdin`. The tested commit is the commit that added the strict preset; the commits after it in this plan change only `.planning/`.

## Method

- Before the run: `git status --porcelain -- src CMakeLists.txt CMakePresets.json cmake .claude/CLAUDE.md sanitizer` printed nothing, and the sanitizer tree was reconfigured and rebuilt with `cmake --preset macos-asan && cmake --build --preset macos-asan` (full rebuild after the CMake change).
- Command: `ctest --preset macos-asan > build/scratch/asan-strict-t11.txt 2>&1` (first attempt, exit 0).
- Test preset environment of `macos-asan`: `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1`, `UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1`. No suppression file exists, so none is read.
- Test preset environment of `macos-asan-collect` (same build, not used in this run): `ASAN_OPTIONS=detect_leaks=0:halt_on_error=0`, `UBSAN_OPTIONS=print_stacktrace=1`.

## Result

`100% tests passed out of 7`, `Total Test time (real) = 725.67 sec`.

| Test | Result | Time |
|------|--------|------|
| foundation_tests | Passed | 0.06 s |
| math_tests | Passed | 0.05 s |
| imageproc_tests | Passed | 18.97 s |
| core_tests | Passed | 0.78 s |
| qt_tests | Passed | 15.64 s |
| pipeline_tests | Passed | 682.88 s |
| dewarping_tests | Passed | 7.29 s |

The log holds no `runtime error:` line (`grep -c 'runtime error:'` prints 0) and no `ERROR: AddressSanitizer` line (`grep -c 'ERROR: AddressSanitizer'` prints 0). With `halt_on_error=1` a report would have aborted its test, so the green run also means that no report was printed in a passing test.

## For the closing plan

06.1-12 may skip a second strict run when HEAD has the same input key as above and the same content of `.claude/CLAUDE.md` and `sanitizer/` (the directory does not exist) as the tested commit: `git diff --quiet dec6a64c3b46a764fa1fe303dec419bdfe38f735 HEAD -- src CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in .claude/CLAUDE.md sanitizer` succeeds.
