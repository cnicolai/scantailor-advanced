# Project Research Summary

**Project:** ScanTailor Advanced (personal macOS fork, 1.2.1)
**Researched:** 2026-10-18
**Confidence:** MEDIUM overall (HIGH on versions and build mechanics; MEDIUM on macOS runtime behaviour; nothing built yet on the target Mac)

## Executive Summary

This is a brownfield port, not a new product: a working six-stage scan-cleaning pipeline that has never been built and tested on Apple Silicon, with almost no tests where it matters most (`OutputGenerator.cpp`, 2814 lines, and dewarping have none). The research converges on one approach: change nothing that alters output until a characterisation baseline exists, then make platform and cleanup changes under that baseline. Use Homebrew Qt 6.12 (`qtbase`, `qtsvg`, `qttools` only, not the `qt` meta-formula), Boost 1.92, jpeg-turbo, libpng and libtiff, zlib from the SDK, built with CMake 3.25+ and Ninja using Apple Clang and libc++, staying on C++17. Run the binary from the build directory; no `.app` bundle.

Sequencing: (1) a deletion-free arm64 build; (2) a synthetic-image harness with goldens recorded from that unmodified code; (3) the mechanical strip of Qt 5, Windows, Linux and CI; (4) Mac view work (Retina, then menus and Cmd shortcuts); (5) the risky splits and debt fixes. One measurement gates the Retina scope: whether an unbundled Qt 6 executable runs at backing scale 2.0 on this Mac. It is taken in phase 1.

Main risks: goldens captured after the code has changed; a tolerance policy that is either byte-exact or too loose; Retina fixed only in the main view; Qt 5 removal done by hand, which activates never-tested Qt 6 branches. Each has a mitigation in the phase order below.

## Key Findings

### Stack
- Apple Clang + libc++ (CLT only). Do not use Homebrew GCC or LLVM for the build.
- CMake 4.4.4, `cmake_minimum_required(VERSION 3.25)`, `CMP0167` NEW so `find_package(Boost CONFIG)` is used.
- Ninja 1.13.2 for generation and `compile_commands.json`.
- Qt 6.12.0 via `qtbase qtsvg qttools`; declare floor `Qt6 6.8`. Do not install `qt` (38 sub-formulae).
- Boost 1.92.0 (fallback `boost@1.85`); Boost.Test dynamic with `BOOST_TEST_DYN_LINK`. Bind, Lambda, MultiIndex not removed in 1.92.
- jpeg-turbo 3.2.0, libpng 1.6.59, libtiff 4.7.2: one copy each, shared with Qt. Do not install `jpeg` (keg-only, conflicts).
- zlib from the SDK, not the keg-only Homebrew formula.
- Stay on C++17. No `-march=native`. Consider `-ffp-contract=off` for the baseline (arm64 Clang fuses multiply-add by default).
- Pin `CMAKE_PREFIX_PATH=/opt/homebrew` in `CMakePresets.json`. After `brew upgrade boost`, clear `Boost_DIR` or delete the build dir.

### Features
- **Table stakes, build (BLD-01..06):** arm64 binary from one documented command; Qt 5 and `QT_VERSION` branches removed; `ctest` green; batch alarm fixed (currently runs a Linux `play` command through `std::system`, silently failing on macOS); Wayland/xcb/Windows code removed; drop `DontUseNativeDialog` in `RelinkingDialog.cpp`.
- **Table stakes, Retina (HDP-00..05):** measure DPR on the bare binary first; HQ page pixmap, thumbnails and small glyphs at `logical * DPR`; DPR-keyed thumbnail cache; rebuild on DPR change.
- **Table stakes, Mac UX (MAC-01..14):** explicit `menuRole` for About, Settings (Preferences) and Quit; `Cmd+,`; `Cmd+Shift+S`; keep `Ctrl` shortcuts in code (Qt maps them to Cmd); native-name text instead of hard-coded "Ctrl"; zone delete also on Backspace; `Cmd+=` zoom; populated default menu bar; Control-click and two-finger click open context menus.
- **Table stakes, tests (TST-01..10):** seeded font-free generator; comparator with per-kind tolerances; explicit-flag golden regeneration; flat-document, book-page and mixed-page tests; project round trip with special characters; offscreen Qt in `ctest`; run the existing suite first.
- **Differentiators:** embedded `Info.plist` via `-sectcreate` if Retina probe fails (BLD-10); Dock icon (BLD-12); Open Recent (MAC-20); proxy icon and modified dot (MAC-21); Dock alert (MAC-22); metamorphic tests (TST-20); sanitiser option (TST-21); defaults round trip (TST-08); HiDPI render test (HDP-10).
- **Defer / avoid:** bundle, DMG, signing; Intel/universal; file associations; gestures; dark-mode following; shortcut customisation; `AA_MacDontSwapCtrlAndMeta`; `QT_SCALE_FACTOR` as a fix; `drawText` in fixtures; byte-exact goldens; `@2x` assets; auto-updating goldens.

### Architecture
- Layers: `foundation`/`math` (pure) → `imageproc` → `dewarping` → `core` (Task chain, Settings, project XML, views) → `app`.
- Verified: each `Task::process` never dereferences its `Filter`, so the six-stage chain runs headlessly with `filter = nullptr` and `NullTaskStatus`. `OutputGenerator::process` takes all inputs as arguments.
- Retina touches only the view layer; never `imageproc`, `dewarping` or `Task::process`. Goldens therefore independently prove Retina changes did not alter output.
- New components: `src/test_support/` (generator, comparator, golden store; depends on `imageproc` and `Qt::Gui`, not `core`); `src/pipeline_tests/` (Boost.Test executable, offscreen `QApplication`, isolated `QSettings`); `src/dewarping/tests/` (no Qt app).
- Split pattern for `OutputGenerator.cpp`: utils first, then `Processor` into a private header, then Dewarp/Binarize/Pictures topic files; no signature or ownership change.
- Comparison rules: bi-level by edge-tolerant XOR plus component count; grey/colour by MAE and p99.9; geometry exact or numeric; decode pixels, never compare file bytes.
- Pattern: `clang++ -E -P` before/after identity proves preprocessor-only deletions.

### Pitfalls
1. **Goldens captured after changes.** Capture from the first green unmodified build; record toolchain (`brew list --versions`, `xcodebuild -version`, `cmake --version`, Qt build string) in `MANIFEST.txt`; commit before any strip or refactor.
2. **Wrong tolerance policy.** Per-kind policy written once in the helper; measure spread on two builds to set numbers.
3. **Qt 5 deletion by hand.** Use `unifdef`; keep Qt 6 `#else` unchanged. Polarity matters: `DistortionModelBuilder.cpp` and `DetectVertContentBounds.cpp` RANSAC path changes dewarp output if wrong.
4. **Retina in the main view only.** One rule: logical view coordinates, buffers `logical * dpr` with `setDevicePixelRatio`. Cover HQ pixmap, `PictureZoneEditor` mask, thumbnails (DPR in key), colour picker `grab()`.
5. **Platform defaults.** `Q_OS_UNIX` selects the Linux alarm on macOS; `PORTABLE_VERSION` splits settings between `build/config` and Application Support; decide explicitly.
6. **Ctrl/Cmd.** Leave `Qt::ControlModifier` gesture logic alone; change text only; bind Backspace with Delete; never use `Meta`.

## Implications for Roadmap

### Conflict on phase order (decision required)

- **STACK.md** describes the CMake modernisation as a first step that also deletes the `if(Qt6_FOUND)` branches, `QT_BINDIR`, Windows `Boost_*` and `DEPENDENCIES_ROOT` blocks, and CPack/packaging, with the install section saying "after the CMake modernisation". Read as a whole, it implies the strip happens in the first build.
- **ARCHITECTURE.md and PITFALLS.md** argue for a deletion-free bring-up, goldens from unmodified code, then the strip. Their build order and phase mapping both place the strip after the goldens.

**Recommendation: follow ARCHITECTURE.md and PITFALLS.md.** Phase 1 allows only edits needed to compile: the `cmake_minimum_required` bump with `CMP0167`, Boost config discovery, `Qt6::` target names where the `Qt_*_lib` variables resolve badly, and `BOOST_TEST_DYN_LINK`. Goldens come from that code. The strip follows. Reason: every "output unchanged" claim needs a baseline built from the code the user actually had. If the strip happens first, a wrong `#if` deletion and a wrong Homebrew discovery look identical, and the goldens encode the mistake. The cost is one extra phase, which is small against an unprovable baseline. Two STACK items still belong in phase 1 because they are needed to compile: the CMake minimum and the Homebrew package set. The `-ffp-contract` choice is also made in phase 1, before goldens.

### Suggested phases

**Phase 1: Bring-up (deletion-free)**
- Delivers: arm64 binary running from the build dir; `CMakePresets.json` pinned to `/opt/homebrew`; `otool -L` shows one libjpeg, libpng, libtiff; baseline `ctest` recorded (failures recorded, not fixed); toolchain versions recorded; `compile_commands.json`; decisions on `-ffp-contract`, `PORTABLE_VERSION`, bundle.
- Addresses: BLD-01, BLD-03, TST-10, HDP-00 (DPR probe, gates Retina scope).
- Avoids: Pitfalls 5, 11, 12 (smallest diffs, one commit per error class).
- Research flag: light, empirical.

**Phase 2: Harness and goldens on unmodified code**
- Delivers: `test_support`; revived `TestBinarize` with synthetic truth (replaces `#if 0`); skew and dewarper tests; `pipeline_tests` with `HeadlessPipeline`, per-stage truth tests, `OutputGenerator` per colour mode, scenarios S1–S5; project round trip with special characters and NFC/NFD names; `MANIFEST.txt`; offscreen `ctest`; one sanitiser run (ASan/UBSan, plus Debug).
- Addresses: TST-01..07, TST-09, TST-10, TST-21 (run once).
- Avoids: Pitfalls 1–5, 22–24. No `drawText`, no `rand()`, test through `Task::process` and `OutputGenerator::process`, not the thread pool or `MainWindow`.
- Research flag: yes, short spike. Tolerance spread across two builds (with/without `-ffp-contract=off`); `ApplicationSettings` isolation; `ThumbnailPixmapCache` under a test app.

**Phase 3: Mechanical strip (Qt 5, Windows, Linux, CI)**
- Delivers: `unifdef -DQT_VERSION_MAJOR=6` with every diff read; `Qt_*_lib` and `Qt5::` removed; Windows blocks removed; CPack, `flatpak/`, `build-*.sh`, desktop/mime installs removed; `.github/workflows/` removed; translations fixed or removed cleanly; `src/resources/unix` deleted only after grep confirms no references.
- Addresses: BLD-02, BLD-05, HDP-07, strip and CI requirements.
- Avoids: Pitfalls 7, 14, 25. Each commit proven by `clang++ -E -P` identity, then goldens, then full `ctest`.
- Research flag: no.

**Phase 4: Mac view layer (Retina, then menus and keys)**
- Retina: HQ pixmap at physical resolution with DPR-change rebuild; DPR-aware thumbnails with DPR-keyed cache; small glyph pixmaps DPR-aware or vector; pure geometry helper for HQ target rect (unit-testable).
- Menus and keys: explicit roles; `Cmd+,`; `Cmd+Shift+S`; `Cmd+=`; native-name tooltips and status tips; Backspace for zone delete; populated default menu bar; alarm default to `QApplication::beep()` (BLD-04); native dialogs.
- Addresses: HDP-01..06, MAC-01..14, BLD-04, BLD-06, BLD-10 if needed.
- Avoids: Pitfalls 8, 9, 10, 16 (one `+0.5` decision), 21.
- Research flag: yes for Retina (per-pipeline experiment on the Mac; `widthMM()` and smoothing heuristic under Qt 6). Goldens must not move in this phase.

**Phase 5: Safe splits and debt**
- Delivers: `OutputGenerator.cpp` split, golden-checked after each step; `MainWindow.cpp` split only after menus, mechanical moves only; TIFF dimension cap before allocation (test first); XML special-character fix; dead `boost/foreach` includes removed; OpenGL decision (recommended: remove option and link entries after the colour-picker sampling test exists); thread-count cap by RAM; `ThumbnailSequence` and `ContentBoxFinder` only if touched.
- Addresses: TST-08, TST-23, known-debt requirement, BLD-11, BLD-12, HDP-10, TST-25.
- Avoids: Pitfalls 6 (convert `SIGNAL`/`SLOT` string connections first, own commit), 13, 15, 19, 20.
- Research flag: `OutputGenerator` split standard once goldens exist; OpenGL removal needs a colour-picker check.

### Ordering rationale
- Goldens before any deletion or refactor: the central constraint, and where the research files disagree.
- Retina depends on the strip (same hunks in `ImageViewBase`, `main.cpp`).
- Menu roles depend on final text and a pruned `MainWindow.cpp`.
- Splits and security fixes last, when the safety net exists.

### Research flags
- Needs research or a spike: phase 1 (light), phase 2 (tolerance spike), phase 4 (Retina experiment).
- Standard: phase 3, phase 5 splits and alarm, menu roles from Qt docs.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH (versions, CMake/Qt/Boost mechanics); MEDIUM (bare-executable behaviour) | Re-confirm `brew info` before build; Apple Clang version not found |
| Features | MEDIUM | LOW for Backspace on Qt 6 and unbundled Retina, flagged for empirical checks |
| Architecture | HIGH (seams, build order, verified in source); MEDIUM (Retina, Homebrew details) | Nothing built on the Mac yet |
| Pitfalls | MEDIUM | Repo-grounded items HIGH; macOS/libc++ items general knowledge, tagged for first-build verification |

**Overall: MEDIUM.** The plan is sound; the first build settles several items and the Retina scope depends on one measurement.

### Gaps to address
- **Unbundled Retina (gates HiDPI scope):** measure `devicePixelRatio` in phase 1. If 1.0, the fix is `Info.plist` (BLD-10) and most Retina code shrinks. No HiDPI code before this check.
- **`-ffp-contract=off`:** decide in phase 1, freeze before goldens, record; measure spread in phase 2.
- **CMake minimum:** STACK says 3.25 (Qt 6.12 requires it); PITFALLS suggests 3.21. Use 3.25; confirm at first configure.
- **Boost.Lambda/Function in 1.92:** not listed as removed; compile is the proof.
- **Dewarper stability:** `QRandomGenerator(0)` is reproducible per Qt build; record the Qt build string; do not change the PRNG (it changes output).
- **Backspace on Qt 6:** verify on the Mac in phase 4.
- **OpenGL on macOS:** macOS 27 removal claim unconfirmed (LOW); decide in phase 5.
- **Baseline `ctest`:** never run on this Mac; record in phase 1.
- **Tolerances:** starting points; set on first golden run, then freeze.
- **Retina and menu behaviour:** manual checklist only (100%/200% zoom, thumbnails, zone mask, colour picker, toolbar icons, moving between displays).

## Sources
- Homebrew formulae JSON API (2026-10-18): qt, qtbase, qtsvg, qttools, boost, jpeg-turbo, jpeg, libpng, libtiff, zlib, cmake, ninja, clang-format
- Qt 6.12 docs (macOS platform, What's New, `qt_standard_project_setup`, High DPI, `QAction` roles, `QMenuBar`, macOS issues, `QKeySequence`, `QIcon`); Qt source `qcocoaintegration.mm`, `qcocoaapplicationdelegate.mm`, `qiconengine.cpp`
- CMake docs (CMP0167, `cmake_minimum_required`, 4.0 notes, 3.19.2 notes); Boost 1.92.0 release notes; Boost.Test `config.hpp`
- libtiff `tiffio.h`; libc++ hardening; Clang `-ffp-contract` manual
- Repo: `.planning/PROJECT.md`, `.planning/codebase/*`, `CMakeLists.txt`, `src/app/main.cpp`, `src/app/MainWindow.{ui,cpp}`, `src/core/ImageViewBase.cpp`, `ThumbnailBase.cpp`, `ThumbnailPixmapCache.cpp`, `src/core/filters/output/*`, `src/core/filters/*/Task.cpp`, `src/dewarping/*`, `src/imageproc/tests/*`
- Secondary: Qt mailing list on Qt 6.12 CMake minimum; CMake Discourse and Orthanc (Boost config, `boost_system` stub removal); golden tolerance practice (unpaper, WebKit 149828, Hugo FMA patch, Flutter); arm64 FMA notes; KDE bug 358192; Qt OpenGL/macOS thread; `QRhiWidget` docs
- Tertiary (LOW): Xcode 27 developer blog; macOS 27 OpenGL forum post; QTBUG-33015 (Qt 5 era); fruitbasket and Qt Retina blog

---
*Research completed: 2026-10-18. Ready for roadmap once the phase-order decision is confirmed.*
