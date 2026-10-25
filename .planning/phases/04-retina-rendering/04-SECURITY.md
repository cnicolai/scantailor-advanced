---
phase: "4"
slug: "retina-rendering"
status: verified
# threats_open = count of OPEN threats at or above workflow.security_block_on severity (the blocking gate)
threats_open: 0
asvs_level: 1
created: "2026-10-25"
---

# Phase 4 — Security

> Per-phase security contract: threat register, accepted risks, and audit trail.

---

## Trust Boundaries

| Boundary | Description | Data Crossing |
|----------|-------------|---------------|
| View rendering ↔ output generation | Retina changes are confined to on-screen buffers; output files must not change | Page images, transforms; output bytes and goldens |
| Test and harness processes ↔ file system | `qt_tests` and `retina_check` write configuration, settings, caches and crops | Files below `build/` only; records under `.planning/` hold synthetic crops |
| Thumbnail cache ↔ project output folder | Thumbnails are written into `<output>/cache/thumbs` | Thumbnail PNG files of the user's pages |
| Settings ↔ Settings dialog | The Quality setting is read and written back by the dialog | Integer setting in the user's INI file |

---

## Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation | Status |
|-----------|----------|-----------|----------|-------------|------------|--------|
| T-04-01 | Denial of service | ImageViewBase HQ pixmap at viewport × ratio² | low | accept | Bounded by the viewport (~59 MB full-screen on 5K); see Accepted Risks | closed |
| T-04-02 | Tampering | Output generation (transform, downscale, Task::process) | medium | mitigate | No edit under `src/imageproc`, `src/dewarping`, any `Task.cpp` or `OutputGenerator.cpp`: `git diff 03083f55` over them is empty; goldens unchanged in every gate | closed |
| T-04-03 | Tampering | qt_tests screen configuration file | low | mitigate | Written under the build tree's `test-tmp`; `src/qt_tests/main.cpp` returns 2 if the write fails | closed |
| T-04-04 | Denial of service | `hidpi::deviceSize` / `targetRect` with an absurd ratio | low | mitigate | `hidpi::sanitizedRatio` maps non-finite and non-positive ratios to 1.0; `testHelpersAtFractionalRatio` | closed |
| T-04-05 | Information disclosure | retina_check settings and temporary files | medium | mitigate | Output directory must lie under `build/` ("is not under" → exit 2); settings, TMPDIR and outputs redirected below it | closed |
| T-04-06 | Information disclosure | records/hidpi-01.md, live-page-view.md, crops | low | mitigate | Synthetic page crops only; `grep -c "$HOME"` over records prints 0 | closed |
| T-04-07 | Tampering | Conditional Info.plist embedded by the linker | low | mitigate | Branch not taken (measured ratio 2); `src/app/Info.plist` absent, no `__info_plist` in CMake | closed |
| T-04-08 | Spoofing | The live measurement | low | mitigate | `retina_check` clears QT_QPA_PLATFORM and scale-factor variables (`qunsetenv`) and exits 3 unless the platform is cocoa | closed |
| T-04-09 | Denial of service | Thumbnail effective size from Quality × ratio | low | mitigate | `effectiveThumbSize` clamps each side to 1..`MAX_EFFECTIVE_THUMB_SIDE` (3000); INT_MAX tested | closed |
| T-04-10 | Tampering | Thumbnail files in `<output>/cache/thumbs` | low | mitigate | Width-encoded names, atomic writes (`AtomicFileOverwriter`), files of other widths never touched (`testOldThumbnailLeftInPlace`) | closed |
| T-04-11 | Tampering | Stored Quality setting | low | mitigate | No migration code; `thumbnailQualitySB` range stays 100..1500 | closed |
| T-04-12 | Tampering | Data race and stale results in the cache | low | mitigate | Effective size and generation copied under `m_mutex`; older-generation results discarded (`testStaleLoadDiscarded`) | closed |
| T-04-13 | Denial of service | QPixmapCache temp pixmap at the largest thumbnail Size | low | accept | Re-allocation per paint at extreme Size, not wrong output; see Accepted Risks | closed |
| T-04-14 | Denial of service | ThumbnailDprTracker holding a replaced cache | low | mitigate | `std::weak_ptr` in `ThumbnailDprTracker.h`; null cache ignored | closed |
| T-04-15 | Tampering | Tracker ownership | low | mitigate | `new ThumbnailDprTracker(thumbView)` — owned by the Qt parent; MainWindow keeps a non-owning pointer | closed |
| T-04-16 | Denial of service | PictureZoneEditor per-frame mask copy | low | accept | Four times larger at ratio 2, bounded by the viewport; see Accepted Risks | closed |
| T-04-17 | Denial of service | Repeated DevicePixelRatioChange events | low | mitigate | `qFuzzyCompare` guard in `PictureZoneEditor.cpp` and `StageListView.cpp`; repeated-event tests | closed |
| T-04-18 | Tampering | Output generation through transformToGray | low | mitigate | `transformToGray` not edited; `git diff 03083f55 -- src/imageproc src/core/filters/output/Task.cpp` empty | closed |
| T-04-19 | Tampering | page_split::UnremoveButton hit rectangle | low | mitigate | Logical rect; `testIconPixmapLogicalGeometry` checks hover inside 64 px and not at 100 px | closed |
| T-04-20 | Tampering | Fill colour stored by the colour picker | low | mitigate | Median moved unchanged, ratio-1 predicate identical; `testMedianColorRatioInvariant` | closed |
| T-04-21 | Tampering | IconProvider singleton in qt_tests | low | mitigate | `IconPackReset` scope guard in both icon tests | closed |
| T-04-22 | Information disclosure | retina_check thumbnail cache, settings and files | medium | mitigate | All paths below the output directory under `build/`; cursor not moved; app binary not launched | closed |
| T-04-23 | Information disclosure | records/live-overlays.md and crops | low | mitigate | Generated page, masks and project icons only; `grep -c "$HOME"` prints 0 | closed |
| T-04-24 | Repudiation | Live verdicts | low | mitigate | Thresholds fixed in plan and record; two recipe corrections documented with first-trial values, no threshold changed | closed |
| T-04-25 | Repudiation | Criterion 4 claim | low | mitigate | `records/closing-check.md` states criterion 4 rests on the offscreen simulation and names A1 | closed |
| T-04-26 | Tampering | Output files and goldens | medium | mitigate | Output-code and golden diffs against 059f4800/03083f55 and a796c2ae empty; every gate-log row shows unchanged goldens | closed |
| T-04-27 | Tampering | Memory errors in the new pixel loops | low | mitigate | ASan/UBSan run of qt_tests: 0 ASan errors; the single UBSan report is the pre-existing `BinaryImage.cpp:1044` finding, not a Phase 4 file | closed |
| T-04-28 | Tampering | Conditional recalibration of the smoothing switch | low | mitigate | Branch not taken (`Smoothing switch: no Retina correction needed (ratio 0.9978)`) | closed |
| T-04-SC | Tampering | Package installs | low | accept | No package installed or added in this phase; see Accepted Risks | closed |

*Status: open · closed · open — below high threshold (non-blocking)*
*Severity: critical > high > medium > low — only open threats at or above workflow.security_block_on count toward threats_open*
*Disposition: mitigate (implementation required) · accept (documented risk) · transfer (third-party)*

---

## Accepted Risks Log

| Risk ID | Threat Ref | Rationale | Accepted By | Date |
|---------|------------|-----------|-------------|------|
| AR-04-01 | T-04-01 | HQ pixmap memory at ratio 2 (~59 MB full-screen on 5K) is bounded by the viewport; accepted in CONTEXT (Memory) | Plan 04-01 threat model, per CONTEXT decision | 2026-10-25 |
| AR-04-02 | T-04-13 | Temp pixmap may exceed the default QPixmapCache limit only at the largest thumbnail Size; cost is a re-allocation, not wrong output | Plan 04-04 threat model (RESEARCH Pitfall 6) | 2026-10-25 |
| AR-04-03 | T-04-16 | Per-frame mask copy four times larger at ratio 2, bounded by the viewport | Plan 04-05 threat model, per CONTEXT decision | 2026-10-25 |
| AR-04-04 | T-04-SC | No package is installed or added; Qt and image libraries remain the Phase 1 Homebrew installs | All plan threat models | 2026-10-25 |

*Accepted risks do not resurface in future audit runs.*

---

## Security Audit Trail

| Audit Date | Threats Total | Closed | Open | Run By |
|------------|---------------|--------|------|--------|
| 2026-10-25 | 29 | 29 | 0 | secure-phase orchestrator (L1 grep-depth, auditor short-circuit) |

---

## Sign-Off

- [x] All threats have a disposition (mitigate / accept / transfer)
- [x] Accepted risks documented in Accepted Risks Log
- [x] `threats_open: 0` confirmed
- [x] `status: verified` set in frontmatter

**Approval:** verified 2026-10-25
