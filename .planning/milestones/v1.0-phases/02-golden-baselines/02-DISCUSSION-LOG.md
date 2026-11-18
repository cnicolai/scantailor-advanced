# Phase 2: Golden Baselines - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-10-23
**Phase:** 02-golden-baselines
**Areas discussed:** Fixture realism & scope, Golden update & review, Tolerance strictness, Known defects & no-change rule

---

## Fixture realism & scope

**Scan resolution**

| Option | Description | Selected |
|--------|-------------|----------|
| 300 dpi (Recommended) | Fixtures carry 300 dpi; small pixel sizes work because pages can be physically small | |
| 600 dpi | Fixtures carry 600 dpi; 4× the pixels and runtime for the same physical page | |
| Both 300 and 600 | One scenario per DPI; catches DPI-dependent paths at the cost of more goldens | ✓ |

**Scanner input type**

| Option | Description | Selected |
|--------|-------------|----------|
| Colour | 24-bit RGB input; colour-to-grey conversion exercised | ✓ |
| Greyscale | 8-bit grey input; only the mixed fixture gets colour | |
| Both | Flat and book in grey, mixed in colour | |

**Output settings used in real work** (multi-select)

| Option | Description | Selected |
|--------|-------------|----------|
| Black and white | Bi-level output (binarization, despeckling) | ✓ |
| Colour / greyscale | Continuous-tone output | ✓ |
| Mixed | Text binarized, picture zones kept in colour/grey | |
| Dewarping (book pages) | Automatic or manual dewarping in the output stage | |

**User's note:** "So far, this might change in the future."

**Coverage depth for mixed output and dewarping**

| Option | Description | Selected |
|--------|-------------|----------|
| Same depth as B/W and colour (Recommended) | Goldens at both DPIs, protected through later phases | |
| Requirement minimum | Ground-truth checks plus one golden each at 300 dpi | ✓ |

**User's note:** "I don't want to test this myself. No manual tests for these please."

**More questions or next area:** Next area. Page size, runtime budget and fixture storage left to Claude.

---

## Golden update & review

**How to trigger re-recording**

| Option | Description | Selected |
|--------|-------------|----------|
| Environment variable (Recommended) | `UPDATE_GOLDENS=1 ctest --preset macos-release`, review with `git diff` | ✓ |
| Dedicated CMake target | `--target update-goldens`; harder to trigger by accident | |
| Script in scripts/ | `scripts/update-goldens.sh` wraps build, re-record and manifest | |

**What to see on a failed comparison**

| Option | Description | Selected |
|--------|-------------|----------|
| Files + summary line (Recommended) | expected/actual/diff PNGs in `build/<preset>/test-artifacts/<test>/`, failing metric and path in ctest output | ✓ |
| Files + HTML report | Same plus a side-by-side `report.html` | |
| Summary line only | Metric only, no images | |

**Toolchain differs from manifest**

| Option | Description | Selected |
|--------|-------------|----------|
| Warn, still compare (Recommended) | Prints differing versions, compares as usual | ✓ |
| Fail until re-recorded | Any mismatch fails the suite | |
| Skip golden comparisons | Golden checks skipped on mismatch | |

**Which goldens `UPDATE_GOLDENS=1` rewrites**

| Option | Description | Selected |
|--------|-------------|----------|
| Only those that fail (Recommended) | Passing goldens stay byte-identical; manifest refreshed either way | ✓ |
| All of them | Every golden re-encoded | |

**More questions or next area:** Next area. Default accepted: golden update is its own commit, reason in the commit message, no extra log file.

---

## Tolerance strictness

**Which builds must pass goldens**

| Option | Description | Selected |
|--------|-------------|----------|
| Release + Debug; ASan once (Recommended) | Release and Debug gate; ASan/UBSan runs once, findings recorded | ✓ |
| All three, every time | Strongest, three slow runs per check | |
| Release only | Fastest; optimisation-level differences unnoticed | |

**How tight**

| Option | Description | Selected |
|--------|-------------|----------|
| Measured, just above noise (Recommended) | Spike measures Release vs Debug spread; limits just above it, research values as ceilings, then frozen | ✓ |
| Research starting points as-is | No measurement; may be looser than needed | |
| Upgrade-tolerant | Wide enough to survive minor upgrades; may hide regressions | |

**Proof that altered output fails (success criterion 3)**

| Option | Description | Selected |
|--------|-------------|----------|
| Permanent automated test (Recommended) | Comparator self-test with perturbed goldens in every ctest | ✓ |
| One-off check during the phase | Perturb once, record, revert | |

**Record stage decisions as goldens**

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, as text goldens (Recommended) | Per-scenario text file of stage decisions, tight tolerance | ✓ |
| Final images only | Stage decisions only against ground truth | |

**More questions or next area:** Next area. Default accepted: ground-truth tolerances start from research values and are tightened if the unmodified code does clearly better.

---

## Known defects & no-change rule

**Production code changes in Phase 2**

| Option | Description | Selected |
|--------|-------------|----------|
| No, non-test src/ stays identical (Recommended) | Tests use existing public interfaces only; `git diff cde6825` on non-test src/ is empty | ✓ |
| Small test hooks allowed | e.g. expose OutputGenerator helpers; weakens the "unmodified" claim | |

**`ScaleTestSuite/test_random_image` failure**

| Option | Description | Selected |
|--------|-------------|----------|
| Investigate, then fix test or mark (Recommended) | Fix test expectation if Qt's reference moved; otherwise expected failure + backlog item | ✓ |
| Mark as expected failure only | Quickest; cause stays unexamined | |

**New defects found by ground-truth checks**

| Option | Description | Selected |
|--------|-------------|----------|
| Record, mark, don't fix (Recommended) | Golden records current behaviour; expected failure naming the defect; findings file + backlog | ✓ |
| Stop and ask me each time | Pause execution per finding | |
| Change the fixture to avoid it | Clean suite, weakness unrecorded | |

**Manual steps in the phase**

| Option | Description | Selected |
|--------|-------------|----------|
| Fully automated phase (Recommended) | No GUI walkthroughs or visual checks; written summary at the end | ✓ |
| One visual look at the goldens | Contact sheet glance before committing goldens | |

**More questions or next area:** Next area. Default accepted: new sanitizer reports are recorded, not fixed.

---

## Closing

**User's choice:** "Ready for context, and tell me whether to run ui phase or not". Answer given: no UI phase. Phase 2 is marked `UI hint: no` and adds test-only code.

## Claude's Discretion

- Fixture pixel size and total runtime budget
- Whether input fixtures are committed as PNGs or generated in memory
- Generator internals (PRNG, glyph shapes) within the no-`rand()`, no-font constraints
- Code layout of the test support library, pipeline tests and dewarping tests; golden location and text-golden format
- Headless pipeline entry (direct `Task::process` chain or `StageSequence`)
- Settings isolation mechanics
- Special-character set for the round trip (including NFC/NFD)
- How expected failures are expressed (Boost `expected_failures`, `QEXPECT_FAIL`)

## Deferred Ideas

None.
