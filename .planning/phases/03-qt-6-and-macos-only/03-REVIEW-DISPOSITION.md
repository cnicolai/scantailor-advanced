---
phase: 03
review: 03-REVIEW.md
titles: json
findings:
  - id: WR-01
    severity: warning
    disposition: fixed
    title: "`ui_snapshot` recompiles the entire GUI sources and is built by default"
  - id: WR-02
    severity: warning
    disposition: deferred
    title: "Orphaned comment in the top-level CMakeLists refers to a block this phase deleted"
  - id: IN-01
    severity: info
    disposition: fixed
    title: "Unused includes left behind by the removal of the language and portable code"
  - id: IN-02
    severity: info
    disposition: fixed
    title: "`MainWindow::changeEvent` still handles `QEvent::LanguageChange`"
  - id: IN-03
    severity: info
    disposition: fixed
    title: "Windows-specific comments and a dead UNC branch survive in the touched files"
  - id: IN-04
    severity: info
    disposition: fixed
    title: "`ui_snapshot` confines output with a lexical prefix check only"
  - id: IN-05
    severity: info
    disposition: fixed
    title: "`ui_snapshot` argument parsing accepts unknown options as the output directory"
  - id: IN-06
    severity: info
    disposition: skipped
    title: "`qt_compile` in the golden manifest is now the configure-time Qt version, and a miswired definition reads as \"unknown\""
  - id: IN-07
    severity: info
    disposition: fixed
    title: "Windows-DLL leftovers outside the strip's reach, plus stale ignore rules"
open: 0
total: 9
recorded: 2026-10-24T21:54:44.269Z
---

# Phase 03: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-01 | warning | fixed | fixed by 05-01 (df295683): the GUI sources compile once into the app_gui static library that the app, ui_snapshot, qt_tests and mac_check link; ui_snapshot is still built with the tests but no longer recompiles the GUI |
| WR-02 | warning | deferred | Kept on purpose: the comment sits in the Clang flag block that D-01 keeps byte-for-byte (Phase 1 D-03, D-08); RESEARCH.md Open Questions resolved to leave it. Revisit when that block is next edited. |
| IN-01 | info | fixed | fixed by 06.1-03 (f6ef6bcc): unused Application.h and QLocale includes removed, main.cpp includes QTimer and memory itself |
| IN-02 | info | fixed | fixed by 06.1-03 (f6ef6bcc): MainWindow::changeEvent deleted, the UI is English only |
| IN-03 | info | fixed | fixed by 06.1-03 (1f1f21ed, f6ef6bcc): UNC branch proven unreachable by TestRelinkablePath and deleted, the four Windows comments reworded |
| IN-04 | info | fixed | fixed by 06.1-07 (db8e618b): ui_snapshot resolves its output directory through symlinks and accepts only a strict descendant of build (harness::outputDirIsStrictlyInside); tests cover inside, the root, a .. escape, a prefix sibling and a symlink out of the root, and a real run with build itself exits 2 without creating anything |
| IN-05 | info | fixed | fixed by 06.1-07 (acc893f0): harness::parseSnapshotArgs refuses every option except --assert-no-language and anything but exactly one output directory, with a separate counter instead of a doubled flag; unit test and a real run with --bogus (exit 2, nothing created) |
| IN-06 | info | skipped | skipped: any fix changes the golden manifest, which D-00a forbids (D-02); TESTING.md documents qt_compile as the configure-time Qt version |
| IN-07 | info | fixed | fixed by 06.1-03 (f6ef6bcc): dead remove_definitions line and packaging and flatpak ignore rules removed |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
