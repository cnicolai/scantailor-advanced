Decision: list-corrected
Date: 2026-10-23

# Prior ScanTailor settings state before the first launch (plan 01-05 Task 3, D-02)

The user answered `list-corrected`. This permits directory listings (names, sizes and modification dates only; no file opened, nothing moved or deleted) of exactly these locations, which come from `records/probe-output.txt` and supersede the plist wording of D-01/D-02 (01-CONTEXT.md, "Amendment 2026-10-18"):

- `~/.config/scantailor-advanced/` (holds the INI file `~/.config/scantailor-advanced/scantailor-advanced.ini`, derived from `QSettings::IniFormat` at `src/app/main.cpp:47`)
- `~/Library/Application Support/scantailor-advanced/`, and, only if present, its subdirectory `scantailor-advanced/` (the AppDataLocation printed by the probe) and `scantailor-advanced/scantailor-advanced/profiles/`
- `~/Library/Preferences`, depth 1 only, entries whose names contain "scantailor" (case-insensitive); no other names printed or stored

## Listing

Commands run (all with `~` expanded by the shell; output normalised to `~`):

```
ls -la ~/.config/scantailor-advanced/
  ls: ~/.config/scantailor-advanced/: No such file or directory

ls -la ~/Library/Application\ Support/scantailor-advanced/
  ls: ~/Library/Application Support/scantailor-advanced/: No such file or directory

ls -la ~/Library/Application\ Support/scantailor-advanced/scantailor-advanced/
  not listed: the parent does not exist, so this directory does not exist

ls -la ~/Library/Application\ Support/scantailor-advanced/scantailor-advanced/profiles/
  not listed: the parent does not exist, so this directory does not exist

find ~/Library/Preferences -maxdepth 1 -iname '*scantailor*' -exec ls -ld {} +
  (no output: no entry at depth 1 contains "scantailor")
```

Result per location:

| Location | State |
|----------|-------|
| `~/.config/scantailor-advanced/` | does not exist |
| `~/Library/Application Support/scantailor-advanced/` | does not exist |
| `~/Library/Application Support/scantailor-advanced/scantailor-advanced/` | does not exist |
| `~/Library/Application Support/scantailor-advanced/scantailor-advanced/profiles/` | does not exist |
| `~/Library/Preferences` entries containing "scantailor" | none |

No other location was searched.

No ScanTailor entries found.
