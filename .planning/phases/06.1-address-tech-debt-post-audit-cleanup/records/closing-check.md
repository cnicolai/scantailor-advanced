# Closing check of Phase 06.1 (plan 06.1-12)

Every check ran on the final tree (HEAD `c911a79e` at check time; the last source commit is `751f75b2`, the last gated commit `dec6a64c`, and only documentation commits follow). Outputs are under `build/scratch/` and are not committed. The positive control of each check says that the check can see what it looks for.

## 1. Planning ids in comments (CONTEXT regex only)

Command: `git grep -n -P '\b(D-[0-9]{2}[a-z]?|WR-[0-9]{2}|IN-[0-9]{2}|Phase [0-9]|plan 0[0-9]-[0-9]+)\b' -- src CMakeLists.txt cmake CMakePresets.json`

- Result: exit status 1, no output (0 lines).
- Positive control: the same regex on `0364f81d` prints 93 lines.
- Every path printed by `git grep -h -o -P '\.planning/phases/[\w.-]+/records/[\w.-]+\.md' -- src CMakeLists.txt` exists (8 distinct paths, `test -f` for each succeeded).
- The reverted family checks of 06.1-09 are not run (amendment 1 of `06.1-10-PLAN.md`).

## 2. Review dispositions

Command: `git grep -n -F -e 'disposition: open' -e '| open |' -- '.planning/phases/*-REVIEW-DISPOSITION.md'`

- Result: no output. Positive control: on `0364f81d` the same command prints 66 lines.
- `git grep -h -E '^open: 0$' -- '.planning/phases/*-REVIEW-DISPOSITION.md'` prints `open: 0` six times (open: 0 in each of the six files).
- `records/disposition_state.py` exits 0 for all six files (frontmatter, table and open count agree). Findings: 59 in six files.

| Phase | Findings | open |
|-------|----------|------|
| 1 | 8 | open: 0 |
| 2 | 13 | open: 0 |
| 3 | 9 | open: 0 |
| 4 | 8 | open: 0 |
| 5 | 8 | open: 0 |
| 6 | 13 | open: 0 |

Totals counted from the checker output: 54 fixed, 4 skipped (Phase 3 IN-06, Phase 5 IN-02, IN-03, IN-04), 1 deferred (Phase 1 WR-04), 0 open. The only line ending `deferred` is `WR-04 deferred deferred`.

## 3. Gate coverage

`git log --format=%H 0364f81d..HEAD -- src CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in` lists 22 commits. Each has a gate-log row (Commit column, first eight characters), its `build/scratch/gate/<label>/summary.txt` says `result: PASS`, and its `tested-commit.txt` equals the commit.

| Label | Commit | Gate summary |
|-------|--------|--------------|
| t11-build-files | dec6a64c | result: PASS, tested-commit equals the commit |
| t11-ubsan-fixes | 751f75b2 | result: PASS, tested-commit equals the commit |
| t10-comments-pipeline | 60b708d1 | result: PASS, tested-commit equals the commit |
| t10-comments-tests | 022f24dc | result: PASS, tested-commit equals the commit |
| t09-comments-app | 4b74447c | result: PASS, tested-commit equals the commit |
| t09-comments-build | 3f155d41 | result: PASS, tested-commit equals the commit |
| t08-prune-app | 13dc112f | result: PASS, tested-commit equals the commit |
| t08-prune-output | 352f8b93 | result: PASS, tested-commit equals the commit |
| t07-empty-batch | 73b538a5 | result: PASS, tested-commit equals the commit |
| t07-guard-harnesses | acc893f0 | result: PASS, tested-commit equals the commit |
| t07-guard-snapshot | db8e618b | result: PASS, tested-commit equals the commit |
| t06-own-regen | 26451961 | result: PASS, tested-commit equals the commit |
| t06-own-loader | afcaa7ee | result: PASS, tested-commit equals the commit |
| t06-own-ceiling | d9a32fd9 | result: PASS, tested-commit equals the commit |
| t05-mac-tests | c2e8f81a | result: PASS, tested-commit equals the commit |
| t05-diagnostics | 076021ab | result: PASS, tested-commit equals the commit |
| t05-test-env | 46e8a86b | result: PASS, tested-commit equals the commit |
| t04-generator-tests | 18456d09 | result: PASS, tested-commit equals the commit |
| t04-cache-test | 928d2a24 | result: PASS, tested-commit equals the commit |
| t03-leftovers | f6ef6bcc | result: PASS, tested-commit equals the commit |
| t03-unc-branch | 1f1f21ed | result: PASS, tested-commit equals the commit |
| t01-opengl-doc | a9c698c0 | result: PASS, tested-commit equals the commit |

`git diff --quiet a796c2ae -- src/pipeline_tests/golden` succeeds: no golden or manifest file differs from the recorded baseline.

## Ungated commits

`git log --format=%H 0364f81d..HEAD` lists 65 commits at check time; 22 have a gate-log row (section 3) and 43 have none: the 42 that existed when this plan started and the Task 1 commit of this plan (`c911a79e`, `.planning/` only). Each of the 43 has the gate input key of its parent (`git ls-tree -r <rev> -- src CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in | git hash-object --stdin`, taken through a file so a failing `git ls-tree` cannot pass) and leaves `.claude/CLAUDE.md` unchanged (`git diff --quiet <commit>~1 <commit> -- .claude/CLAUDE.md`).

Positive control: the same key comparison on `a9c698c0` (`t01-opengl-doc`, gated) reports a difference (`59060c36...` against `70fa7c4a...` for its parent), so the comparison can see an input change.

docs-only commits proven input-neutral rather than rebuilt: each has the gate input key of its parent and leaves .claude/CLAUDE.md unchanged, so D-00a's "every commit passes the gate" holds for them in this narrowed form.

| Commit | Subject | Gate input key |
|--------|---------|----------------|
| c911a79e | docs(06.1-12): fp-contract measurement and the last Phase 1 dispositions | equal to parent (644c473c) |
| 0a4fe7c7 | docs(state): record completion of plan 06.1-11 | equal to parent (644c473c) |
| b4e45db9 | docs(06.1-11): complete the sanitizer fixes and build-file hardening plan | equal to parent (644c473c) |
| cdaee322 | docs(06.1-11): strict sanitizer run, closures and gate record for t11-build-files | equal to parent (644c473c) |
| 52c6bbf7 | docs(06.1-11): sanitizer inventory and gate record for t11-ubsan-fixes | equal to parent (4a9e54ce) |
| ce3e6760 | docs(state): pause phase 06.1 before plan 11 | equal to parent (12cd8bb8) |
| 7b140c31 | docs(06.1-10): complete the test-side comment plan | equal to parent (12cd8bb8) |
| e1e8d985 | docs(06.1-10): gate records for t10-comments-tests and t10-comments-pipeline | equal to parent (12cd8bb8) |
| b284c307 | docs(06.1): trim remaining plans 10-15 to three at the user's request | equal to parent (59d7ecbb) |
| 7e9a388b | docs(06.1-09): complete the build file and app comment plan | equal to parent (59d7ecbb) |
| 273c6748 | docs(06.1-09): summary of the build file and app comment plan | equal to parent (59d7ecbb) |
| 4a1b287d | docs(06.1-09): print string tokens of comment_only.py as plain text | equal to parent (59d7ecbb) |
| a71d10f3 | docs(06.1-09): gate record for t09-comments-app | equal to parent (59d7ecbb) |
| 09217405 | docs(06.1-09): gate record for t09-comments-build and the comment-only proof | equal to parent (490f000a) |
| 40d1394d | docs(06.1-08): record plan state | equal to parent (7244e7ea) |
| 34b25238 | docs(06.1-08): summary of the include prune plan | equal to parent (7244e7ea) |
| db78e544 | docs(06.1-08): gate records for t08-prune-output and t08-prune-app | equal to parent (7244e7ea) |
| d712c66c | docs(06.1-07): record plan state | equal to parent (6ad0446e) |
| 35e587bb | docs(06.1-07): summary of the harness guard and empty batch plan | equal to parent (6ad0446e) |
| c2cfcc4f | docs(06.1-07): gate records for t07-guard-snapshot, t07-guard-harnesses and t07-empty-batch | equal to parent (6ad0446e) |
| 75cf206d | docs(06.1-06): record plan state | equal to parent (06978043) |
| 734609ca | docs(06.1-06): summary of the own-file ceiling plan | equal to parent (06978043) |
| 064b13e9 | docs(06.1-06): gate records for t06-own-ceiling, t06-own-loader and t06-own-regen | equal to parent (06978043) |
| 709b79a0 | docs(06.1-05): record plan state | equal to parent (78da563f) |
| eeb3033a | docs(06.1-05): complete offscreen test info findings plan | equal to parent (78da563f) |
| d004d6e3 | docs(06.1-05): summary of the offscreen test info findings plan | equal to parent (78da563f) |
| f5c10167 | docs(06.1-05): gate records for t05-test-env, t05-diagnostics and t05-mac-tests | equal to parent (78da563f) |
| e53448e4 | docs(06.1-04): complete test-code info findings plan | equal to parent (d3d41e7d) |
| e3ce5c07 | docs(06.1-04): summary of the test-code info findings plan | equal to parent (d3d41e7d) |
| 1227f5e5 | docs(06.1-04): gate records for t04-cache-test and t04-generator-tests | equal to parent (d3d41e7d) |
| 27088d50 | docs(06.1-03): complete UNC branch and language leftovers plan | equal to parent (c4cbcaed) |
| 7d4682e7 | docs(06.1-03): gate records for t03-unc-branch and t03-leftovers | equal to parent (c4cbcaed) |
| 42506138 | docs(06.1-02): complete close findings that need no code plan | equal to parent (59060c36) |
| c89a9b32 | docs(06.1-02): close Phase 1 IN-03 | equal to parent (59060c36) |
| 1f6c2af9 | docs(06.1-02): document how the debug and sanitizer presets differ from release | equal to parent (59060c36) |
| 17a6d529 | docs(06.1-02): close moot and won't-fix review findings and correct stale bookkeeping | equal to parent (59060c36) |
| f8bc46c1 | docs(06.1-02): close Phase 3 WR-01 as fixed by the app_gui library | equal to parent (59060c36) |
| f04c8138 | docs(state): record completion of plan 06.1-01 | equal to parent (59060c36) |
| c8e0eab3 | docs(06.1-01): complete OpenGL doc drift and gate infrastructure plan | equal to parent (59060c36) |
| 529d4b2c | docs(06.1-01): add the token comparison for gate command diffs | equal to parent (59060c36) |
| cd68982c | docs(06.1-01): gate record for t01-opengl-doc | equal to parent (59060c36) |
| e8f798d6 | docs(state): begin phase 06.1 execution | equal to parent (70fa7c4a) |
| 4b74bc23 | docs(06.1): create phase plan | equal to parent (70fa7c4a) |

## Formatting

`git diff --name-only --diff-filter=AM 0364f81d..HEAD -- '*.cpp' '*.h' '*.mm'` lists 72 files (49 were expected as a minimum). `xcrun clang-format --dry-run -Werror <file>` succeeds for every file except `src/dewarping/TextLineTracer.cpp`, which is in the list and still has exactly its two old warnings (`xcrun clang-format --dry-run src/dewarping/TextLineTracer.cpp` prints 2 warnings).

## Strict sanitizer run

route: input key equal to the recorded strict run

- Recorded run (`records/asan-strict-run.md`): tested commit `dec6a64c3b46a764fa1fe303dec419bdfe38f735`, input key `644c473c208a3fff65f273a63a627b5135394f44`, `100% tests passed out of 7`, no `runtime error:` and no `ERROR: AddressSanitizer` line.
- Key of HEAD (same two commands as `input_key` in `gate.sh`): `644c473c208a3fff65f273a63a627b5135394f44`. Equal.
- `git diff --quiet dec6a64c HEAD -- .claude/CLAUDE.md sanitizer` succeeds (the `sanitizer/` directory does not exist at either commit; `.claude/CLAUDE.md` is unchanged). `git diff --quiet dec6a64c HEAD -- src CMakeLists.txt CMakePresets.json cmake config.h.in version.h.in .claude/CLAUDE.md sanitizer` succeeds as well; the 4 commits after `dec6a64c` (three of 06.1-11 and the Task 1 commit of this plan) changed only `.planning/`.
- Positive control: the key of `0364f81d` is `70fa7c4a4e2cabd656f585eaa9d253716d2ea6bc`, which differs from the recorded key.
- Consequence: no second 11-minute strict run was made; the run of 06.1-11 is the strict run of the final source tree.

## Deferred findings

Phase 1 WR-04 (Boost handling relies on FindBoost-only variables under CMP0167 NEW) is deferred. Reason, verbatim from the user decision: hygiene only — Homebrew's Boost config sets Boost_INCLUDE_DIRS today (RESEARCH probe), deferred to keep the pre-milestone cleanup small; user decision 2026-11-17. D-07 is not implemented; the Boost block of `CMakeLists.txt` is unchanged and `include_directories(${Boost_INCLUDE_DIRS})` still works with Homebrew's Boost config. The finding stays visible as `deferred` in `.planning/phases/01-deletion-free-macos-build/01-REVIEW-DISPOSITION.md`.

Phase 1 WR-02 is fixed: the Clang block comment was narrowed in 06.1-11, and `records/fp-contract-measurement.md` records the one-off `-ffp-contract=on` measurement (34 of 35 goldens identical, `book/curved_300_dewarp` differs by 426 bilevel pixels; default unchanged).

## Accepted residuals

New in this phase:

- **Phase 6 memory-cap advisory** ("memory-cap reset after batch stop or project switch verified by code reading only"). `06.1-CONTEXT.md`, Claude's Discretion: "add an offscreen test if it fits naturally next to D-14; otherwise leave the advisory as is". It does not fit naturally: `WorkerThreadPool` and `SystemLoadWidget` have only `setMemoryCap` and no getter for the cap, so a test would need a test-only accessor, which adds surface to production classes for a reset already verified by code reading; and an empty project's cap is `kNoCap`, so the empty-batch test of 06.1-07 cannot observe a reset. The advisory stays as it is.
- **In-ceiling own-file residual of 06.1-06** (threat T-06.1-08, D-11). A forged own-file header inside the ceiling, for example 32767 x 32767 RGB (1073676289 px, below `kMaxOwnFilePixels` = 1073741823), is not refused and may cost about 4 GiB (RGB32). Accepted because D-11 deliberately chose a constant ceiling derived from the largest output the app can write, so that every file the app wrote itself loads.

Earlier accepted residuals, unchanged:

- MAC-10 beep: a source-verified override, because the user skipped the listening check (Phase 5).
- HIDPI-01 proxy: the DPR was measured with `retina_check`, which shares the app's Mach-O properties; the app binary itself was not launched (Phase 4).
- HIDPI-05 simulation: a real drag between a 1x and a 2x display is unverified; criterion 4 rests on the offscreen two-screen simulation (Phase 4).
- Phase 1 smoke-run cause: the first BUILD-03 smoke run failed, its settings never took effect, and the re-check passed; no app defect was found (Phase 1).

## Out of scope and unchanged (D-00f, D-11)

Backlog 999.1 to 999.8, `WINDOWS.md` entries 1, 2, 3, 4 and 6, the accepted residuals above, and the Margins panel width at the Filters dock's minimum width are unchanged by this phase.
