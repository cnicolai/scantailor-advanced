# Gate log, Phase 4 Retina commits

Each Retina commit that changes a path outside `.planning/` and `.claude/` was gated by `gate.sh check <label>` on the committed tree: configure, build and ctest of macos-release and macos-debug, goldens compared with a796c2ae. The command diff compares the sorted build commands of the eight existing targets (plus `ui_snapshot`) with those of the previous commit.

| Label | Commit | Subject | Release ctest | Debug ctest | Goldens | Command diff (release / debug) | Expected diff met |
|-------|--------|---------|---------------|-------------|---------|--------------------------------|-------------------|
| r01-hq-tracer | 7c730ab2 | feat(04-01): build the page view HQ pixmap at device resolution | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 6 / 6 | yes: only the HiDpi.cpp and tst_hidpi_qt.cpp compile lines, the core archive line and the qt_tests link line (no automoc change) |
| r02-hq-dpr-change | 288ea84b | feat(04-01): rebuild the HQ pixmap when the device pixel ratio changes | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 0 / 0 | yes: empty (edits only) |
| r03-retina-check | e028d3b1 | test(04-02): add the live Retina check harness | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 0 / 0 | yes: empty (retina_check is not a gated target) |
| r04-thumb-cache | 7ccaa186 | feat(04-03): key the thumbnail cache by device pixel ratio | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 0 / 0 | yes: empty (edits only) |
| r05-thumb-quality | 18371848 | feat(04-03): raise the thumbnail quality default to 250 | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 0 / 0 | yes: empty (edits only) |
| r06-thumb-paint | a2396af5 | feat(04-04): draw thumbnails at device resolution | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 0 / 0 | yes: empty (edits only) |
| r07-thumb-dpr-follow | 489165ea | feat(04-04): follow display changes in the thumbnail cache | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 3 / 3 | yes: only the new ThumbnailDprTracker.cpp compile line and the core archive line (no automoc change) |
| r08-zone-mask | 6e7a4eff | feat(04-05): build the picture zone mask at device resolution | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 12 / 12 | no, deviation: the plan expected an empty diff, but qt_tests now links dewarping (output/Settings.h includes DistortionModel.h); the diff is only the -I src/dewarping flag on the 5 qt_tests compile lines (4 sources and the automoc unit) and libdewarping.a on the qt_tests link line, no automoc change |
| r09-stage-animation | 3899d951 | feat(04-05): render the stage list animation at device resolution | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 0 / 0 | yes: empty (edits only) |
| r10-icon-geometry | 3064968e | fix(04-06): place icon pixmaps by their logical size | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 0 / 0 | yes: empty (edits only) |
| r11-color-picker | 6dac9b1c | feat(04-06): sample the colour picker by logical pixels | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 0 / 0 | yes: empty (edits only) |
| r12-retina-check-overlays | 9f9fdabe | test(04-07): extend the live Retina check to thumbnails and overlays | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 0 / 0 | yes: empty (retina_check is not a gated target; its link line gained libdewarping.a) |
