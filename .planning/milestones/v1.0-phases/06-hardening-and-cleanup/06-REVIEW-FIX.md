---
phase: 06-hardening-and-cleanup
fixed_at: 2026-11-17T00:00:00Z
review_path: .planning/phases/06-hardening-and-cleanup/06-REVIEW.md
iteration: 1
findings_in_scope: 1
fixed: 1
skipped: 0
status: all_fixed
---

# Phase 6: Code Review Fix Report

**Fixed at:** 2026-11-17
**Source review:** .planning/phases/06-hardening-and-cleanup/06-REVIEW.md
**Iteration:** 1

**Summary:**
- Findings in scope: 1 (WR-07; IN-04..IN-06 are Info and out of scope for `critical_warning`)
- Fixed: 1
- Skipped: 0

## Fixed Issues

### WR-07: WR-04 fix still allocates the full coefficient image for non-interleaved baseline JPEGs

**Files modified:** `src/core/JpegMetadataLoader.cpp`, `src/core/tests/TestImageSizeLimit.cpp`, `src/test_support/ForgedImages.cpp`, `src/test_support/ForgedImages.h`
**Commit:** 5bf8fe1e
**Applied fix:**
- `JpegMetadataLoader.cpp`: the `jpeg_start_decompress` call is now skipped whenever `jpeg_has_multiple_scans(cinfo.ptr())` is true (progressive, or baseline with one component per scan), instead of only when `progressive_mode` is set. Both nearby comments now say "multi-scan".
- `ForgedImages.{h,cpp}`: new `forgedNonInterleavedJpeg(width, height)`, a baseline (SOF0) file encoded with three one-component scans and its SOF0 patched to the claimed size. The existing `forgedJpeg` keeps its signature and behaviour (both share an internal helper).
- `TestImageSizeLimit.cpp`: new case `test_non_interleaved_baseline_jpeg_inside_the_limit_is_not_decompressed_for_metadata`, next to the progressive one. It forges 65500 x 8191, checks the reported size, `!ImageSizeLimit::exceeds`, and the same `FootprintSampler` bound (64 MiB) and 1 s time bound.

**Status:** fixed (logic change; the test below demonstrates it, but a quick human look at the condition is cheap).

**Discrimination (footprint growth during the metadata read, release build):**

| State | Growth | Result |
|-------|--------|--------|
| New test, `JpegMetadataLoader.cpp` as before the fix | 1,610,892,440 bytes (about 1.5 GiB) | fails: `growth < kFootprintBoundBytes` `[1610892440 >= 67108864]` |
| New test, with the fix | 32,768 bytes (measured with the bound temporarily set to 1 byte, then restored) | passes |

## Verification

Ran in the main checkout (see Deviations), not in an isolated worktree, so the numbers are reproducible from the tree you are looking at.

- `xcrun clang-format --dry-run -Werror` on all four touched files: clean.
- `cmake --build --preset macos-release --target core_tests`, then `core_tests --run_test=CoreImageSizeLimitTestSuite`: 17 cases, no errors.
- Full `ctest --preset macos-release`: 7 of 7 suites passed.

## Deviations

- `workflow.use_worktrees` is not set in `.planning/config.json` (default true), but no worktree was created: the build tree (`build/macos-release`) and the scratch probe live in the main checkout, and a fresh worktree would need a full rebuild of the project to run the gates. The orchestrator was waiting on this run, so nothing raced the working tree. Commits went onto `gsd/phase-01-deletion-free-macos-build` directly and no recovery sentinel exists.
- `gsd_run query commit` is not available in this shell; the commit was made with `git commit` using the required message form and trailer.

---

_Fixed: 2026-11-17_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
