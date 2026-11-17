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
    disposition: open
    title: "Unused includes left behind by the removal of the language and portable code"
  - id: IN-02
    severity: info
    disposition: open
    title: "`MainWindow::changeEvent` still handles `QEvent::LanguageChange`"
  - id: IN-03
    severity: info
    disposition: open
    title: "Windows-specific comments and a dead UNC branch survive in the touched files"
  - id: IN-04
    severity: info
    disposition: open
    title: "`ui_snapshot` confines output with a lexical prefix check only"
  - id: IN-05
    severity: info
    disposition: open
    title: "`ui_snapshot` argument parsing accepts unknown options as the output directory"
  - id: IN-06
    severity: info
    disposition: open
    title: "`qt_compile` in the golden manifest is now the configure-time Qt version, and a miswired definition reads as \"unknown\""
  - id: IN-07
    severity: info
    disposition: open
    title: "Windows-DLL leftovers outside the strip's reach, plus stale ignore rules"
open: 7
total: 9
recorded: 2026-10-24T21:54:44.269Z
---

# Phase 03: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-01 | warning | fixed | fixed by 05-01 (df295683): the GUI sources compile once into the app_gui static library that the app, ui_snapshot, qt_tests and mac_check link; ui_snapshot is still built with the tests but no longer recompiles the GUI |
| WR-02 | warning | deferred | Kept on purpose: the comment sits in the Clang flag block that D-01 keeps byte-for-byte (Phase 1 D-03, D-08); RESEARCH.md Open Questions resolved to leave it. Revisit when that block is next edited. |
| IN-01 | info | open | - |
| IN-02 | info | open | - |
| IN-03 | info | open | - |
| IN-04 | info | open | - |
| IN-05 | info | open | - |
| IN-06 | info | open | - |
| IN-07 | info | open | - |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
