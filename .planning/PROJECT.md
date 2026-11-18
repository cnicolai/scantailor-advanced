# ScanTailor Advanced (personal macOS fork)

## What This Is

A personal fork (`cnicolai/scantailor-advanced`) of ScanTailor Advanced 1.2.1, the C++17/Qt desktop tool that turns raw page scans into clean output through a fixed six-stage pipeline (fix orientation → page split → deskew → select content → page layout → output). The fork exists to make it run well on a modern Apple Silicon Mac for one user's own scanning of books, flat documents and mixed text-and-image pages. Changes are not intended for upstream.

## Core Value

The app builds from source and works reliably for daily scanning work on a current Apple Silicon Mac, with a test suite that makes later changes safe.

## Current State

**Shipped v1.0 macOS (2026-11-18).** The fork builds natively as arm64 against Homebrew Qt 6 and runs from `build/<preset>/scantailor-advanced`. Seven ctest suites (synthetic images, goldens recorded from the unmodified 1.2.1 code) pass in Release and Debug. The tree holds only the Qt 6 / macOS code path. The UI renders sharply on Retina, follows Mac menu and shortcut conventions, refuses forged image headers, and caps batch threads by memory. Archive: `.planning/milestones/v1.0-ROADMAP.md`, `.planning/milestones/v1.0-REQUIREMENTS.md`.

**Next milestone:** not defined yet. Candidates are the eight backlog defects (999.1–999.8: grey-input binarizers, dewarping edge lines, residual skew, page-split cutter position, mixed-page AUTO split, profile layout type, backslash in file names, curved-page test metric), the v2 polish items (app icon, Open Recent, proxy icon, Dock bounce) and the v2 test and cleanup items (TESTX-01/02, CLEANX-01..03). Start with `/gsd-new-milestone`.

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
- ✓ Light and dark colour schemes — existing (translations removed in Phase 3; the UI is English only)
- ✓ Boost.Test unit test executables (math, imageproc, core, foundation, qt) run via CTest — existing
- ✓ Test suite runs green locally and covers the pipeline paths used for books, flat documents and mixed pages, using synthetic generated test images — v1.0 Phase 2 (`pipeline_tests` and `dewarping_tests` with goldens from the unmodified code at `a796c2ae`; `ctest` 7/7 in Release and Debug; known shortfalls are `XFAIL_CHECK` with backlog 999.1–999.7)
- ✓ Qt 6 only — Qt 5 support and version branches removed — v1.0 Phase 3 (`find_package(Qt6 6.5 …)`; Qt 5 conditionals resolved with `unifdef`; goldens unchanged)
- ✓ Linux and Windows packaging and platform-specific code paths removed — v1.0 Phase 3 (also portable mode, install rules and translations)
- ✓ All GitHub Actions CI disabled — v1.0 Phase 3 (`.github/` deleted)
- ✓ README documents the macOS build and test commands — v1.0 Phase 3 (proven by a fresh clone that ran only README lines)
- ✓ Page views, thumbnails and icons render sharply on Retina (HiDPI) displays — v1.0 Phase 4 (DPR 2 measured on the Studio Display without an `Info.plist`; page view pixel-exact against a device-resolution reference at fit/100%/200%; thumbnails, zone mask, badges, handles, stage animation and colour picker checked live; output files and goldens unchanged)
- ✓ Mac-conventional shortcuts and menus (Cmd-based shortcuts, standard app menu, native menu bar) — v1.0 Phase 5 (application menu named ScanTailor Advanced with About, Preferences and Quit; Cmd shortcuts, Cmd+1..6 stages, Option-Command Output tabs and thumbnail keys measured on a German layout; Mac key glyphs in the UI text; Delete/Backspace on zones; right-click, Control-click and two-finger click menus; native file panels including relinking; menu bar kept during dialogs; batch-finished alarm is the system beep. Verified 8/8, the beep source-verified only)
- ✓ Builds on current macOS (Apple Silicon, arm64) with Homebrew dependencies and runs from the build directory — v1.0 Phase 1 (re-proven by every later phase's Release and Debug gate)
- ✓ Known safety defects fixed: forged image headers refused before allocation, unusual file names round-trip through the project file, batch threads capped by physical memory — v1.0 Phase 6 (SAFE-01..03; limit 2^29 − 1 px for source scans only, own output files unlimited; multi-scan JPEGs never decompressed for metadata; D-08 run: swap 0)
- ✓ OpenGL viewer and every OpenGL dependency removed — v1.0 Phase 6 (STRIP-07; `otool -L` shows no QtOpenGL; colour picker unchanged)
- ✓ Known-debt cleanup that makes later feature work safe: `OutputGenerator.cpp` and `MainWindow.cpp` split by pure moves, `#if 0` blocks and `boost/foreach` removed, one save-as flow — v1.0 Phase 6 (CLEAN-01..05; move checks PASS, output digests and goldens unchanged, live `mac_check` PASS)

### Active

(none: every v1 requirement is validated; new work starts with the next milestone)

### Out of Scope

- New features — none planned for v1.0; the next milestone decides (see Current State)
- Upstream contributions — personal fork, no need to keep changes mergeable
- Linux and Windows support — fork is macOS-only by choice
- Intel/universal builds — Apple Silicon only
- Self-contained `.app` bundle, DMG, code signing, notarization — running from the build directory is enough
- CI of any kind (GitHub Actions, macOS runners) — local builds and tests are enough
- Real personal scans as test fixtures — tests use synthetic images only
- Following macOS system dark mode, trackpad gesture support — not requested for this step

## Context

- Brownfield: full codebase map in `.planning/codebase/` (STACK, ARCHITECTURE, STRUCTURE, CONVENTIONS, TESTING, INTEGRATIONS, CONCERNS), mapped at commit `5eaac18`.
- Stack: C++17, CMake 3.25+ with `CMakePresets.json` (`macos-release`, `macos-debug`, `macos-asan`), Qt 6.5+ only, Boost 1.60+, libjpeg (jpeg-turbo), libpng, libtiff, zlib from the macOS SDK. Boost.Test + CTest, seven suites.
- Since Phase 3 the tree holds only the macOS code path: no platform conditionals, no Qt version checks, no packaging, CI or translations. `RelinkablePath::normalize` still rewrites backslashes in file names (backlog 999.8).
- Since Phase 4 every on-screen raster buffer is built at the device pixel ratio through `src/core/HiDpi.{h,cpp}`; `qt_tests` runs on two offscreen screens (`OneX` at 1, `TwoX` at 2) and `build/macos-release/retina_check` is the live Cocoa check. A real drag between a 1x and a 2x display is untested (one display attached; re-check procedure in `milestones/v1.0-phases/04-retina-rendering/records/closing-check.md`). Thumbnail files are now built at Quality × DPR (`q500` at the new default Quality 250).
- The Phase 4 review warnings WR-01 to WR-03 (thumbnail list end marker, stale other-ratio thumbnail file, device-pixel-ratio read in `ImageViewBase::event()`) were fixed after the review (`milestones/v1.0-phases/04-retina-rendering/04-REVIEW-FIX.md`).
- v1.0 shipped 2026-11-18: 7 phases (1–6 plus inserted 06.1), 77 plans, 588 commits over 32 days. About 120,000 lines of C++ under `src/`, of which about 19,500 are tests and test support.
- The test gaps flagged in CONCERNS.md at mapping time (untested output stage and dewarping, disabled `TestBinarize.cpp`, untested XML round-trip) were closed in Phase 2. Known shortfalls of the unmodified code are `XFAIL_CHECK`s tied to backlog 999.1–999.7.
- Fragile areas: output generation, deskew overlay geometry (recent upstream change d710384), hand-written threading/wait code, mixed raw/smart-pointer ownership.
- Since Phase 5 the app carries an embedded `Info.plist` section (`CFBundleName` only) and `build/macos-release/mac_check` is the live Cocoa check for menus, keys, zones, context menus, dialogs and file panels. Its sections move the pointer and take focus, so they run only when the user says go. `TestMacQt` in `qt_tests` covers the menus and shortcuts offscreen. The findings of the Phase 5 review were closed in Phase 06.1 (`milestones/v1.0-phases/05-mac-menus-and-shortcuts/05-REVIEW-DISPOSITION.md`). At the Filters dock's minimum width the Margins options panel is cut on the right (older layout limit, noted in `05-UAT.md`).
- Oversized files left: `ThumbnailSequence.cpp` (1711 lines) and `ContentBoxFinder.cpp` (1331), both v2 item CLEANX-03. `OutputGenerator.cpp` (now 219) and `MainWindow.cpp` (now 1182) were split in Phase 6.
- Phase 06.1 closed every review finding of Phases 1–6 (each `fixed`, or `skipped`/`deferred` with a reason in its `*-REVIEW-DISPOSITION.md`) and replaced bare planning IDs in `src/` comments with plain reasons. Remaining v1.0 tech debt is listed in `.planning/milestones/v1.0-MILESTONE-AUDIT.md`: strict `macos-asan` and live `mac_check` not re-run on the final tree, Boost still found through FindBoost variables (WR-04, deferred), HIDPI-01 measured on a proxy binary.
- Security notes: oversized image headers are refused before allocation since Phase 6. (The `std::system` call on a settings string for the batch-finished alarm was removed in Phase 5.)

## Constraints

- **Platform**: macOS on Apple Silicon (arm64), current macOS release — the only target machine
- **Dependencies**: Homebrew-provided Qt 6, Boost, libjpeg, libpng, libtiff, zlib — what's available on the Mac without manual builds
- **Tech stack**: Keep C++17/Qt/CMake architecture and the six-stage pipeline — this is a port and hardening, not a rewrite
- **Behaviour**: Output for existing settings should not change unintentionally — the user relies on it for real scanning work
- **Test data**: Synthetic images only in the repository — no personal scans committed

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Qt 6 only, drop Qt 5 | Homebrew ships Qt 6; a single-user Mac fork gains nothing from an untested second code path (user left the choice to Claude) | ✓ Good — v1.0 Phase 3: Qt 5 removed with `unifdef`, build commands and goldens unchanged |
| macOS-only: remove Linux/Windows packaging and platform branches | Personal fork, Mac is the only machine | ✓ Good — v1.0 Phase 3: platform code, packaging, install rules and portable mode removed; settings and profile locations unchanged |
| Disable all CI | User wants local-only builds and tests | ✓ Good — v1.0 Phase 3: `.github/` deleted; formatting is a documented manual `xcrun clang-format` step |
| Run from build dir, no `.app` bundling | User's choice; bundling can come later | ✓ Good — v1.0: every check runs from `build/<preset>`; the menu name comes from an embedded `Info.plist` section; bundling stays v2 (POLISH-05) |
| Synthetic test images | Keeps personal material out of the repo; deterministic fixtures | ✓ Good — v1.0 Phase 2: a seeded in-memory page generator (flat, spread, mixed, curved) feeds every golden; no scan is committed |
| No upstream compatibility | Changes are for personal use only | ✓ Good — v1.0: the strip, Mac UI and file splits were done without regard to mergeability |
| Baselines before strip: first macOS build with compile fixes only, golden images from that unmodified code, then remove Qt 5/Windows/Linux | Proves the strip and later refactors changed no output | ✓ Good — v1.0 Phase 3: twelve strip commits each passed Release and Debug `ctest` 7/7 with every golden byte-identical to `a796c2ae`; ASan/UBSan findings unchanged |
| Remove OpenGL viewer | Off by default, deprecated by Apple, no Metal path | ✓ Good — v1.0 Phase 6: module, setting control and sampling branch removed; only OpenGL items left the build commands |
| English-only UI, translations removed | User doesn't need them; less to maintain | ✓ Good — v1.0 Phase 3: translator code and the Language row removed in one commit; `tr()` calls kept, an existing `language` setting is left untouched |
| Horizontal-layer phases | Port/hardening where each step protects the next | ✓ Good — v1.0: goldens from Phase 2 guarded every later phase; no golden changed after `a796c2ae` |
| Gate every strip commit (`gate.sh`): Release and Debug `ctest`, goldens, build-command diff | Deletions must not change output; the command diff catches CMake edits that tests would miss | ✓ Good — v1.0 Phase 3: 12 of 12 commits passed; reused for all 13 Phase 4 code commits |
| One buffer rule for Retina: build view buffers at the device pixel ratio via a small `hidpi` helper module, never touch output generation | Keeps the change view-only and testable; output files must not change | ✓ Good — v1.0 Phase 4: no diff under `imageproc`, `dewarping`, `Task.cpp` or `OutputGenerator.cpp`; goldens byte-identical |
| Prove Retina offscreen (two simulated screens) plus a committed live harness, not by launching the app | Automated tests need no display; the app itself would touch `~/.config` and `~/Library` | ✓ Good — v1.0 Phase 4: 24 `TestHiDpiQt` tests in `ctest`; `retina_check` measured every element on the Studio Display |
| Thumbnail Quality is multiplied by the DPR; default raised to 250; files keyed by effective width | Sharp thumbnails without rewriting stored settings or deleting old cache files | ✓ Good — v1.0 Phase 4: `q500` files on Retina, old `q200` files left in place |
| Name the application menu through an embedded `Info.plist` section, not a bundle | Running from the build directory stays the rule; a plist next to the executable would fake a bundle | ✓ Good — v1.0 Phase 5: menu reads ScanTailor Advanced; settings and profile paths measured unchanged |
| Option-Command for the Output tabs and thumbnail keys | Measured on the user's German layout: Option-Command digits and plus/minus fire once each | ✓ Good — v1.0 Phase 5: live keys run PASS |
| Batch-finished alarm is `QApplication::beep()`; the old alarm command setting is no longer read | Removes the `std::system` call on a settings string | ✓ Good — v1.0 Phase 5: test-pinned; the user waived hearing the sound |
| Live Cocoa checks (`mac_check`) run only on the user's go | They move the pointer and take focus while the user works | ✓ Good — v1.0 Phase 5: offscreen tests carry the automated gate |
| Image size limit applies to source scans only (`ImageLoader::loadSource`), checked on header values before any allocation | Header bombs cost 6–12 GB from a few KB; the app's own output files may legitimately exceed the source limit | ✓ Good — v1.0 Phase 6: forged TIFF/PNG/JPEG tests with sampled-footprint bounds and mutation runs; review fixes WR-01/WR-04/WR-07 |
| Cap parallel batch pages by physical memory and each page's output dpi | Large pages in parallel pushed the Mac into swap | ✓ Good — v1.0 Phase 6: D-08 run capped at 5 of 10 threads, peak 17.7 of 32 GiB, swap 0 |
| Split oversized files only by verified pure moves (chunk-multiset check, output digests) | Refactoring the output stage must not change a pixel | ✓ Good — v1.0 Phase 6: move checks PASS (79/79), seven output digests equal before and after |
| Insert a tech-debt phase (06.1) before closing v1.0 instead of carrying review findings forward | The first milestone audit found open review findings and planning IDs in code comments | ✓ Good — v1.0 Phase 06.1: every finding dispositioned, goldens unchanged; re-audit 52/52 requirements, 7/7 phases |

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
*Last updated: 2026-11-18 after v1.0 milestone*
