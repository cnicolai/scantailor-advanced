---
phase: 03-qt-6-and-macos-only
plan: 06
subsystem: build
tags: [translations, qt6, cmake, offscreen-ui-check, gate, strip]

requires:
  - phase: 03-qt-6-and-macos-only
    provides: gate.sh and ui_snapshot (03-01), Linux and portable-mode removal (03-05)
provides:
  - Translations removed in one gated commit (c10): .ts files, UpdateTranslations module, targets, LinguistTools, translator code, language accessors, Language row
  - D-12 after record: assertion run, static counts, texts diff, screenshots and the executor's visual verdict
affects: [03-07, 03-08, 03-09]

actuals:
  tokens: 7400
  tasks: 2
  commits: 3
plan_head_before: 09bc98e4618cbf60fbd6b82784aa8cb985573ec3
plan_head_after: ff254242ce831ad9596ec6f64f9a56c4e8b6bb39

tech-stack:
  added: []
  patterns:
    - "A CMake module, every call of it, the targets it creates and the code that consumed its defines leave in one commit"
    - "D-12: executor reads offscreen PNGs itself and records a verdict; no GUI step for the user"

key-files:
  created:
    - .planning/phases/03-qt-6-and-macos-only/records/ui-screenshots.md
    - .planning/phases/03-qt-6-and-macos-only/records/screenshots/after/main-window.png
    - .planning/phases/03-qt-6-and-macos-only/records/screenshots/after/settings-dialog.png
    - .planning/phases/03-qt-6-and-macos-only/records/screenshots/after/texts.txt
  modified:
    - CMakeLists.txt
    - config.h.in
    - src/core/Application.h
    - src/core/Application.cpp
    - src/core/ApplicationSettings.h
    - src/core/ApplicationSettings.cpp
    - src/app/main.cpp
    - src/app/MainWindow.cpp
    - src/app/SettingsDialog.cpp
    - src/app/SettingsDialog.ui
    - src/app/CMakeLists.txt
    - src/core/CMakeLists.txt
    - src/core/zones/CMakeLists.txt
    - src/core/interaction/CMakeLists.txt
    - src/core/filters/*/CMakeLists.txt (six filters)
    - .planning/phases/03-qt-6-and-macos-only/records/gate-log.md
  deleted:
    - cmake/UpdateTranslations.cmake
    - src/translations/ (eight .ts files)

key-decisions:
  - "#include \"Application.h\" stays in MainWindow.cpp and SettingsDialog.cpp as the plan says; only the includes <config.h> and <QDir> of Application.cpp went, which had no user left"
  - "The seven CMake files that had translation_sources as last line without trailing newline now end in exactly one newline (the preceding blank line was stripped too)"

requirements-completed: [STRIP-06]

coverage:
  - id: D1
    description: "Translations leave in one atomic commit: eight .ts files, UpdateTranslations module, compile_translations/update_translations targets, LinguistTools, ten translation_sources calls, TRANSLATION_DIRS/PLUGIN_DIRS, translator code, language accessors and the Language row"
    requirement: STRIP-06
    verification:
      - kind: other
        ref: "git show --name-only 56b5a540 lists UpdateTranslations.cmake, SettingsDialog.ui, Application.cpp and the .ts files; both leftover searches print nothing; gate.sh check c10-translations (result: PASS, 0-line command diff in both presets)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The rebuilt app neither builds nor loads a .qm file and the dialog form has no language widget"
    requirement: STRIP-06
    verification:
      - kind: other
        ref: "ui_snapshot build/scratch/ui-after --assert-no-language exit 0 (language control present: no); languageBox 0, translator symbols 0, scantailor_*.qm string 0, build.ninja references 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "D-06 holds: 249 tr()/translate() calls and 573 .ui strings (574 minus the Language label); D-07: no remaining code reads or writes the language key"
    requirement: STRIP-06
    verification:
      - kind: other
        ref: "git grep counts on HEAD: 249 and 573; search for getLanguage/setLanguage/LANGUAGE_KEY/DEFAULT_LANGUAGE in src prints nothing"
        status: pass
    human_judgment: false
  - id: D4
    description: "Settings dialog after c10 shows Color Scheme followed directly by the end of its group, only the Language row is gone, all text English"
    requirement: STRIP-06
    verification:
      - kind: other
        ref: "records/ui-screenshots.md (texts diff: only the two Language lines removed; Verdict: PASS)"
        status: pass
    human_judgment: true
    rationale: "Layout adequacy of the dialog is a visual judgment; the executor viewed the before and after PNGs and recorded the verdict, a reviewer may re-read them"

duration: 7min
completed: 2026-10-24
status: complete
---

# Phase 3 Plan 06: Translation removal Summary

**Translations are gone in one gated commit (eight .ts files, UpdateTranslations module, both targets, LinguistTools, Application's translator code, the language accessors and the Settings dialog's Language row) with an empty build-command diff, and the D-12 after check passes: no language control, no translator symbol, no .qm string, no translation step in build.ninja, English-only screenshots**

## Performance

- **Duration:** about 7 min (the c10 gate took about 4 min)
- **Started:** 2026-10-24T20:58:43Z
- **Completed:** 2026-10-24T21:06:00Z
- **Tasks:** 2
- **Files modified:** 29 in c10 (8 .ts and 1 CMake module deleted, 20 edited), plus 5 record files

## Accomplishments

- c10 (`56b5a540`): removed `src/translations/*.ts`, `cmake/UpdateTranslations.cmake`, the `# Translations` block (`finalize_translations`, `update_translations_target`, `Qt_add_translation`, `compile_translations`), `include(cmake/UpdateTranslations.cmake)`, `TRANSLATION_DIRS`, `PLUGIN_DIRS`, `SCANTAILOR_TRANSLATIONS_DIR`, the `LinguistTools` component and the ten `translation_sources` lines. `config.h.in` now holds only `ENABLE_DEBUG_FEATURES`, `APPLICATION_NAME` and `ORGANIZATION_NAME`.
- `Application` lost `getCurrentLocale`, `installLanguage`, `getLanguagesList`, `initTranslations`, `m_translator`, `m_currentLocale`, `m_translationsMap`, the QTranslator include, and the includes `<config.h>` and `<QDir>`. `ApplicationSettings` lost `getLanguage`, `setLanguage`, `DEFAULT_LANGUAGE`, `LANGUAGE_KEY`. The callers in `main.cpp`, `MainWindow::onSettingsChanged` and `SettingsDialog` went with them, and the whole `horizontalLayout_4` item left `SettingsDialog.ui` (27 lines, `xmllint` clean).
- Source-diff measure: 2 additions (the shortened Qt `find_package` line and the shortened `Application` constructor line), 29250 deletions, nearly all of it the `.ts` files.
- Gate `c10-translations`: `100% tests passed out of 7` in Release and Debug, goldens unchanged against a796c2ae, 0-line build-command diff in both presets. Row in `records/gate-log.md` (`5eb6f5d4`).
- D-12 after check: assertion run exit 0 with `language control present: no`; static counts 0, 0, 0, 0 (before 4, 5, 1, 40); texts diff shows only the removed lines `label: Language: ` and `combo languageBox: English`; executor's reading of the four PNGs recorded in `records/ui-screenshots.md` with `Verdict: PASS`.

## Task Commits

1. **Task 1 (c10): remove translations as one atomic commit** - `56b5a540` (refactor), gate row `5eb6f5d4` (docs)
2. **Task 2: D-12 after checks and visual verdict** - `ff254242` (docs)

**Plan metadata:** added by the final docs commit of this plan.

## Files Created/Modified

- `CMakeLists.txt`, `config.h.in`, ten `CMakeLists.txt` of core, filters, zones, interaction and app - translation machinery removed
- `src/core/Application.{h,cpp}`, `src/core/ApplicationSettings.{h,cpp}`, `src/app/{main,MainWindow,SettingsDialog}.cpp`, `src/app/SettingsDialog.ui` - translator code, language accessors and the Language row removed
- `records/ui-screenshots.md`, `records/screenshots/after/*` - D-12 after record

## Decisions Made

- `#include "Application.h"` stays in `MainWindow.cpp` and `SettingsDialog.cpp` (plan instruction; removing it is not part of the strip).
- A `language` key already present in the user's settings file is left alone: nothing reads, deletes or rewrites it any more (D-07). No real settings were read or written; the snapshot harness kept its settings below `build/scratch/ui-after/`.

## Deviations from Plan

None - plan executed exactly as written.

Environment notes, not deviations:
- The first cut of the `SettingsDialog.ui` item ended at an inner `</item>` and left the XML broken (`xmllint` caught it before any build); it was restored with `git checkout` and re-cut at the right closing tag. This happened before the commit.
- `find build/macos-release -name '*.qm'` still lists 7 stale files of earlier builds; the plan expects this and defers the fresh-directory check to 03-07. `build.ninja` has no `.qm` rule.
- The unrelated pre-existing `.planning/config.json` change and the untracked `.planning/milestone.lock` and `.planning/state.json` were not touched or committed.

## Issues Encountered

None.

## Known Stubs

None.

## Threat Flags

None. T-03-13 mitigated (search for every language reader and writer in `src` is empty; nothing deletes or rewrites an existing key). T-03-14 mitigated (output and settings below `build/scratch/ui-after`, PNGs show synthetic UI only). T-03-SC: no package installed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 03-07 can start. Remaining for it: README and docs rewrite, the fresh-clone check (including the "no `.qm` built" check on a fresh build directory) and the leftover searches; `README.md` and other docs may still mention translations.
- The gate reference for the current tree is written by the c10 gate.

## Self-Check: PASSED

- Commits `56b5a540`, `5eb6f5d4`, `ff254242` are ancestors of HEAD (`git rev-list --count 09bc98e4..HEAD` = 3 before this SUMMARY).
- Gate summary of c10-translations ends with `result: PASS`; `tested-commit.txt` equals `56b5a540`; both `cmds-*.diff` are empty.
- `src/translations` and `cmake/UpdateTranslations.cmake` do not exist; `git ls-files cmake` lists `ListItemsPrepend.cmake` and `SetDefaultBuildType.cmake` only; both leftover searches print nothing; tr()/translate() count 249, `.ui` string count 573; `git diff a796c2ae -- src/pipeline_tests/golden` is empty; `xcrun clang-format --dry-run -Werror` passes for the seven C++ files; `records/ui-screenshots.md` ends with `Verdict: PASS`.

---
*Phase: 03-qt-6-and-macos-only*
*Completed: 2026-10-24*
