# Phase 3: Qt 6 and macOS Only - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md. This log preserves the alternatives considered.

**Date:** 2026-10-24
**Phase:** 03-qt-6-and-macos-only
**Areas discussed:** How far the strip goes, Translations leftovers, README and test docs, How the result is checked

---

## How far the strip goes

**Question 1:** Besides what the requirements name, which code that never runs on the Mac should also go in Phase 3? (multi-select)

| Option | Description | Selected |
|--------|-------------|----------|
| PORTABLE_VERSION option | Portable mode keeps settings next to the binary. Presets already turn it off; CMake's default is ON. Removing it makes every build use `~/.config`. | ✓ |
| install/uninstall rules | `make install` targets, Windows DLL copying (CopyToBuildDir, AddDynamicLibraryLocations, cmake_uninstall). Only the build directory is used. | ✓ |
| GCC-only compiler block | The `CMAKE_COMPILER_IS_GNUCC` flags including `-march=native`. Apple Clang never takes it. | ✓ |
| Upstream issue script | `scripts/post_issue_followups.py` posts comments on upstream issues through `gh`. | ✓ |

**User's choice:** All four, plus free text: "Everything that never runs on the Mac shall go".
**Notes:** Recorded as the guiding rule of the phase (CONTEXT D-01, D-02).

**Question 2:** Windows-minded code without a platform label that runs on the Mac (`RelinkablePath::normalize` turns backslashes into slashes and checks for drive letters). What should Phase 3 do?

| Option | Description | Selected |
|--------|-------------|----------|
| Leave it, log a backlog item (Recommended) | It runs on the Mac, so removing it changes behaviour in a no-behaviour-change phase. The backslash case goes into the backlog. | ✓ |
| Remove it in Phase 3 | Treat it as Windows code. A real change in path handling, which needs its own test and commit. | |

**User's choice:** Leave it, log a backlog item.

---

## Translations leftovers

**Question 1:** Keep or strip the ~220 `tr()` calls in 42 files plus the `.ui` translatable strings?

| Option | Description | Selected |
|--------|-------------|----------|
| Keep tr() as is (Recommended) | They run and return English unchanged. Stripping them makes a noisy diff with no visible effect; the convention keeps `tr()`. | ✓ |
| Strip tr() everywhere | Replace with plain strings and turn off translatable flags in the `.ui` forms. A large mechanical diff. | |

**User's choice:** Keep tr() as is.

**Question 2:** What should happen to a `language` value already in the user's settings file?

| Option | Description | Selected |
|--------|-------------|----------|
| Leave it in the file (Recommended) | Ignored by the app; nothing touches the settings file. | ✓ |
| Remove it on startup | The app deletes the stale key once. Adds code that writes to personal settings. | |

**User's choice:** Leave it in the file.

---

## README and test docs

**Question 1:** What shape should the README take?

| Option | Description | Selected |
|--------|-------------|----------|
| New top, keep Features (Recommended) | Rewrite the top (fork, upstream link, Homebrew, presets, tests, settings location); delete the Windows/Linux/Flatpak/MXE sections and the old "About this fork"; keep Features unchanged. | ✓ |
| Short README only | About 60 lines; drop Features and link upstream. | |
| Short README + docs/FEATURES.md | Short README; move Features into its own file. | |

**User's choice:** New top, keep Features.

**Question 2:** `TESTING.md` is in Spanish and covers only the five old executables. What should happen to it?

| Option | Description | Selected |
|--------|-------------|----------|
| Rewrite in English, keep (Recommended) | README has the commands; `TESTING.md` covers the seven suites, `UPDATE_GOLDENS=1`, artifacts, the manifest and the expected failures. | ✓ |
| Merge into README, delete | One document with a testing section. | |
| Delete it | README commands are enough. | |

**User's choice:** Rewrite in English, keep.

**Question 3:** How should the agent docs (codebase maps, generated `.claude/CLAUDE.md` sections) catch up with the strip?

| Option | Description | Selected |
|--------|-------------|----------|
| Patch maps + regenerate (Recommended) | Correct the stale facts in the maps at the end of the phase, then regenerate the GSD-managed CLAUDE.md sections. | ✓ |
| Full remap after Phase 3 | Run `/gsd-map-codebase` afterwards; more complete, far more tokens. | |
| Leave them stale | No doc update. | |

**User's choice:** Patch maps + regenerate.

---

## How the result is checked

**Question 1:** How should success criterion 4 (English UI, Settings dialog without the Language row) be checked?

| Option | Description | Selected |
|--------|-------------|----------|
| Automated, Claude reads screenshots (Recommended) | A test asserts no language control and no `.qm`; the executor grabs the main window and Settings dialog offscreen and inspects the PNGs. | ✓ |
| Automated only, no screenshots | Test plus an offscreen launch that must not crash. | |
| Automated + short GUI look by you | Three steps walked through one at a time. | |

**User's choice:** Automated, Claude reads screenshots.

**Question 2:** How should success criterion 5 (the README alone is enough to build and test) be proven?

| Option | Description | Selected |
|--------|-------------|----------|
| Fresh clone, README only (Recommended) | The executor clones into a fresh folder under `build/` and runs only the README commands. | ✓ |
| You follow the README | The user runs the commands in a clean checkout. | |

**User's choice:** Fresh clone, README only.
**Notes:** While writing CONTEXT.md, Claude added that the README's `brew install` line is not executed during the check, because it could upgrade Qt or Boost and change the toolchain. `brew list --versions` confirms the packages instead (CONTEXT D-13).

**Question 3:** Should Phase 3 end with one ASan/UBSan run of the full suite?

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, once at the end (Recommended) | Compare with the Phase 2 list (12 UBSan, 0 ASan at 9 locations); new reports block until explained. | ✓ |
| No sanitizer run | ctest in Release and Debug is enough. | |

**User's choice:** Yes, once at the end.

---

## Claude's Discretion

- Plan and commit split, and the deletion order (PITFALLS Pitfalls 7 and 14)
- The minimum Qt 6 version in `find_package` (a floor, not a pin)
- Replacing the `Qt_*_lib` indirection variables, and whether the remaining `cmake/*.cmake` helpers are still used
- How the `QT_VERSION_STR` token in `src/test_support/Manifest.cpp` is replaced while the manifest value stays identical
- Where the screenshots land and whether they are committed
- Whether `.github/` is deleted as a whole
- README details (the demo-video link, wording); British spelling, matching the planning documents
- Filing the RelinkablePath backlog item during execution

## Deferred Ideas

- Backslash and drive-letter handling in `RelinkablePath::normalize`: backlog item, fixed with a test in a later phase
- Local clang-format enforcement (a format target or pre-commit hook) as a replacement for the removed lint workflow
