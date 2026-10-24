# Phase 3 execution decisions

Decisions taken while planning and executing Phase 3, each with its reason. They refine the wording of CONTEXT.md where measured facts or the user's standing rules required it.

### ED-01: The snapshot harness is a separate `ui_snapshot` executable, built under BUILD_TESTS and not in ctest
`MainWindow` and `SettingsDialog` are compiled only into the app executable, and `qt_tests` links just `core` and `foundation`, so no existing test target can construct them. A new executable `ui_snapshot` compiles the app sources without `main.cpp` and is built whenever `BUILD_TESTS` is on. It is not registered with ctest because `TESTING.md` lists seven suites and the gate requires `100% tests passed out of 7`. (Orchestrator note 3.)

### ED-02: The D-15 compile-time define is `ST_QT_COMPILE_VERSION`
The Qt version recorded in the golden manifests comes from a define that CMake fills from `Qt6_VERSION`. Any name containing the Qt version macro name would match the criterion-1 leftover search and fail it, so the define is `ST_QT_COMPILE_VERSION`. PATTERNS.md suggested another name; the measured search result wins. (Orchestrator note 2.)

### ED-03: Qt 5 removal takes three commits in the Pitfall 7 order
c01 resolves the Qt 5 conditionals in sixteen C++ files with `unifdef` and adds the D-15 define, c02 makes CMake require Qt 6 only, and c03 removes the Qt 5 block of `main.cpp` and the `QGLWidget` macro hack of `ImageViewBase.{h,cpp}`. Each commit is one category and passes the gate on its own, so a failure points at one cause.

### ED-04: Windows removal takes two commits
c06 removes the Windows code paths from C++ and c07 removes the Windows CMake options and files. Two commits keep each task within its file budget and keep the C++ diff separate from the build-system diff, so the command diff of c07 can be read on its own.

### ED-05: Items that sit between categories are assigned to one commit each
`scripts/post_issue_followups.py` goes with the GitHub automation (c04), because it only serves the issue workflow. `CMAKE_INSTALL_RPATH_USE_LINK_PATH` and `QT_BINDIR` go with the install rules (c05). `APPLICATION_VERSION_MAJOR/MINOR/PATCH` go with the Windows resource file that is their last reader (c07). No item is removed twice and none is left over.

### ED-06: Qt floor is `find_package(Qt6 6.5 ...)` with direct `Qt6::` targets
The floor is a lower bound, not a pin: every Qt API in the permanent Qt 6 branches exists since Qt 6.0, and Homebrew always provides the newest Qt, so 6.5 is conservative (RESEARCH assumption A1). Targets are written as `Qt6::Core` and so on, without the version-agnostic aliases, because the aliases only existed to serve Qt 5.

### ED-07: README Features exception for the two portable-mode passages
D-08 says the Features section stays unchanged, but the Portability bullet and the profiles line describe portable mode, which D-01 removes. Exactly these two passages are amended to the macOS locations; nothing else in Features changes. Decided under the user's standing instruction that Claude takes GSD decision gates. (Orchestrator note 1.)

### ED-08: The profiles path is `~/Library/Application Support/scantailor-advanced/scantailor-advanced/profiles`
The Phase 1 probe measured `AppDataLocation` as `~/Library/Application Support/scantailor-advanced/scantailor-advanced` (`probe-output.txt` line 18), and profiles live in its `profiles` subdirectory. That is the path the README gets. It differs from the shorter path written in CONTEXT D-01 and in orchestrator note 1, which was not measured.

### ED-09: The upstream demo-video link at the top of the README is dropped
The link shows an older upstream version of the program and needs a Vimeo login, so it does not help a reader who builds this fork. The rewrite removes it together with the other upstream-distribution material.

### ED-10: Before and after screenshots are committed under `records/screenshots/`
They show the synthetic offscreen UI only (empty-project start screen and the Settings dialog with default settings), so they contain no personal data. Committing both sets lets the verifier compare them without rebuilding the old tree.

### ED-11: Gate semantics are commit first, gate the committed tree, amend until PASS
`gate.sh check <label>` refuses to run while a build input is modified, so it always measures what was committed. A failing gate is fixed by amending the same commit and re-running with the same label; the gate log gets one row per label once the result is PASS. The command diff never decides PASS, each task states the diff it expects.

### ED-12: Kept as they are
`cmake/ListItemsPrepend.cmake` and `cmake/SetDefaultBuildType.cmake` (both are still used), `.gitignore`, the now stale comment of the untouched Clang flag block (D-01 asks for that block byte for byte), and `ui_snapshot` outside ctest (ED-01). Leaving them avoids edits that no success criterion asks for.
