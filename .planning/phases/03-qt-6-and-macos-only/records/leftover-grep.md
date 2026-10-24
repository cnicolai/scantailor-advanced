# Final leftover sweeps and gate-log completeness (plan 03-08)

Tree: the last strip commit `cf4da61c` (c11-docs); later commits of this phase touch only `.planning/`. All checks are read-only and were run from the repository root with bash; each section gives the exact command, its output, its exit status and the expected result. Paths are normalised (`{repo}`, `~`). Baseline for the comparisons is `a796c2ae` (end of Phase 2).

Notes on the commands: check 3b uses `grep -F` so that `install(` is read literally (the plan 03-03 search with the pattern as a regular expression would be unbalanced under the ugrep of this environment); the set of patterns and files is the same. Check 6e quotes the two rows of `records/fresh-clone.md` that prove the clean directory has no `.qm` file and no translation rule.

### 1. Criterion 1: Qt 5 and version tests

```
grep -rnI -e Qt5 -e QT_VERSION CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in src
```

Output:

```
(no output)
```

Exit status: 1

Expected: no output, exit 1

### 2. Criterion 2, code: platform macros, options and words

```
grep -rnI -e _WIN32 -e Q_OS_ -e WIN32 -e MSVC -e MINGW -e __APPLE__ -e UNIX -e PORTABLE -e TRANSLATION_DIRS -e PLUGIN_DIRS -e LinguistTools -e lupdate -e lrelease -e QTranslator -e XDG_SESSION_TYPE -e SCANTAILOR_NO_XCB_FALLBACK -e Wayland -e wayland CMakeLists.txt CMakePresets.json cmake config.h.in src
```

Output:

```
(no output)
```

Exit status: 1

Expected: no output, exit 1 (60 lines at planning)

### 3a. Criterion 2, packaging files absent

```
test ! -e flatpak -a ! -e build-deb.sh -a ! -e build-windows.sh -a ! -e scripts -a ! -e src/resources/unix -a ! -e src/resources/win32 -a ! -e cmake/cmake_uninstall.cmake.in
```

Output:

```
(no output)
```

Exit status: 0

Expected: no output, exit 0

### 3b. Criterion 2, packaging search (plan 03-03; -F because of the parenthesis)

```
grep -rnIF -e 'install(' -e CPack -e CPACK -e uninstall -e InstallRequiredSystemLibraries -e CMAKE_INSTALL -e windeployqt -e QT_BINDIR CMakeLists.txt cmake src --include=CMakeLists.txt --include='*.cmake' --include='*.in'
```

Output:

```
(no output)
```

Exit status: 1

Expected: no output, exit 1

### 4. Criterion 2, clean directory: verdict line of fresh-clone.md

```
grep -x 'Verdict: PASS' .planning/phases/03-qt-6-and-macos-only/records/fresh-clone.md
```

Output:

```
Verdict: PASS
```

Exit status: 0

Expected: one line, Verdict: PASS, exit 0

### 5a. Criterion 3: .github absent

```
test ! -e .github
```

Output:

```
(no output)
```

Exit status: 0

Expected: no output, exit 0

### 5b. Criterion 3: no tracked file under .github

```
test -z "$(git ls-files .github)" && git ls-files .github
```

Output:

```
(no output)
```

Exit status: 0

Expected: no output, exit 0

### 6a. Criterion 4: src/translations absent

```
test ! -e src/translations
```

Output:

```
(no output)
```

Exit status: 0

Expected: no output, exit 0

### 6b. Criterion 4: translation build search (plan 03-06)

```
grep -rnI -e translation_sources -e finalize_translations -e update_translations_target -e Qt_add_translation -e compile_translations -e LinguistTools -e UpdateTranslations -e SCANTAILOR_TRANSLATIONS_DIR -e TRANSLATION_DIRS -e PLUGIN_DIRS CMakeLists.txt CMakePresets.json cmake config.h.in src
```

Output:

```
(no output)
```

Exit status: 1

Expected: no output, exit 1

### 6c. Criterion 4: translation code search (plan 03-06)

```
grep -rnI -e QTranslator -e installLanguage -e getLanguagesList -e getCurrentLocale -e initTranslations -e m_translationsMap -e m_currentLocale -e getLanguage -e setLanguage -e LANGUAGE_KEY -e DEFAULT_LANGUAGE -e languageBox -e languageLabel src --exclude-dir=ui_snapshot
```

Output:

```
(no output)
```

Exit status: 1

Expected: no output, exit 1

### 6d. Criterion 4: verdict line of ui-screenshots.md

```
grep -x 'Verdict: PASS' .planning/phases/03-qt-6-and-macos-only/records/ui-screenshots.md
```

Output:

```
Verdict: PASS
```

Exit status: 0

Expected: one line, Verdict: PASS, exit 0

### 6e. Criterion 4: .qm check of fresh-clone.md

```
grep -nF -e "find build/fresh-clone/build/macos-release -name '*.qm'" -e 'build.ninja' .planning/phases/03-qt-6-and-macos-only/records/fresh-clone.md
```

Output:

```
49:| `find build/fresh-clone/build/macos-release -name '*.qm'` | nothing |
50:| `grep -ci -e '\.qm' -e lrelease -e lupdate -e translations build/fresh-clone/build/macos-release/build.ninja` | 0 |
```

Exit status: 0

Expected: both rows say nothing / 0

### 7a. D-06: tr()/translate() calls on HEAD

```
git grep -h -o -E '(^|[^A-Za-z0-9_])tr\(|QCoreApplication::translate\(' HEAD -- 'src/*.cpp' 'src/*.h' | wc -l | tr -d ' '
```

Output:

```
249
```

Exit status: 0

Expected: 249

### 7b. D-06: <string> entries in .ui files on HEAD

```
git grep -h -o '<string>' HEAD -- 'src/*.ui' | wc -l | tr -d ' '
```

Output:

```
573
```

Exit status: 0

Expected: 573

### 8. cmake/ holds exactly two tracked files

```
git ls-files cmake
```

Output:

```
cmake/ListItemsPrepend.cmake
cmake/SetDefaultBuildType.cmake
```

Exit status: 0

Expected: cmake/ListItemsPrepend.cmake and cmake/SetDefaultBuildType.cmake

### 9a. D-11: goldens equal the baseline

```
git diff --quiet a796c2ae -- src/pipeline_tests/golden
```

Output:

```
(no output)
```

Exit status: 0

Expected: no output, exit 0

### 9b. D-11: no commit touched the goldens after the baseline

```
git log --format=%h a796c2ae..HEAD -- src/pipeline_tests/golden
```

Output:

```
(no output)
```

Exit status: 0

Expected: no output

### 10. D-11 completeness: every Phase 3 commit outside .planning/.claude has a gate-log row with both ctest summaries

```
H=$(git log --format=%h --grep='(03-' a796c2ae..HEAD -- . ':(exclude).planning' ':(exclude).claude'); echo "commits (newest first): $(echo $H | tr '\n' ' ')"; n=0; for h in $H; do n=$((n+1)); row=$(grep -E "^[|] c[0-9]+[a-z0-9-]* [|] $h [|]" .planning/phases/03-qt-6-and-macos-only/records/gate-log.md); lab=$(printf '%s' "$row" | cut -d'|' -f2 | tr -d ' '); c=$(printf '%s' "$row" | grep -o '100% tests passed out of 7' | wc -l | tr -d ' '); subj=$(git log -1 --format=%s $h); if [ -n "$row" ] && [ "$c" = 2 ]; then echo "OK  $lab $h ctest-summaries=$c  $subj"; else echo "MISSING $h ctest-summaries=$c  $subj"; fi; done; echo "count=$n"; O=$(echo $H | tr ' ' '\n' | tail -1); git merge-base --is-ancestor a796c2ae $O && echo "ancestor: a796c2ae is an ancestor of oldest ($O)"
```

Output:

```
commits (newest first): cf4da61c 56b5a540 1b1bec27 450ff19f 85e095d4 ad15b3ef 017e977c 462f22cd 81a27cb1 c63a74c2 9a68f985 8d0105ab 
OK  c11-docs cf4da61c ctest-summaries=2  docs(03-07): rewrite README and TESTING.md for the macOS build
OK  c10-translations 56b5a540 ctest-summaries=2  refactor(03-06): remove translations
OK  c09-portable 1b1bec27 ctest-summaries=2  refactor(03-05): remove portable mode
OK  c08-linux 450ff19f ctest-summaries=2  refactor(03-05): remove Linux code paths and files
OK  c07-windows-cmake 85e095d4 ctest-summaries=2  build(03-04): remove Windows build options and files
OK  c06-windows-cpp ad15b3ef ctest-summaries=2  refactor(03-04): remove Windows code paths
OK  c05-packaging 017e977c ctest-summaries=2  build(03-03): remove packaging and install rules
OK  c04-ci 462f22cd ctest-summaries=2  chore(03-03): remove GitHub automation
OK  c03-qt5-main-imageview 81a27cb1 ctest-summaries=2  refactor(03-02): resolve the Qt 5 blocks in main.cpp and ImageViewBase
OK  c02-qt6-cmake c63a74c2 ctest-summaries=2  build(03-02): require Qt 6 only in CMake
OK  c01-qt5-cpp 9a68f985 ctest-summaries=2  refactor(03-02): resolve Qt 5 conditionals in C++ with unifdef
OK  c00-ui-snapshot 8d0105ab ctest-summaries=2  test(03-01): add offscreen UI snapshot harness
count=12
ancestor: a796c2ae is an ancestor of oldest (8d0105ab)
```

Exit status: 0

Expected: one row per commit, every line OK, then ancestor check exit 0

### 11a. Kept on purpose: Clang flag block equals the baseline (D-01)

```
diff <(git show a796c2ae:CMakeLists.txt | sed -n '/^if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")/,/^endif()/p') <(sed -n '/^if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")/,/^endif()/p' CMakeLists.txt) && test -n "$(sed -n '/^if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")/,/^endif()/p' CMakeLists.txt)"
```

Output:

```
(no output)
```

Exit status: 0

Expected: no output, exit 0

### 11b. Kept on purpose: SANITIZE block equals the baseline (D-01)

```
diff <(git show a796c2ae:CMakeLists.txt | sed -n '/^set(SANITIZE /,/^endif()/p') <(sed -n '/^set(SANITIZE /,/^endif()/p' CMakeLists.txt) && test -n "$(sed -n '/^set(SANITIZE /,/^endif()/p' CMakeLists.txt)"
```

Output:

```
(no output)
```

Exit status: 0

Expected: no output, exit 0

### 11c. Kept on purpose: batch alarm command line (D-04)

```
grep -nF -e 'QString extPlayCmd("play /usr/share/sounds/freedesktop/stereo/bell.oga");' -e 'main_window/external_alarm_cmd' src/app/MainWindow.cpp
```

Output:

```
1327:        QString extPlayCmd("play /usr/share/sounds/freedesktop/stereo/bell.oga");
1329:        QString cmd = settings.value("main_window/external_alarm_cmd", extPlayCmd).toString();
```

Exit status: 0

Expected: two matching lines, exit 0

### 11d. Kept on purpose: RelinkablePath backslash replacement and drive-letter test (D-03)

```
grep -nF -e "frontSlashes.replace(QChar('\\\\'), QLatin1String(\"/\"));" -e "lastComp.endsWith(QChar(':'))" src/core/RelinkablePath.cpp
```

Output:

```
12:  frontSlashes.replace(QChar('\\'), QLatin1String("/"));
31:      if (lastComp.isEmpty() || lastComp.endsWith(QChar(':'))) {
```

Exit status: 0

Expected: two matching lines, exit 0

### 11e. Kept on purpose: ctest preset lists (release, debug)

```
ctest --preset macos-release -N | grep 'Total Tests'; ctest --preset macos-debug -N | grep 'Total Tests'
```

Output:

```
Total Tests: 7
Total Tests: 7
```

Exit status: 0

Expected: Total Tests: 7 twice

## Result

| ROADMAP criterion | Checks | Result |
|-------------------|--------|--------|
| 1. Qt 6 only: no Qt 5 or `QT_VERSION` branch left | 1 | PASS |
| 2. macOS only: no Windows, Linux, portable or packaging code, files or options; a clean directory builds and tests from the README | 2, 3a, 3b, 4, 8, 11a to 11e | PASS |
| 3. No GitHub automation | 5a, 5b | PASS |
| 4. No translations; 249 `tr()`/`translate()` calls and 573 `.ui` strings kept; no `.qm` file or translation rule in a clean build | 6a to 6e, 7a, 7b | PASS |
| D-11. Every Phase 3 strip commit gated; goldens untouched since `a796c2ae` | 9a, 9b, 10 | PASS |

Check 10 lists the twelve strip commits c00 to c11 (no c12 was needed), each with its `records/gate-log.md` label and two `100% tests passed out of 7` summaries; `a796c2ae` is an ancestor of the oldest (`8d0105ab`). The five searches that must print nothing (1, 2, 3b, 6b, 6c) exit 1 with `(no output)`.
