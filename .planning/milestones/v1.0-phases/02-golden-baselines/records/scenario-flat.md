# Flat-document scenarios: ground-truth measurements, tolerances, goldens (plan 02-05)

Measured with Qt 6.12.0 (Homebrew), preset `macos-release`, on the unmodified production code of commit `cde6825` (the only non-test path that differs is `src/CMakeLists.txt`, the two test `add_subdirectory` lines). Fixture: `PageSpec{FLAT, dpi, seed 1, skewDeg 1.5}` for dpi 300 and 600, pipeline defaults plus `outputDpi` 600, AUTO layout, despeckle 1.0; `BLACK_AND_WHITE` for `_bw`, `COLOR_GRAYSCALE` for `_color`. Every value below is a `TRUTH-METRIC suite=flat` line of `build/macos-release/pipeline_tests --run_test=FlatDocumentTestSuite --log_level=message`.

## Measurements

| Scenario | Stored deskew angle (deg) | Deskew error (deg) | Content-box IoU | Binarization mismatch (edge radius) | Ink/paper contrast (grey levels) | Output (px) | Registration offset (px) |
|----------|--------------------------|--------------------|-----------------|--------------------------------------|----------------------------------|-------------|--------------------------|
| flat_300_bw | -1.5 | 0 | 0.9813122924 | 0 (2) | n/a | 1676 x 1636 | (-161, -332) |
| flat_300_color | -1.5 | 0 | 0.9813122924 | n/a | 178.4609922 | 1676 x 1636 | (-161, -332) |
| flat_600_bw | -1.5 | 0 | 0.9813122924 | 0.08290381125 (1), see F-05-1 | n/a | 1676 x 1636 | (-163, -332) |
| flat_600_color | -1.5 | 0 | 0.9813122924 | n/a | 179.569152 | 1676 x 1636 | (-163, -332) |

Further numbers of the stage decisions (identical for black-and-white and colour at a given input dpi): content box 602 x 700 px at 300 dpi and 1204 x 1400 px at 600 dpi against the truth text block of 595 x 695 and 1190 x 1390 px; page rect 1031.344251 x 1175.782872 (300 dpi) and 2066.64211 x 2351.565744 (600 dpi); hard margins left 10, top 5, right 10, bottom 5 mm. The default margins make the output nearly square (1676 x 1636 px, wider than high by 40 px), see "Sanity" below.

Registration offset: translation of the truth mask (rendered at 600 dpi, net rotation 0) onto the output, from `registerMask(output, mask, 4)`.

Sanity (D-05): all four scenarios pass `outputSanity` (size, ink fraction 0.5% to 50%), the text-line count within one of 13, and the left/top glyph position within 5 mm of the stored hard margins. The plan names "portrait" as one `outputSanity` condition; with the default hard margins the whole output is 1676 x 1636 px, so that condition failed on all four scenarios although the content is upright. The orientation is therefore checked on the glyph ink box (`nonBackgroundBox`, portrait required: 1204 x 1400 px at the content level), not on the page with its margins. A rotated page would still fail it. See the deviation in the plan summary.

## Deskew sweep

Applied angle is `PageSpec::skewDeg` (positive turns the content clockwise); the deskew stage stores the angle that straightens the page, the negative of the applied one. Error is the absolute value of stored plus applied. `LastStage::DESKEW`, seed 1.

| DPI | Applied (deg) | Stored (deg) | Error (deg) |
|-----|---------------|--------------|-------------|
| 300 | -3.0 | 3 | 0 |
| 300 | -1.5 | 1.5 | 0 |
| 300 | -0.5 | 0.375 | 0.125 |
| 300 | 0.5 | -0.5 | 0 |
| 300 | 1.5 | -1.5 | 0 |
| 300 | 2.0 | -2.0625 | 0.0625 |
| 300 | 3.0 | -3 | 0 |
| 600 | -3.0 | 3 | 0 |
| 600 | -1.5 | 1.5 | 0 |
| 600 | -0.5 | 0.5 | 0 |
| 600 | 0.5 | -0.5 | 0 |
| 600 | 1.5 | -1.5 | 0 |
| 600 | 2.0 | -2 | 0 |
| 600 | 3.0 | -3 | 0 |

Worst case 0.125 deg (300 dpi, applied -0.5), as predicted by research P4.

## Tolerance decisions

Rule: D-15 as written in plan 02-04 (worst case over all fixtures of the check; at most half the starting tolerance gives twice the worst case rounded up to the step; otherwise the starting tolerance stays; above the starting tolerance D-18 applies).

| Check | Starting tolerance | Worst case | Outcome | Constant in `TestFlatDocument.cpp` |
|-------|--------------------|------------|---------|------------------------------------|
| Deskew error, four scenarios and the 14 sweep runs | 0.15 deg | 0.125 deg | kept: 0.125 is above half of the starting tolerance (0.075) | `kAngleToleranceDeg = 0.15` |
| Content-box IoU, four scenarios | 0.95 (1 - IoU 0.05) | 1 - IoU = 0.0186877 | tightened: twice the worst case is 0.0373754, rounded up to the step 0.005 gives 0.04, so IoU at least 0.96 | `kMinContentIou = 0.96` |
| Binarization mismatch, `_bw` scenarios | 0.5% of the ink area (0.005) | 0.0829038 (flat_600_bw) | kept; the worst case exceeds the starting tolerance, so D-18 applies to flat_600_bw: the check is an `XFAIL_CHECK` (F-05-1). flat_300_bw measures 0 and passes the unchanged bound | `kMaxBinarizationMismatch = 0.005` |
| Ink/paper contrast, `_color` scenarios | 100 grey levels (own check, D-15 names no colour value) | smallest contrast 178.46 | not subject to D-15, kept | `kMinInkPaperContrast = 100.0` |

## Findings

### F-05-1: Output of the 600 dpi flat page keeps about 0.1 degrees of residual skew
- Check: `FlatDocumentTestSuite/test_flat_600_bw_truth`, binarization mismatch of the output against `renderInkMask(spec, 600, netRotation)` after `registerMask`.
- Measured: 0.0829038 (edge-tolerant mismatch, edge radius 1) at 600 dpi input; 0 at 300 dpi input (edge radius 2).
- Bound: 0.005.
- Fixture: FLAT, 600 dpi, seed 1, skew 1.5 degrees, black-and-white output at 600 dpi.
- Mark: `XFAIL_CHECK(binarizationOk, "output of the 600 dpi flat page keeps about 0.1 deg residual skew, binarization mismatch 8.3% against 0.5% (F-05-1): no requirement")`
- Fixing requirement: none
- Backlog: 999.3 (ROADMAP.md, Backlog section; consolidated in 02-FINDINGS.md)

Evidence that the cause is a rotation the stored angle does not show: the stored deskew angle is exactly -1.5 deg, and the generated input matches the truth mask rotated by 1.5 deg (exclusive-or 1833 pixels at 600 dpi, rising to over 15000 at 1.46 and 1.54 deg). In the output, the best-registered local offset of the truth mask changes with the row: in three horizontal bands of the page it is -1, 0 and +2 px in x at 600 dpi (0, 0 and +1 px at 300 dpi), a shear that grows with y. Searching the rotation of the truth mask gives the best match at net rotation -0.08 to -0.12 deg for the 600 dpi input (mismatch 0.0070 at -0.12 deg, 0.0105 at -0.08 deg, 0.0829 at 0) and about -0.04 deg for the 300 dpi input. The cause inside the output or deskew transformation was not analysed; only the effect is recorded. The golden of flat_600_bw records this behaviour (D-18).

## Goldens

Recorded with `UPDATE_GOLDENS=1 build/macos-release/pipeline_tests --run_test=FlatDocumentTestSuite` from a tree whose non-test paths equal `cde6825` apart from `src/CMakeLists.txt` (the plan's gate command printed `src/CMakeLists.txt` only). Every scenario logged `RECORDED flat/<name>` and no `sanity failed`. The suite directory is `src/pipeline_tests/golden/flat/`. Golden commit: `2f6df32` (`test(02-05): record flat-document goldens from macos-release`, ten files, all under `golden/flat/`). The manifest names `build_type = Release`, `ffp_contract = off` and `source_commit = 24d18b6` (the test-code commit; the manifest was refreshed once after that commit, no golden file changed).

| File | Kind | Bytes | Pixels |
|------|------|-------|--------|
| flat_300_bw.png | bi-level (1 bit) | 14127 | 1676 x 1636 |
| flat_300_bw.txt | text, 20 keys | 447 | n/a |
| flat_300_color.png | colour (8 bit RGB) | 1129151 | 1676 x 1636 |
| flat_300_color.txt | text, 20 keys | 447 | n/a |
| flat_600_bw.png | bi-level (1 bit) | 11087 | 1676 x 1636 |
| flat_600_bw.txt | text, 20 keys | 449 | n/a |
| flat_600_color.png | colour (8 bit RGB) | 1235349 | 1676 x 1636 |
| flat_600_color.txt | text, 20 keys | 449 | n/a |
| fixtures.txt | text, 18 keys (14 sweep inputs `flat_<dpi>_skew_<angle>`, 4 scenario inputs `scenario_<name>`) | 1562 | n/a |
| MANIFEST.txt | toolchain | 333 | n/a |

The flat_600_bw golden records the behaviour of F-05-1 (output with about 0.1 degrees of residual skew), as D-18 requires.

Checks after recording: `FlatDocumentTestSuite` passes again in Release without `UPDATE_GOLDENS` (114 `GOLDEN-METRIC suite=flat` lines, `git status` shows nothing under `golden/flat`); `ctest --preset macos-debug -R pipeline_tests` passes (114 s) against the Release goldens; `ctest --preset macos-release -R pipeline_tests` passes (24 s).
