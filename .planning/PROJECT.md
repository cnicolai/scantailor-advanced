# ScanTailor Advanced (personal macOS fork)

## What This Is

A personal fork (`cnicolai/scantailor-advanced`) of ScanTailor Advanced 1.2.1, the C++17/Qt desktop tool that turns raw page scans into clean output through a fixed six-stage pipeline (fix orientation → page split → deskew → select content → page layout → output). The fork exists to make it run well on a modern Apple Silicon Mac for one user's own scanning of books, flat documents and mixed text-and-image pages. Changes are not intended for upstream.

## Core Value

The app builds from source and works reliably for daily scanning work on a current Apple Silicon Mac, with a test suite that makes later changes safe.

## Requirements

### Validated

<!-- Inferred from the existing codebase (.planning/codebase/, mapped 2026-10-18). -->

- ✓ Six-stage per-page pipeline: fix orientation, page split, deskew, select content, page layout, output — existing
- ✓ Dewarping of curved book pages (`src/dewarping/`) — existing
- ✓ Binarization, despeckling, colour/greyscale output, colour segmentation — existing
- ✓ Interactive picture/fill/content zones and spline editing — existing
- ✓ Background processing on a worker thread pool with per-page result caching — existing
- ✓ Batch processing of whole projects — existing
- ✓ XML project files (`.ScanTailor`) with save/reload, default parameter profiles — existing
- ✓ Image I/O for TIFF (incl. multi-page), PNG, JPEG — existing
- ✓ Light and dark colour schemes, translations — existing
- ✓ Boost.Test unit test executables (math, imageproc, core, foundation, qt) run via CTest — existing
- ✓ Test suite runs green locally and covers the pipeline paths used for books, flat documents and mixed pages, using synthetic generated test images — Phase 2 (`pipeline_tests` and `dewarping_tests` with goldens from the unmodified code at `a796c2ae`; `ctest` 7/7 in Release and Debug; known shortfalls are `XFAIL_CHECK` with backlog 999.1–999.7)

### Active

- [ ] Builds on current macOS (Apple Silicon, arm64) with Homebrew dependencies and runs from the build directory
- [ ] Qt 6 only — Qt 5 support and version branches removed
- [ ] Page views, thumbnails and icons render sharply on Retina (HiDPI) displays
- [ ] Mac-conventional shortcuts and menus (Cmd-based shortcuts, standard app menu, native menu bar)
- [ ] Linux and Windows packaging and platform-specific code paths removed
- [ ] All GitHub Actions CI disabled
- [ ] Known-debt cleanup that makes later feature work safe (e.g. disabled tests, dead `#if 0` blocks, oversized files where touched)

### Out of Scope

- New features — none planned yet; revisit after the macOS milestone
- Upstream contributions — personal fork, no need to keep changes mergeable
- Linux and Windows support — fork is macOS-only by choice
- Intel/universal builds — Apple Silicon only
- Self-contained `.app` bundle, DMG, code signing, notarization — running from the build directory is enough
- CI of any kind (GitHub Actions, macOS runners) — local builds and tests are enough
- Real personal scans as test fixtures — tests use synthetic images only
- Following macOS system dark mode, trackpad gesture support — not requested for this step

## Context

- Brownfield: full codebase map in `.planning/codebase/` (STACK, ARCHITECTURE, STRUCTURE, CONVENTIONS, TESTING, INTEGRATIONS, CONCERNS), mapped at commit `5eaac18`.
- Stack: C++17, CMake 3.9+, Qt 6 preferred with Qt 5.9+ fallback, Boost 1.60+, libjpeg, libpng, libtiff, zlib. Boost.Test + CTest.
- macOS has had almost no attention: the only `Q_OS_MAC` branches are in `src/core/zones/ZoneContextMenuInteraction.{h,cpp}`. About 8 files carry Windows branches; ~19 files contain Qt version checks.
- The user tried a macOS build a long time ago and it did not work; current state is unknown.
- Test gaps flagged in CONCERNS.md: output stage (`OutputGenerator.cpp`, 2814 lines) has no tests; `TestBinarize.cpp` is disabled with `#if 0`; dewarping has no tests; project-file XML round-trip with special characters is untested.
- Fragile areas: output generation, deskew overlay geometry (recent upstream change d710384), hand-written threading/wait code, mixed raw/smart-pointer ownership.
- Oversized files: `OutputGenerator.cpp`, `MainWindow.cpp` (2245), `ThumbnailSequence.cpp` (1712), `ContentBoxFinder.cpp` (1331).
- Security notes worth fixing while there: `std::system` on a settings string for the batch-finished alarm; unbounded TIFF dimensions before allocation.

## Constraints

- **Platform**: macOS on Apple Silicon (arm64), current macOS release — the only target machine
- **Dependencies**: Homebrew-provided Qt 6, Boost, libjpeg, libpng, libtiff, zlib — what's available on the Mac without manual builds
- **Tech stack**: Keep C++17/Qt/CMake architecture and the six-stage pipeline — this is a port and hardening, not a rewrite
- **Behaviour**: Output for existing settings should not change unintentionally — the user relies on it for real scanning work
- **Test data**: Synthetic images only in the repository — no personal scans committed

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Qt 6 only, drop Qt 5 | Homebrew ships Qt 6; a single-user Mac fork gains nothing from an untested second code path (user left the choice to Claude) | — Pending |
| macOS-only: remove Linux/Windows packaging and platform branches | Personal fork, Mac is the only machine | — Pending |
| Disable all CI | User wants local-only builds and tests | — Pending |
| Run from build dir, no `.app` bundling | User's choice; bundling can come later | — Pending |
| Synthetic test images | Keeps personal material out of the repo; deterministic fixtures | ✓ Good — Phase 2: a seeded in-memory page generator (flat, spread, mixed, curved) feeds every golden; no scan is committed |
| No upstream compatibility | Changes are for personal use only | — Pending |
| Baselines before strip: first macOS build with compile fixes only, golden images from that unmodified code, then remove Qt 5/Windows/Linux | Proves the strip and later refactors changed no output | ◆ In progress — Phase 1 build and Phase 2 goldens done (baseline `a796c2ae`); the strip follows in Phase 3 |
| Remove OpenGL viewer | Off by default, deprecated by Apple, no Metal path | — Pending |
| English-only UI, translations removed | User doesn't need them; less to maintain | — Pending |
| Horizontal-layer phases | Port/hardening where each step protects the next | — Pending |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-10-24 after Phase 2*
