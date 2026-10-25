# Gate log, Phase 4 Retina commits

Each Retina commit that changes a path outside `.planning/` and `.claude/` was gated by `gate.sh check <label>` on the committed tree: configure, build and ctest of macos-release and macos-debug, goldens compared with a796c2ae. The command diff compares the sorted build commands of the eight existing targets (plus `ui_snapshot`) with those of the previous commit.

| Label | Commit | Subject | Release ctest | Debug ctest | Goldens | Command diff (release / debug) | Expected diff met |
|-------|--------|---------|---------------|-------------|---------|--------------------------------|-------------------|
| r01-hq-tracer | 7c730ab2 | feat(04-01): build the page view HQ pixmap at device resolution | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 6 / 6 | yes: only the HiDpi.cpp and tst_hidpi_qt.cpp compile lines, the core archive line and the qt_tests link line (no automoc change) |
| r02-hq-dpr-change | 288ea84b | feat(04-01): rebuild the HQ pixmap when the device pixel ratio changes | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 0 / 0 | yes: empty (edits only) |
