# Closing check of Phase 3 (plan 03-09)

Checked on the tree below. This plan changes only `.planning/` and `.claude/`; nothing was fixed here, and a failing criterion would carry a FAIL line. Output is normalised: `{repo}` for the repository path, `~` for the home directory. The evidence is in the records of this directory and in the commits named; labels (c00 to c11) and hashes come from `records/gate-log.md`.

## Commit checked and toolchain

```
$ git log -1 --format=%H -- . ':(exclude).planning' ':(exclude).claude'
cf4da61c76195f4f23740f2a726b235e80186848
exit status: 0
```

`cf4da61c` is c11-docs, the last strip commit. Every commit after it in this phase touches only `.planning/` or `.claude/` (the docs commit of this plan `e1451ad1` changed no build input: `git diff --quiet` over `src CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in` exits 0).

```
$ git rev-parse HEAD     (before the commit of this record)
e1451ad15237277947d42c95bb0f596827208d44
exit status: 0
```

```
$ brew list --versions cmake ninja boost qtbase qtsvg jpeg-turbo libpng libtiff
boost 1.92.0
cmake 4.4.4
jpeg-turbo 3.2.0
libpng 1.6.59
libtiff 4.7.2
ninja 1.13.2
qtbase 6.12.0_1
qtsvg 6.12.0_1
exit status: 0
```

```
$ cmake --version | head -1
cmake version 4.4.4
$ xcrun clang --version | head -1
Apple clang version 21.0.0 (clang-2100.3.34.2)
$ sw_vers -productVersion; xcodebuild -version | head -2
26.7
Xcode 27.1
Build version 27A9275
```

Manifest of the goldens (`src/pipeline_tests/golden/flat/MANIFEST.txt`; `book` and `mixed` differ from it only in `source_commit`, checked with `diff`):

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

Comparison: every version equals the manifest (qtbase `6.12.0_1` is the Homebrew revision of Qt 6.12.0). There is no difference to record. The toolchain did not move during the phase: the packages of `records/before-state.md` (measured on c00) have the same versions as above.

Correction of a 03-01 record: `records/before-state.md` says that `brew list --versions` printed no `cmake` line. Plan 03-07 measured `cmake 4.4.4` as a Homebrew formula (`/opt/homebrew/bin/cmake`), and the output above lists it too, with the other seven packages identical to before-state. The 03-01 query evidently omitted `cmake`. `before-state.md` is left as measured; this paragraph is the correction.

## Success criteria

ROADMAP Phase 3, criteria 1 to 5. The evidence is the output recorded in `records/leftover-grep.md` (plan 03-08, run on `cf4da61c`), which this section does not repeat.

1. PASS - `grep -rnI -e Qt5 -e QT_VERSION CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in src` prints nothing and exits 1 (`records/leftover-grep.md` check 1; 54 lines before, `records/before-state.md`). The ctest summary is `100% tests passed out of 7` for every strip commit in Release and Debug, and the goldens equal `a796c2ae` (checks 9a, 9b and 10, `records/gate-log.md`: twelve rows, all with an unchanged-goldens verdict).
2. PASS - The platform-macro, option and word search is empty (check 2; 60 lines before), the packaging files are absent (check 3a) and no `install(`, CPack or uninstall text remains (check 3b). A fresh clone configured, built from nothing (510 steps) and ran `ctest --preset macos-release` to `100% tests passed out of 7` with the README commands only (`records/fresh-clone.md`, verdict PASS; check 4).
3. PASS - `.github` does not exist and `git ls-files .github` prints nothing (check 5a, 5b); commit c04 `462f22cd` deleted the five workflows and `dependabot.yml`, so a push starts no CI run.
4. PASS - `src/translations` is absent (check 6a), no translation target, translator code or Language control is left (checks 6b, 6c) and the fresh build directory has no `.qm` file and no translation rule (check 6e). The offscreen screenshots show English text only and no gap where the Language row was (`records/ui-screenshots.md`, verdict PASS; check 6d). 249 `tr()`/`translate()` calls and 573 `.ui` strings stay (D-06; checks 7a, 7b).
5. PASS - The README holds the configure, build and test commands, and the fresh clone ran exactly those lines and passed; no step needed knowledge the README does not give (`records/fresh-clone.md`, section "README gaps": none).

## Requirements

STRIP-01 to STRIP-06 and STRIP-08. STRIP-07 (OpenGL viewer, Phase 6) is outside this phase. The gate-log columns for every commit below read: Release and Debug `100% tests passed out of 7`, goldens unchanged.

| Requirement | Commits (label, hash) | Records |
|-------------|-----------------------|---------|
| STRIP-01 Qt 6 only | c01-qt5-cpp `9a68f985`, c02-qt6-cmake `c63a74c2`, c03-qt5-main-imageview `81a27cb1` | `leftover-grep.md` check 1; `gate-log.md` rows c01 to c03 (build-command diff only the D-15 define for c01, empty for c02 and c03) |
| STRIP-02 Windows | c06-windows-cpp `ad15b3ef`, c07-windows-cmake `85e095d4` | `leftover-grep.md` checks 2, 3a, 11d (the kept normaliser); `gate-log.md` rows c06, c07 |
| STRIP-03 Linux | c08-linux `450ff19f` (also the portable-mode removal c09-portable `1b1bec27`, D-01) | `leftover-grep.md` checks 2 and 3a; `gate-log.md` rows c08, c09 |
| STRIP-04 Packaging | c05-packaging `017e977c` (scripts and `post_issue_followups.py` with c04, ED-05) | `leftover-grep.md` checks 3a, 3b, 8; `gate-log.md` row c05 |
| STRIP-05 GitHub automation | c04-ci `462f22cd` | `leftover-grep.md` checks 5a, 5b; `gate-log.md` row c04 |
| STRIP-06 Translations | c10-translations `56b5a540` | `ui-screenshots.md`; `leftover-grep.md` checks 6a to 6e, 7a, 7b; `gate-log.md` row c10 |
| STRIP-08 README | c11-docs `cf4da61c` | `fresh-clone.md`; `leftover-grep.md` check 4; `gate-log.md` row c11 |

The first strip-phase commit c00-ui-snapshot `8d0105ab` is the offscreen harness behind `ui-screenshots.md`; it removes nothing. STRIP-01 to STRIP-06 and STRIP-08 were held back from `requirements.mark-complete` by every earlier plan because this plan declares them too; they are marked complete after this plan's SUMMARY exists.

## Decisions

| Decision | Implemented or checked |
|----------|------------------------|
| D-01 everything that never runs on the Mac goes | c04 to c10: scripts and `.github/` (c04), install rules, uninstall target, Windows runtime helpers (c05), MSVC, MinGW, `WIN_XP`, `DEPENDENCIES_ROOT` (c07), GCC flag block, `src/resources/unix`, the `.png`, non-Mac sides of `Q_OS_MAC`/`Q_OS_UNIX` (c08), `PORTABLE_VERSION` and its callers (c09), `TRANSLATION_DIRS`/`PLUGIN_DIRS` (c07, c10); `leftover-grep.md` checks 2, 3a, 3b; the Clang block is byte-identical to `a796c2ae` (check 11a) |
| D-02 platform macros resolved to the macOS side | c01, c03, c06, c08 (unifdef, each diff read); no behaviour change shown by goldens unchanged and empty command diffs |
| D-03 Windows-minded code that runs on the Mac stays | `RelinkablePath::normalize` unchanged outside its `_WIN32` lines (`leftover-grep.md` check 11d); backlog item 999.8 (commit `0312ab01`) |
| D-04 batch alarm keeps its Unix `play` command until Phase 5 | `leftover-grep.md` check 11c; only the `#else` side went (c08) |
| D-05 translations removed together | c10 `56b5a540`; checks 6a to 6c |
| D-06 `tr()` and `.ui` strings stay | checks 7a (249) and 7b (573); `ui-screenshots.md` |
| D-07 the user's `language` setting left alone, nothing writes to settings | no code reads `language` (check 6c); the harness and the tests keep their settings in `build/` (`ui-screenshots.md`, `TESTING.md`) |
| D-08 README new top, Features kept | c11 `cf4da61c`; Features changed only by the ED-07 exception below |
| D-09 `TESTING.md` rewritten in English | c11 `cf4da61c` (seven suites, presets, goldens, asan preset, formatting) |
| D-10 agent docs up to date as the last step | `e1451ad1` (seven maps and the GSD sections of `.claude/CLAUDE.md`); `build/scratch/d10-before.txt` (95 lines) and `d10-after.txt` (40 lines); one exception below |
| D-11 every strip commit gated | `gate-log.md` (twelve rows c00 to c11); `leftover-grep.md` checks 9a, 9b, 10 |
| D-12 automated UI verification | c00 harness `8d0105ab`; `ui-screenshots.md`, `before-state.md`, `records/screenshots/` |
| D-13 fresh-clone proof | `fresh-clone.md` (clone in `build/fresh-clone`, no `brew install`) |
| D-14 one `macos-asan` run compared with Phase 2 | `sanitizer-compare.md`: 0 ASan, 12 UBSan, 9 project locations, 0 new |
| D-15 no `QT_VERSION` token, manifest string unchanged | c01 `9a68f985` (`ST_QT_COMPILE_VERSION`, ED-02); `leftover-grep.md` check 1; no golden `MANIFEST.txt` changed (checks 9a, 9b) |

## Edge probe and prohibitions

The four resolved edges:

- STRIP-03 concurrency: commit c08 `450ff19f` lists `CMakeLists.txt`, `src/resources/unix/scantailor-advanced.desktop.in` and `src/resources/unix/mime/scantailor-project.xml` together (`git show --name-only`), so no commit configures with a missing resource (Pitfall 14).
- STRIP-05 adjacency: before c04 `git ls-files .github` listed exactly six files (five workflows and `dependabot.yml`); c04 changed seven files in total, the seventh being `scripts/post_issue_followups.py`, assigned to c04 by ED-05. Nothing outside STRIP-05 was in `.github/`.
- STRIP-05 empty: after c04 `.github` does not exist and `git ls-files .github` prints nothing (`leftover-grep.md` checks 5a, 5b).
- STRIP-05 ordering: `git merge-base --is-ancestor a796c2ae` exits 0 for c04 `462f22cd` and c05 `017e977c`, and for the oldest commit of the phase (`leftover-grep.md` check 10).

The five flagged readings, as applied:

- STRIP-01: the criterion-1 search over CMake files and `src/` (test code included) is empty, `find_package` names only Qt 6 with the floor 6.5 and no fallback, and "baseline images unchanged" is the gate's golden check on every commit.
- STRIP-02: every `_WIN32`, `Q_OS_WIN`, `WIN32`, `MSVC`, `MINGW`, `WIN_XP` and `DEPENDENCIES_ROOT` site, the Windows helpers, `EXTRA_LIBS` and `src/resources/win32/` went; Windows-minded code that runs on the Mac stayed (D-03).
- STRIP-04: no Flatpak manifest, `build-*.sh`, CPack or NSIS variable, `install()` rule, uninstall target or Windows deployment step is left; `.gitignore` keeps its packaging patterns (ED-12).
- STRIP-06: no `.ts` or `.qm` file exists or is built, nothing installs a translator, no Language control exists, and the source strings stay.
- STRIP-08: the D-13 proof, a fresh clone running only README lines, with the Homebrew list confirmed by `brew list --versions`.

Prohibitions of plans 03-01 to 03-09 and the check behind each:

| Plan | Prohibition | Check |
|------|-------------|-------|
| 03-01 | No access to settings or profiles, nothing outside the project | scratch output in `build/scratch`; the harness keeps its own settings file below `build/` (`ui-screenshots.md`) |
| 03-01 to 03-06 | No golden changed, `UPDATE_GOLDENS` never set | the gate refuses when the variable is set (ED-11); `leftover-grep.md` checks 9a, 9b |
| 03-02 | No hand-resolved conditional, `TextLineTracer.cpp` not reformatted | only c01 touched `src/dewarping/TextLineTracer.cpp` (unifdef), and `xcrun clang-format --dry-run` still prints exactly the two warnings of `before-state.md`; unifdef counts in the commit messages |
| 03-03 | Nothing the Mac build reads removed early | empty command diffs for c04 and c05 (`gate-log.md`) |
| 03-04 | No behaviour change in code that runs on the Mac | empty command diffs, goldens unchanged, `leftover-grep.md` check 11d |
| 03-05 | Alarm command unchanged; settings and profiles untouched | check 11c; the plan summary reports that no real settings or profiles were read |
| 03-06 | No `tr()` or source string edit; no manual step for the user | checks 7a, 7b; `ui-screenshots.md` is the executor's own look |
| 03-07 | No `brew install` or upgrade; clone and scratch inside the project | `fresh-clone.md` (Homebrew section, clone path `build/fresh-clone`) |
| 03-08 | No sanitizer report fixed, no source edit, nothing outside the project | `sanitizer-compare.md`; `git diff --quiet` of the five report files against `a796c2ae` |
| 03-09 | No touch of the `GSD:profile` section, no remap, no stamp, no ask of the user | profile section diffed byte for byte with its parent (identical); `git diff` of the map frontmatter shows no `last_mapped_*` change; the summary below asks nothing |

## Exceptions and deviations

Entries of `records/execution-decisions.md` that depart from a CONTEXT wording:

- ED-02: the D-15 define is `ST_QT_COMPILE_VERSION`, not a name containing the Qt version macro, so the criterion-1 search stays empty.
- ED-03: Qt 5 removal takes three commits (c01 to c03) in the Pitfall 7 order instead of one Qt 5 commit.
- ED-04: Windows removal takes two commits (c06 C++, c07 CMake and files).
- ED-07: the README Features section is kept except for the two portable-mode passages (the Portability bullet and the profiles line), which now name the macOS locations; decided under the user's standing instruction that Claude takes GSD decision gates.
- ED-08: the profiles path in the README is `~/Library/Application Support/scantailor-advanced/scantailor-advanced/profiles`, the path measured in Phase 1, not the shorter one written in CONTEXT D-01.

Other decisions taken by the orchestrator during planning and execution (the user delegates GSD decisions): the ED-07 and ED-08 choices above; the plan-checker warning on `type="tracer"` was overridden because that is the executor's defined tracer type; the replacement of the 03-07 `brew list` check, which turned out unnecessary (see 03-07 SUMMARY).

Deviations reported in the plan summaries: 03-01 to 03-06 and 03-08 report none; the environment notes in them (ugrep patterns with `${` or `\|`, the unrelated `.planning/config.json` change, a `SettingsDialog.ui` cut restored before commit) changed no scope. 03-07 reports one check adjustment that proved unnecessary (CMake in `brew list --versions`), covered by the correction in the first section.

Deviation of this plan: the D-10 search of Task 1 leaves one line without a `Phase 3` or `999.8` marker, `.claude/CLAUDE.md` line 298, `No project skills found. Add skills to any of: ... .github/skills/ ...`. It is boilerplate the generator writes into the `skills` section (the report of `generate-claude-md` lists `skills` as a fallback section) and names generic places where skills may be put, not the removed workflows. `.claude/CLAUDE.md` is edited only through the generator, and a hand edit would be overwritten by the next run, so the line stays. All other 39 lines of `build/scratch/d10-after.txt` carry the marker. A later check of D-10 should skip this line.

Two points that are outside this phase and unchanged: the alarm default `play /usr/share/sounds/freedesktop/stereo/bell.oga` (Phase 5, MAC-10) and the Windows-minded normaliser (999.8).

## Gate cost

Twelve strip commits were gated (c00 to c11), each once, with Release and Debug configure, build and ctest. The last one, c11-docs on `cf4da61c`, took `Total Test time (real) = 56.03 sec` for Release and `232.79 sec` for Debug (`macos-release-ctest.txt`, `macos-debug-ctest.txt` in `build/scratch/gate/c11-docs/`, not committed). The run with the sanitizers (`macos-asan`, D-14) was done once, after the last strip commit.

## Summary for the user

Phase 3 removed everything the Mac build does not need: Qt 5, the Windows and Linux code and files, Flatpak, Debian and NSIS packaging, the install rules, all GitHub workflows and Dependabot, the portable mode and the translations.
Qt 6 is now the only supported version, and the UI is English only (the Language row is gone from Settings and the text strings stay in the code).
Every one of the twelve strip commits passed Release and Debug 7 of 7 with the golden images unchanged since the Phase 2 baseline `a796c2ae`, so the output did not change.
The one sanitizer run after the strip found the same 12 UBSan reports at the same 9 places as in Phase 2 and no ASan report; nothing is new.
Settings are in `~/.config/scantailor-advanced/scantailor-advanced.ini`, and profiles are in `~/Library/Application Support/scantailor-advanced/scantailor-advanced/profiles`.
The README alone is enough to build the app and run the tests: a fresh clone passed with only its commands.
The codebase maps and the GSD part of `.claude/CLAUDE.md` now describe the stripped tree.
One new backlog item, 999.8, records that the path normaliser still treats a backslash in a macOS file name as a separator; it was kept on purpose and is not fixed yet.
There is no CI any more, so the local Release and Debug test run is the gate before a commit.
Phase 4 starts with a measurement of the device pixel ratio the build-directory binary reports on the Retina display, and then makes the page view, thumbnails and overlays sharp.
