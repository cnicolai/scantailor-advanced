# CLEAN-02: MainWindow.cpp split

Decision D-17: `src/app/MainWindow.cpp` (2319 lines at the planning base `e5319cd9`, 2305 lines at `590e751f`, the last commit that changed it before plan 06-15) is split into same-class translation units by verbatim moves, in three gated steps. Every step is proven by `records/move_check.py` (chunk multiset against `590e751f`), by the offscreen `TestMacQt` suite (and the other three Qt test classes of `qt_tests`) and by the gate (Release and Debug ctest 7 of 7, goldens unchanged). `MainWindow.ui` is unchanged (`git diff e5319cd9 -- src/app/MainWindow.ui` is empty). Output is normalised: `{repo}` for the repository path, `~` for the home folder.

The live `mac_check` confirmation of menus, keys and the batch flow is not part of this plan; it follows in plan 06-16 after the user's go.

## Method

- Same-class translation units (RESEARCH Pattern 3), not new classes: slots stay connected by `SIGNAL()`/`SLOT()` strings and the tests find actions by object name, so moving member definitions between `.cpp` files keeps all of that.
- The files are produced by a scratch cut script (`build/scratch/s15/cut_s15.py`) that copies line ranges of `MainWindow.cpp` at `590e751f` (chunk index of `build/scratch/s15/chunk_index_mw.py`, 116 chunks) and retypes nothing. Each new file has the two copyright lines, `MainWindow.cpp`'s full include block (unused includes are harmless) and `using namespace core;`, then its chunks in the original order. `xcrun clang-format -i` changed no moved line (the move check ran after formatting).
- The four new files are in `gui_only_sources` of `src/app/CMakeLists.txt`, so they are compiled into the `app_gui` library like `MainWindow.cpp`. No header was added.

## Split map

Original line numbers are the definition line at `e5319cd9` and at `590e751f` (the base of the move check). `MainWindow::setupActions` is new (see the boundary below).

| Function | Line at e5319cd9 | Line at 590e751f | New file |
|----------|------------------|------------------|----------|
| `MainWindow::createBatchProcessingWidget` | 487 | 490 | `MainWindowBatch.cpp` |
| `MainWindow::startBatchProcessing` | 1237 | 1241 | `MainWindowBatch.cpp` |
| `MainWindow::stopBatchProcessing` | 1282 | 1291 | `MainWindowBatch.cpp` |
| `MainWindow::filterResult` | 1310 | 1319 | `MainWindowBatch.cpp` |
| `MainWindow::isBatchProcessingInProgress` | 1688 | 1673 | `MainWindowBatch.cpp` |
| `MainWindow::createCompositeTask` | 2132 | 2118 | `MainWindowBatch.cpp` |
| `MainWindow::createCompositeCacheDrivenTask` | 2176 | 2162 | `MainWindowBatch.cpp` |
| `MainWindow::setupActions` (the constructor block, new function) | 213-304 (block) | 216-307 (block) | `MainWindowActions.cpp` |
| `MainWindow::goFirstPage` | 927 | 931 | `MainWindowActions.cpp` |
| `MainWindow::goLastPage` | 938 | 942 | `MainWindowActions.cpp` |
| `MainWindow::goNextPage` | 949 | 953 | `MainWindowActions.cpp` |
| `MainWindow::goPrevPage` | 960 | 964 | `MainWindowActions.cpp` |
| `MainWindow::goNextSelectedPage` | 971 | 975 | `MainWindowActions.cpp` |
| `MainWindow::goPrevSelectedPage` | 982 | 986 | `MainWindowActions.cpp` |
| `MainWindow::switchFilter1` to `switchFilter6` | 1190, 1194, 1198, 1202, 1206, 1210 | 1194, 1198, 1202, 1206, 1210, 1214 | `MainWindowActions.cpp` |
| `MainWindow::updateProjectActions` | 1613 | 1598 | `MainWindowActions.cpp` |
| `MainWindow::setupMacKeys` | 1628 | 1613 | `MainWindowActions.cpp` |
| `MainWindow::updateZoomActions` | 1651 | 1636 | `MainWindowActions.cpp` |
| `MainWindow::zoomCurrentViewIn` | 1665 | 1650 | `MainWindowActions.cpp` |
| `MainWindow::zoomCurrentViewOut` | 1669 | 1654 | `MainWindowActions.cpp` |
| `MainWindow::zoomCurrentView` | 1673 | 1658 | `MainWindowActions.cpp` |
| `MainWindow::execGotoPageDialog` | 2272 | 2258 | `MainWindowActions.cpp` |
| `MainWindow::switchToNewProject` | 388 | 391 | `MainWindowProject.cpp` |
| `MainWindow::showNewOpenProjectPanel` | 463 | 466 | `MainWindowProject.cpp` |
| `MainWindow::promptProjectSave` | 583 | 587 | `MainWindowProject.cpp` |
| `MainWindow::autoSaveProject` | 1032 | 1036 | `MainWindowProject.cpp` |
| `MainWindow::saveProjectTriggered` | 1414 | 1423 | `MainWindowProject.cpp` |
| `MainWindow::saveProjectAsTriggered` | 1425 | 1434 | `MainWindowProject.cpp` |
| `MainWindow::newProject` | 1460 | 1445 | `MainWindowProject.cpp` |
| `MainWindow::newProjectCreated` | 1470 | 1455 | `MainWindowProject.cpp` |
| `MainWindow::openProject()` | 1475 | 1460 | `MainWindowProject.cpp` |
| `MainWindow::openProject(const QString&)` | 1491 | 1476 | `MainWindowProject.cpp` |
| `MainWindow::projectOpened` | 1511 | 1496 | `MainWindowProject.cpp` |
| `MainWindow::closeProject` | 1523 | 1508 | `MainWindowProject.cpp` |
| `MainWindow::closeProjectInteractive` | 1808 | 1793 | `MainWindowProject.cpp` |
| `MainWindow::closeProjectWithoutSaving` | 1874 | 1859 | `MainWindowProject.cpp` |
| `MainWindow::saveProjectWithFeedback` | 1879 | 1864 | `MainWindowProject.cpp` |
| `MainWindow::showInsertFileDialog` | 1892 | 1871 | `MainWindowPages.cpp` |
| `MainWindow::showRemovePagesDialog` | 2016 | 2002 | `MainWindowPages.cpp` |
| `MainWindow::insertImage` | 2037 | 2023 | `MainWindowPages.cpp` |
| `MainWindow::removeFromProject` | 2053 | 2039 | `MainWindowPages.cpp` |
| `MainWindow::eraseOutputFiles` | 2104 | 2090 | `MainWindowPages.cpp` |

46 of the 116 chunks moved (7 batch, 19 action, 15 project, 5 page chunks); `setupActions` is the new 117th chunk after the constructor lost its block. `goToPage` (the one that takes a `PageId`) is not one of the six page-navigation actions of the plan's list and stays.

## What stayed in MainWindow.cpp, and why

- `MainWindow::PageSelectionProviderImpl` (file-local class, used only by `newPageSelectionAccessor`) and `newPageSelectionAccessor` itself: the decision of the plan keeps the file-local class and its only user together, so no internal header is needed. No moved function refers to the class (checked with `grep PageSelectionProviderImpl`: two places, both in `MainWindow.cpp`).
- The constructor (without the action block), the destructor, the page and thumbnail handling, the image and options widgets, the relinking, the page context menus, settings and about dialogs, window title, main-area update, event handlers and the small `is*` helpers: 1237 lines, below the 1300 of the plan.
- The plan's list for the project group named `saveProject*`, `newProject*`, `openProject*`, `closeProject*`, `promptProjectSave` and `autoSaveProject`. Those alone left `MainWindow.cpp` at 1336 lines (measured with the cut script before any file was written), so `switchToNewProject` (74 lines) and `showNewOpenProjectPanel` (23 lines) moved with them, because they are the project switch every one of these functions ends in. `MainWindow.cpp` became 1237 lines.

## The setupActions boundary

The constructor block of the plan runs from `addAction(actionFirstPage)` to `connect(actionQuit, ...)` (lines 213 to 342 at `e5319cd9`). It is not free of locals: the lambdas of `connect(thumbColumnViewBtn, ...)` and `connect(deviationHighlightingBtn, ...)` capture the constructor's local `settings` (`ApplicationSettings& settings = ...`). Declaring a second local in `setupActions()` would add a line, and the plan allows only the call, the signature line and one closing brace. The block was therefore shrunk, as the plan says:

- Moved into `MainWindow::setupActions()`: lines 216 to 307 at `590e751f` (213 to 304 at `e5319cd9`), from `addAction(actionFirstPage);` to `connect(sortOptions, SIGNAL(currentIndexChanged(int)), this, SLOT(pageOrderingChanged(int)));`. That is the 22 `addAction` calls, the emergency-memory allocation that sits between them and the first connects, the page, zoom, thumbnail-size and stage action connects, the filter list, worker pool and thumbnail sequence connects, and the scroll-bar and sort connects. It refers to members and `this` only; the two local lambdas `magnifyThumbnails` and `diminishThumbnails` are declared and used inside the block.
- Stays in the constructor: from `connect(thumbColumnViewBtn, ...)` (the first user of `settings`) to `connect(actionQuit, ...)` and the tail. The call `setupActions();` stands at the position of the first moved line, so the order of all `addAction` and `connect` calls is unchanged.
- Declared in `MainWindow.h` as a private non-slot method next to `setupIcons()`; no moc output changes.

Order proof, stronger than the chunk multiset (which compares the lines of the two exempt chunks as multisets): the lines of `setupActions()` were put back at the position of the call, and the result equals the constructor at `590e751f` line for line (`constructor with setupActions() inlined equals original constructor: True 256 256`).

## Move check

Base `590e751f`, from `git log --grep`, not a relative revision. The original is read from git, the new files from the working tree after formatting. The three exempt-line patterns are the call `  setupActions();`, the signature `void MainWindow::setupActions() {` and the closing brace `}`.

```
# step 1 (s20): one new file
$ python3 -I .planning/phases/06-hardening-and-cleanup/records/move_check.py --base 590e751f --orig src/app/MainWindow.cpp --new src/app/MainWindow.cpp src/app/MainWindowBatch.cpp
chunks: orig=116 new=116
RESULT: PASS

# step 2 (s21): constructor and setupActions exempt
$ python3 -I .../move_check.py --base 590e751f --orig src/app/MainWindow.cpp --new src/app/MainWindow.cpp src/app/MainWindowBatch.cpp src/app/MainWindowActions.cpp --allow-changed 'MainWindow::MainWindow(' --allow-changed 'void MainWindow::setupActions(' --allow-line '^  setupActions\(\);$' --allow-line '^void MainWindow::setupActions\(\) \{$' --allow-line '^}$'
chunks: orig=116 new=117
RESULT: PASS

# step 3 (s22): all five files, same allowances
$ python3 -I .../move_check.py --base 590e751f --orig src/app/MainWindow.cpp --new src/app/MainWindow.cpp src/app/MainWindowBatch.cpp src/app/MainWindowActions.cpp src/app/MainWindowProject.cpp src/app/MainWindowPages.cpp --allow-changed ... (as in step 2)
chunks: orig=116 new=117
RESULT: PASS
```

`new=117` is the 116 original chunks plus `setupActions` (the constructor remains one chunk).

Positive controls (run on copies under `build/scratch/s15/ctl/`):

```
# one space inserted in a batch function (allProcessed ()), step 1 layout
LOST  x1: void MainWindow::filterResult(const BackgroundTaskPtr& task, const FilterResultPtr& result) {
ADDED x1: void MainWindow::filterResult(const BackgroundTaskPtr& task, const FilterResultPtr& result) {
RESULT: FAIL

# the new file left out of --new (step 1)
LOST  x1: void MainWindow::filterResult(...)   (and startBatchProcessing, stopBatchProcessing)
RESULT: FAIL

# step 2, one connect line (actionSwitchFilter3) dropped from the copy of MainWindowActions.cpp
LINE LOST  x1: connect(actionSwitchFilter3, SIGNAL(triggered(bool)), SLOT(switchFilter3()));
RESULT: FAIL

# step 3, the chunk saveProjectWithFeedback dropped from the copy of MainWindowProject.cpp
chunks: orig=116 new=116
LOST  x1: bool MainWindow::saveProjectWithFeedback(const QString& projectFile) {
RESULT: FAIL
```

## The beep test

`TestMacQt::testBatchAlarmUsesBeep` (`src/qt_tests/tst_mac_qt.cpp`) changed in one commit with step 1, because `filterResult` moved:

- The forbidden-word check (`std::system`, `system(`, `QProcess`, `popen`, `/usr/share/sounds`, `external_alarm_cmd`) runs over every file `QDir(root.filePath("src/app")).entryList({"MainWindow*.cpp"}, QDir::Files)` returns. The file-count control requires at least two such files and that each is non-empty (five exist now).
- The batch-finished block is read from `src/app/MainWindowBatch.cpp`, which must contain `void MainWindow::filterResult(`. All assertions are unchanged: one `QApplication::alert(this);`, one `m_checkBeepWhenFinished()`, one `QApplication::beep();`, alert before beep.
- The scope-wide scan for `std::system`, `popen(` and `QProcess` with `fileCount >= 500` is untouched.

## TestMacQt and the other qt_tests slots

`qt_tests` was run with the ctest environment (`QT_QPA_PLATFORM=offscreen`, `LC_ALL=en_US.UTF-8`, `TMPDIR={repo}/build/macos-release/test-tmp`) and the `TestMacQt` working directory of `build/macos-release`, once per step.

| Step | Totals lines (TestCoreQt, TestUxQt, TestHiDpiQt, TestMacQt) | PASS lines | FAIL, XFAIL, SKIP lines |
|------|-------------------------------------------------------------|------------|-------------------------|
| s20 | 14 / 15 / 28 / 33 passed, all `0 failed, 0 skipped` | 90 | 0 |
| s21 | 14 / 15 / 28 / 33 passed, all `0 failed, 0 skipped` | 90 | 0 |
| s22 | 14 / 15 / 28 / 33 passed, all `0 failed, 0 skipped` | 90 | 0 |

The sorted PASS lines of s20 and s22 are identical (`cmp` finds no difference). The `TestMacQt` slots that pass in each step: `testSettingsInFileWithPreferencesRole`, `testSettingsShortcutOpensSettings`, `testSettingsHasNoOpenGlControl`, `testMenuRoles`, `testFileMenuTextsAndKeys`, `testToolsMenuWithoutSettings`, `testQuitClosesWindow`, `testProjectActionsDisabledWithoutProject`, `testStageActions`, `testViewMenuStages`, `testOutputTabKeys`, `testZoomActions`, `testZoomDisabledWithoutView`, `testZoomFollowsOutputTabs`, `testThumbnailKeys`, `testViewMenuContents`, `testZoneRemoveKeys`, `testZoneKeysIgnoredInTextFields`, `testZoneStatusTips`, `testContentBoxKeys`, `testContentMenuShowsKeys`, `testRightClickOpensZoneMenu`, `testZoneMenuDismissRestoresDefaultAtOnce`, `testNoPcKeyNames`, `testRewrittenStrings`, `testBatchAlarmUsesBeep`, `testNoDontUseNativeDialog`, `testNoOpenGlInSourcesOrBuild`, `testProjectExtension`, `testSaveAsFlowFromOutOfMemoryDialog`, `testSaveAsFlowRunsTwice` (plus `initTestCase` and `cleanupTestCase`).

## Line counts

| File | At 590e751f | After s20 | After s21 | After s22 |
|------|-------------|-----------|-----------|-----------|
| `MainWindow.cpp` | 2305 | 2052 | 1781 | 1237 |
| `MainWindowBatch.cpp` | | 344 | 344 | 344 |
| `MainWindowActions.cpp` | | | 366 | 366 |
| `MainWindowProject.cpp` | | | | 385 |
| `MainWindowPages.cpp` | | | | 341 |

`MainWindow.cpp` at `e5319cd9` had 2319 lines. `MainWindow.h` gained 3 lines (comment and `void setupActions();`).

## Gate rows

| Label | Commit | Release ctest | Debug ctest | Goldens | Command diff (release / debug) |
|-------|--------|---------------|-------------|---------|--------------------------------|
| s20-split-batch | 6079f902 | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 3 / 3: the `MainWindowBatch.cpp` compile line (with -ffp-contract=off) and the `libapp_gui.a` archive line (one object added) |
| s21-split-actions | 62e21bb9 | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 3 / 3: the `MainWindowActions.cpp` compile line and the archive line (one object added); `MainWindow.h` changes no moc line |
| s22-split-project | a9d17d1c | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 4 / 4: the `MainWindowProject.cpp` and `MainWindowPages.cpp` compile lines and the archive line (two objects added) |

`gate.sh` was called through `bash` and in the Bash tool's background mode; `UPDATE_GOLDENS` was never set. No source file was deleted by any of the three commits. Nothing outside the project was read or written and the application was not launched. The live `mac_check` run of both invocations follows in plan 06-16.

RESULT: PASS
