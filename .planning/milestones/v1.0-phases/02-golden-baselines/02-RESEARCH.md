# Phase 2: Golden Baselines - Research

**Researched:** 2026-10-23
**Domain:** Deterministic synthetic-image test harness and golden-image regression suite for a C++17/Qt 6 image pipeline (Boost.Test, CTest, macOS arm64)
**Confidence:** HIGH for seams, wiring and measured behaviour (read in source and exercised by throw-away spikes on this Mac); MEDIUM for tolerance numbers (measured on one toolchain, one machine)

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Fixtures and coverage
- **D-01:** Fixtures exist at both 300 dpi and 600 dpi, with the DPI set explicitly in the image metadata. The user scans at both resolutions. Every full-coverage scenario (D-03) runs at both DPIs.
- **D-02:** Fixtures are generated as 24-bit colour (RGB) images, because the user's scanner delivers colour. This applies to the flat-document and book-spread fixtures as well, so the early stages' colour handling runs in every scenario.
- **D-03:** Full golden coverage, at both DPIs, goes to the two output modes the user uses in real work: **black-and-white** (binarization, despeckling) and **colour/greyscale**. This covers the flat-document, book-spread (page split, without dewarping) and mixed-page fixtures. The user added "so far, this might change in the future".
- **D-04:** **Mixed output mode** (picture zones) and **dewarping** get the requirement minimum. That means the ground-truth checks TEST-06 and TEST-07 require (dewarped text lines straighter than the input; picture zone kept inside, pure black and white outside), the dewarping unit tests (TEST-09), and one golden each at 300 dpi. The user does not use these modes yet.
- **D-05:** The whole phase is fully automated. It has no GUI walkthroughs, no "open this image and look" steps and no human-verify checkpoints. The user said "I don't want to test this myself. No manual tests for these please", then extended this to the whole phase. Consequence: nobody inspects the goldens before they are frozen, so every golden scenario must pass automated sanity checks against the generator's ground truth before it is recorded. Examples: the output is not blank, has the right orientation and size, and its content sits where the ground truth puts it. At the end the user gets a short written summary of results and expected failures, with nothing to click through.

#### Golden update and review
- **D-06:** Goldens are re-recorded only with the environment variable `UPDATE_GOLDENS=1` (for example `UPDATE_GOLDENS=1 ctest --preset macos-release`). A normal run never writes into the source tree.
- **D-07:** Under `UPDATE_GOLDENS=1`, only goldens that fail their comparison are rewritten. Goldens that still pass stay byte-for-byte untouched, so `git diff` shows only the images that really changed. Every update run refreshes the manifest.
- **D-08:** On a failed comparison, the test writes expected, actual and diff PNGs to `build/<preset>/test-artifacts/<test>/`. The `ctest` output prints the failing metric, its value, its limit and that folder's path. There is no HTML report.
- **D-09:** A manifest next to the goldens records the toolchain they were made with: Qt, Boost, libtiff, libpng, jpeg-turbo, Apple Clang/Xcode, CMake, macOS, the `-ffp-contract` setting and the source commit. When the running toolchain differs, the test prints which versions differ and still compares (warn, do not fail, do not skip).
- **D-10:** A golden update is its own commit, with the reason in the commit message. There is no separate change-log file next to the goldens.

#### Tolerances
- **D-11:** Goldens are recorded from the `macos-release` build. The `macos-release` and `macos-debug` builds must pass every golden. The `macos-asan` build runs the full new suite once in this phase. Its findings are recorded, as in Phase 1 `records/sanitizer-findings.md`, but they neither gate the suite nor get fixed.
- **D-12:** Tolerances are measured, not guessed. A short spike measures the output spread between the Release and Debug builds for each image kind. Each limit is set just above the measured spread, with the research starting values as ceilings: bi-level ≤ 0.1% differing pixels after an edge-tolerant XOR, plus a connected-component count check; grey/colour mean absolute error ≤ 0.5 and maximum error ≤ 8 levels. The limits are then frozen as constants in one place in the comparator, one set per image kind, and are not overridden per test without a comment. — **Reversibility:** costly — once goldens are committed and later phases are checked against them, loosening a limit means re-validating every phase gate that relied on it, and tightening one may force a re-record.
- **D-13:** The comparator has a permanent automated self-test. It takes a recorded golden, alters it the way a real regression would (flips a few hundred pixels, shifts it by 1 px, raises a grey level by 10), and asserts that each comparison fails and writes its diff files. The self-test runs in every `ctest` and satisfies success criterion 3 without a manual step.
- **D-14:** Each stage's decisions are also recorded as **text goldens**, one small text file per scenario: deskew angle, page-split line, content box, margins/page layout, output size and DPI. Later runs must match them within a tight tolerance (on the order of 0.01° and 1 px). They use the same `UPDATE_GOLDENS=1` flag and manifest as the image goldens, and `git diff` shows them as readable numbers.
- **D-15:** Ground-truth tolerances start from the research values: deskew within ±0.15°, content-box IoU ≥ 0.95, page split within ±2 px, binarization mismatch ≤ 0.5% of the ink area, dewarped baseline residual ≤ 1.5 px at 300 dpi. They are tightened wherever the unmodified code does clearly better.

#### Production code and known defects
- **D-16:** Non-test `src/` stays identical to `cde6825`. Tests reach the code only through interfaces that already exist: each stage's `Task::process`, `OutputGenerator::process`, `ProjectWriter`/`ProjectReader`, and the `imageproc` and `dewarping` classes. Allowed additions: new test and test-support directories, edits to existing test files, CMake wiring for tests (new targets, `add_subdirectory` under `BUILD_TESTS`, `set_tests_properties` environment) and test settings in `CMakePresets.json`. Compile flags of production targets must not change. Proof for TEST-04: `git diff cde6825` restricted to non-test `src/` paths is empty. — **Reversibility:** costly — a production edit made before the goldens are committed breaks the "recorded from unmodified code" claim, and the goldens would have to be re-recorded from a clean tree.
- **D-17:** The `ScaleTestSuite/test_random_image` failure (`src/imageproc/tests/TestScale.cpp`) is investigated briefly to find which side moved. If Qt 6's `QImage::scaled(SmoothTransformation)` reference changed, the test's expectation is fixed in test code only. If `scaleToGray()` itself is off, the two failing checks are marked as a Boost expected failure with the reason, and a backlog item is added. Production code is untouched either way.
- **D-18:** When a new ground-truth check shows the unmodified code getting something wrong, the golden records the current behaviour. The failing ground-truth assertion is marked as an expected failure that names the defect, and the requirement that will fix it where one exists. The defect also goes into a phase findings file and a backlog item. Nothing is fixed in Phase 2, and the fixture is never changed to dodge a defect.
- **D-19:** New sanitizer reports from the one-off ASan/UBSan run are recorded like the Phase 1 ones and not fixed.
- **Carried from the roadmap:** if the unmodified code fails the special-character project round trip (TEST-10), that case is an expected failure naming SAFE-02. Phase 6 removes the mark.

### Claude's Discretion
- **Fixture pixel size and runtime budget.** The user left this open. Keep pages physically small, so 600 dpi stays affordable, and the full Release `ctest` run in the order of a minute or two.
- **Fixture storage.** Input pages may be committed as PNGs or generated in memory on each run. If they are committed, add a test that regenerates them and compares exactly, so generator drift is caught separately from pipeline drift (PITFALLS.md Pitfall 3).
- **Generator internals.** Choose the PRNG and the glyph shapes. The constraints: no `rand()`, no font rendering, and integer pixel operations where possible. The research files disagree on the PRNG: ARCHITECTURE.md uses `std::mt19937`, while PITFALLS.md asks for an own xorshift and no `<random>` distributions. Either is acceptable if the output is identical across libc++ versions. Do not use the code under test (`imageproc::transform`, `dewarping::`) to build the inputs.
- **Code layout.** The research suggests `src/test_support/` (depends on `imageproc` and `Qt::Gui`, not `core`), `src/pipeline_tests/` (Boost.Test with a global offscreen `QApplication` fixture) and `src/dewarping/tests/`. Where the goldens live and which text-golden format to use are open.
- **Headless pipeline entry.** Chain the `Task::process` calls directly or use `StageSequence`, whichever the seams allow without production edits.
- **Settings isolation.** Set the organisation and application name, the INI format and `QSettings::setPath` to a temp dir before the first `ApplicationSettings::getInstance()`. Run in `QT_QPA_PLATFORM=offscreen`, with temp dirs per test.
- **Special-character set for TEST-10:** `&`, `<`, `>`, `"`, `'`, spaces and non-ASCII (umlauts in both NFC and NFD form, because of macOS file-name normalisation).
- **How expected failures are expressed:** Boost `expected_failures` decorator; `QEXPECT_FAIL` in Qt tests.

### Deferred Ideas (OUT OF SCOPE)
None. The discussion stayed within the phase scope. Defects the new tests uncover become backlog items under D-18; they are not deferred ideas from this discussion.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| TEST-01 | Seeded generator: flat, book-spread, mixed pages with ground truth, no fonts, no `rand()` | Generator design in Pattern 1; prototype produced usable flat, spread, mixed and warped pages (spike S1-S5); splitmix64 + 5x7 glyph table; pitfalls P6, P9 |
| TEST-02 | Comparator with per-kind tolerances; failure writes actual and diff images | Pattern 3; measured Release/Debug/ASan spread = 0 (spike S6); tolerance ceilings kept; self-test D-13 |
| TEST-03 | Goldens change only via explicit flag; manifest with toolchain versions | Pattern 4 (GoldenStore, `UPDATE_GOLDENS`, manifest sources per tool); toolchain commands from Phase 1 `records/toolchain.txt` |
| TEST-04 | Baselines recorded from unmodified code | Only `src/CMakeLists.txt` needs wiring edits (Pattern 6); proof pathspec in Validation Architecture; baseline commit `cde6825` |
| TEST-05 | Flat-document deskew angle, content box, binarization vs ground truth | Headless chain (Pattern 2) measured: angle error 0 to 0.125 degrees, content box found; pitfall P1 (defaults must be loaded) |
| TEST-06 | Page split position and dewarp straightening | Spread spike: TWO_PAGES detected, slanted cutter (3.7 degrees) so ±2 px at mid-height not achievable; dewarp spike: residual 18.1 px to 7.3 px at 600 dpi; ratio criterion recommended |
| TEST-07 | Picture zones, colour/grey output, despeckling | Mixed spike: MIXED mode keeps picture in colour, text in B/W; page_split AUTO mis-cuts mixed page (P3); despeckle path exercised |
| TEST-08 | Revive `TestBinarize` with synthetic inputs | Nine binarizers exist in `Binarize.h`; test_support generator links into `imageproc_tests`; `TestScale` finding (D-17) measured |
| TEST-09 | Dewarping unit tests (distortion model, dewarper) | `dewarping_tests` links without `core` or QApplication (spike S8: model valid, round trip 6e-13 px, tracer valid); needs Qt::Xml |
| TEST-10 | Project round trip, non-default params in every stage, special-character names | StageSequence + `loadDefaultSettings` + `DefaultParamsProvider::setParams`; spike S7: special-character round trip PASSES on unmodified code for file and directory names |
| TEST-11 | One `ctest` runs everything offscreen with isolated settings, passes | `Application` (not plain `QApplication`) required; locale pinning; environment via `set_tests_properties`; expected-failure trip-wire |
</phase_requirements>

## Project Constraints (from CLAUDE.md)

Source: `/Users/christoph/Developer/scantailor-advanced/.claude/CLAUDE.md` (read this session). The planner must verify compliance.

- Platform is macOS arm64 only. Dependencies are Homebrew Qt 6, Boost, libjpeg, libpng, libtiff, zlib.
- Keep the C++17/Qt/CMake architecture and the six-stage pipeline. **Do not introduce C++20 features.**
- Output for existing settings must not change unintentionally. **Synthetic test images only; no personal scans committed.**
- Test naming: Boost suites are `<Module><Subject>TestSuite`, cases `test_<snake_case>`; test sources `Test<Name>.cpp`; `add_test(NAME x COMMAND x --log_level=message)`.
- Formatting: `clang-format` Chromium style, 2-space indent, `BinPackParameters: false`; clang-format is not installed on this Mac (Phase 1 decision), so follow `.clang-format` by hand.
- New source files start with the project copyright and licence header (copy from an existing file).
- Include guards `SCANTAILOR_<MODULE>_<NAME>_H_`; `#pragma once` is not used. Module directories are include roots (`#include <PageId.h>`).
- Errors: catch by const reference; do not swallow exceptions; parse functions use an out-parameter flag. User-visible strings use `tr()`; not relevant to tests.
- Do not read, list or search outside the project directory without asking; never touch OmniFocus. This research respected that: only the build tree, project files and the scratchpad were read; compiler runs read Homebrew headers as the project's own build does (no direct reads of `/opt/homebrew`).
- GSD workflow: direct repo edits only through a GSD command.

## Summary

Phase 2 is feasible with **no production edit**. Every seam the context names exists and was exercised this session with throw-away spikes in the session scratchpad (nothing in the repo was changed): the six stage `Task`s chain headlessly with `filter = nullptr` and `NullTaskStatus`; `OutputGenerator` is reached through `output::Task`; `ProjectWriter`/`ProjectReader` round-trip through `StageSequence`; `dewarping` links and runs without `core` or a Qt application. A 300 dpi page runs the whole chain in about 0.2 to 0.6 s in Release (3 s Debug, 9 s ASan), so the whole suite fits the "a minute or two" budget.

Four findings change the plan versus the research files and CONTEXT. (1) **The tests need `core/Application`, not a plain `QApplication`**: `DefaultParamsProfileManager` does `static_cast<Application*>(qApp)` and `DefaultParamsProvider` is reached from `deskew::Task` and the stage `Utils`. (2) **A stage chain on a page with no stored parameters silently does nothing** (select_content returned an empty content box) until each stage's defaults are loaded via `Filter::loadDefaultSettings` or the `Utils::buildDefault*` functions; the plan must do this explicitly. (3) **Release, Debug and ASan outputs were bit-identical** (0 differing pixels) for flat, spread, mixed and dewarped pages in both colour modes, so the measured spread for D-12 is zero on this machine; the research ceilings stay as the frozen limits and the comparator self-test (D-13) is what proves they bite. (4) **Several D-15 ground-truth tolerances do not match what the unmodified code does on the first synthetic fixtures** (page-split line is slanted 3.7 degrees for a 1 degree tilt; dewarp residual only drops 18.1 to 7.3 px at 600 dpi), so ground-truth tolerances need their own measurement step, and "tightened where the code does clearly better" must also allow "re-based where the research starting value is unreachable, with the measurement recorded".

Also verified: the special-character project round trip **passes** on the unmodified code for file and directory names (including `&`, `<`, `>`, `"`, `'`, NFC and NFD umlauts), so the SAFE-02 expected failure may not be needed; Boost 1.92 `expected_failures` is **silent when the test unexpectedly passes**, so the "Phase 6 removes the mark" step needs an explicit trip-wire helper; and the user's `de_DE.UTF-8` locale makes `atof("1.5")` return 1 after `QCoreApplication` starts, so the test environment must pin the locale.

**Primary recommendation:** Build `src/test_support/` (splitmix64, 5x7 glyph generator with hand-written trig, comparator, golden store, manifest), a `pipeline_tests` Boost.Test executable that creates a `core/Application` offscreen with isolated `QSettings` and a pinned locale, a manually assembled headless Task chain (`filter = nullptr`, explicit per-stage defaults), and `dewarping_tests`; wire only through `src/CMakeLists.txt` inside `if (BUILD_TESTS)`; record goldens from `macos-release` in one baseline commit before any strip.

## Architectural Responsibility Map

This phase adds only test code; "tiers" are the existing layers, and the question is where each test capability belongs so layering is not inverted.

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Synthetic page generation, PRNG, glyph table | `test_support` (new, depends on `Qt::Gui` only) | `imageproc_tests`, `dewarping_tests`, `pipeline_tests` link it | Lower-layer tests must reach the generator without linking `core` |
| Image comparison, golden store, manifest, text goldens | `test_support` | `pipeline_tests` | Pure QImage/file logic; no pipeline dependency |
| Binarization tests (TEST-08) | `imageproc` tests | `test_support` | Existing `imageproc_tests` executable |
| Distortion model and dewarper tests (TEST-09) | `dewarping/tests` (new executable) | `test_support` | `dewarping` has no tests; needs no Qt application |
| Stage decisions and output goldens (TEST-05/06/07) | `core` tier via `pipeline_tests` | `test_support` | Stage `Task`s and `output::Task` live in `core` filters |
| Project round trip (TEST-10) | `core` (`ProjectWriter`/`Reader`, `StageSequence`) via `pipeline_tests` | — | Needs `Filter` objects, hence an `Application` |
| Settings isolation, locale, offscreen platform | `pipeline_tests` global fixture + CMake `set_tests_properties` | — | Process-wide singletons (`ApplicationSettings`, `DefaultParamsProvider`) |
| Test registration and the single `ctest` | CMake (`src/CMakeLists.txt` test block, `CMakePresets.json` test presets) | — | D-16 allows wiring only |

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Boost.Test (dynamic) | 1.92.0 | All new C++ tests, `expected_failures`, global fixture | Already the project's framework; `BOOST_TEST_DYN_LINK` is already defined by the top-level CMake `[VERIFIED: /Users/christoph/Developer/scantailor-advanced/CMakeLists.txt "add_definitions(-DBOOST_TEST_DYN_LINK)"]`; versions `[VERIFIED: .planning/phases/01-deletion-free-macos-build/records/toolchain.txt "boost 1.92.0"]` |
| Qt 6 (Core, Gui, Widgets, Xml) | 6.12.0 (`qtbase 6.12.0_1`) | `QImage`, `QSettings`, `QDomDocument`, `Application` | The code under test; `qtbase` now `6.12.0_1` `[VERIFIED: records/closing-check.md]` |
| CTest / CMake | 4.4.4 | One `ctest` command, presets | `[VERIFIED: records/toolchain.txt "cmake version 4.4.4"]`; `cmakeMinimumRequired 3.25` in `CMakePresets.json` |
| Apple Clang | 21.0.0 (Xcode 27.1) | Compiler; `-ffp-contract=off` frozen | `[VERIFIED: records/fp-contract.md, records/toolchain.txt]` |
| libtiff / libpng / jpeg-turbo | 4.7.2 / 1.6.59 / 3.2.0 | Image I/O inside the code under test | `[VERIFIED: records/closing-check.md "brew list --versions"]` |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `QCryptographicHash` (Qt) | 6.12 | SHA-256 fingerprint of generated fixture pixels | Instead of committing input PNGs (Claude's-discretion storage choice) |
| `QProcess` (Qt) | 6.12 | `git rev-parse HEAD` for the manifest at update time only | Manifest source commit |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Own splitmix64 PRNG | `std::mt19937` | mt19937 output is fixed by the standard, but `std::uniform_*_distribution` is not (libc++ vs libstdc++); splitmix64 is 6 lines and has no such trap. Use splitmix64 with an explicit modulo. |
| Generate inputs in memory + SHA-256 fingerprint file | Commit input PNGs | A 300 dpi RGB fixture PNG is 0.7 MB and a 600 dpi one 2.6 MB `[VERIFIED: spike S1 ls]`; fingerprints keep the repo small and still catch generator drift |
| Manual Task chain (typed access to `Settings`) | `StageSequence` + `createTask` + XML parse | `Filter` keeps its `Settings` private `[VERIFIED: src/core/filters/*/Filter.h]`; the manual chain gives typed reads. Use `StageSequence` only for TEST-10 and one cross-check. |

**Installation:** none. Nothing new is installed; all dependencies are already in the Phase 1 build (`brew list --versions` recorded). No new CMake `find_package` is needed: `Qt::Core Qt::Gui Qt::Widgets Qt::Xml Qt::Test` and `Boost::unit_test_framework` are already found by the top-level CMake.

**Version verification:** versions above come from the Phase 1 records, re-read this session (`closing-check.md` shows `qtbase 6.12.0_1`, others unchanged since the first install).

## Package Legitimacy Audit

No external packages are added or installed in this phase; every library is already part of the Phase 1 build. `gsd_run query package-legitimacy check` was therefore not run.

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| (none) | — | — | — | — | — | — |

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

## Architecture Patterns

### System Architecture Diagram

```
 seed + scenario spec
        |
        v
 test_support::PageGenerator  --(QImage RGB32, dpi in metadata)--+--> GroundTruth{angle, content box, split x,
 (splitmix64, 5x7 glyphs,                                        |     picture rect, line rows, curl amplitude}
  hand-written sin/cos,                                          |
  own rotation/warp sampler)                                     v
        |                                            fingerprint test (SHA-256 of pixels)
        v
 pipeline_tests global fixture: core/Application (offscreen), LC_ALL pinned, QSettings -> temp INI
        |
        v
 HeadlessPipeline (per scenario, fresh Settings objects, temp in/out dirs, out/cache pre-created)
   write in.png -> fix_orientation::Task -> page_split::Task -> deskew::Task -> select_content::Task
        -> page_layout::Task -> output::Task (real ThumbnailPixmapCache in temp dir) -> out TIFF
        |                                                        |
        | reads typed Settings                                   | ImageLoader::load(out TIFF)
        v                                                        v
 stage decisions (angle, split cutter,                    output QImage (Mono or RGB32)
  content rect, page rect, margins, size, dpi)                   |
        |                                                        |
        +--> ground-truth assertions (truth first) --------------+--> sanity checks (D-05: not blank, size,
        |                                                        |        content where truth says)
        +--> TextGolden compare (0.01 deg / 1 px)                +--> GoldenStore::compare(kind tolerance)
                                                                      |-- pass: nothing written
                                                                      |-- fail: expected/actual/diff PNG ->
                                                                      |         build/<preset>/test-artifacts/<test>/
                                                                      '-- UPDATE_GOLDENS=1 and sanity ok:
                                                                            rewrite failing golden + manifest
```

### Recommended Project Structure

```
src/
├── CMakeLists.txt                 # only wiring edit: add_subdirectory(test_support / pipeline_tests / dewarping/tests) inside the existing if (BUILD_TESTS)
├── test_support/                  # NEW static lib -> Qt::Gui, Qt::Core (+ imageproc only if needed)
│   ├── CMakeLists.txt
│   ├── Rng.h                      # splitmix64
│   ├── BitmapFont.{h,cpp}         # 5x7 glyph table (ASCII art strings)
│   ├── PageGenerator.{h,cpp}      # flat / spread / mixed / curved + GroundTruth
│   ├── ImageCompare.{h,cpp}       # metrics + Tolerance constants (one set per kind, one file)
│   ├── GoldenStore.{h,cpp}        # compare / UPDATE_GOLDENS / artifacts / manifest
│   ├── TextGolden.{h,cpp}         # "key = value" files, QString::number formatting
│   ├── Manifest.{h,cpp}
│   └── TestEnvironment.{h,cpp}    # settings isolation, locale pin, temp dirs
├── pipeline_tests/                # NEW Boost.Test exe -> core, test_support
│   ├── CMakeLists.txt
│   ├── main.cpp                   # BOOST_TEST_MODULE + BOOST_GLOBAL_FIXTURE (core/Application)
│   ├── HeadlessPipeline.{h,cpp}
│   ├── TestGeneratorDeterminism.cpp   # same seed twice, fingerprints
│   ├── TestComparatorSelfTest.cpp     # D-13
│   ├── TestFlatDocument.cpp / TestBookSpread.cpp / TestMixedPage.cpp
│   ├── TestProjectRoundTrip.cpp
│   └── golden/                    # PNG + *.txt + MANIFEST.txt
├── dewarping/tests/               # NEW (wired from src/CMakeLists.txt, so dewarping/CMakeLists.txt stays untouched)
│   ├── CMakeLists.txt  main.cpp  TestDistortionModel.cpp  TestCylindricalDewarper.cpp  TestTextLineTracer.cpp
└── imageproc/tests/               # EXISTING: TestBinarize.cpp revived; TestScale.cpp expectation fixed (test code only)
```

### Pattern 1: Deterministic font-free generator

**What:** Pages are described by an `ink(x, y)` predicate on flat coordinates plus an inverse-mapping sampler (3x3 supersampling) that applies rotation or warp. Text is rows of 5x7 glyph cells scaled by `cell` px (cell 5 at 300 dpi, 10 at 600 dpi gives about 3 mm x-height). Randomness comes only from splitmix64; paper noise is a pure hash of `(x, y, seed)`.
**When to use:** all fixtures.
**Rules (each verified or measured in the spikes):**
- Use own splitmix64 and an explicit `% n`. No `<random>` distributions, no `rand()`.
- **Do not call libm `sin`/`cos`** for the rotation; use a hand-written series. A Taylor series with 12 terms is exact for the small angles used (|a| under 0.1 rad), but **diverges for large arguments**: the first picture prototype used `dsin(14*u)` and produced garbage (rainbow noise). Keep series arguments small or range-reduce, and use value noise (bilinear on a hash grid) for pictures, not trig. `[VERIFIED: spike S5 first prototype]`
- Set DPI via `QImage::setDotsPerMeterX/Y(int(dpi / 0.0254 + 0.5))`; `FilterData(const QImage&)` reads it with `Dpm(image)` `[VERIFIED: src/core/FilterData.cpp "m_xform(image.rect(), Dpm(image))"]`.
- Keep paper noise small and smooth enough for PNG: colour output goldens were about 1.1 to 1.2 MB each at 1680x1748 `[VERIFIED: spike S1/S6 ls]`; budget roughly 15 MB for about a dozen colour goldens, B/W goldens are 13 to 20 KB.
- For a dewarp fixture avoid empty text strips (spaces are 1 in 6 per glyph in the prototype; a 150 px strip held all 16 lines in every strip) or use a space-free block.
- Use `QImage::Format_RGB32`; compare and fingerprint after explicit conversion.

**Example** (prototype used in the spikes, trimmed):
```cpp
// Source: scratchpad spike gen.h (not committed); deterministic, no libm, no <random>
struct Rng {
  uint64_t s;
  explicit Rng(uint64_t seed) : s(seed) {}
  uint64_t next() {
    uint64_t z = (s += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
  }
  uint32_t below(uint32_t n) { return uint32_t(next() % n); }
};
```

### Pattern 2: Headless Task chain with explicit defaults

**What:** Build the six `Task`s from the last stage back, `filter = nullptr`, owning the `Settings` objects, exactly like `MainWindow::createCompositeTask` `[VERIFIED: src/app/MainWindow.cpp:2059-2101]`, and **load each stage's defaults before processing**.
**Constructor signatures (read this session, verbatim argument order):**
```cpp
// fix_orientation::Task(const PageId&, std::shared_ptr<Filter>, std::shared_ptr<Settings>,
//                       std::shared_ptr<ImageSettings>, std::shared_ptr<page_split::Task>, bool batchProcessing)
// page_split::Task(filter, settings, std::shared_ptr<ProjectPages>, std::shared_ptr<deskew::Task>,
//                  const PageInfo&, bool batchProcessing, bool debug)
// deskew::Task(filter, settings, std::shared_ptr<ImageSettings>, std::shared_ptr<select_content::Task>,
//              const PageId&, bool batchProcessing, bool debug)
// select_content::Task(filter, std::shared_ptr<page_layout::Task>, settings, const PageId&, bool batch, bool debug)
// page_layout::Task(filter, std::shared_ptr<output::Task>, settings, const PageId&, bool batch, bool debug)
// output::Task(filter, settings, std::shared_ptr<ThumbnailPixmapCache>, const PageId&,
//              const OutputFileNameGenerator&, ImageViewTab lastTab, bool batch, bool debug)
// ThumbnailPixmapCache(const QString& thumbDir, const QSize& maxSize, int maxCachedPixmaps, int expirationThreshold)
```
`[VERIFIED: src/core/filters/*/Task.h and src/core/ThumbnailPixmapCache.h, read this session]`. Entry call: `fixTask->process(NullTaskStatus(), FilterData(image))`. `ImageViewTab` is `output::TAB_OUTPUT` (enum `{ TAB_OUTPUT, TAB_PICTURE_ZONES, TAB_FILL_ZONES, TAB_DEWARPING, TAB_DESPECKLING }` `[VERIFIED: src/core/filters/output/ImageViewTab.h:8]`).

**Default loading (required, else nothing is computed):**
```cpp
// Source: spike S1 (works); same calls as each Filter::loadDefaultSettings
fixS->applyRotation(imageId, fix_orientation::Utils::getDefaultOrthogonalRotation());
splitS->updatePage(imageId, page_split::Utils::buildDefaultUpdateAction());   // or set SINGLE_PAGE_UNCUT explicitly
deskS->setPageParams(pageId, deskew::Utils::buildDefaultParams());
selS->setPageParams(pageId, select_content::Utils::buildDefaultParams(Dpi(dpi, dpi)));
layS->setPageParams(pageId, page_layout::Utils::buildDefaultParams(Dpi(dpi, dpi)));
outS->setParams(pageId, output::Utils::buildDefaultParams());
```
Verbatim evidence for the trap: `select_content::Task::process` initialises `bool needUpdateContentBox = false; bool needUpdatePageBox = false;` and only calls `deps.compatibleWith(...)` when `params` is non-null, so a page with no stored params is never detected `[VERIFIED: src/core/filters/select_content/Task.cpp:~85-95]`; the spike without defaults returned `content 0,0 0x0` and a 472x236 white output.

**Read results** (all typed, all public): `deskew::Settings::getPageParams(pageId)->deskewAngle()`; `page_split::Settings::getPageRecord(imageId).params()->pageLayout()` (`type()`, `numCutters()`, `cutterLine(i)`); `select_content::Settings::getPageParams(pageId)` (`contentRect()`, `pageRect()`); `output::Settings::getParams`, plus `ImageLoader::load(names.filePathFor(pageId), 0)` for the output pixels.

**Explicit output parameters:** set `output::ColorParams` mode (`BLACK_AND_WHITE`, `COLOR_GRAYSCALE`, `MIXED`), `outS->setDespeckleLevel(pageId, double)`, `outS->setDewarpingOptions(pageId, DewarpingOptions(AUTO))` and the **output DPI** (`setDpi`): the default output DPI is not the input DPI (spike output at 300 dpi input was 1680x1748, i.e. 600 dpi) `[VERIFIED: spike S1 output size]`. Set it explicitly so the text golden is intentional.

### Pattern 3: Ground truth first, goldens second; per-kind comparison

Truth assertions (angle, content box, split, straightness) run first; the image golden is the net for pixels with no analytic truth. Comparator metrics per kind (limits frozen in one header, D-12): bi-level = edge-tolerant XOR fraction + foreground connected-component count; grey/colour = MAE and max error; geometry/text goldens = numeric tolerance. Compare after decoding (never TIFF bytes). The measured Release/Debug/ASan spread was exactly zero (spike S6), so keep the research ceilings (0.1 %; MAE 0.5, max 8) as the frozen constants and prove they bite with the D-13 self-test. Failed comparisons: write `expected.png`, `actual.png`, `diff.png` to `${CMAKE_BINARY_DIR}/test-artifacts/<test>/` and print metric, value, limit and folder (D-08).

### Pattern 4: Golden store and manifest

- Pass the golden directory and artifact directory as compile definitions from CMake (`CMAKE_CURRENT_SOURCE_DIR`, `CMAKE_BINARY_DIR`); never relative paths (PITFALLS Pitfall 5).
- `UPDATE_GOLDENS=1` (D-06): rewrite a golden only if its comparison failed **and** its sanity checks passed (D-05); passing goldens stay untouched (D-07). Refresh `MANIFEST.txt` on every update run.
- Manifest sources, all obtainable without new dependencies: `qVersion()` and `QT_VERSION_STR` (Qt); `BOOST_LIB_VERSION` (Boost); `TIFFGetVersion()` (libtiff); `PNG_LIBPNG_VER_STRING` (libpng); `LIBJPEG_TURBO_VERSION` or `JPEG_LIB_VERSION` (jpeg-turbo) `[ASSUMED: macro names]`; `__clang_version__` (Apple Clang); CMake, macOS, Xcode and the `-ffp-contract` value as CMake-time compile definitions (`CMAKE_VERSION`, `CMAKE_HOST_SYSTEM_VERSION`, a literal matching the `-ffp-contract=off` line in `CMakeLists.txt`); source commit via `git rev-parse HEAD` at update time. On a toolchain mismatch print the differing keys once per process and continue (D-09).
- Text golden format (open in CONTEXT): sorted `key = value` lines, numbers formatted with `QString::number(v, 'f', 4)` (locale independent), so `git diff` reads as numbers.

### Pattern 5: Expected failures with a trip-wire

`expected_failures` in Boost 1.92 is **silent when the case passes** `[VERIFIED: spike S9, test "xfail_but_passes" printed "*** No errors detected"]`, so Phase 6 would never be told to remove the mark. Write one helper, for example `XFAIL_CHECK(condition, "SAFE-02: <reason>")`, that logs `KNOWN DEFECT SAFE-02: ...` when the condition is false and raises `BOOST_ERROR("expected failure now passes: remove the mark for SAFE-02")` when it is true. Name the fixing requirement in the message (D-18). Use it for every D-18 mark and for the `TestScale` fallback branch.

### Pattern 6: CMake wiring that keeps the non-test `src/` diff empty

`src/CMakeLists.txt` already contains `if (BUILD_TESTS) add_subdirectory(qt_tests) endif()` `[VERIFIED: src/CMakeLists.txt]`. Add `add_subdirectory(test_support)`, `add_subdirectory(pipeline_tests)` and `add_subdirectory(dewarping/tests)` there. Then `src/dewarping/CMakeLists.txt`, `src/imageproc/CMakeLists.txt` and `src/core/CMakeLists.txt` stay untouched, and the only non-test `src/` diff is those three lines. `imageproc_tests` can link `test_support` by target name even though it is defined later (resolved at generate time). New executables mirror the existing ones, for example `imageproc_tests`: `remove_definitions(-DBUILDING_IMAGEPROC)`, `target_link_libraries(... PRIVATE imageproc math foundation ${Qt_Widgets_lib} ${Qt_Xml_lib} Boost::unit_test_framework ${EXTRA_LIBS})` `[VERIFIED: src/imageproc/tests/CMakeLists.txt]`. `dewarping_tests` **needs `${Qt_Xml_lib}`**: linking `dewarping imageproc math foundation Qt::Gui Qt::Core` without Xml failed with undefined `QDomElement` symbols `[VERIFIED: spike S8 link error]`.

### Anti-Patterns to Avoid

- **Plain `QApplication` in the fixture:** `DefaultParamsProfileManager::DefaultParamsProfileManager()` does `auto* app = static_cast<Application*>(qApp); if (app->isPortableVersion())` `[VERIFIED: src/core/DefaultParamsProfileManager.cpp:16-22]`. Create `Application` from `core/Application.h`.
- **Running a chain without loading defaults** (empty content box, white output).
- **`std::atof`/`strtod`/iostream number parsing or printing in tests** (locale; see Pitfall P5).
- **Relying on `expected_failures` alone** (silent pass).
- **Asserting the AUTO page split on the mixed page** (see P3).
- **Using `imageproc::transform` or `dewarping::` to build fixtures** (CONTEXT).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| TIFF/PNG decoding for comparison | A reader | `ImageLoader::load(path, 0)` and `QImage` | Already used by `output::Task` itself; decode before compare, never compare bytes |
| SHA-256 of fixture pixels | A hash | `QCryptographicHash::Sha256` on raw `QImage` bytes | Standard, no dependency |
| Project XML parse/escape | Custom escaping | `QDomDocument` + `ProjectReader` | Escaping is done by Qt DOM; round trip verified (S7) |
| Non-default parameters per stage | Hand-written XML | `DefaultParams` setters + `DefaultParamsProvider::setParams(std::unique_ptr<DefaultParams>, name)` + `Filter::loadDefaultSettings` `[VERIFIED: src/core/DefaultParams.h setters; src/core/DefaultParamsProvider.cpp setParams]` | Public API already used by the app's dialog; every stage gets non-default values |
| Temp directories | Manual cleanup | `QTemporaryDir` | Removed on scope exit |
| Test-per-seed expected-failure logic | Boost-only decorators | Pattern 5 helper | Boost is silent on unexpected pass |
| Straightness metric | A new algorithm in production code | A test-side line-band fit (per-strip band centres, residual from best-fit line) | Prototype in S10: 16 lines found in each of 7 strips |

**Key insight:** every behaviour the suite needs to observe is already reachable through public headers; the hard part is setup (defaults, application object, locale, isolation), not new production hooks.

## Runtime State Inventory

Not a rename/refactor/migration phase. Omitted. (For completeness, relevant runtime state for tests: `ApplicationSettings` and `DefaultParamsProvider` are process-wide singletons; `QSettings` and the profile directory are the only persisted state, covered by isolation below.)

## Common Pitfalls

### P1: Stage chain silently does nothing without defaults
**What goes wrong:** content box 0x0, output 472x236 white.
**Why:** `Task::process` computes only when params exist (see Pattern 2).
**How to avoid:** load defaults per stage, and add a sanity assertion that the content rect is valid and the output is not blank before any golden is recorded (D-05).
**Warning signs:** `contentRect().isValid()` false; output smaller than input.

### P2: Plain `QApplication` crashes or misbehaves in `DefaultParamsProvider`
See Anti-Patterns. The `Application` constructor also scans translation dirs and fonts (`:/fonts`); with `PORTABLE_CONFIG_DIR ""` (preset `PORTABLE_VERSION=OFF`) it is not portable `[VERIFIED: build/macos-release/config.h]`. Creating a `StageSequence` prints many `qt.svg: Cannot open file ':/icons/...'` warnings because the icon resources live in `app` `[VERIFIED: spike S7 stderr; src/resources/resources.qrc exists under src/resources]`. They are harmless; silence with `QT_LOGGING_RULES="qt.svg=false"` in the test environment if the output is noisy.

### P3: `page_split` AUTO is unreliable on the synthetic mixed page
Spike S5: with the default (AUTO) layout the text-plus-picture page produced `SINGLE_PAGE_CUT` (`type()==1`), cropped the first text column and **dropped the picture** from the output; forcing `SINGLE_PAGE_UNCUT` gave the correct output (picture kept in colour, text B/W, content 600x674). The flat page and the spread were detected correctly under AUTO. **Recommendation:** force `page_split::SINGLE_PAGE_UNCUT` for the mixed scenario (and keep AUTO for flat and spread, where it works), and record the AUTO mis-cut as a findings-file observation rather than an expected failure unless the planner decides the heuristic's behaviour is a defect. This is the "never change the fixture to dodge a defect" boundary of D-18: say explicitly in the findings file that the AUTO case was observed and why it is not asserted.

### P4: Research ground-truth tolerances do not hold on the first fixtures (measured)
- **Page split:** 1.0 degree tilt, spread 1654x1181 at 300 dpi: layout `TWO_PAGES` (`type()==2`), one cutter `795.519,0 -> 872.992,1182` (slanted about 3.7 degrees). At mid-height x is about 834 against a gap centre near 827: 7 px off, so "within ±2 px" fails while the line is still well inside the 227 px inter-page gap. Assert instead that the cutter lies **inside the gap between the two text blocks at every y** and layout is two-page; record the exact line in the text golden.
- **Deskew:** applied 0.5, 1.5, 2, -1.5, -3 degrees gave stored angles `-0.5, -1.625 (one run) / -1.5 (others), -2.0625, 1.5, 3`: errors 0 to 0.125 degrees; stored sign is the negative of the generator's applied angle. ±0.15 holds but with little margin; do not tighten below 0.15 without a sweep.
- **Content box:** detected `126,224 604x756` against truth `600x750` (about 2 to 3 px larger per side); the box lives in the deskewed frame, so compare size and centre-relative-to-frame, or IoU, not raw origin.
- **Dewarp residual:** analytic parabolic vertical-shear warp, amplitude 25 px at 300 dpi, output at 600 dpi: max band-centre residual 18.1 px without dewarping, **7.3 px with AUTO dewarp** (about 3.6 px at 300 dpi equivalent). The research bound "≤ 1.5 px at 300 dpi" is not met by this fixture. **Recommendation:** assert the ratio `residual_out / residual_in <= 0.5` (requirement wording is "straightens") plus an absolute bound set from the measurement; if a more cylinder-like warp does better, say so; if the unmodified code cannot reach the research bound, D-18 applies (golden records current behaviour; mark the bound check as an expected failure naming the shortfall; no requirement fixes it, so the backlog item stands alone).
All of these are `[VERIFIED: scratchpad spikes S1, S4, S6; numbers reproducible]`; they feed "tightened wherever the code does clearly better" in D-15 and require a short **ground-truth measurement step** in addition to the Release/Debug spread spike.

### P5: Locale makes number parsing and printing differ
The user's shell has `LANG="de_DE.UTF-8"` `[VERIFIED: spike `locale` output]`; after `QCoreApplication` starts, `atof("1.5")` returned **1** (comma decimal), and `LC_ALL=en_US.UTF-8` gave 1.5 `[VERIFIED: spike S11]`. `LC_ALL=C` works for `atof` but Qt prints "Detected locale "C" ... not UTF-8" warnings. **Fix:** `set_tests_properties(... ENVIRONMENT "LC_ALL=en_US.UTF-8")`, and in test code use `QByteArray::toDouble()`/`QString::number`, never `atof`/`printf`.

### P6: Hand-rolled trig diverges (see Pattern 1)

### P7: Boost `expected_failures` silent pass (see Pattern 5)

### P8: Singletons leak between tests
`DefaultParamsProvider` and `ApplicationSettings` persist for the process. If TEST-10 installs a non-default `DefaultParams` profile, restore the original (`createDefaultProfile`) in a fixture teardown, or run TEST-10 in its own executable or run order. Keep scenario Settings objects fresh per scenario; `page_layout::Settings` documents aggregate-size logic across pages `[CITED: src/core/filters/page_layout/Settings.h doc comment "Soft margins ... extend the page to match its size with other pages"]`, so one page per Settings object avoids cross-scenario coupling `[ASSUMED: effect not measured]`.

### P9: Thumbnail cache and event loop
`output::Task` calls `m_thumbnailCache->recreateThumbnail(...)` unconditionally `[VERIFIED: src/core/filters/output/Task.cpp:~425]`. A real `ThumbnailPixmapCache` in a temp dir worked without `exec()` in all spikes; destroy it before the `Application` (scope order). In Debug builds its `assert(QCoreApplication::instance()->thread() ...)` lines are active; Debug runs passed (S6).

### P10: `$OUT/cache` must exist
`QDir().mkdir(automaskDir)` deliberately fails when the parent does not exist `[VERIFIED: src/core/filters/output/Task.cpp comment "Note that QDir::mkdir() will fail if the parent directory, that is $OUT/cache doesn't exist"]`, which makes `invalidateParams` drop stored output params. Create `out/cache` up front (done in all spikes).

### P11: UBSan will report new findings (expected, D-19)
The one-off ASan/UBSan run of the pipeline paths printed `left shift of negative value` at `src/core/Despeckle.cpp:368, 392, 418, 446` and `shift exponent -367 is negative` at `src/core/EstimateBackground.cpp:243` `[VERIFIED: spike S12, macos-asan binary]`; none appeared in Phase 1's record. Record them in a new findings file in Phase 1's format; do not fix; the outputs were nevertheless identical to Release.

### P12: `ScaleTestSuite/test_random_image` (D-17 evidence)
`scaleToGray()` preserves the mean (50x50: 127.92) while Qt 6 `QImage::scaled(SmoothTransformation)` is biased about 1.1 lower (126.81); for the exact 2x case the difference is up to 3 levels, for 80x80 up to 28 levels with 5131 of 6400 pixels off by more than 1 `[VERIFIED: spike S13, imageproc static lib]`. A constant image gives equal results (100 vs 100). So the **Qt reference side moved/differs**: D-17's first branch applies. Fix in test code: compare the 50x50 (exact 2x) case against an own box-average reference with tolerance 1, and replace the 80x80 comparison with a mean-error bound or an own reference; no production edit.

### P13: Non-test diff proof must tolerate CMake wiring
`git diff cde6825 -- src` restricted to non-test paths is currently empty `[VERIFIED: `git diff --stat cde6825 HEAD -- src CMakeLists.txt CMakePresets.json` printed nothing]`. After wiring, `src/CMakeLists.txt` differs by design. Define the proof as: no `*.cpp`, `*.h`, `*.ui`, `*.qrc` change outside `src/**/tests/`, `src/test_support/`, `src/pipeline_tests/`, `src/qt_tests/`; and the `src/CMakeLists.txt` hunk contains only the three `add_subdirectory` lines inside `if (BUILD_TESTS)`; and the top-level `CMakeLists.txt` and the production `add_compile_options` are unchanged.

## Code Examples

### Global fixture (Boost.Test, offscreen `Application`, isolated settings)
```cpp
// Source: pattern verified in spike S1/S7 (works); ARCHITECTURE.md Test fixture contract adapted
#define BOOST_TEST_MODULE pipeline_tests
#include <Application.h>            // core/Application.h, NOT QApplication
#include <QSettings>
#include <QTemporaryDir>
#include <boost/test/unit_test.hpp>

struct GlobalFixture {
  GlobalFixture() {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    qputenv("LC_ALL", "en_US.UTF-8");                     // before the application object
    QCoreApplication::setOrganizationName("scantailor-tests");
    QCoreApplication::setApplicationName("pipeline_tests");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_tmp.path() + "/settings");
    static int argc = 1;
    static char arg0[] = "pipeline_tests";
    static char* argv[] = {arg0, nullptr};
    m_app = std::make_unique<Application>(argc, argv);    // after settings path, before ApplicationSettings::getInstance()
    // assert QSettings().fileName() starts with m_tmp.path()
  }
  QTemporaryDir m_tmp;
  std::unique_ptr<Application> m_app;
};
BOOST_GLOBAL_FIXTURE(GlobalFixture);
```
`QSettings().fileName()` resolved to `<tmp>/scantailor-tests/<app>.ini` in the spike `[VERIFIED: spike S11]`. Plain `std::make_unique<GlobalFixture>` lifetime ordering: members destroyed in reverse declaration order, so the application object goes before the temp dir.

### Expected-failure helper (trip-wire)
```cpp
// Source: design for Pattern 5; Boost 1.92 unexpected-pass behaviour verified in spike S9
#define XFAIL_CHECK(cond, tag)                                                          \
  do {                                                                                  \
    if (!(cond)) {                                                                      \
      BOOST_TEST_MESSAGE("KNOWN DEFECT " << (tag));                                      \
    } else {                                                                            \
      BOOST_ERROR("expected failure now passes, remove the mark: " << (tag));          \
    }                                                                                   \
  } while (0)
```

### CTest registration with environment
```cmake
# Source: existing registration style in src/imageproc/tests/CMakeLists.txt; environment per PITFALLS Pitfall 5
add_test(NAME pipeline_tests COMMAND pipeline_tests --log_level=message)
set_tests_properties(pipeline_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LC_ALL=en_US.UTF-8;QT_LOGGING_RULES=qt.svg=false")
```

### Project round trip (verified shape)
```cpp
// Source: spike S7 (passes on unmodified code); uses only existing public API
// pages  = std::make_shared<ProjectPages>(std::vector<ImageInfo>{...}, Qt::LeftToRight);
// stages = StageSequence(pages, PageSelectionAccessor(providerStub));      // needs a PageSelectionProvider stub
// for each page and filter: filter->loadDefaultSettings(pageInfo);          // after DefaultParamsProvider::setParams(nonDefault)
// ProjectWriter(pages, SelectedPage(first, PAGE_VIEW), OutputFileNameGenerator(...)).write(f1, stages.filters());
// QDomDocument doc; doc.setContent(&file); ProjectReader r(doc, f1); StageSequence s2(r.pages(), ...);
// r.readFilterSettings(s2.filters());
// ProjectWriter(r.pages(), r.selectedPage(), OutputFileNameGenerator(r.namingDisambiguator(), r.outputDirectory(), r.pages()->layoutDirection())).write(f2, s2.filters());
// require f1 bytes == f2 bytes; require key non-default values present in f1
```
Escaping seen in the written file: `outputDirectory="out &amp; &lt;x>"`, `name="a&amp;b &lt;c> &quot;d&quot; 'e'.png"` `[VERIFIED: spike S7 proj.xml]`.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `BOOST_AUTO_TEST_MAIN` in each `main.cpp` | `BOOST_TEST_MODULE` + `BOOST_GLOBAL_FIXTURE` for new executables | Boost 1.92 still supports both | Keep old style in existing executables; new `pipeline_tests` uses the module style (compiled and ran in S9) |
| Byte-compare images | Decoded, per-kind tolerance comparison | Project decision (REQUIREMENTS "byte-exact goldens" anti-pattern) | Tolerances exist but measured spread is 0 here |
| `qrand`/`qsrand` in dewarping RANSAC | `QRandomGenerator prng(0)` active under Qt 6 | already in source | Reproducible per Qt build; record Qt version in manifest `[VERIFIED: CONTEXT code insights citing DistortionModelBuilder.cpp:165]` |

**Deprecated/outdated:** `rand()`-based helpers in `src/imageproc/tests/Utils.{h,cpp}` (do not reuse for the generator).

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Manifest macro names `LIBJPEG_TURBO_VERSION` / `JPEG_LIB_VERSION`, `TIFFGetVersion()`, `PNG_LIBPNG_VER_STRING`, `BOOST_LIB_VERSION` exist as written | Pattern 4 | Planner picks another source for a manifest line; low impact (compile error found at once) |
| A2 | One page per `page_layout::Settings` avoids cross-page size coupling | P8 | Page layout goldens could depend on scenario order if shared; mitigated by fresh objects either way |
| A3 | Release==Debug==ASan bit equality also holds for 600 dpi mixed/dewarp and every planned scenario | Pattern 3 | Tolerance spike (Wave 0) may show non-zero spread; ceilings are the fallback |
| A4 | A cylinder-like warp (rather than parabolic vertical shear) would straighten better than 7.3 px / 600 dpi | P4 | If not, the research bound of 1.5 px is simply unreachable; D-18 mark applies |
| A5 | Golden repository size of about 15 MB (colour PNGs) is acceptable to the user | Pattern 1 | User may prefer fewer colour goldens or smaller output DPI |
| A6 | `LC_ALL=en_US.UTF-8` exists on this Mac (it worked in the spike) and on any other target | P5 | Only this Mac is a target (CLAUDE.md), verified here |

## Open Questions

1. **Does the special-character failure SAFE-02 describes exist at all?**
   - Known: file and directory names with `&`, `<`, `>`, `"`, `'`, NFC and NFD umlauts round-trip byte-identically on unmodified code (S7). `XmlUnmarshaller::string` is only used for numbers/enums (`Dependencies.cpp:16`, grep this session), so its FIXME does not touch file names.
   - Unclear: other string-bearing fields (profile names, label text, free text) and non-default parameters in every stage; and what SAFE-02 will then fix.
   - Recommendation: write TEST-10 with real non-default parameters in every stage and the special-character set; **only add the expected-failure mark if it actually fails**; if it passes, record in the findings file that the SAFE-02 premise was not reproduced for paths and let Phase 6 decide.
2. **Which dewarp warp model gives a fair bound?** Parabolic shear gives 7.3 px (600 dpi). The planner should let the executor try a perspective/cylinder-like generator warp (not using `dewarping::`) before freezing the bound.
3. **Output DPI per scenario.** Default output DPI is not the input DPI. Planner decides whether output DPI follows input (300 to 300, 600 to 600) or is fixed; this sets the golden sizes.
4. **Where `ctest` environment lives.** Environment is proposed per test via `set_tests_properties`; the alternative (test-preset `environment`) misses plain `ctest` outside presets. Planner may do both.
5. **Qt Test `QEXPECT_FAIL`.** Not needed unless a failing check lands in `qt_tests`; none is expected.
6. **Needs an outside read (not done):** nothing required. If the manifest needs libjpeg/libpng macro spellings confirmed, read them from the build's own compile output rather than `/opt/homebrew` directly.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Apple Clang | build | ✓ | 21.0.0 (clang-2100.3.34.2) `[VERIFIED: records/closing-check.md]` | — |
| CMake / Ninja | build, ctest | ✓ | 4.4.4 / 1.13.2 | — |
| Qt 6 (Core, Gui, Widgets, Xml, Test) incl. offscreen plugin | all tests | ✓ | 6.12.0 (`qtbase 6.12.0_1`); offscreen platform worked in spikes and in Phase 1 | — |
| Boost.Test (dynamic) | all C++ tests | ✓ | 1.92.0 | — |
| `jq` | spike tooling only | ✓ | 1.7.1-apple | not needed by the final suite |
| Python 3 (no Pillow/NumPy) | none required | ✓ (bare) | — | write metrics in C++ (planned) |
| clang-format | style gate | ✗ | — | Format by hand per `.clang-format` (Phase 1 decision) |
| Existing build trees | fast iteration | ✓ | `build/macos-release`, `macos-debug`, `macos-asan` present | `cmake --preset ... --fresh` |

**Missing dependencies with no fallback:** none.
**Missing dependencies with fallback:** clang-format (manual formatting).

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Boost.Test 1.92 (dynamic) for all new tests; Qt Test unchanged in `qt_tests` |
| Config file | none; registration via `add_test` in each test `CMakeLists.txt`; presets in `/Users/christoph/Developer/scantailor-advanced/CMakePresets.json` (test presets `macos-release`, `macos-debug`, `macos-asan`, `noTestsAction: error`, `outputOnFailure: true`) |
| Quick run command | `ctest --preset macos-release -R pipeline_tests --output-on-failure` (or one binary with `--run_test=<Suite>/<case>`) |
| Full suite command | `cmake --build --preset macos-release && ctest --preset macos-release` (also `macos-debug` must pass; `macos-asan` once, findings recorded) |

Build tree locations: `build/macos-release`, `build/macos-debug`, `build/macos-asan`. Phase 1 proved `ctest --preset macos-release -R foundation_tests --output-on-failure`.

### Phase Requirements to Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| TEST-01 | Same seed twice gives identical pixels; no `rand()`/fonts; fingerprints match committed hashes | unit | `ctest --preset macos-release -R pipeline_tests` (`GeneratorDeterminismTestSuite`) | ❌ Wave 0 |
| TEST-02 | Altered golden (flip hundreds of px, shift 1 px, +10 grey) fails each metric and writes actual/diff PNGs | unit | `pipeline_tests --run_test=ComparatorSelfTestSuite` | ❌ Wave 0 |
| TEST-03 | Normal run never writes goldens; `UPDATE_GOLDENS=1` rewrites only failing goldens and refreshes `MANIFEST.txt`; toolchain mismatch warns | integration | `pipeline_tests --run_test=GoldenStoreTestSuite` (temp copy of golden dir) | ❌ Wave 0 |
| TEST-04 | Non-test `src/` identical to `cde6825`; baseline commit precedes strips | script/git | `git diff --stat cde6825 -- src ':!src/**/tests' ':!src/test_support' ':!src/pipeline_tests' ':!src/qt_tests'` shows only `src/CMakeLists.txt` with three `add_subdirectory` lines | ❌ Wave 0 (record in closing check) |
| TEST-05 | Deskew angle, content box, binarization vs truth, at 300 and 600 dpi | integration | `pipeline_tests --run_test=FlatDocumentTestSuite` | ❌ Wave 0 |
| TEST-06 | Split within gap, two sub-pages; dewarped residual ratio and golden | integration | `pipeline_tests --run_test=BookSpreadTestSuite` | ❌ Wave 0 |
| TEST-07 | Picture zone colour inside, pure B/W outside; colour/grey and despeckle goldens | integration | `pipeline_tests --run_test=MixedPageTestSuite` | ❌ Wave 0 |
| TEST-08 | Nine binarizers on synthetic ground truth (not `#if 0`) | unit | `ctest --preset macos-release -R imageproc_tests` | partial (file exists, `#if 0`) |
| TEST-09 | Distortion model valid, dewarper round trip < 1e-9 px, tracer builds a valid model, `RasterDewarper` output size | unit | `ctest --preset macos-release -R dewarping_tests` | ❌ Wave 0 |
| TEST-10 | write, read, write identical; non-default values present; special-character names | integration | `pipeline_tests --run_test=ProjectRoundTripTestSuite` | ❌ Wave 0 |
| TEST-11 | One `ctest` passes offscreen with isolated settings and pinned locale | system | `ctest --preset macos-release` | ❌ Wave 0 (environment wiring) |

### Sampling Rate
- **Per task commit:** `cmake --build --preset macos-release && ctest --preset macos-release -R "<new_or_changed_test>" --output-on-failure` (each under about 30 s in Release; a 300 dpi chain is 0.2 to 0.6 s).
- **Per wave merge:** `ctest --preset macos-release` and `ctest --preset macos-debug` (Debug chain measured 0.5 to 3 s per 300 dpi page).
- **Phase gate:** both presets green, then one `macos-asan` run (a 300 dpi chain took 1.6 to 9 s; expect a few minutes), findings recorded; `git diff` proof for TEST-04; full suite green before `/gsd-verify-work`.

### Wave 0 Gaps
- [ ] `src/test_support/` library (generator, comparator, golden store, manifest, text golden, environment)
- [ ] `src/pipeline_tests/` executable, global fixture, `HeadlessPipeline`
- [ ] `src/dewarping/tests/` executable
- [ ] Wiring lines in `src/CMakeLists.txt` (inside `if (BUILD_TESTS)`)
- [ ] `ENVIRONMENT` (offscreen, locale) for every new `add_test`, plus `qt_tests` (TEST-11 says offscreen)
- [ ] Measurement step 1 (D-12): Release vs Debug spread per image kind (already 0 on flat/spread/mixed/dewarp in spikes; repeat on the final fixtures)
- [ ] Measurement step 2 (new): ground-truth tolerance sweep (angle sweep, split gap, dewarp ratio) before freezing D-15 values
- [ ] Findings file for D-18/D-19 and a backlog entry for each expected failure

## Security Domain

`security_enforcement` is on (absent = enabled), ASVS level 1 per `.planning/config.json`. The phase adds test code only; no network, no authentication, no user input at runtime.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | — |
| V3 Session Management | no | — |
| V4 Access Control | no | — |
| V5 Input Validation | limited | Golden/artifact paths come from CMake compile definitions, never from test input; `UPDATE_GOLDENS` is a boolean read of one environment variable |
| V6 Cryptography | no | SHA-256 only as a fingerprint via `QCryptographicHash` (not a security control) |
| V12 Files and Resources | yes | Tests write only to `QTemporaryDir`, `build/<preset>/test-artifacts/`, and (under `UPDATE_GOLDENS=1` only) the golden directory; never outside the project |

### Known Threat Patterns for this stack

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Test run writes into the source tree unintentionally | Tampering | Writes gated by `UPDATE_GOLDENS=1` and by a sanity-check pass; compile-time golden dir (D-06, D-07) |
| Tests read the developer's real preferences or profiles | Information disclosure | `QSettings::setPath` to a temp dir, `QCoreApplication` names set, assertion on `QSettings().fileName()`; profile name stays `Default` so no profile directory is read `[VERIFIED: spike printed profile=Default]` |
| Personal scans leak into the repo | Information disclosure | Only generated images; inputs are not committed (fingerprints only) |
| Temp-dir collisions or leftover files | Tampering | `QTemporaryDir` per test, removed on scope exit |

## Sources

### Primary (HIGH confidence)
- Repository source read this session: `src/core/filters/{fix_orientation,page_split,deskew,select_content,page_layout,output}/Task.{h,cpp}`, `src/core/{FilterData,ProjectPages,ProjectWriter,ProjectReader,StageSequence,Application,ApplicationSettings,DefaultParams*,ThumbnailPixmapCache,OutputFileNameGenerator}.*`, `src/foundation/{XmlUnmarshaller,XmlMarshaller,TaskStatus}.*`, `src/dewarping/*.h`, `src/imageproc/{Binarize.h,tests/*}`, `src/CMakeLists.txt`, top-level `CMakeLists.txt`, `CMakePresets.json`, `build/macos-release/{config.h,build.ninja,compile_commands.json}`
- Phase 1 records: `records/toolchain.txt`, `ctest-summary.md`, `closing-check.md`, `fp-contract.md`, `deferred-items.md`, `sanitizer-findings.md`
- Planning inputs: `02-CONTEXT.md`, `REQUIREMENTS.md`, `STATE.md`, `.planning/research/{ARCHITECTURE,PITFALLS,SUMMARY,FEATURES}.md`, `.planning/codebase/CONCERNS.md`
- Spikes in `/private/tmp/claude-501/-Users-christoph-Developer-scantailor-advanced/b9f55f70-f82c-4f49-b1ff-308d5faf0317/scratchpad/spike/` (not part of the repo): S1 flat chain; S4 spread; S5 mixed; S6 dewarp chain; S6b Release/Debug/ASan bit comparison; S7 project round trip; S8 dewarping library without core; S9 Boost `expected_failures`; S10 straightness metric; S11 locale and `QSettings`; S12 ASan/UBSan on pipeline; S13 `scaleToGray` vs `QImage::scaled`

### Secondary (MEDIUM confidence)
- None used.

### Tertiary (LOW confidence)
- Macro names for the manifest (A1) from memory of the libraries' headers; confirm at compile time.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH, nothing new; all versions re-read from Phase 1 records.
- Architecture: HIGH, seams read in source and exercised end to end.
- Pitfalls: HIGH for the ones measured (P1, P2, P3, P4, P5, P7, P11, P12); MEDIUM for P8, P9.
- Tolerance numbers: MEDIUM, measured on this toolchain with the first fixtures; the planner must schedule the measurement steps above rather than trust them as final.

**Research date:** 2026-10-23
**Valid until:** 2026-11-22 for source facts (they change only with a `brew upgrade` of Qt/Boost, which the manifest tracks); tolerance numbers should be re-measured on the final fixtures.
