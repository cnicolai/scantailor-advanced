# Gate log, Phase 5 Mac menu and shortcut commits

Each Phase 5 commit that changes a path outside `.planning/` and `.claude/` is gated by `gate.sh check <label>` on the committed tree: configure, build and ctest of macos-release and macos-debug, goldens compared with a796c2ae. The command diff compares the sorted build commands of the eight existing targets (plus `ui_snapshot`) with those of the previous commit.

| Label | Commit | Subject | Release ctest | Debug ctest | Goldens | Command diff (release / debug) | Expected diff met |
|-------|--------|---------|---------------|-------------|---------|--------------------------------|-------------------|
| m01-tracer | df295683 | feat(05-01): move Settings to the application menu with Cmd-comma | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 100 / 100 | yes: only the classes the plan names: the 20 GUI-source compile lines moved from the `scantailor-advanced.dir` and `ui_snapshot.dir` object paths to `app_gui.dir` (the `ui_snapshot` duplicates are gone), the autogen include flag, the new `libapp_gui.a` archive line, the link lines of `scantailor-advanced`, `ui_snapshot` and `qt_tests`, the `scantailor-advanced` and `ui_snapshot` main.cpp and rcc compile lines, and the `tst_mac_qt.cpp`, resource (rcc) and automoc lines of `qt_tests` |
| m02-menu-roles | 47a22164 | feat(05-01): give the menu actions explicit roles and Mac menu texts | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 0 / 0 | yes: empty (edits only) |
