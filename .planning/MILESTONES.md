# Milestones

## v1.0 macOS (Shipped: 2026-11-18)

**Delivered:** ScanTailor Advanced 1.2.1 builds natively on Apple Silicon against Homebrew Qt 6, carries only the macOS code path, renders sharply on Retina, follows Mac menu and shortcut conventions, and is protected by a synthetic-image test suite whose goldens never changed after the baseline.

**Phases completed:** 7 phases (1–6, plus inserted 06.1), 77 plans, 145 tasks

**Key accomplishments:**

1. **Native build without source edits.** The unmodified 1.2.1 sources build as arm64 with Apple Clang against Homebrew Qt 6.12 and Boost 1.92. Three CMake presets (`macos-release`, `macos-debug`, `macos-asan`) exist, and `-ffp-contract=off` was frozen before any baseline was recorded.
2. **Golden baselines from the unmodified code.** A seeded, font-free page generator (flat, book spread, mixed, curved) feeds a headless six-stage pipeline. 38 goldens were recorded at `a796c2ae`, and no golden file changed afterwards. Seven ctest suites pass in Release and Debug, and seven defects of the original code are expected failures tied to backlog 999.1–999.7.
3. **Qt 6 / macOS-only tree.** Twelve gated commits removed Qt 5, Windows, Linux, packaging, CI, portable mode and translations. Every gate passed with byte-identical goldens and an unchanged macOS build-command diff. README builds the app from a fresh clone.
4. **Retina rendering.** The page view, thumbnails (`q500` at Quality 250), zone mask, handles, stage animation and colour picker are built at the device pixel ratio. 24 offscreen `TestHiDpiQt` tests cover them, and the live `retina_check` passed on the Studio Display. No output code changed.
5. **Mac menus and shortcuts.** The application menu holds About, Settings (⌘,) and Quit. ⌘1–⌘6 switch stages, Option-⌘ selects the Output tabs and thumbnail keys, the UI text uses Mac key glyphs, file panels are native (relinking included), and the batch alarm is the system beep. Live `mac_check` passed on the German layout.
6. **Hardening and cleanup.** Forged TIFF/PNG/JPEG headers are refused before allocation. Control characters in names round-trip through the project file. Batch threads are capped by physical memory (D-08 run: 5 threads, no swap). The OpenGL viewer is removed. `OutputGenerator.cpp` (2814 → 219 lines) and `MainWindow.cpp` (2305 → 1182) were split by verified pure moves with output digests unchanged. Phase 06.1 dispositioned every review finding of Phases 1–6.

**Stats:**

- 588 commits, 2026-10-18 → 2026-11-18 (32 days)
- 293 files changed outside `.planning/` (+26,042 / −35,706 lines)
- About 120,000 lines of C++ under `src/`, about 19,500 of them tests and test support
- Requirements: 52/52 v1 requirements complete

**Closeout:** override_closeout

- Known verification overrides: 1 newly acknowledged, 0 carried forward from a prior close (see STATE.md Deferred Items). The acknowledged item is the Phase 01 deferred note on `ScaleTestSuite/test_random_image`, already fixed by plan 02-04 (commit `184fe855`); the audit scanner did not read its `status: fixed` line.
- Phase verification override: `init.manager` reported Phases 1–6 as `stale` although each `*-VERIFICATION.md` says `status: passed`, no SUMMARY was committed after its verification, and the re-audit of 2026-11-18 (after 06.1) found 7/7 phases and 52/52 requirements satisfied. Phase 06.1 reported `passed`.

**Tech debt carried forward:** see `.planning/milestones/v1.0-MILESTONE-AUDIT.md` (strict `macos-asan` and live `mac_check` not re-run on the final tree; Boost found through FindBoost variables, WR-04; HIDPI-01 measured on a proxy binary; UI review 21/24 notes).

**Archives:** `milestones/v1.0-ROADMAP.md`, `milestones/v1.0-REQUIREMENTS.md`, `milestones/v1.0-MILESTONE-AUDIT.md`, `milestones/v1.0-phases/`

---
