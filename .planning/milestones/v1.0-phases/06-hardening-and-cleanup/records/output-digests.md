# Output digests, CLEAN-01 split proof

SHA-1 of the decoded output pixels (the output file read back through `ImageLoader`), row by row without padding; a bilevel output is expanded to `Format_Grayscale8` first, because the padding bits of its rows are unspecified. Line format: `DIGEST <kind> <dpi> <colour> <dewarping> - <sha1>`. The seven pages come from `test_support::generatePage` with default seed and skew, all with `SINGLE_PAGE_UNCUT` and `outputDpi` equal to the page dpi.

Tool: `pipeline_tests --run_test=OutputDigestProbeTestSuite --log_level=message` (`src/pipeline_tests/TestOutputDigestProbe.cpp`, disabled by default, never run by ctest), Release build.

## Research harness (HEAD e0b632f7, `build/scratch/phase6/m4`)

```
DIGEST flat 300 bw off - d76527cfd84d79f5784c6967e970efb3a5d76c87
DIGEST flat 600 gray off - 1e78a1c0424fd3ef129c9a840a0de3100104d49b
DIGEST mixed 300 mixed off - b7e3a693d73c03ebc79546385c601c537a7cac8b
DIGEST mixed 600 mixed off - 718260ea80a1b8883bf8945ea0e3a23694f7e587
DIGEST curved 300 gray auto - 0476631ce760feb5fb117efd9720e83df0772f8d
DIGEST curved 600 mixed auto - c48ff8a8d7a4be446c75355380ed9bf055675dd7
DIGEST curved 600 bw auto - dd346ae2c91df2c7529f26f9c2ba317d25fb4241
```

## s17 probe, before the split (commit f899b433, tested as gate `s17-digest-probe`)

```
DIGEST flat 300 bw off - d76527cfd84d79f5784c6967e970efb3a5d76c87
DIGEST flat 600 gray off - 1e78a1c0424fd3ef129c9a840a0de3100104d49b
DIGEST mixed 300 mixed off - b7e3a693d73c03ebc79546385c601c537a7cac8b
DIGEST mixed 600 mixed off - 718260ea80a1b8883bf8945ea0e3a23694f7e587
DIGEST curved 300 gray auto - 0476631ce760feb5fb117efd9720e83df0772f8d
DIGEST curved 600 mixed auto - c48ff8a8d7a4be446c75355380ed9bf055675dd7
DIGEST curved 600 bw auto - dd346ae2c91df2c7529f26f9c2ba317d25fb4241
```

Verdict: the probe's seven lines equal the research lines (`diff` of the two line sets is empty). The probe is a faithful copy of the harness, and the output of the stage did not change between e0b632f7 and the start of this plan. The probe's own output is the reference for the split steps.

## s18 probe, after the first split step (commit ff354702, tested as gate `s18-split-nodewarp`)

```
DIGEST flat 300 bw off - d76527cfd84d79f5784c6967e970efb3a5d76c87
DIGEST flat 600 gray off - 1e78a1c0424fd3ef129c9a840a0de3100104d49b
DIGEST mixed 300 mixed off - b7e3a693d73c03ebc79546385c601c537a7cac8b
DIGEST mixed 600 mixed off - 718260ea80a1b8883bf8945ea0e3a23694f7e587
DIGEST curved 300 gray auto - 0476631ce760feb5fb117efd9720e83df0772f8d
DIGEST curved 600 mixed auto - c48ff8a8d7a4be446c75355380ed9bf055675dd7
DIGEST curved 600 bw auto - dd346ae2c91df2c7529f26f9c2ba317d25fb4241
```

Verdict: equal to the s17 lines, line for line (`diff` of the two sorted line sets is empty, seven lines each). The first split step does not change a decoded output pixel of any of the seven pages. The Debug build of `pipeline_tests` was built too; the gate runs Release and Debug ctest, goldens unchanged.

## s19 probe, after the second split step (commit 4650ec47, tested as gate `s19-split-rest`)

```
DIGEST flat 300 bw off - d76527cfd84d79f5784c6967e970efb3a5d76c87
DIGEST flat 600 gray off - 1e78a1c0424fd3ef129c9a840a0de3100104d49b
DIGEST mixed 300 mixed off - b7e3a693d73c03ebc79546385c601c537a7cac8b
DIGEST mixed 600 mixed off - 718260ea80a1b8883bf8945ea0e3a23694f7e587
DIGEST curved 300 gray auto - 0476631ce760feb5fb117efd9720e83df0772f8d
DIGEST curved 600 mixed auto - c48ff8a8d7a4be446c75355380ed9bf055675dd7
DIGEST curved 600 bw auto - dd346ae2c91df2c7529f26f9c2ba317d25fb4241
```

Verdict: equal to the s17 lines, line for line (`diff` of the two line sets is empty, seven lines each). The whole split (both steps) does not change a decoded output pixel of any of the seven pages. Release and Debug ctest ran in the gate, goldens unchanged.
