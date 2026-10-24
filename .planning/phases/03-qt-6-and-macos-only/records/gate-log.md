# Gate log, Phase 3 strip commits

Each strip commit was gated by `gate.sh check <label>` on the committed tree: configure, build and ctest of macos-release and macos-debug, goldens compared with a796c2ae. The command diff compares the sorted build commands of the eight existing targets (plus `ui_snapshot` once it exists) with those of the previous strip commit.

| Label | Commit | Subject | Release ctest | Debug ctest | Goldens | Command diff (release / debug) | Expected diff met |
|-------|--------|---------|---------------|-------------|---------|--------------------------------|-------------------|
| c00-ui-snapshot | 8d0105ab | test(03-01): add offscreen UI snapshot harness | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 28 / 28 | yes: only ui_snapshot added |
| c01-qt5-cpp | 9a68f985 | refactor(03-02): resolve Qt 5 conditionals in C++ with unifdef | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 14 / 14 | yes: only ST_QT_COMPILE_VERSION on test_support (7 compile lines per preset, value 6.12.0; no TOOLCHAIN DIFFERS line, 0 as in before-state) |
| c02-qt6-cmake | c63a74c2 | build(03-02): require Qt 6 only in CMake | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 0 / 0 | yes: empty |
| c03-qt5-main-imageview | 81a27cb1 | refactor(03-02): resolve the Qt 5 blocks in main.cpp and ImageViewBase | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 0 / 0 | yes: empty |
| c04-ci | 462f22cd | chore(03-03): remove GitHub automation | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 0 / 0 | yes: empty |
