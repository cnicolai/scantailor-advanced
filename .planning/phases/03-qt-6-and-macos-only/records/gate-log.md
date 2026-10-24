# Gate log, Phase 3 strip commits

Each strip commit was gated by `gate.sh check <label>` on the committed tree: configure, build and ctest of macos-release and macos-debug, goldens compared with a796c2ae. The command diff compares the sorted build commands of the eight existing targets (plus `ui_snapshot` once it exists) with those of the previous strip commit.

| Label | Commit | Subject | Release ctest | Debug ctest | Goldens | Command diff (release / debug) | Expected diff met |
|-------|--------|---------|---------------|-------------|---------|--------------------------------|-------------------|
| c00-ui-snapshot | 8d0105ab | test(03-01): add offscreen UI snapshot harness | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 28 / 28 | yes: only ui_snapshot added |
