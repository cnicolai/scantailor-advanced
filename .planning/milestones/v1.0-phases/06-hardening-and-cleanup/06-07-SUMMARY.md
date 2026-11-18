---
phase: 06-hardening-and-cleanup
plan: 07
subsystem: project-file-format
tags: [cpp, safety, xml, qdom, project-files, escape, round-trip, boost-test, gate]

requires:
  - phase: 06-hardening-and-cleanup
    provides: "plan 06-01 conventions (gate, label table, output-path guard) and the gate log"
provides:
  - "XmlMarshaller::needsEscaping, escapeInvalidXmlChars, setEscapedAttribute and XmlUnmarshaller::unescapeInvalidXmlChars, escapedAttribute: percent-encoding of attribute values that hold XML-1.0-forbidden characters, announced by <name>Escaped=1"
  - "ProjectWriter and ProjectReader use them for outputDirectory, directory path and file name"
  - "FoundationXmlEscapeTestSuite (7 cases) and round-trip cases for control characters, ordinary projects, ten unusual file names, tab/CR/CRLF folder names and a backslash"
  - "records/safe-02.md: QDom matrix, observe-first failure, escape format, tab/LF/CR/CRLF outcome against D-10's premise, backslash outcome, gate rows"
affects: [06-16]

actuals:
  tokens: 4100
  tasks: 2
  commits: 5

plan_head_before: 208650f5e781266ee9c00040d3baa1d37e9ef1f2
plan_head_after: 3b9d977cbb2f64b0d929829c86629ffd61ec55d5

tech-stack:
  added: []
  patterns:
    - "Escape only when needed, announced by a flag attribute, so ordinary project files stay byte-identical"
    - "Observe first: the new round-trip case was run against the unmodified writer and failed before the fix"
    - "Every new test proven to discriminate with an uncommitted mutation run, restored from a byte copy"

key-files:
  created:
    - src/foundation/tests/TestXmlEscape.cpp
    - .planning/phases/06-hardening-and-cleanup/records/safe-02.md
  modified:
    - src/foundation/XmlMarshaller.h
    - src/foundation/XmlMarshaller.cpp
    - src/foundation/XmlUnmarshaller.h
    - src/foundation/XmlUnmarshaller.cpp
    - src/foundation/tests/CMakeLists.txt
    - src/core/ProjectWriter.cpp
    - src/core/ProjectReader.cpp
    - src/pipeline_tests/TestProjectRoundTrip.cpp
    - .planning/phases/06-hardening-and-cleanup/records/gate-log.md

key-decisions:
  - "Fix, not mark: the flagged percent-encoding escape is implemented; no XFAIL and no backlog item 999.9 was needed"
  - "The flag attribute is exactly `1` and only then is the value decoded, so a literal %01 in an ordinary name is never changed (T-06-11)"
  - "The backslash round-trips through the project file with plain checks; it stays in its own suite and backlog 999.8 (RelinkablePath::normalize) is untouched"
  - "D-10's premise (tab, LF, CR, CRLF become spaces) is false on Qt 6.12.0: QDom writes character references and the names come back identical, so those cases are plain tests, not fixes"

requirements-completed: []

coverage:
  - id: D1
    description: "A project with U+0001 and U+001F in the input folder, two file names and the output folder saves, parses through both read paths and reloads with every path identical; the second save is byte-identical; the file carries nameEscaped, pathEscaped and outputDirectoryEscaped"
    requirement: SAFE-02
    verification:
      - kind: unit
        ref: "src/pipeline_tests/TestProjectRoundTrip.cpp#test_control_characters_survive"
        status: pass
    human_judgment: false
  - id: D2
    description: "An ordinary project's file carries no flag attribute and stays byte-identical (round-trip and Phase 2 goldens unchanged)"
    requirement: SAFE-02
    verification:
      - kind: unit
        ref: "src/pipeline_tests/TestProjectRoundTrip.cpp#test_ordinary_projects_carry_no_escape_flag"
        status: pass
    human_judgment: false
  - id: D3
    description: "Escape helpers: all 30 forbidden code units are detected and reversed exactly (UTF-8, non-BMP included); ordinary values, empty value and an unflagged %01 are left alone"
    requirement: SAFE-02
    verification:
      - kind: unit
        ref: "src/foundation/tests/TestXmlEscape.cpp#FoundationXmlEscapeTestSuite"
        status: pass
    human_judgment: false
  - id: D4
    description: "Ten unusual file names, a folder with a leading space and LF, and tab, CR and CRLF folder names round-trip with identical code units, byte-identical files and no flag; ProjectRoundTripTestSuite holds no expected-failure mark"
    requirement: SAFE-02
    verification:
      - kind: unit
        ref: "src/pipeline_tests/TestProjectRoundTrip.cpp#test_unusual_names_survive"
        status: pass
    human_judgment: false
  - id: D5
    description: "A backslash in a file name round-trips plainly, in its own suite"
    requirement: SAFE-02
    verification:
      - kind: unit
        ref: "src/pipeline_tests/TestProjectRoundTrip.cpp#test_backslash_in_file_name"
        status: pass
    human_judgment: false
  - id: D6
    description: "Both code commits pass the gate: 100% tests passed out of 7 in Release and Debug, goldens unchanged"
    requirement: SAFE-02
    verification:
      - kind: other
        ref: "gate.sh check s08-name-escape (tested 01cb8f15) and s09-name-matrix (tested 18affb3d) -> result: PASS"
        status: pass
    human_judgment: false

duration: 20min
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 07: SAFE-02, names with control characters in project files Summary

**A file, folder or output-folder name with an XML-1.0-forbidden control character used to make the saved project unloadable; such values are now written percent-encoded with a `<name>Escaped="1"` flag and decoded on load, while ordinary project files stay byte-identical, and the D-10 matrix of unusual names (markup, whitespace, tab/LF/CR/CRLF, emoji, DEL, backslash) is pinned by tests with no expected-failure mark.**

## Performance

- **Duration:** about 20 min of clock time (two gate runs in the background)
- **Completed:** 2026-11-17
- **Tasks:** 2 (tracer, auto)
- **Files:** 2 created (test, record), 9 modified (7 under `src/`, gate log and CMake list)

## Accomplishments

- `XmlMarshaller::setEscapedAttribute` writes a value that `needsEscaping` (code unit below U+0020 except tab, LF, CR, or U+FFFE or U+FFFF) as `QString::fromLatin1(toUtf8().toPercentEncoding())` plus `<name>Escaped="1"`; any other value is written as before. `XmlUnmarshaller::escapedAttribute` decodes only when the flag is exactly `1`. The three name attributes of `ProjectWriter`/`ProjectReader` use the pair.
- The FIXME in `XmlUnmarshaller::string` is replaced by a comment (D-09): `QDomElement::text()` already returns unescaped text.
- Observe first: `test_control_characters_survive` failed on the unmodified code with `project file is not well-formed XML: Unexpected ''.` (`build/scratch/rt-s08-before.txt`).
- `roundTrip` takes an output-folder override and parses the first file through a `QFile` as well (the read path of `MainWindow::openProject`); the fixture gained `writePng`.
- `test_unusual_names_survive`: ten file names (tab, LF, CR, CRLF, leading and trailing space, `'`, `>`, emoji, DEL) in a folder ` in<LF>dir`, plus folders with tab, CR and CRLF: all identical after reload, files byte-identical, no flag.
- `ProjectRoundTripBackslashTestSuite/test_backslash_in_file_name`: the backslash round-trips with plain checks, so no `XFAIL_CHECK` is used anywhere in the plan.
- `records/safe-02.md`: the QDom matrix, escape format, byte-identity evidence, every case, and the observed attribute text for tab, LF, CR and CRLF in file and folder names set against D-10's premise (they are written as `&#x9;`, `&#xa;`, `&#xd;` and come back identical; they do not become spaces).
- Mutation runs (not committed, restored from byte copies): `needsEscaping` forced false fails the helper and the round-trip tests; flag compared with `"2"` fails the attribute round trip; removing the tab exception fails `test_unusual_names_survive`.
- Gates: `s08-name-escape` (tested `01cb8f15`, command diff 3 / 3: the new `TestXmlEscape.cpp` compile line and the `foundation_tests` link line) and `s09-name-matrix` (tested `18affb3d`, command diff 0 / 0): Release and Debug `100% tests passed out of 7`, goldens unchanged. Output-path guard of convention 13 prints nothing; every touched C++ file passes `clang-format --dry-run -Werror`.

## Task Commits

1. **Task 1 (tracer): control characters in names survive save and reload; ordinary files unchanged** - `01cb8f15` (fix), gated as `s08-name-escape`; gate record `870e284b`
2. **Task 2: the D-10 matrix, the backslash case and the SAFE-02 record** - `18affb3d` (test), gated as `s09-name-matrix`; gate record `ce682f48`; record `3b9d977c` (docs)

## Deviations from Plan

None - plan executed as written. Small additions, none changing the outcome:

- `test_unusual_names_survive` also covers tab, CR and CRLF in folder names (three extra files), because the plan's record asks for those characters in the folder name and the plan's single LF folder would not have produced the attribute text.
- Gate runs were started from a background subshell and waited for with a polling loop on the gate log, since the harness runs a command started with a trailing `&` to completion immediately.

## Issues Encountered

None open.

## Known Stubs

None.

## Threat Flags

None beyond the plan's threat model. T-06-04 (project file unloadable because of raw forbidden characters) is mitigated and tested through both parse paths; T-06-11 (decoding of attacker-written percent sequences) is mitigated by decoding only under the exact flag `1`, with a unit test that an unflagged `%01` and a flag value other than `1` stay literal. The new project-file surface is the three optional flag attributes `outputDirectoryEscaped`, `pathEscaped`, `nameEscaped`, which appear only in files that could not be loaded before; an older build reading such a file sees percent-encoded text.

## Next Phase Readiness

SAFE-02 stays unmarked in REQUIREMENTS.md until plan 06-16 has finished (`requirements.ready-ids` holds it). Plan 06-08 (SAFE-03, memory cap for the worker threads) is next. No backlog item 999.9 was created; backlog 999.8 is unchanged.

## Self-Check: PASSED

- Created and modified files exist (`TestXmlEscape.cpp`, `records/safe-02.md`, the seven source files); commits `01cb8f15`, `870e284b`, `18affb3d`, `ce682f48`, `3b9d977c` are ancestors of HEAD and `git rev-list --count` from the recorded base gives 5 (pass).
- Acceptance criteria re-run: no `FIXME` in `XmlUnmarshaller.cpp`; `setEscapedAttribute` 3 times in `ProjectWriter.cpp`, `escapedAttribute` 3 times in `ProjectReader.cpp`; `rt-s08-before.txt` shows the failure, `rt-s08.txt` and `fnd-s08.txt` hold `No errors detected` and no `KNOWN DEFECT`; no `XFAIL_CHECK` between the suite start and end in `ProjectRoundTripTestSuite`; gate summaries `result: PASS`, `goldens: unchanged`, tested commits equal the commits with the plan subjects, both gate-log rows exist; `safe-02.md` holds `nameEscaped`, `backslash`, `CRLF`, `D-10` and no home path (pass).
- Output-path guard prints nothing; clang-format dry-run clean on all touched C++ files (pass).
