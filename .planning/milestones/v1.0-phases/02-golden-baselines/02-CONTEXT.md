# Phase 2: Golden Baselines - Context

**Gathered:** 2026-10-23
**Status:** Ready for planning

<domain>
## Phase Boundary

The phase adds a synthetic-image test suite and records golden baselines from code whose non-test `src/` is identical to the Phase 1 baseline commit `cde6825`. It delivers a seeded, font-free page generator (flat document, book spread, mixed text-and-image page, each with known ground truth); an image comparator with per-kind tolerances; a golden store with an explicit update flag and a toolchain manifest; ground-truth and golden tests for flat documents, book spreads and mixed pages; the revived `TestBinarize`; new dewarping unit tests; a project-file round trip; and one `ctest` command that runs the whole suite offscreen with isolated settings and passes. Requirements: TEST-01 to TEST-11.

Not in this phase: fixing any defect the tests uncover (SAFE-02 and others are recorded, not fixed), removing Qt 5, Windows or Linux code (Phase 3), and any production refactor or test hook in production code.

</domain>

<decisions>
## Implementation Decisions

### Fixtures and coverage
- **D-01:** Fixtures exist at both 300 dpi and 600 dpi, with the DPI set explicitly in the image metadata. The user scans at both resolutions. Every full-coverage scenario (D-03) runs at both DPIs.
- **D-02:** Fixtures are generated as 24-bit colour (RGB) images, because the user's scanner delivers colour. This applies to the flat-document and book-spread fixtures as well, so the early stages' colour handling runs in every scenario.
- **D-03:** Full golden coverage, at both DPIs, goes to the two output modes the user uses in real work: **black-and-white** (binarization, despeckling) and **colour/greyscale**. This covers the flat-document, book-spread (page split, without dewarping) and mixed-page fixtures. The user added "so far, this might change in the future".
- **D-04:** **Mixed output mode** (picture zones) and **dewarping** get the requirement minimum. That means the ground-truth checks TEST-06 and TEST-07 require (dewarped text lines straighter than the input; picture zone kept inside, pure black and white outside), the dewarping unit tests (TEST-09), and one golden each at 300 dpi. The user does not use these modes yet.
- **D-05:** The whole phase is fully automated. It has no GUI walkthroughs, no "open this image and look" steps and no human-verify checkpoints. The user said "I don't want to test this myself. No manual tests for these please", then extended this to the whole phase. Consequence: nobody inspects the goldens before they are frozen, so every golden scenario must pass automated sanity checks against the generator's ground truth before it is recorded. Examples: the output is not blank, has the right orientation and size, and its content sits where the ground truth puts it. At the end the user gets a short written summary of results and expected failures, with nothing to click through.

### Golden update and review
- **D-06:** Goldens are re-recorded only with the environment variable `UPDATE_GOLDENS=1` (for example `UPDATE_GOLDENS=1 ctest --preset macos-release`). A normal run never writes into the source tree.
- **D-07:** Under `UPDATE_GOLDENS=1`, only goldens that fail their comparison are rewritten. Goldens that still pass stay byte-for-byte untouched, so `git diff` shows only the images that really changed. Every update run refreshes the manifest.
- **D-08:** On a failed comparison, the test writes expected, actual and diff PNGs to `build/<preset>/test-artifacts/<test>/`. The `ctest` output prints the failing metric, its value, its limit and that folder's path. There is no HTML report.
- **D-09:** A manifest next to the goldens records the toolchain they were made with: Qt, Boost, libtiff, libpng, jpeg-turbo, Apple Clang/Xcode, CMake, macOS, the `-ffp-contract` setting and the source commit. When the running toolchain differs, the test prints which versions differ and still compares (warn, do not fail, do not skip).
- **D-10:** A golden update is its own commit, with the reason in the commit message. There is no separate change-log file next to the goldens.

### Tolerances
- **D-11:** Goldens are recorded from the `macos-release` build. The `macos-release` and `macos-debug` builds must pass every golden. The `macos-asan` build runs the full new suite once in this phase. Its findings are recorded, as in Phase 1 `records/sanitizer-findings.md`, but they neither gate the suite nor get fixed.
- **D-12:** Tolerances are measured, not guessed. A short spike measures the output spread between the Release and Debug builds for each image kind. Each limit is set just above the measured spread, with the research starting values as ceilings: bi-level ≤ 0.1% differing pixels after an edge-tolerant XOR, plus a connected-component count check; grey/colour mean absolute error ≤ 0.5 and maximum error ≤ 8 levels. The limits are then frozen as constants in one place in the comparator, one set per image kind, and are not overridden per test without a comment. — **Reversibility:** costly — once goldens are committed and later phases are checked against them, loosening a limit means re-validating every phase gate that relied on it, and tightening one may force a re-record.
- **D-13:** The comparator has a permanent automated self-test. It takes a recorded golden, alters it the way a real regression would (flips a few hundred pixels, shifts it by 1 px, raises a grey level by 10), and asserts that each comparison fails and writes its diff files. The self-test runs in every `ctest` and satisfies success criterion 3 without a manual step.
- **D-14:** Each stage's decisions are also recorded as **text goldens**, one small text file per scenario: deskew angle, page-split line, content box, margins/page layout, output size and DPI. Later runs must match them within a tight tolerance (on the order of 0.01° and 1 px). They use the same `UPDATE_GOLDENS=1` flag and manifest as the image goldens, and `git diff` shows them as readable numbers.
- **D-15:** Ground-truth tolerances start from the research values: deskew within ±0.15°, content-box IoU ≥ 0.95, page split within ±2 px, binarization mismatch ≤ 0.5% of the ink area, dewarped baseline residual ≤ 1.5 px at 300 dpi. They are tightened wherever the unmodified code does clearly better.

### Production code and known defects
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

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase scope and requirements
- `.planning/ROADMAP.md` § Phase 2: goal and the five success criteria
- `.planning/REQUIREMENTS.md`: TEST-01 to TEST-11; SAFE-02 (named by the round-trip expected failure)
- `.planning/PROJECT.md`: constraints (output must not change unintentionally, synthetic test data only) and the Key Decisions table
- `.planning/STATE.md` § Blockers/Concerns: Phase 2 research flag (tolerance spike, `ApplicationSettings` isolation, `ThumbnailPixmapCache` under a test app)

### Phase 1 outputs this phase builds on
- `.planning/phases/01-deletion-free-macos-build/01-CONTEXT.md`: D-03 (`-ffp-contract=off` frozen before goldens), D-07/D-08 (fix boundary: record, don't fix)
- `.planning/phases/01-deletion-free-macos-build/records/closing-check.md`: baseline commit `cde6825`
- `.planning/phases/01-deletion-free-macos-build/records/toolchain.txt`: commands and versions to reuse for the golden manifest
- `.planning/phases/01-deletion-free-macos-build/records/ctest-summary.md`: starting state of the existing suite
- `.planning/phases/01-deletion-free-macos-build/records/fp-contract.md`: the frozen floating-point setting
- `.planning/phases/01-deletion-free-macos-build/records/sanitizer-findings.md`: Phase 1 ASan/UBSan reports (format for D-19)
- `.planning/phases/01-deletion-free-macos-build/deferred-items.md`: the `ScaleTestSuite/test_random_image` failure (D-17)

### Research
- `.planning/research/ARCHITECTURE.md`: headless seams table, proposed `test_support`/`pipeline_tests`/`dewarping/tests` layout, Pattern 2 (ground truth first, goldens second), Pattern 3 (per-kind comparison metrics and starting tolerances), Pattern 4 (font-free deterministic pages)
- `.planning/research/PITFALLS.md`: Pitfalls 1–5 (golden timing, tolerance policy, fixture determinism, threading, macOS test runner), 20 (Unicode normalisation of file names), 22–24, 26
- `.planning/research/FEATURES.md`: TST-01 to TST-10 table (generator primitives, comparator, golden workflow, per-use-case test content)
- `.planning/research/SUMMARY.md` § Phase 2 and § Gaps to address

### Codebase maps and docs
- `.planning/codebase/TESTING.md`: existing Boost.Test and Qt Test patterns, naming, `add_test` wiring
- `.planning/codebase/CONVENTIONS.md`: naming, formatting, copyright header for new files
- `.planning/codebase/CONCERNS.md`: test gaps (OutputGenerator, dewarping, `TestBinarize`, XML special characters)
- `TESTING.md` (repo root): existing test run instructions
- `CMakePresets.json`: `macos-release`, `macos-debug`, `macos-asan` configure, build and test presets

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `OutputGenerator::process` (`src/core/filters/output/OutputGenerator.h:64`) takes all inputs as arguments. It is the main seam for output-stage goldens, with no `Task`, files or thumbnails needed.
- `ProjectWriter` / `ProjectReader` (`src/core/ProjectWriter.h`, `src/core/ProjectReader.h`) live in `core`, which existing test targets already link. Use them for TEST-10.
- `CMakePresets.json` already has test presets for all three builds; per-test environment (offscreen, settings path) can be set there or with `set_tests_properties`.
- `records/toolchain.txt` lists the exact commands to reproduce for the manifest.
- `src/imageproc/tests/Utils.{h,cpp}` has image helpers, but they use `rand()`. Do not reuse them for the generator.

### Established Patterns
- Boost.Test: one suite per file named `<Module><Subject>TestSuite`, cases `test_<behaviour>`, a `main.cpp` per test directory, and `add_test(NAME x COMMAND x --log_level=message)`.
- `ApplicationSettings` is a process-wide singleton read inside processing: `src/core/filters/deskew/Task.cpp:282` (black-on-white detection) and `src/core/filters/output/OutputGenerator.cpp:1230`. Only `src/app/main.cpp:44-47` sets the app and organisation name and the INI format, so tests must isolate settings themselves.
- `src/core/filters/output/Task.cpp:425` calls `m_thumbnailCache->recreateThumbnail(...)` unconditionally, so a stage-chain test through `output::Task` needs a real `ThumbnailPixmapCache`, and with it an application object.
- Dewarping RANSAC is seeded with `QRandomGenerator prng(0)` (`src/dewarping/DistortionModelBuilder.cpp:165`, `src/dewarping/DetectVertContentBounds.cpp:253`). It is reproducible on a given Qt build, which is why the manifest records the Qt version.
- `src/foundation/XmlUnmarshaller.cpp:14`: `XmlUnmarshaller::string` returns `el.text()` with a FIXME about unescaping. This is the SAFE-02 defect TEST-10 is expected to hit.
- `src/imageproc/tests/TestBinarize.cpp`: the only case is inside `#if 0` and reads `"test.png"` from the working directory. TEST-08 replaces it with synthetic inputs.
- `src/imageproc/tests/TestScale.cpp:55-73`: `test_random_image` builds its input with unseeded `rand()` (same sequence every run) and compares `scaleToGray()` with `QImage::scaled` at a tolerance of 1.

### Integration Points
- `src/CMakeLists.txt` adds the modules in layer order (foundation → math → imageproc → dewarping → core → app) and `qt_tests` under `BUILD_TESTS`. New test directories hook in here or through each module's `add_subdirectory(tests)` (as in `src/imageproc/CMakeLists.txt:62`).
- `src/dewarping/` has no `tests/` directory yet.
- `src/qt_tests/CMakeLists.txt:12` registers `qt_tests` with no environment. It passes under both Cocoa and offscreen (Phase 1 record).

</code_context>

<specifics>
## Specific Ideas

- The user's real work: colour scanner input at 300 and 600 dpi, output in black-and-white and colour/greyscale. Mixed output and dewarping are not used "so far". This may change, which is why those modes still get a minimal golden.
- The user wants nothing to test by hand in this phase. Proof comes from `ctest` output and committed records. The end-of-phase report is a short written summary of results and expected failures.
- No UI design contract is needed for this phase (`UI hint: no`, test-only work).

</specifics>

<deferred>
## Deferred Ideas

None. The discussion stayed within the phase scope. Defects the new tests uncover become backlog items under D-18; they are not deferred ideas from this discussion.

</deferred>

---

*Phase: 02-golden-baselines*
*Context gathered: 2026-10-23*
