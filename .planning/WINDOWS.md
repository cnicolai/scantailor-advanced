---
schema_version: 1
open_count: 1
waived_count: 0
fixed_count: 0
total_count: 1
last_updated: 2026-10-23T19:13:34.694Z
---

# Broken Windows Ledger

> Cross-phase defect register. With `workflow.windows_enforce` enabled, `/gsd-ship` blocks while `open_count > 0`.
> Waive with `gsd-tools windows waive <id> "<reason>"` (reason required).
> Mark fixed with `gsd-tools windows fixed <id>`.

| id | phase | kind | file | line | description | status | reason | recorded_at | resolved_at |
|----|-------|------|------|------|-------------|--------|--------|-------------|-------------|
| 1 | 02 | unmet-truth | src/pipeline_tests/TestFlatDocument.cpp |  | F-05-1: flat_600_bw binarization mismatch 8.3% against 0.5% (output keeps about 0.1 deg residual skew at 600 dpi input), XFAIL_CHECK, no requirement | open |  | 2026-10-23T19:13:34.694Z |  |

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
  }
]
````
