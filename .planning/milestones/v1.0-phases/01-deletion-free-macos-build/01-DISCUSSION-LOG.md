# Phase 1: Deletion-Free macOS Build - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-10-18
**Phase:** 01-deletion-free-macos-build
**Areas discussed:** Settings location, Floating-point rounding, End-to-end smoke test, Fix boundary and setup

---

## Settings location

| Option | Description | Selected |
|--------|-------------|----------|
| ~/Library | PORTABLE_VERSION OFF in the preset; settings survive deleting the build dir and are shared across builds | ✓ |
| Build directory | Keep default ON; each build dir has its own `config` folder, deleted with it | |
| You decide | Claude picks | |

**User's choice:** ~/Library

| Option | Description | Selected |
|--------|-------------|----------|
| Check, then ask | Ask permission to list leftover ScanTailor entries in ~/Library before first launch; user decides; nothing deleted | ✓ |
| Use whatever is there | Launch normally | |
| Nothing to worry about | Skip the check | |

**User's choice:** Check, then ask

---

## Floating-point rounding

| Option | Description | Selected |
|--------|-------------|----------|
| No, Mac is the start | Mac build defines the reference | ✓ |
| Yes, older outputs exist | x86 outputs to match | |
| Not sure | | |

**User's choice:** No, Mac is the start

| Option | Description | Selected |
|--------|-------------|----------|
| off everywhere | `-ffp-contract=off` in all presets; stable across build types and refactors | ✓ |
| Clang default (on) | Fastest; baselines may need wider tolerances | |
| Decide after measuring | Build both ways in Phase 1 (duplicates the Phase 2 spike) | |

**User's choice:** off everywhere

---

## End-to-end smoke test

| Option | Description | Selected |
|--------|-------------|----------|
| A few of my own scans | Real scans outside the repo, never committed | |
| Throwaway generated pages | Generated in the scratchpad; less realistic | ✓ |
| Both | Generated first, then own scans | |

**User's choice:** Throwaway generated pages

| Option | Description | Selected |
|--------|-------------|----------|
| Batch plus a few toggles | Batch all stages; toggle output mode, dewarping, a picture zone; save/close/reopen | ✓ |
| Batch with defaults only | Minimal | |
| Full manual walkthrough | Every stage by hand | |

**User's choice:** Batch plus a few toggles

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, observe and log | Log Retina blur, shortcuts/menus, alarm and the device pixel ratio; fix nothing | ✓ |
| Only pass/fail of BUILD-03 | Stick to the requirement | |

**User's choice:** Yes, observe and log

---

## Fix boundary and setup

| Option | Description | Selected |
|--------|-------------|----------|
| Minimal fix, then ask if it touches math | Platform/toolchain fixes allowed in own commits; stop and ask if output would change | ✓ |
| Log and stop | Compile fixes only | |
| Fix whatever is needed | Weakens the "unmodified" claim | |

**User's choice:** Minimal fix, then ask if it touches math

| Option | Description | Selected |
|--------|-------------|----------|
| Record, don't fix | Save warning log and counts; no source edits | ✓ |
| Fix trivial ones now | Adds diff noise before baseline | |

**User's choice:** Record, don't fix

| Option | Description | Selected |
|--------|-------------|----------|
| Claude runs brew install | Install cmake ninja boost qtbase qtsvg qttools, record versions | ✓ |
| I install them myself | Plan waits for the user | |

**User's choice:** Claude runs brew install

---

## Claude's Discretion

- Location of the Phase 1 records (suggested: phase directory)
- Preset layout (release, debug, asan, plus test presets)
- Exact CMake minimum (3.25 suggested, confirm at first configure)
- Commit granularity (one per error class)
- Offscreen platform for `qt_tests` under `ctest`

## Deferred Ideas

None.
