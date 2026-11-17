---
phase: "6"
slug: "hardening-and-cleanup"
status: verified
# threats_open = count of OPEN threats at or above workflow.security_block_on severity (the blocking gate)
threats_open: 0
asvs_level: 1
created: "2026-11-17"
---

# Phase 6 — Security

> Per-phase security contract: threat register, accepted risks, and audit trail.

---

## Trust Boundaries

| Boundary | Description | Data Crossing |
|----------|-------------|---------------|
| source edit → compiled program | Dead-code removal must not change the compiled code or the output | Source text |
| image file on disk → image readers | Header values are attacker-controlled and decide allocation sizes | TIFF/PNG/JPEG headers |
| user-chosen folder → project file dialog, insert flow | Every file's header is read; an oversized file must not enter the project | Image headers |
| thumbnail worker thread → image readers | Thumbnails load in a background thread from project files | Image files |
| file-system names → project file | APFS allows names that XML 1.0 forbids; they flow into attributes | File, folder and output-folder names |
| project file → ProjectReader | A project file may be hand-edited or come from another build | XML attributes, percent sequences |
| project page sizes, user setting → worker pool | Large pages in parallel can exhaust RAM; the stored slider value may be any integer | Page sizes, `settings/batch_processing_threads` |
| user settings file → application | A stored `enable_opengl` value stays untouched and unused | INI value |
| build configuration → binaries | Removing modules changes every link line; nothing else may change | Compile and link commands |
| user-chosen save path → project file | The panel returns any writable path | Path string |
| test processes → file system and settings | Tests write forged files and settings only under `build/<preset>/test-tmp` | Test files, QSettings |
| source layout → output stage and GUI | Moving code between translation units must not change pixels, menus, keys or connections | Source text |
| probes and closing runs → the user's live session | The memory run uses gigabytes; mac_check takes the pointer and focus | RAM, pointer, focus |
| records → repository history | Closing evidence could carry local paths | Record text |

---

## Threat Register

Register authored at plan time (the `<threat_model>` blocks of plans 06-01 to 06-16); verified at ASVS level 1 (grep depth plus the passing tests and closing runs named in each row). `T-06-SC` repeats in every plan by convention.

| Threat ID | Category | Component | Severity | Disposition | Mitigation | Status |
|-----------|----------|-----------|----------|-------------|------------|--------|
| T-06-01 | Denial of service | TiffReader::readImage (header bomb: 13 KB TIFF cost 6.1 GB) | high | mitigate | ImageSizeLimit::exceeds on header values before either QImage constructor; peak-footprint tests with a mutation run that proves they detect the unmodified behaviour. Evidence: `ImageSizeLimit::exceeds` in `TiffReader.cpp:277`, enforced for every source scan (`ImageLoader::loadSource` → `enforceSizeLimit`; WR-01 left only the app's own output and cache files unlimited); `CoreImageSizeLimitTestSuite` 16 cases green; mutation runs in 06-04 and the WR-05 mutation run with the sampled footprint | closed |
| T-06-02 | Denial of service | JpegMetadataLoader (1 KB progressive JPEG cost 12.3 GB in jpeg_start_decompress) | high | mitigate | size check between jpeg_read_header and jpeg_start_decompress; test with a progressive forgery (a baseline forgery is harmless and would prove nothing, Pitfall 9). Evidence: check between `jpeg_read_header` and `jpeg_start_decompress` in `JpegMetadataLoader.cpp:214`; since WR-04 progressive files inside the limit skip `jpeg_start_decompress` too (`:225`); progressive-forgery tests green (06-05), `test_progressive_jpeg_inside_the_limit_is_not_decompressed_for_metadata` green | closed |
| T-06-03 | Denial of service (self-inflicted swap) | WorkerThreadPool running too many large page tasks | high | mitigate | MemoryBudget cap from hw.memsize and the largest page, applied at batch start before the first submit; arithmetic and pool tests; the D-08 run in plan 06-16 drives large pages through the real pool with the same cap and checks swap and peak. Evidence: `MainWindowBatch.cpp:148-151` sets `batchMemoryCap` (each page's own output dpi, WR-03) before submit; the cap resets to `kNoCap` when a batch stops or the project changes (WR-02, `MainWindowBatch.cpp:193`, `MainWindowProject.cpp:99`; UAT test 1 pass); `CoreMemoryBudgetTestSuite` 10 cases, `testWorkerPoolHonoursMemoryCap`; D-08 run: cap 5 of 10, peak 17737 MiB, swap 0 | closed |
| T-06-04 | Tampering (data loss) | ProjectWriter writing raw XML-1.0-forbidden characters, making the project unloadable | high | mitigate | escape plus flag attribute only when needed; round-trip test through both parse paths with control characters in file, folder and output-folder names. Evidence: escaped attributes in `ProjectWriter`/`ProjectReader`; `test_control_characters_survive`, `test_unusual_names_survive` green | closed |
| T-06-05 | Tampering | the five two-branch #if 0 blocks (live #else code) | high | mitigate | pp_equiv.py proves equal Release preprocessor output for all seven files, with a positive control that it detects a one-line change; gate runs Release and Debug ctest and compares goldens. Evidence: `records/pp_equiv.py` proof with positive control (06-01/06-02); gates green, goldens unchanged | closed |
| T-06-06 | Tampering | include removal in the 13 foundation, math and core files (two headers included elsewhere) | medium | mitigate | full Release and Debug build in the gate; missing names are fixed with the direct header, never by restoring the removed one. Evidence: Release and Debug gates green (06-02/06-03) | closed |
| T-06-07 | Tampering | header width or height of 2^31 or more read into int | medium | mitigate | negative sides count as too large; sizeText prints the unsigned value; 2000000000 x 2 test case. Evidence: 2000000000 x 2 case in `TestImageSizeLimit.cpp`; green | closed |
| T-06-08 | Denial of service | Qt reader limit raised from 256 MB to 2048 MB | low | accept | 2048 MB is the smallest whole-MB value that holds the largest accepted RGB32 image (4 B x (2^29 - 1) px), which ImageSizeLimit enforces first; the limit is never 0. Evidence: `setAllocationLimit(0)` nowhere in `src`; limit set once to 2048 via `std::call_once` | closed |
| T-06-09 | Denial of service | ProjectFilesDialog and the insert flow admitting an oversized file into a project | medium | mitigate | oversizedPagesText after every LOADED status; the file is refused and named; dialog test with forged and two-page files and a control folder. Evidence: `oversizedPagesText` in `ProjectFilesDialog.cpp` and the insert flow; `testProjectFilesDialogMarksOversizedFile` green | closed |
| T-06-10 | Denial of service | ThumbnailPixmapCache background loading of oversized files | high | mitigate | source images load only through ImageLoader::load (source assertion); thumbnail test checks LOAD_FAILED and peak footprint. Evidence: `ThumbnailPixmapCache.cpp:695` loads source images through `ImageLoader::loadSource` (the unlimited `load` at `:690` reads only the cache's own thumbnail file); `testOversizedThumbnailFails` green | closed |
| T-06-11 | Tampering | ProjectReader decoding attacker-written percent sequences | low | mitigate | decoding only when the flag is exactly 1; an unflagged %01 stays literal (unit test); decoded value is used as a path exactly like an unescaped one, no new path handling. Evidence: flag compared to exactly `"1"` in `XmlUnmarshaller.cpp`; `FoundationXmlEscapeTestSuite` green | closed |
| T-06-12 | Tampering | stored slider value out of range (0, negative, huge) | low | mitigate | storedThreadCount clamps to [1, idealThreadCount]; effectiveThreads never returns below 1. Evidence: `WorkerThreadPool::storedThreadCount` clamps; `testStoredThreadCountClamps` green | closed |
| T-06-13 | Denial of service | integer overflow in the cap arithmetic for huge pages | medium | mitigate | qint64 arithmetic, estimate clamped to 2^40 px, result clamped to [1, INT_MAX]; overflow vectors in the tests. Evidence: qint64 arithmetic and clamps in `MemoryBudget.cpp`; overflow vector in `test_page_pixel_estimate` | closed |
| T-06-14 | Tampering | colour picker after removing the OpenGL sampling branch | medium | mitigate | the branch was guarded by a setting that defaults to false; picker PASS lines before and after; tst_hidpi_qt.cpp unchanged. Evidence: colour-picker PASS lines before and after (06-10); `tst_hidpi_qt.cpp` unchanged | closed |
| T-06-15 | Tampering | build commands of all targets (module removal) | medium | mitigate | normalised command-diff comparison: only QtOpenGL include and link items and OpenGLSupport lines may differ; Release and Debug ctest in the gate. Evidence: token-level command diff: only OpenGL items removed (06-11, `records/strip-07.md`); `otool -L` shows no QtOpenGL | closed |
| T-06-16 | Tampering | ProjectSaveFlow replacing two copies (a behaviour drift between them) | low | mitigate | bodies moved verbatim; end-to-end test through the out-of-memory dialog; File-menu and native-panel tests unchanged and passing. Evidence: `testSaveAsFlowFromOutOfMemoryDialog`, `testSaveAsFlowRunsTwice` green; File-menu tests unchanged | closed |
| T-06-17 | Information disclosure | settings written by the save tests | low | mitigate | qt_tests redirects QSettings to build/<preset>/test-tmp and refuses to run otherwise; the tests restore the keys they touch. Evidence: `src/qt_tests/main.cpp` redirects QSettings under test-tmp and refuses otherwise | closed |
| T-06-18 | Tampering | output stage split, first step (Processor class, shared helpers, no-dewarping path; a changed body or flag would change output) | high | mitigate | move_check.py chunk multiset with positive controls; decoded-pixel digests before and after the step; goldens via the gate; -ffp-contract=off verified for both units. Evidence: move check PASS, digests equal, gates s17/s18 green, `-ffp-contract=off` on both units | closed |
| T-06-19 | Tampering | helpers losing internal linkage (ODR clashes across the output library) | medium | mitigate | helpers stay in anonymous namespaces, either in their file or in the internal header. Evidence: helpers in anonymous namespaces in `OutputGeneratorPixelOps.h` and the topic files | closed |
| T-06-20 | Tampering | MainWindow split (changed connection order or body) | medium | mitigate | move_check.py chunk multiset against the pre-split file, with the constructor/setupActions exemption limited to three allowed lines; the full offscreen TestMacQt suite on every commit; live mac_check in plan 06-16. Evidence: move check against 590e751f PASS; `setupActions` reproduces the constructor 256/256 lines; live mac_check PASS | closed |
| T-06-21 | Repudiation | source scan tests reading a file that no longer holds the code | low | mitigate | testBatchAlarmUsesBeep reads MainWindowBatch.cpp and scans every MainWindow*.cpp with a file-count control. Evidence: `testBatchAlarmUsesBeep` reads `MainWindowBatch.cpp` with a file-count control; green | closed |
| T-06-22 | Denial of service | the D-08 memory run on the user's Mac (about 25 to 31 GB) | medium | mitigate | blocking-human checkpoint that states the memory use and asks to quit memory-heavy apps; single-page calibration first; one process with at most the computed cap of pages at once (5 on this Mac), total estimate inside the half-of-RAM budget; no uncapped comparison; swap sampled every 2 s; the probe stops submitting on its 3600 s deadline. Evidence: run only after the user's go on 2026-11-17; calibration first; capped at 5; peak 17737 MiB of 32768; swap 0 | closed |
| T-06-23 | Information disclosure | mac_check dialogs and panels reaching the home folder | high | mitigate | the panels invocation runs only with HOME and CFFIXED_USER_HOME redirected below build/ (harness guard from Phase 5). Evidence: panels run with HOME and CFFIXED_USER_HOME under `build/scratch/mac/phase6-panels/home` | closed |
| T-06-24 | Information disclosure | closing-check.md, memory-run.md, TESTING.md | low | mitigate | {repo} and ~ substitution; verify counts $HOME occurrences (must be 0). Evidence: `$HOME` count 0 in closing-check.md, memory-run.md and TESTING.md | closed |
| T-06-25 | Tampering | include removal in the 13 imageproc, dewarping and app files (output-generation code under src/imageproc and src/dewarping) | medium | mitigate | full Release and Debug build and ctest with golden comparison in the gate; the output-path guard allows only the allowlisted CLEAN-03/04 files; missing names are fixed with the direct header, never by restoring the removed one. Evidence: gates green with goldens unchanged; output-path guard clean (06-03) | closed |
| T-06-26 | Denial of service | ImageLoader::load for PNG and JPEG (a forged IHDR or SOF header claims the image size; Qt allocates the claimed size once its limit is raised) | high | mitigate | reader.size() checked with ImageSizeLimit::exceeds before QImageReader::read; forged PNG and progressive JPEG tests with peak-footprint and time limits. Evidence: `ImageSizeLimit::exceeds` before `QImageReader::read` in `ImageLoader.cpp:77`, enforced on the `loadSource` path; `test_own_files_are_not_limited_but_source_files_are` green; real wide-PNG cases in `testLargeGreyPngLoads` discriminate (06-05) | closed |
| T-06-27 | Denial of service | BatchMemoryProbeTestSuite started at a large size outside the D-08 checkpoint | low | mitigate | disabled by default and absent from ctest (list_content check); defaults are small (4 pages of 1.15 MP); TESTING.md and plan 06-16 run it large only after the user's go; the probe stops submitting after 3600 s. Evidence: both probes `disabled()`; not listed as enabled in `--list_content`; small defaults | closed |
| T-06-28 | Tampering (integer overflow) | int pixel and byte arithmetic on an image at the size ceiling (an RGB32 image of exactly 2^29 px takes 2^31 bytes, one more than INT_MAX) | medium | mitigate | the total-pixel limit is exclusive (kMaxPixels = 2^29 - 1) with static_assert(4 * kMaxPixels <= INT_MAX); the RGB header of exactly 2^29 px is refused in test_rgb_tiff_at_the_ceiling_is_refused_without_allocating; the int arithmetic reachable for a loaded source image is audited in plan 06-06. Evidence: `kMaxPixels = (qint64(1) << 29) - 1` and `static_assert` in `ImageSizeLimit.h`; `test_rgb_tiff_at_the_ceiling` green | closed |
| T-06-29 | Tampering | output stage split, second step (dewarping and binarization paths; a changed body or flag would change output) | high | mitigate | move_check.py over all six files against e5319cd9; decoded-pixel digests against the pre-split reference; goldens via the gate; -ffp-contract=off verified for all four units. Evidence: move check over six files PASS (79/79), digests equal, gate s19 green, `-ffp-contract=off` on all four units | closed |
| T-06-30 | Tampering | helper placement in the second step (a helper given external linkage clashes across the output library, or a caller-less helper is dropped) | medium | mitigate | helpers stay in anonymous namespaces in their file or in OutputGeneratorPixelOps.h; the move check counts every chunk; grep for calcLocalWindowSize and from300dpi in OutputGeneratorBinarize.cpp. Evidence: helpers in anonymous namespaces; `calcLocalWindowSize` and `from300dpi` present in `OutputGeneratorBinarize.cpp` | closed |
| T-06-31 | Denial of service | PngMetadataLoader with libpng's side limit lifted | low | accept | png_read_info reads the header and ancillary chunks only (chunk sizes keep libpng's own malloc limit); nothing is allocated in proportion to the claimed size, and every consumer applies ImageSizeLimit to the reported size. Evidence: `png_set_user_limits` only lifts the side limit; consumers apply `ImageSizeLimit` to the reported size | closed |
| T-06-SC | Tampering | npm/pip/cargo installs | low | accept | No package is installed or added in this phase (RESEARCH "Package Legitimacy Audit": none). Evidence: no package added (`git diff e5319cd9..HEAD` touches no manifest) | closed |

*Status: open · closed · open — below high threshold (non-blocking)*
*Severity: critical > high > medium > low — only open threats at or above workflow.security_block_on count toward threats_open*
*Disposition: mitigate (implementation required) · accept (documented risk) · transfer (third-party)*

Summary threat flag (06-04 `transient-header-reread`, `src/core/ImageSizeLimit.cpp`): the window it named closed with plan 06-05, which added the JPEG size check before `jpeg_start_decompress` (T-06-02).

---

## Accepted Risks Log

| Risk ID | Threat Ref | Rationale | Accepted By | Date |
|---------|------------|-----------|-------------|------|
| AR-06-01 | T-06-08 | 2048 MB is the smallest whole-MB value that holds the largest accepted RGB32 image (4 B x (2^29 - 1) px), which ImageSizeLimit enforces first; the limit is never 0 | plan-time threat model (06 plans) | 2026-11-16 |
| AR-06-02 | T-06-31 | png_read_info reads the header and ancillary chunks only (chunk sizes keep libpng's own malloc limit); nothing is allocated in proportion to the claimed size, and every consumer applies ImageSizeLimit to the reported size | plan-time threat model (06 plans) | 2026-11-16 |
| AR-06-03 | T-06-SC | No package is installed or added in this phase (RESEARCH "Package Legitimacy Audit": none) | plan-time threat model (06 plans) | 2026-11-16 |

*Accepted risks do not resurface in future audit runs.*

---

## Security Audit Trail

| Audit Date | Threats Total | Closed | Open | Run By |
|------------|---------------|--------|------|--------|
| 2026-11-17 | 32 | 32 | 0 | secure-phase orchestrator (L1 short-circuit: plan-time register, 0 open, ASVS 1) |
| 2026-11-17 | 32 | 32 | 0 | secure-phase orchestrator, re-run after the WR-01–WR-06 review fixes (L1 short-circuit; evidence for T-06-01/02/03/10/26 re-pointed at grep depth) |

---

## Sign-Off

- [x] All threats have a disposition (mitigate / accept / transfer)
- [x] Accepted risks documented in Accepted Risks Log
- [x] `threats_open: 0` confirmed
- [x] `status: verified` set in frontmatter

**Approval:** verified 2026-11-17

## Security Audit 2026-11-17

| Metric | Count |
|---|---|
| Threats found | 32 |
| Closed | 32 |
| Open | 0 |
