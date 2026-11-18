# Fresh-clone proof of the README (D-13, plan 03-07 task 2)

Paths below are normalised: `<repo>` stands for the project directory, `<clone>` for `<repo>/build/fresh-clone`.

## Clone

- Commit: `8f578ff4a0deec9b251e314d7554ff0f4710ab31` (the commit after the gate record of c11-docs; the clone and the main tree report the same `HEAD`).
- Clone path: `build/fresh-clone` (gitignored, inside the project). Scratch output: `build/scratch/fresh-clone/`.
- Substitution: the README's `git clone https://github.com/cnicolai/scantailor-advanced.git` and `cd scantailor-advanced` lines were replaced by `git clone --quiet . build/fresh-clone`, a clone of the local repository, and a change into `build/fresh-clone`. This is the only substitution. Nothing was fetched from the network.
- The README in the clone contains the three command lines run below, each as a whole line (README lines 70, 71 and 105).

## Commands run

`build/scratch/fresh-clone/commands.txt` holds exactly the three `macos-release` lines of the README. `run.sh` ran each in `<clone>`; the exit status is the second column of `status.txt`.

| N | Command (whole README line) | Exit status | End of the output |
|---|-----------------------------|-------------|-------------------|
| 1 | `cmake --preset macos-release` | 0 | `-- Configuring done (1.4s)`, `-- Generating done (0.4s)`, `-- Build files have been written to: <clone>/build/macos-release` |
| 2 | `cmake --build --preset macos-release` | 0 | `[509/510] Linking CXX executable pipeline_tests` (510 steps, from nothing) |
| 3 | `ctest --preset macos-release` | 0 | `7/7 Test #7: dewarping_tests ... Passed 0.63 sec`, `100% tests passed out of 7`, `Total Test time (real) = 57.69 sec` |

ctest per test: foundation_tests, math_tests, imageproc_tests, core_tests, qt_tests, pipeline_tests (53.93 s) and dewarping_tests all `Passed`.

No command, environment variable or option outside the README was used.

## Homebrew packages

`brew install` was not run (D-13). `brew list --versions cmake ninja boost qtbase qtsvg jpeg-turbo libpng libtiff` (read-only) printed eight lines and exited 0:

```
boost 1.92.0
cmake 4.4.4
jpeg-turbo 3.2.0
libpng 1.6.59
libtiff 4.7.2
ninja 1.13.2
qtbase 6.12.0_1
qtsvg 6.12.0_1
```

Note on the orchestrator's decision about CMake: `records/before-state.md` (plan 03-01) recorded that `brew list --versions` printed no `cmake` line. When this plan ran, `brew list --formula --versions cmake` printed `cmake 4.4.4`, `which cmake` gave `/opt/homebrew/bin/cmake`, and `cmake --version` gave 4.4.4, which satisfies the 3.25 of `cmakeMinimumRequired` in `CMakePresets.json`. So all eight packages are Homebrew formulae here and the original eight-line check holds; the replacement check (seven packages plus `cmake --version`) that the orchestrator approved is met as well and was not needed. Nothing was installed or upgraded during the phase, so the 03-01 measurement was either taken with a different query or is out of date; this plan did not investigate which.

## Clean build directory

All three checks ran on `<clone>/build/macos-release`, a directory that did not exist before step 1:

| Check | Output |
|-------|--------|
| `find build/fresh-clone/build/macos-release -name '*.qm'` | nothing |
| `grep -ci -e '\.qm' -e lrelease -e lupdate -e translations build/fresh-clone/build/macos-release/build.ninja` | 0 |
| `nm -u build/fresh-clone/build/macos-release/scantailor-advanced \| grep -ci translator` | 0 |

## Build commands

`K=12603ed1e06e11db86d2d7f83cad210897788de2` (hash of `git ls-tree -r HEAD` over `src CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in`). The clone's command list for the eight targets plus `ui_snapshot` (`ninja -t commands`, `CMakeFiles/d/*.d` hash names replaced by `HASH`, `<clone>` replaced by `<repo>`, `LC_ALL=C sort`) has 509 lines, the same as `build/scratch/gate/ref/cmds-macos-release-$K.txt`. `build/scratch/fresh-clone/cmds.diff` is empty (0 lines): a fresh build directory schedules the same commands as the existing one, so no stale cache entry of the existing build directory hid a change of a strip commit.

## README gaps

None. The three README lines were enough; no fix commit and no `c12-readme-fix` gate were needed.

Verdict: PASS
