---
schema_version: 1
open_count: 3
waived_count: 0
fixed_count: 0
total_count: 3
last_updated: 2026-10-23T19:43:07.270Z
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
  }
]
````
