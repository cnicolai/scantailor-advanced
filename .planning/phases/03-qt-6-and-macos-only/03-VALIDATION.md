---
phase: "3"
slug: "qt-6-and-macos-only"
# status lifecycle: draft (seeded by plan-phase) → validated (set by validate-phase §6)
# audit-milestone §5.5 distinguishes NOT-VALIDATED (draft) from PARTIAL (validated + nyquist_compliant: false) (#2117)
status: validated
nyquist_compliant: true
wave_0_complete: true
created: "2026-10-24"
---

# Phase 3 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Boost.Test (6 executables) and Qt Test (`qt_tests`), run through CTest presets |
| **Config file** | `CMakePresets.json` (`macos-release`, `macos-debug`, `macos-asan`) and the per-directory `CMakeLists.txt` files |
| **Quick run command** | `cmake --build --preset macos-release && ctest --preset macos-release` |
| **Full suite command** | the quick command, then `cmake --preset macos-debug && cmake --build --preset macos-debug && ctest --preset macos-debug`; both must report 7/7 with `git status --porcelain -- src/pipeline_tests/golden` empty |
| **Estimated runtime** | ~120 seconds for Release (build about 60 s, ctest about 59 s); ~330 seconds for Debug (build about 58 s, ctest 230–270 s); worst-case full gate about 7–8 minutes |

---

## Sampling Rate

- **After every task commit:** Run the quick command for C++-only edits. For any CMake edit, also diff `ninja -t commands` for the 8 targets against the baseline lists in `build/scratch/`.
- **After every strip commit (D-11):** Run the full suite command, the goldens-unchanged check (`git diff --quiet a796c2ae -- src/pipeline_tests/golden`) and the leftover greps from RESEARCH.md § Validation Architecture.
- **Before `/gsd-verify-work`:** Full suite green, plus the fresh clone (D-13), the `macos-asan` comparison (D-14), the offscreen screenshots (D-12) and every leftover grep empty.
- **Max feedback latency:** 480 seconds (one full Release and Debug gate)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 03-01-T1 | 03-01 | 1 | STRIP-06 | T-03-01 | harness settings isolated below its output dir; output dir must lie below `build/` | tracer: build + offscreen run + gate | `bash .planning/phases/03-qt-6-and-macos-only/gate.sh check c00-ui-snapshot`; `build/macos-release/ui_snapshot build/scratch/ui-verify` (exit 0, `language control present: yes`); outside-dir run exits 2; `ctest --preset macos-release -N` lists 7 | created by the task | ✅ green |
| 03-01-T2 | 03-01 | 1 | STRIP-06 | T-03-02 | records normalised (`grep -c "$HOME"` = 0) | record | section greps on `records/before-state.md`, `records/execution-decisions.md` | created by the task | ✅ green |
| 03-02-T1 | 03-02 | 2 | STRIP-01 | T-03-03, T-03-05 | unifdef only, zero additions, dewarping polarity kept | numstat + grep + gate + D-15 run | `build/scratch/c01-numstat-unifdef.txt` 16 files / 0 added / 135 deleted; polarity greps; gate `c01-qt5-cpp` PASS; FlatDocumentTestSuite has no `qt_compile` TOOLCHAIN DIFFERS | yes (gate.sh from 03-01) | ✅ green |
| 03-02-T2 | 03-02 | 2 | STRIP-01 | T-03-04 | build commands unchanged | grep + command diff | no Qt5/Qt6_FOUND/Qt_*_lib in CMake; gate `c02-qt6-cmake` PASS with empty `cmds-*.diff` | yes | ✅ green |
| 03-02-T3 | 03-02 | 2 | STRIP-01 | T-03-03 | criterion-1 search empty | grep + gate | `! grep -rnI -e Qt5 -e QT_VERSION CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in src`; gate `c03-qt5-main-imageview` PASS | yes | ✅ green |
| 03-03-T1 | 03-03 | 3 | STRIP-05 | T-03-06 | no CI automation with repository credentials | file test + gate | `.github` and `scripts` absent; gate `c04-ci` PASS; ancestor of a796c2ae check | yes | ✅ green |
| 03-03-T2 | 03-03 | 3 | STRIP-04 | T-03-07 | no install or packaging step | file test + grep + gate | packaging search empty; gate `c05-packaging` PASS with empty diff | yes | ✅ green |
| 03-04-T1 | 03-04 | 4 | STRIP-02 | T-03-08, T-03-09 | macOS sides of conditionals kept; normaliser kept (D-03) | numstat + grep + gate | no `_WIN32`/`Q_OS_WIN` in C++; kept-line greps; gate `c06-windows-cpp` PASS; `### Phase 999.8:` in ROADMAP | yes | ✅ green |
| 03-04-T2 | 03-04 | 4 | STRIP-02 | T-03-08 | build commands unchanged | grep + gate | Windows CMake search empty; `git ls-files cmake` three files; gate `c07-windows-cmake` PASS with empty diff | yes | ✅ green |
| 03-05-T1 | 03-05 | 5 | STRIP-03 | T-03-11, T-03-12 | configure never sees a missing resource; alarm unchanged (D-04) | grep + git show + gate | Linux search empty; `git show --name-only` of c08 lists CMakeLists.txt and the `.desktop.in`; Clang/SANITIZE blocks equal a796c2ae; gate `c08-linux` PASS | yes | ✅ green |
| 03-05-T2 | 03-05 | 5 | STRIP-02 | T-03-10 | settings and profile locations unchanged | grep + json + gate | portable search empty; INI and AppData lines kept; `cmake --list-presets`; gate `c09-portable` PASS | yes | ✅ green |
| 03-06-T1 | 03-06 | 6 | STRIP-06 | T-03-13 | no reader or writer of the `language` key | grep + git show + counts + gate | translation searches empty; one commit holds module, form, Application, .ts; tr() 249 and `<string>` 573; gate `c10-translations` PASS | yes | ✅ green |
| 03-06-T2 | 03-06 | 6 | STRIP-06 | T-03-14 | harness isolated | harness + static + view | `ui_snapshot build/scratch/ui-after --assert-no-language` exit 0; four static counts 0; texts diff only the Language row; `Verdict: PASS` in `records/ui-screenshots.md` | yes (ui_snapshot from 03-01) | ✅ green |
| 03-07-T1 | 03-07 | 7 | STRIP-08, STRIP-04 | T-03-16 | only `~`-relative paths documented | grep + diff + gate | README sections, Homebrew line, preset lines, Features diff limited to two passages; TESTING.md sections; gate `c11-docs` PASS | yes | ✅ green |
| 03-07-T2 | 03-07 | 7 | STRIP-08 | T-03-15, T-03-17 | `brew install` not run; clone under `build/` | fresh clone | README lines only in `build/fresh-clone`, 7 of 7, no `.qm`, empty `cmds.diff`, `brew list --versions` eight lines, `Verdict: PASS` | created by the task | ✅ green |
| 03-08-T1 | 03-08 | 8 | STRIP-01..06 | T-03-18, T-03-19 | sanitizer reports seen (executables run directly) | ASan run + script | seven sections in `records/test-cases-asan.txt`; `New project locations:` with explanations; `Verdict: PASS` | created by the task | ✅ green |
| 03-08-T2 | 03-08 | 8 | STRIP-01..06 | T-03-19 | every strip commit gated | grep + git | criteria 1 to 4 searches empty; every `(03-` strip commit has a PASS gate-log row; goldens untouched since a796c2ae | yes | ✅ green |
| 03-09-T1 | 03-09 | 9 | STRIP-08 | T-03-20 | profile section untouched | grep + diff | D-10 search has no unmarked line; profile section equal to the parent commit; map frontmatter unchanged | yes | ✅ green |
| 03-09-T2 | 03-09 | 9 | all | T-03-21 | no manual step in the summary | record | `records/closing-check.md` sections, five criterion lines, summary at most 15 lines | created by the task | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `src/app/ui_snapshot/main.cpp` and its `BUILD_TESTS` block in `src/app/CMakeLists.txt`: offscreen grabs of the main window and the Settings dialog, plus the `--assert-no-language` check (SC4, D-12). Not registered with ctest, so the 7/7 gate stays as it is.
- [x] `build/scratch/cmds-*-baseline.txt`: baseline `ninja -t commands` lists for Release and Debug, taken on the unmodified tree before the first strip commit (kept by `gate.sh` under `build/scratch/gate/ref/`, keyed by the build-input tree).
- [x] "Before" artefacts: `nm -u` translator count (5), `strings` hits for `scantailor_*.qm` (1), `build.ninja` translation references (40), `languageBox` in `ui_SettingsDialog.h` (4), baseline PNGs.

*No test framework install is needed.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| — | — | — | — |

*All phase behaviors have automated verification. Per D-12, the executor views the offscreen PNGs itself and records the verdict; the user does no GUI step.*

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 480s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved 2026-10-24 (orchestrator audit: every requirement re-checked green on HEAD 25f729fc, with positive controls for the empty-search checks)

## Validation Audit 2026-10-24

| Metric | Count |
|---|---|
| Gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |
