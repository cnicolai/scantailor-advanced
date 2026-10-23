# Smoke run findings (plan 01-06, BUILD-03, D-05)

BUILD-03 verdict: PENDING

## Environment

- Date: 2026-10-23
- Commit at build time: `015b7a4`
- Preset: `macos-release` (`cmake --preset macos-release --fresh`, then `cmake --build --preset macos-release`; both exit 0)
- Binary: `{repo}/build/macos-release/scantailor-advanced`
- Pages directory: `{scratch}/smoke/pages/` (regenerated in this session with the commands in `records/smoke-pages.md` `## Regenerate`; the earlier session's copy was gone)
- Output directory: `{scratch}/smoke/out/` (created empty)
- SHA-256 of the regenerated pages against the table in `records/smoke-pages.md`:

| Page | SHA-256 | Match |
|------|---------|-------|
| `01-flat-text.tif` | `2d47f7364b7c087e202076096848a7f4bf309aae002b8052fd8f46cabfea2c8f` | match |
| `02-book-spread.tif` | `de3b20d497c2802f716fd1c1cd5ee430ab2855ea7aeee2ed944f170f05f2a416` | match |
| `03-mixed.tif` | `d7876136fd3c39e212bd932702765a97f64a649b9bd737013f2bba07dc8c5d7b` | match |

- Prior settings state (D-02 outcome, `records/prior-state.md`): `list-corrected`. The listed locations (`~/.config/scantailor-advanced/`, `~/Library/Application Support/scantailor-advanced/`, `~/Library/Preferences` entries containing "scantailor") did not exist: no ScanTailor entries found. The walkthrough therefore starts from a clean settings state.

## Walkthrough

| Step | What the user did | What to look for | User report | Result |
|------|-------------------|------------------|-------------|--------|
