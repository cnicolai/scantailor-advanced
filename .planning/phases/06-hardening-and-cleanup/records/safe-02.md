# SAFE-02: file and folder names in project files (plan 06-07)

Checked on 2026-11-17, Qt 6.12.0, macOS on Apple Silicon. Code commits: `01cb8f15` (the escape, gate `s08-name-escape`) and `18affb3d` (the name matrix, gate `s09-name-matrix`). Evidence files are under `build/scratch/` in `{repo}` (not committed): `rt-s08-before.txt`, `rt-s08.txt`, `fnd-s08.txt`, `rt-s09.txt`, `rt-s09-backslash.txt`, `rt-s09-evidence-keep.txt`.

## D-09: the FIXME described no real defect

`XmlUnmarshaller::string` carried the comment "this needs unescaping, but Qt doesn't provide such functionality". `QDomElement::text()` returns text with character and entity references already resolved by the parser, and the Phase 2 record (`.planning/phases/02-golden-baselines/records/round-trip.md`) measured that `&`, `<`, `"`, NFC and NFD names already round-trip byte for byte. The FIXME is replaced by a comment saying so. Names are written as attribute values (`ProjectWriter.cpp`: `outputDirectory`, directory `path`, file `name`), never as element text, so `XmlUnmarshaller::string` is not on the name path at all.

## What QDom writes (research measurement, Qt 6.12.0)

| Case | Round trips on the unmodified code | Written form |
|------|-----------------------------------|--------------|
| `&`, `<`, `>`, `"`, `'`, NFC, NFD, emoji U+1F4C4 | yes, both read paths | normal escaping |
| tab, LF, CR, CRLF | yes | character references `&#x9;`, `&#xa;`, `&#xd;` |
| leading, trailing, doubled spaces | yes in attributes | literal |
| U+007F, U+0085, U+00A0, U+2028, `]]>`, U+FEFF | yes | literal |
| U+0001 to U+0008, U+000B, U+000C, U+000E to U+001F, U+FFFE, U+FFFF | no: written raw, the file does not parse | raw byte |
| `&#x1;` and `&#1;` character references, XML 1.1 declaration | the parser rejects them | cannot be used as a fix |

APFS refuses U+FFFE and U+FFFF in names (`EILSEQ`), so the realistic failing class is the C0 controls. Before this plan such a name made the project file unloadable ("The project file is broken.").

## Observe first

The new case `test_control_characters_survive` was added before any source change and run against the unmodified writer and reader (`build/scratch/rt-s08-before.txt`):

```
fatal error: in "ProjectRoundTripTestSuite/test_control_characters_survive": project file is not well-formed XML: Unexpected ''.
```

The first file written does not parse, as the research predicted.

## The fix: escape only when needed, announced by a flag

- `XmlMarshaller::needsEscaping(value)`: true if a code unit is below U+0020 other than tab, LF and CR, or is U+FFFE or U+FFFF.
- `XmlMarshaller::escapeInvalidXmlChars(value)`: `QString::fromLatin1(value.toUtf8().toPercentEncoding())`, printable ASCII.
- `XmlMarshaller::setEscapedAttribute(el, name, value)`: plain `setAttribute(name, value)` unless `needsEscaping`; then the escaped value and `<name>Escaped="1"`.
- `XmlUnmarshaller::escapedAttribute(el, name)`: percent-decodes only if `<name>Escaped` is exactly `1`; an unflagged `%01` stays literal. `unescapeInvalidXmlChars` is the decoder.
- `ProjectWriter.cpp` writes `outputDirectory`, directory `path` and file `name` with `setEscapedAttribute`; `ProjectReader.cpp` reads the same three with `escapedAttribute`, decoding before `resolvePath`. Flag attributes: `outputDirectoryEscaped`, `pathEscaped`, `nameEscaped`.

Example written by the helper (scratch program, `build/scratch/escdemo`):

```
<project outputDirectory="out%1Fdir" outputDirectoryEscaped="1">
  <file dirId="1" id="2" name="ctrl%01one.png" nameEscaped="1"/>
</project>
```

The format gains three optional attributes. They appear only in files that contain a forbidden character, and those files could not be loaded before. An older build reading such a file sees percent-encoded text instead of a failure (reversibility "costly" in the plan).

## Ordinary files are byte-identical

- `test_ordinary_projects_carry_no_escape_flag`: the default five-name project's first file contains no `Escaped="`.
- `test_round_trip_is_byte_identical`, `test_names_and_order_survive` and the other Phase 2 cases pass unchanged, and both gates report `goldens: unchanged` (the Phase 2 golden text is untouched).
- `test_unusual_names_survive` asserts the same for the ten unusual names below.

## Tests

Unit tests, `FoundationXmlEscapeTestSuite` (`src/foundation/tests/TestXmlEscape.cpp`, 7 cases, all pass): `test_ordinary_values_need_no_escaping`, `test_forbidden_characters_need_escaping` (all 30 forbidden code units), `test_escape_round_trips`, `test_attribute_round_trip_through_xml`, `test_ordinary_attribute_writes_no_flag`, `test_empty_value`, `test_unflagged_percent_is_literal` (also: a flag other than exactly `1` does not decode).

Round trip, `ProjectRoundTripTestSuite` (no `XFAIL_CHECK` in the suite, no `KNOWN DEFECT` in the log): `roundTrip` takes an optional output-folder override and parses the first file twice, from bytes and through a `QFile` (the read path of `MainWindow::openProject`).

| Case | Names | Result |
|------|-------|--------|
| `test_control_characters_survive` | folder `in` U+0001 `dir`; files `ctrl` U+0001 `one.png` and `ctrl` U+001F `unit.png`; output folder `out` U+001F `dir` | read paths and output folder equal as `QString`; second file byte-identical to the first; first file contains `nameEscaped="1"`, `pathEscaped="1"`, `outputDirectoryEscaped="1"` |
| `test_ordinary_projects_carry_no_escape_flag` | the five default names | no `Escaped="` in the file |
| `test_unusual_names_survive` | folder ` in` LF `dir`; files `tab` TAB `name.png`, `line` LF `feed.png`, `carriage` CR `return.png`, `crlf` CR LF `name.png`, ` leading space.png`, `trailing space .png`, `it's.png`, `a>b.png`, emoji `page.png`, `del` U+007F `.png`; folders `tab` TAB `dir`, `carriage` CR `dir`, `crlf` CR LF `dir` with one file each | all read paths equal; files byte-identical; no `Escaped="` |
| `ProjectRoundTripBackslashTestSuite/test_backslash_in_file_name` | `back\slash.png` in the input folder | passes with plain checks (read path equal, files byte-identical) |

Discrimination (uncommitted mutations, restored from byte copies, `git diff` empty afterwards): `needsEscaping` forced to false makes `test_forbidden_characters_need_escaping` report every forbidden code unit and `test_control_characters_survive` fail with "not well-formed XML"; the flag compared with `"2"` instead of `"1"` fails the attribute round trip; removing the tab exception from `needsEscaping` makes `test_unusual_names_survive` fail at `!result.firstFile.contains("Escaped=\"")`.

## Tab, LF, CR and CRLF against D-10's premise

D-10 assumed that attribute-value normalisation turns tab, LF, CR and CRLF in names into spaces, so that they would not survive. Observed (Qt 6.12.0): QDom writes them as character references and the parser restores them, so every one of them round-trips with no fix. Attribute text as written in the first file of `test_unusual_names_survive` (`rt-s09-evidence-keep.txt`):

| Character | In a file name (`<file ... name=...>`) | Name identical after reload | In a folder name (`<directory ... path=...>`) | Name identical after reload |
|-----------|----------------------------------------|-----------------------------|-----------------------------------------------|-----------------------------|
| tab | `name="tab&#x9;name.png"` | yes | `path="tab&#x9;dir"` | yes |
| LF | `name="line&#xa;feed.png"` | yes | `path=" in&#xa;dir"` (with a leading space) | yes |
| CR | `name="carriage&#xd;return.png"` | yes | `path="carriage&#xd;dir"` | yes |
| CRLF | `name="crlf&#xd;&#xa;name.png"` | yes | `path="crlf&#xd;&#xa;dir"` | yes |

Other lines of that file: `name=" leading space.png"`, `name="trailing space .png"`, `name="it's.png"`, `name="a>b.png"` are written literally; the DEL name carries the raw byte U+007F (`name="del<U+007F>.png"`); the emoji name is written as its UTF-8 bytes. None carries a flag.

## Backslash (backlog 999.8)

`back\slash.png` round-trips through the project file: read path equal, second file byte-identical, plain checks, no `XFAIL_CHECK`. The research expected a failure through `RelinkablePath::normalize` (`replace('\\', "/")`), but the project round trip does not go through that class; it concerns relinking. Backlog 999.8 is untouched and `RelinkablePath::normalize` was not changed (D-10, D-11). The case sits in its own suite so `ProjectRoundTripTestSuite` never carries a mark (roadmap criterion 2).

## Equality

Name equality in the round trip is `QString` code-unit equality: NFC and NFD stay distinct and both survive (`test_names_and_order_survive`). The escape works on UTF-8 bytes and reverses exactly, including a non-BMP emoji (`test_escape_round_trips`). An empty value is written as an empty attribute with no flag and reads back empty (`test_empty_value`).

## Gate rows

| Label | Commit | Release ctest | Debug ctest | Goldens | Command diff |
|-------|--------|---------------|-------------|---------|--------------|
| s08-name-escape | `01cb8f15` | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 3 / 3 (the new `TestXmlEscape.cpp` compile line and the `foundation_tests` link line) |
| s09-name-matrix | `18affb3d` | 100% tests passed out of 7 | 100% tests passed out of 7 | unchanged | 0 / 0 |

Full rows: `records/gate-log.md`.

## Not covered

- A lone UTF-16 surrogate in a name silently becomes U+FFFD in `toUtf8()`; it cannot occur in an APFS name (names are UTF-8).
- U+FFFE and U+FFFF are escaped by the helper (unit-tested) but cannot be created on APFS, so no round-trip case creates them.
