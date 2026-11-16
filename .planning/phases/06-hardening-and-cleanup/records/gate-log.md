# Gate log, Phase 6 hardening and cleanup commits

Each Phase 6 commit that changes a path outside `.planning/` and `.claude/` is gated by `gate.sh check <label>` on the committed tree: configure, build and ctest of macos-release and macos-debug, goldens compared with a796c2ae. The command diff compares the sorted build commands of the eight existing targets (plus `ui_snapshot`) with those of the previous commit. Commits that add or remove a source file or a library are expected to change only the compile, archive, link and automoc lines of the classes they name; edit-only commits are expected to leave the diff empty.

| Label | Commit | Subject | Release ctest | Debug ctest | Goldens | Command diff (release / debug) | Expected diff met |
|-------|--------|---------|---------------|-------------|---------|--------------------------------|-------------------|
| s01-if0 | dc138113 | refactor(06-01): remove the dead #if 0 blocks | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 0 / 0 | yes: empty (edits only: seven .cpp files; the build commands do not change) |
