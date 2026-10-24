---
schema_version: 1
open_count: 5
waived_count: 1
fixed_count: 0
total_count: 6
last_updated: 2026-10-24T13:46:14.849Z
---

# Broken Windows Ledger

> Cross-phase defect register. With `workflow.windows_enforce` enabled, `/gsd-ship` blocks while `open_count > 0`.
> Waive with `gsd-tools windows waive <id> "<reason>"` (reason required).
> Mark fixed with `gsd-tools windows fixed <id>`.

| id | phase | kind | file | line | description | status | reason | recorded_at | resolved_at |
|----|-------|------|------|------|-------------|--------|--------|-------------|-------------|
| 1 | 02 | unmet-truth | src/pipeline_tests/TestFlatDocument.cpp |  | F-05-1: flat_600_bw binarization mismatch 8.3% against 0.5% (output keeps about 0.1 deg residual skew at 600 dpi input), XFAIL_CHECK, no requirement | open |  | 2026-10-23T19:13:34.694Z |  |
| 2 | 02 | unmet-truth | src/pipeline_tests/TestBookSpread.cpp |  | F-06-1: page_split cutter 30.4 px (300 dpi) and 55.7 px (600 dpi) from the gutter centre at mid-height against 2 px, XFAIL_CHECK in four spread scenarios, no requirement | open |  | 2026-10-23T19:43:07.149Z |  |
| 3 | 02 | deviation | src/pipeline_tests/TestBookSpread.cpp |  | F-06-2: TruthMetrics straightnessResidual finds no valid strip on the OFF output of the curved page and has a 4 to 9 px glyph-content noise floor; the dewarp checks use a truth-registered line offset measure local to the test | open |  | 2026-10-23T19:43:07.270Z |  |
| 4 | 02 | unmet-truth | src/pipeline_tests/TestMixedPage.cpp |  | F-07-1: page_split AUTO cuts the generated mixed page (SINGLE_PAGE_CUT, two cutters) and keeps only a fragment of its picture, dropping the text; the coverage scenarios force SINGLE_PAGE_UNCUT | open |  | 2026-10-23T20:00:42.678Z |  |
| 5 | 02 | deviation | src/pipeline_tests/TestMixedPage.cpp |  | Plan 02-07 fixture sanity 'at least 90% of the specks present with despeckle off' cannot hold: the output keeps only the content box, 24 of 40 (300 dpi) and 28 of 40 (600 dpi) specks lie outside it; the test checks the specks inside the content box instead | waived | Accepted by the user in Phase 02 UAT (test 2, D7): the plan assumed every speck reaches the output, but the output stage crops to the content box. Counting specks inside the content box (15/15 at 300 dpi, 11/11 at 600 dpi, floor of 10) still tests despeckling; fixture, specks, despeckle level and production code unchanged. | 2026-10-23T20:00:42.800Z | 2026-10-24T13:46:14.849Z |
| 6 | 02 | deviation | src/pipeline_tests/TestProjectRoundTrip.cpp |  | Plan 02-08: page-split element of the project file does not differ with DefaultParams alone (F-08-1); the test runs the page_split stage for every page so the stage carries non-default values | open |  | 2026-10-23T20:14:58.039Z |  |

````json
[
  {
    "id": 1,
    "kind": "unmet-truth",
    "phase": "02",
    "file": "src/pipeline_tests/TestFlatDocument.cpp",
    "line": null,
    "description": "F-05-1: flat_600_bw binarization mismatch 8.3% against 0.5% (output keeps about 0.1 deg residual skew at 600 dpi input), XFAIL_CHECK, no requirement",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-10-23T19:13:34.694Z",
    "resolved_at": null,
    "milestone": "v1.0"
  },
  {
    "id": 2,
    "kind": "unmet-truth",
    "phase": "02",
    "file": "src/pipeline_tests/TestBookSpread.cpp",
    "line": null,
    "description": "F-06-1: page_split cutter 30.4 px (300 dpi) and 55.7 px (600 dpi) from the gutter centre at mid-height against 2 px, XFAIL_CHECK in four spread scenarios, no requirement",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-10-23T19:43:07.149Z",
    "resolved_at": null,
    "milestone": "v1.0"
  },
  {
    "id": 3,
    "kind": "deviation",
    "phase": "02",
    "file": "src/pipeline_tests/TestBookSpread.cpp",
    "line": null,
    "description": "F-06-2: TruthMetrics straightnessResidual finds no valid strip on the OFF output of the curved page and has a 4 to 9 px glyph-content noise floor; the dewarp checks use a truth-registered line offset measure local to the test",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-10-23T19:43:07.270Z",
    "resolved_at": null,
    "milestone": "v1.0"
  },
  {
    "id": 4,
    "kind": "unmet-truth",
    "phase": "02",
    "file": "src/pipeline_tests/TestMixedPage.cpp",
    "line": null,
    "description": "F-07-1: page_split AUTO cuts the generated mixed page (SINGLE_PAGE_CUT, two cutters) and keeps only a fragment of its picture, dropping the text; the coverage scenarios force SINGLE_PAGE_UNCUT",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-10-23T20:00:42.678Z",
    "resolved_at": null,
    "milestone": "v1.0"
  },
  {
    "id": 5,
    "kind": "deviation",
    "phase": "02",
    "file": "src/pipeline_tests/TestMixedPage.cpp",
    "line": null,
    "description": "Plan 02-07 fixture sanity 'at least 90% of the specks present with despeckle off' cannot hold: the output keeps only the content box, 24 of 40 (300 dpi) and 28 of 40 (600 dpi) specks lie outside it; the test checks the specks inside the content box instead",
    "status": "waived",
    "reason": "Accepted by the user in Phase 02 UAT (test 2, D7): the plan assumed every speck reaches the output, but the output stage crops to the content box. Counting specks inside the content box (15/15 at 300 dpi, 11/11 at 600 dpi, floor of 10) still tests despeckling; fixture, specks, despeckle level and production code unchanged.",
    "recorded_at": "2026-10-23T20:00:42.800Z",
    "resolved_at": "2026-10-24T13:46:14.849Z",
    "milestone": "v1.0"
  },
  {
    "id": 6,
    "kind": "deviation",
    "phase": "02",
    "file": "src/pipeline_tests/TestProjectRoundTrip.cpp",
    "line": null,
    "description": "Plan 02-08: page-split element of the project file does not differ with DefaultParams alone (F-08-1); the test runs the page_split stage for every page so the stage carries non-default values",
    "status": "open",
    "reason": "",
    "recorded_at": "2026-10-23T20:14:58.039Z",
    "resolved_at": null,
    "milestone": "v1.0"
  }
]
````
