# Project Retrospective

*A living document updated after each milestone. Lessons feed forward into future planning.*

## Milestone: v1.0 — macOS

**Shipped:** 2026-11-18
**Phases:** 7 (1–6, plus inserted 06.1) | **Plans:** 77 | **Tasks:** 145 | **Commits:** 588 over 32 days

### What Was Built

- A native arm64 build of the unmodified 1.2.1 sources against Homebrew Qt 6 and Boost, with Release, Debug and ASan presets
- A synthetic-image test suite (seeded page generator, headless six-stage pipeline, 38 goldens from the unmodified code) in seven ctest suites
- A Qt 6 / macOS-only tree: Qt 5, Windows, Linux, packaging, CI, portable mode and translations removed with goldens byte-identical
- Retina rendering for the page view, thumbnails, overlays and colour picker; Mac application menu, Cmd shortcuts, Mac key names, native panels
- Safety fixes (forged headers, unusual file names, batch memory cap), OpenGL removal, and the `OutputGenerator.cpp` / `MainWindow.cpp` splits by verified pure moves

### What Worked

- **Baselines before deletions.** Goldens recorded from the unmodified code at `a796c2ae` guarded every later phase; no golden file changed after that commit, so "output unchanged" was a measured fact, not a claim.
- **A per-commit gate** (`gate.sh`: Release and Debug ctest, goldens, build-command diff). It caught CMake edits that tests alone would miss and made twelve strip commits and all later refactors routine.
- **Offscreen tests plus a separate live harness.** Two simulated screens (1x, 2x) and offscreen Qt tests carried the automated gate; `retina_check` and `mac_check` confirmed the real Cocoa behaviour only where it mattered.
- **Pure-move proofs for file splits.** Chunk-multiset checks with positive controls and output-pixel digests made splitting a 2814-line output file safe.
- **Expected-failure marks tied to backlog items.** Defects found in the original code became `XFAIL_CHECK`s that name a backlog item, so the suite stayed green without hiding them.

### What Was Inefficient

- **Plan counts out of proportion.** The post-audit cleanup (06.1) was first planned with 15 sequential plans, and a revision round widened a decision beyond the user's own scope; the user had it trimmed. Each source commit costs a ~6-minute gate, so plan count drives wall-clock time directly.
- **A live harness run moved the pointer while the user worked** (Phase 5). Live runs need the user's go before dispatch, not a status-line warning.
- **Review-fix rounds after phases** (02-11 to 02-14, the 06.1 re-review fixes) added plans that earlier review of the plans themselves might have avoided.
- **Tooling friction** in executors: `clang-format` only via `xcrun`, ugrep pattern quirks, stale isolation sentinels forcing sequential mode, scratch files drifting to `/tmp`. Each was learned once and recorded, but cost time the first time.
- **Tracker drift** at the end (STATE.md progress, the ROADMAP progress table missing 06.1, `init.manager` marking verified phases as `stale`) had to be reconciled by hand at close.

### Patterns Established

- Every output-preserving change goes through the gate with byte-identical goldens; a golden is re-recorded only when a backlog defect is fixed on purpose.
- View-layer changes never touch output generation (`imageproc`, `dewarping`, `Task.cpp`, `OutputGenerator*`).
- Live Cocoa checks run only on the user's go; offscreen tests carry the automated gate.
- Comments under `src/` explain reasons in plain words, not planning IDs.
- Review findings end in a `*-REVIEW-DISPOSITION.md` with every item `fixed`, `skipped` or `deferred` with a reason.

### Key Lessons

1. State a target plan count before planning (cleanup phases: 3–5 plans) and reject checker findings that widen scope beyond the phase's decisions.
2. Ask for a time window before dispatching any plan that moves the pointer or takes focus.
3. Give executors the repo's tool facts up front (`xcrun clang-format`, call `gsd-tools.cjs` directly, scratch in `build/scratch/`, ugrep syntax) instead of letting each rediscover them.
4. Re-run strict ASan and the live harness on the final tree before closing a milestone, so the audit does not carry them as "not re-run".

### Cost Observations

- Model mix: not tracked
- Sessions: not tracked
- Notable: Phases 1–4 took 8 days (2026-10-18 to 10-25); Phases 5, 6 and 06.1 took the remaining time, with the ~6-minute per-commit gate as the dominant cost in refactoring plans.

---

## Cross-Milestone Trends

### Process Evolution

| Milestone | Sessions | Phases | Key Change |
|-----------|----------|--------|------------|
| v1.0 | — | 7 | Golden baselines before deletions; per-commit gate; offscreen tests plus live harness |

### Cumulative Quality

| Milestone | Tests | Coverage | Zero-Dep Additions |
|-----------|-------|----------|-------------------|
| v1.0 | 7 ctest suites, 38 goldens | not measured | 0 (no new third-party dependency) |

### Top Lessons (Verified Across Milestones)

1. (to be filled once a second milestone confirms a v1.0 lesson)
