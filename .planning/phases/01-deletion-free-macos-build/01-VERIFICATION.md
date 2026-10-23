---
phase: 01-deletion-free-macos-build
verified: 2026-10-23T13:36:47Z
status: passed
score: 5/5 must-haves verified
covered_files:
  - .planning/phases/01-deletion-free-macos-build/01-01-PLAN.md
  - .planning/phases/01-deletion-free-macos-build/01-01-SUMMARY.md
  - .planning/phases/01-deletion-free-macos-build/01-02-PLAN.md
  - .planning/phases/01-deletion-free-macos-build/01-02-SUMMARY.md
  - .planning/phases/01-deletion-free-macos-build/01-03-PLAN.md
  - .planning/phases/01-deletion-free-macos-build/01-03-SUMMARY.md
  - .planning/phases/01-deletion-free-macos-build/01-04-PLAN.md
  - .planning/phases/01-deletion-free-macos-build/01-04-SUMMARY.md
  - .planning/phases/01-deletion-free-macos-build/01-05-PLAN.md
  - .planning/phases/01-deletion-free-macos-build/01-05-SUMMARY.md
  - .planning/phases/01-deletion-free-macos-build/01-06-PLAN.md
  - .planning/phases/01-deletion-free-macos-build/01-06-SUMMARY.md
  - .planning/phases/01-deletion-free-macos-build/01-07-PLAN.md
  - .planning/phases/01-deletion-free-macos-build/01-07-SUMMARY.md
  - CMakeLists.txt
  - CMakePresets.json
covered_digest: "v3:sha256:e20375a309569582e2fb20545dd91be8833c11337bf88ac6d61d71fc013a4141"
behavior_unverified: 0
overrides_applied: 0
re_verification: false
advisory:
  - finding: "First smoke run: Auto dewarping and the picture zone never took effect; cause never established (user error vs. app defect). Re-check shows the apply/save/reload path works."
    category: other
    reason: "Recorded in SMOKE-FINDINGS.md as 'resolved, this is what happened' without a root cause. Not blocking: the re-check produced a parsed project file with the settings."
    evidence_status: "re-check evidence present; first-run cause undetermined"
  - finding: "WR-02 (01-REVIEW): -ffp-contract=off changes last-bit FP results versus the unmodified Apple Clang default; no unmodified-vs-fp-off output comparison was recorded."
    category: other
    reason: "Required by BUILD-07 and decided by the user (D-03), so not a violation of the phase goal. Phase 2 baselines are made with the flag, so later comparisons stay consistent. The gap is that nobody measured the size of the difference."
    evidence_status: "decision recorded in fp-contract.md; difference not measured"
---

# Phase 1: Deletion-Free macOS Build Verification Report

**Phase Goal:** The unmodified 1.2.1 code builds as a native arm64 binary on the Mac and processes a real project end to end. The only edits are those needed to build and launch it.
**Verified:** 2026-10-23T13:36:47Z
**Status:** passed
**Re-verification:** No, initial verification

## Goal Achievement

I did not run builds, ctest or the app (user rule). I re-checked what can be read inside the project: git history and diffs, the existing `build/*` trees (`file`, `otool -L`, `jq` over `compile_commands.json`) and the committed records. Runtime facts (ctest results, sanitizer reports, the GUI walkthrough) rest on the committed records, which I cross-read rather than re-ran.

### Observable Truths (ROADMAP success criteria)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | One preset configures and builds with Apple Clang against Homebrew Qt 6, Boost, jpeg-turbo, libpng, libtiff. `file` reports arm64, compile commands carry `-Wall -Wextra`, `otool -L` lists exactly one libjpeg, libpng, libtiff | VERIFIED | `CMakePresets.json` defines `macos-release/debug/asan` with Ninja and `CMAKE_PREFIX_PATH=/opt/homebrew`. Re-run by me on `build/macos-release/scantailor-advanced`: `file` gives `Mach-O 64-bit executable arm64`. `otool -L` gives one line each for `libtiff.6`, `libpng16.16`, `libjpeg.8` (all `/opt/homebrew/opt/...`). `jq` on all three trees: 408 entries each, 0 missing `-Wall`, `-Wextra` or `-ffp-contract=off`. Fresh clean rebuild logged in `closing-check.md` (456/456, exit 0). Runtime single-load check (DYLD_PRINT_LIBRARIES, 1/1/1) is in the record; I did not re-run it. |
| 2 | App launches from the build directory, opens a project, takes it through all six stages, writes output files | VERIFIED (human-attested, with caveats below) | `records/SMOKE-FINDINGS.md`: verdict `PASS (after re-check of steps 4, 6, 8; first run FAIL)`. User launched the unbundled release binary, created a project from three synthetic pages, ran a batch of all six stages (no error panels), got four output TIFFs (depth 8/1/1/8 matching modes Color-Gray/BW/BW/Mixed per `magick identify`), opened four in Preview, saved, closed and reopened. Re-saved project file parses (root `project`, 4 pages), holds `bw`, `colorOrGray`, `mixed`, dewarping `auto` on 02-left (params and output-params) and 1 picture zone on page 03. App quit on Cmd+Q. See Gaps/Advisory for caveats. |
| 3 | `ctest` runs the existing suite; each test's pass/fail from the first build is committed | VERIFIED | `records/ctest-release.txt`, `ctest-debug.txt`, `ctest-summary.md`, `test-cases-*.txt` are tracked in git, each begins with `tested commit:` and an empty-changes line. 5 tests registered: foundation 22/22, math 32/32, core 55/55, qt_tests 15/15 pass; `imageproc_tests` 93/94, failing `ScaleTestSuite/test_random_image` (`TestScale.cpp:68`, `:70`). The failing case is the expected baseline, not a gap (BUILD-04 asks to record the state). Final-tree run in `closing-check.md` reports no difference. |
| 4 | `-ffp-contract` fixed and recorded; a CMake sanitizer option gives an ASan/UBSan build that runs the existing tests; findings recorded, not fixed | VERIFIED | `CMakeLists.txt` lines 68-69 add `-ffp-contract=off` for C and C++ on Clang, outside the cache guard. Present in 408/408 commands of all three trees (my `jq`). `fp-contract.md` records setting, reason and `fmadd` vs `fmul`/`fadd` assembly evidence. `SANITIZE` cache variable at `CMakeLists.txt:72`; `build/macos-asan` has `-fsanitize=` in 408/408 commands, release and debug in 0. `ctest-asan.txt` shows all 5 tests run (same pass/fail as Debug). `sanitizer-findings.md` lists 0 ASan, 3 UBSan reports at 2 project locations (`BinaryImage.cpp:1044:37`, `SlicedHistogram.cpp:49:47`), states that ctest PASS does not mean zero findings. No `src/` file touched. |
| 5 | `git diff 5eaac18` outside `.planning/` shows only build/launch fixes and added build settings: no deleted code paths, no change to processing behaviour | VERIFIED | My own `git diff --stat 5eaac18 -- . ':!.planning' ':!.claude'`: `CMakeLists.txt` (+23/-1) and `CMakePresets.json` (new, 90 lines). `git diff --numstat 5eaac18 -- src` is empty: `src/` is byte-identical to 1.2.1. The single removed line is `cmake_minimum_required(VERSION 3.9)` replaced by 3.25. Other hunks are the CMP0167 policy block, the Clang warning/`-ffp-contract=off` block and the `SANITIZE` block, all additive. `git log 5eaac18..HEAD -- . ':!.planning' ':!.claude'` lists six commits, each with a row in `records/fix-log.md`. Only processing-relevant change is `-ffp-contract=off`, deliberate and user-decided (BUILD-07, D-03). `.claude/CLAUDE.md` differs from 1.2.1 but was added by the roadmap commit before Phase 1 work. |

**Score:** 5/5 truths verified (0 present, behavior-unverified)

### Plan-level must-haves (cross-check)

| Plan | Must-have | Status | Evidence |
|------|-----------|--------|----------|
| 01-01 | Access approval is the first records/ commit | VERIFIED | `records/access-approval.md` starts `Decision: approve`; `git log --diff-filter=A` shows `4dbcfdf docs(01-01): record approval...` as the first records commit |
| 01-01 | Warnings, `-ffp-contract=off`, SANITIZE flags present/absent in the right trees; `PORTABLE_VERSION=OFF` in all presets | VERIFIED | `jq` results above; `CMakePresets.json` `macos-base` sets `PORTABLE_VERSION=OFF` |
| 01-01 | Toolchain versions and flag comparison committed | VERIFIED | `records/toolchain.txt`, `flags-compare.txt` exist and are tracked |
| 01-02 | Both builds finish; error classes recorded; every differing file named in fix-log | VERIFIED | `fix-log.md` "Build attempt 1": release 434/434, debug 456/456, 0 errors, so no `src/` edit was needed. Six rows cover the six commits |
| 01-03 | arm64, one copy of each image library, warning counts, ctest baseline with `tested commit:` header | VERIFIED | `warning-counts.txt` (1143 total, 1125 `-Wunused-parameter`), `binary-check.txt`, `ctest-*.txt` |
| 01-04 | ASan build, 5 tests run, findings listed with counts, "PASS is not zero findings" note | VERIFIED | `sanitizer-findings.md` Method and Totals |
| 01-05 | DPR probe, settings paths, smoke pages by checksum (no images committed), D-02 recorded before first launch | VERIFIED | `probe-output.txt` (screen DPR 2, window DPR 2), `smoke-pages.md`, `prior-state.md` present |
| 01-06 | Output TIFFs open in Preview | PARTIAL | First-run four files opened in Preview (step 7). The two files re-rendered in the re-check (`02-book-spread_1L.tif`, `03-mixed.tif`) were checked with `magick identify` only. Does not affect roadmap SC 2 ("writes output files"). |
| 01-06 | Thumbnails in file-name order before saving and after reopening | PARTIAL | Order confirmed at step 2 (before saving). After reopening only "four pages" (R1) and "came back as I have left it" (step 8) are reported. |
| 01-07 | Closing check shows all five criteria from fresh trees | VERIFIED | `closing-check.md`: all PASS, commit `cde6825` |

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `CMakePresets.json` | Release/debug/asan configure, build and test presets | VERIFIED | 90 lines, schema v6, presets resolve and produced the three existing trees |
| `CMakeLists.txt` | Clang warnings, FP freeze, `SANITIZE`, CMake 3.25, CMP0167 | VERIFIED | Diff reviewed line by line above |
| `build/macos-release/scantailor-advanced` | arm64 binary | VERIFIED | `file`, `otool -L` re-run by me; build dir is git-ignored |
| `records/` evidence set | Tracked in git | VERIFIED | 26 files on disk, 26 tracked by `git ls-files`, none modified |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `CMakePresets.json` (`macos-asan`) | `CMakeLists.txt` `SANITIZE` | `cacheVariables.SANITIZE=address,undefined` | WIRED | 408/408 asan compile commands carry `-fsanitize=` |
| `CMakeLists.txt` Clang block | all targets in `src/` | `add_compile_options` before `add_subdirectory(src)` | WIRED | Flags present in every compile command of all three trees |
| `fix-log.md` | `git log 5eaac18..HEAD` outside `.planning/` | row per commit | WIRED | Six commits, six rows; I confirmed the commit list matches |

### Data-Flow Trace (Level 4)

Not applicable: no code rendering dynamic data was added. `src/` is unchanged.

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Binary is arm64 | `file build/macos-release/scantailor-advanced` | `Mach-O 64-bit executable arm64` | PASS |
| One image library each | `otool -L ... \| grep -E 'libjpeg\|libpng\|libtiff'` | 3 lines, one per library | PASS |
| Warning + FP flags everywhere | `jq` over three `compile_commands.json` | 408/408 each, 0 missing | PASS |
| Sanitizer only in asan tree | `jq` count of `-fsanitize=` | release 0, debug 0, asan 408 | PASS |
| No source diff vs 1.2.1 | `git diff --numstat 5eaac18 -- src` | empty | PASS |
| ctest, sanitizer runs, GUI walkthrough | not re-run (user rule) | taken from committed records | SKIP |

### Probe Execution

No probes declared or found (`scripts/*/tests/probe-*.sh` not applicable). SKIPPED.

### Requirements Coverage

All seven IDs appear in plan frontmatter (01-01: BUILD-01, 05, 06, 07; 01-02: 01; 01-03: 01, 02, 04, 05; 01-04: 06; 01-05 and 01-06: 03; 01-07: all). `REQUIREMENTS.md` maps exactly BUILD-01 to BUILD-07 to Phase 1; no orphans.

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| BUILD-01 | 01-01, 01-02, 01-03, 01-07 | One preset builds arm64 against Homebrew libs | SATISFIED | Presets exist; arm64 binary; clean rebuild in closing check |
| BUILD-02 | 01-03, 01-07 | Exactly one copy each of libjpeg/png/tiff (`otool -L`) | SATISFIED | My `otool -L`: 1/1/1; runtime dyld check 1/1/1 in record |
| BUILD-03 | 01-05, 01-06, 01-07 | App launches, opens project, six stages, writes output | SATISFIED (human-attested) | SMOKE-FINDINGS PASS after re-check; parsed project file; 4 TIFFs identified. See advisory notes |
| BUILD-04 | 01-03, 01-07 | ctest runs; initial state recorded | SATISFIED | 5 tests registered, per-case results tracked; `imageproc_tests` baseline failure recorded |
| BUILD-05 | 01-01, 01-03, 01-07 | Apple Clang warnings (`-Wall -Wextra` equivalents) | SATISFIED | 408/408 commands; 1143 warnings logged, not fixed. Note `-Wno-unused` (copied from the repo's existing GNU set) mutes unused-variable class warnings |
| BUILD-06 | 01-01, 01-04, 01-07 | CMake option for ASan + UBSan | SATISFIED | `SANITIZE` option, `macos-asan` preset, findings recorded |
| BUILD-07 | 01-01, 01-07 | `-ffp-contract` fixed and recorded before baselines | SATISFIED | `-ffp-contract=off`, `fp-contract.md`; Phase 2 baseline pinned to `cde6825` after the flag commit `606bea0` |

### Anti-Patterns Found

No `src/` file was modified, so the stub/TODO scan has nothing to scan. `CMakeLists.txt` and `CMakePresets.json` contain no `TBD`/`FIXME`/`XXX` markers (the added comment lines reference D-03/D-08, noted by IN-04).

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| CMakeLists.txt | 65, 68 | Comments cite planning IDs (D-08, D-03) | Info | Rationale not self-contained (IN-04); no goal impact |

### Code-Review Warnings vs. Must-Haves

`01-REVIEW.md` (0 critical, 4 warning, 4 info; all still `open` in `01-REVIEW-DISPOSITION.md`). None undermines a Phase 1 must-have:

- **WR-01** (`macos-asan` test preset cannot fail on a sanitizer finding): by design for collection; SC 4 asks that findings be recorded, not fixed, and `sanitizer-findings.md` counts them from test output and says so. It matters later if the preset is used as a regression gate.
- **WR-02** (`-ffp-contract=off` differs from the Apple Clang default): real, and the one place where SC 5's "no change to processing behaviour" is not literally true. It is required by BUILD-07 and was a recorded user decision (D-03), so it counts as an "added build setting". The size of the numeric difference was not measured. Carried as advisory.
- **WR-03** (`SANITIZE` unvalidated/unquoted) and **WR-04** (FindBoost variables inert under CMP0167): hygiene and latent issues. The builds work as recorded, so no must-have fails. WR-04's `BOOST_TEST_DYN_LINK` always being defined works today (tests build and run).
- **IN-03** is relevant to Phase 2: take goldens from `macos-release`, since debug/asan define `ENABLE_DEBUG_FEATURES`. `closing-check.md` already pins Phase 2 to release outputs.

### Human Verification Required

None outstanding. BUILD-03 was already executed by the user, and its evidence is judged above.

### Gaps Summary

No gaps against the roadmap contract. Notes worth carrying forward (non-blocking):

1. **First smoke run failure has no root cause.** The user reported steps 4 and 6 as successful, yet the saved file held dewarping `off` and zero zones. The re-check, with the GUI state confirmed at each step, applied, saved and reloaded both. This most likely points to a missed Apply/OK in the first walk, but nothing proves that. The BUILD-03 PASS therefore rests on the re-check plus the user's confirmations. Dewarping straightness and the zone's colour effect were judged by eye, not by an automated check (that is Phase 2's job).
2. **Re-check evidence is thinner than the first run:** no stderr capture, two re-rendered TIFFs checked only by `magick identify`, post-reopen thumbnail order not stated explicitly.
3. **HIDPI-01 remains provisional** (probe says DPR 2.0; the app's own `qt.highdpi` line never appeared). Belongs to Phase 4, not a Phase 1 must-have.
4. **ROADMAP.md bookkeeping:** the Phase 1 line in the phase overview list still shows `[ ]` although all seven plans are marked `[x]`. The orchestrator should tick it when it commits.
5. **Pre-existing baseline defects to keep in view:** `ScaleTestSuite/test_random_image` fails; UBSan flags `BinaryImage.cpp:1044` and `SlicedHistogram.cpp:49`. Phase 2 will need expected-failure markers for the first.

---

_Verified: 2026-10-23T13:36:47Z_
_Verifier: Claude (gsd-verifier)_
