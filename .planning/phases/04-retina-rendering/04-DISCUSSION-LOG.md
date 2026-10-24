# Phase 4: Retina Rendering - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md. This log keeps the alternatives that were considered.

**Date:** 2026-10-25
**Phase:** 04-retina-rendering
**Areas discussed:** Verification on the Mac, Thumbnails and their cache
**Areas offered but not chosen:** Look when zoomed in, Colour picker result (defaults recorded under Claude's Discretion)

---

## Verification on the Mac

### Who judges whether the app is sharp on Retina?

| Option | Description | Selected |
|--------|-------------|----------|
| Claude only (Recommended) | Tests at DPR 2, plus Claude runs the real app on the Studio Display, grabs the window at 2× and compares it with a reference rendering of the same synthetic page. Verdicts go into the phase records. No step for the user. | ✓ |
| Claude, then one look by you | The same automated proof, then a short walkthrough at the end (open a project, zoom to 100% and 200%, look at the thumbnails), about 3 to 4 steps. | |
| You look after each part | The user checks the page view, thumbnails and overlays as each one lands. | |

**User's choice:** Claude only

### Which displays does this Mac have?

| Option | Description | Selected |
|--------|-------------|----------|
| Only the Studio Display | One Retina screen; no non-Retina display to drag to. | ✓ |
| Studio Display + MacBook | Two screens, both DPR 2. | |
| Also a non-Retina monitor | An external screen at DPR 1. | |

**User's choice:** Only the Studio Display

### How should success criterion 4 (moving between a Retina and a non-Retina display) be proven?

| Option | Description | Selected |
|--------|-------------|----------|
| Simulated screens (Recommended) | An automated test with a 1× and a 2× virtual screen moves the view between them and asserts the rebuild at the new ratio. Recorded as proven by simulation. | ✓ |
| Simulation + one switch by you | The same test, plus the user sets the Studio Display to a low-resolution (non-HiDPI) mode while the app runs, then back. | |

**User's choice:** Simulated screens

### Should the 2× checks become permanent ctest tests?

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, all of them (Recommended) | Buffer-size, display-move and colour-picker checks live in `qt_tests` and run on every `ctest`, offscreen. Brings TESTX-03 forward from v2. | ✓ |
| Only the colour picker | Only what success criterion 5 demands becomes permanent; the other checks run once and are recorded. | |

**User's choice:** Yes, all of them

---

## Thumbnails and their cache

### How should thumbnails get enough pixels for Retina?

| Option | Description | Selected |
|--------|-------------|----------|
| Quality × DPR, default 250 (Recommended) | Quality becomes a screen-independent number multiplied by the DPR; the default rises from 200 to 250 to match the default Size; 500-px files on Retina. A user-set value is kept and doubled. | ✓ |
| Quality × DPR, default stays 200 | Same doubling, default 200, so 400-px files drawn into 500 device pixels (slightly soft, as at 1× today). | |
| Raw pixels, raise the default | Quality keeps meaning real file pixels; only the default rises (to 500). No DPR factor. | |

**User's choice:** Quality × DPR, default 250
**Notes:** An option that ties the file size to the display Size was left out, because thumbnails can be resized interactively and every resize would rebuild the files.

### What happens to the old, now-unused thumbnail files in existing projects?

| Option | Description | Selected |
|--------|-------------|----------|
| Leave them (Recommended) | The app never deletes anything in the output folders; the user can delete `cache/thumbs` by hand. | ✓ |
| Delete stale ones | When writing a new thumbnail, delete that page's files with other q-numbers. | |

**User's choice:** Leave them

### What should the strip show while sharp thumbnails build on first open?

| Option | Description | Selected |
|--------|-------------|----------|
| Usual placeholder (Recommended) | Same as today for a missing thumbnail; no new code; once per project. | ✓ |
| Old blurry thumbnail first | Show an existing q200 file stretched until the sharp one arrives. | |

**User's choice:** Usual placeholder

---

## Claude's Discretion

- Zoomed-in look: keep the smooth-to-hard-edge switch at 0.5 mm per source pixel, recalibrated for Retina.
- Colour picker: keep the median of the drawn circle, correct for a 2× grab, circle the same size on screen.
- The buffer rule, the `m_pixmap` preview layer, small icon pixmaps, memory, order and plan split, the HIDPI-01 re-check method, where the live harness lives, the Quality spin box range and tooltip wording.

## Deferred Ideas

- Colour picker sampling the image's own pixels instead of the drawn screen (not chosen for discussion; possible later decision).
- A cache-cleanup command for stale thumbnail files.
