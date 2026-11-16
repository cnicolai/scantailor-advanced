# Phase 6: Hardening and Cleanup - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md. This log preserves the alternatives considered.

**Date:** 2026-11-16
**Phase:** 06-hardening-and-cleanup
**Areas discussed:** Oversized TIFF rejection

Areas offered but not chosen, so Claude decided them: RAM-capped threads, SAFE-02 when nothing fails, fold in open defects.

---

## Oversized TIFF rejection

### Where the limit sits

| Option | Description | Selected |
|--------|-------------|----------|
| Fixed, generous | A fixed ceiling well above any real scan (for example 1 GP or more than 65,535 px on a side); same result on every machine; testable | ✓ |
| Share of the Mac's RAM | Adapts to the machine, but the same file could pass on one Mac and fail on another; harder to test | |
| You decide | Claude picks it in research | |

### Where the user learns about it

| Option | Description | Selected |
|--------|-------------|----------|
| Both places | Project creation lists the file as too large with its claimed size; the page view shows the error for an existing project; no new dialog | ✓ |
| Page view only | Only the existing in-view error with a "too large" message | |
| Project creation only | Stopped at the door; existing projects keep the generic message | |

### PNG and JPEG

| Option | Description | Selected |
|--------|-------------|----------|
| One rule for all | Same limit and message for every format; Qt's own cap (probably 256 MB) raised to match | ✓ |
| TIFF only | Do exactly what SAFE-01 names; backlog the Qt cap question | |
| TIFF only, plus a clear error | Keep Qt's cap but show "too large" when it rejects a file | |

**Closing answer:** "If you have any pressing questions you can't decide on your own, let me know. Otherwise --> context". Claude had none and wrote the context.

---

## Claude's Discretion

- RAM-capped threads (SAFE-03): cap from physical RAM and the largest page in the project, on top of the user's slider; heavy batch measurement only on the user's go; real-scan Activity Monitor check optional in UAT.
- SAFE-02: FIXME is moot (QDom unescapes); extend the round-trip test to CR/LF, tab, control characters, spaces, `'`, `>`, emoji and backslash; fix what fails while ordinary project files stay byte-identical.
- Open defects: none folded in. The Phase 4 WR-01..03 and the Phase 3 `ui_snapshot` finding are already fixed, and the PROJECT.md/STATE.md notes calling them open are out of date.
- OpenGL removal details, cleanup order, and split boundaries.

## Deferred Ideas

- Backlog 999.1–999.8, UBSan reports in imageproc, info review findings, Margins panel width, 1x/2x display drag, and splits of `ThumbnailSequence.cpp` and `ContentBoxFinder.cpp`.
