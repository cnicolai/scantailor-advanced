#!/usr/bin/env python3
"""Pure-move proof by chunk multiset (CLEAN-01 and CLEAN-02, Phase 6).

Usage (from the repository root, run as `python3 -I`):
  move_check.py --base <rev> --orig <path> --new <path> [<path> ...]
                [--allow-changed <text> ...] [--allow-line <regex> ...]

The original file is read with `git show <rev>:<orig>`, the new files from the working tree.
Structural lines are dropped (blank lines, #include, include-guard #ifndef/#define/#endif,
`using namespace ...;`, `namespace ... {`, `}  // namespace ...`, the two copyright lines), trailing
whitespace is stripped, and the rest is cut into top-level chunks that end with a line starting
with `}` in column 0. The chunk multisets of the original and of all new files together must be
equal. A chunk whose first line contains an --allow-changed text may differ: the lines of such
unmatched chunks are compared as multisets, nothing of the original may be lost, and every added
line must match an --allow-line regex.

Exit 0 and `RESULT: PASS` when the multisets agree, exit 1 and `RESULT: FAIL` otherwise.
"""

import argparse
import re
import subprocess
import sys
from collections import Counter

STRUCTURAL = [
    re.compile(r"^\s*$"),
    re.compile(r"^\s*#\s*include\b"),
    re.compile(r"^#ifndef\s+SCANTAILOR_\w+_H_\s*$"),
    re.compile(r"^#define\s+SCANTAILOR_\w+_H_\s*$"),
    re.compile(r"^#endif\s*//\s*SCANTAILOR_\w+_H_\s*$"),
    re.compile(r"^using\s+namespace\s+[\w:]+\s*;\s*$"),
    re.compile(r"^namespace\b[^{]*\{\s*$"),
    re.compile(r"^\}\s*//\s*namespace\b.*$"),
    re.compile(r"^// Copyright \(C\) 2019\s+Joseph Artsimovich\b.*$"),
    re.compile(r"^// Use of this source code is governed by the GNU GPLv3 license\b.*$"),
]


def is_structural(line):
    return any(p.match(line) for p in STRUCTURAL)


def chunks_of(text):
    """Top-level chunks of one file as tuples of stripped lines."""
    result = []
    current = []
    for raw in text.splitlines():
        line = raw.rstrip()
        if is_structural(line):
            continue
        current.append(line)
        if line.startswith("}"):
            result.append(tuple(current))
            current = []
    if current:
        result.append(tuple(current))
    return result


def first_line(chunk):
    for line in chunk:
        if not line.lstrip().startswith("//"):
            return line.strip()
    return chunk[0].strip()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", required=True)
    ap.add_argument("--orig", required=True)
    ap.add_argument("--new", required=True, nargs="+")
    ap.add_argument("--allow-changed", action="append", default=[])
    ap.add_argument("--allow-line", action="append", default=[])
    args = ap.parse_args()

    orig_text = subprocess.run(
        ["git", "show", "%s:%s" % (args.base, args.orig)],
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    orig = Counter(chunks_of(orig_text))
    new = Counter()
    for path in args.new:
        with open(path, encoding="utf-8") as f:
            new.update(chunks_of(f.read()))

    print("chunks: orig=%d new=%d" % (sum(orig.values()), sum(new.values())))

    lost = orig - new
    added = new - orig

    def tolerated(chunk):
        head = first_line(chunk)
        return any(t in head for t in args.allow_changed)

    ok = True
    for chunk, n in sorted(lost.items(), key=lambda kv: kv[0]):
        if not tolerated(chunk):
            ok = False
            print("LOST  x%d: %s" % (n, first_line(chunk)))
    for chunk, n in sorted(added.items(), key=lambda kv: kv[0]):
        if not tolerated(chunk):
            ok = False
            print("ADDED x%d: %s" % (n, first_line(chunk)))

    tol_lost = Counter()
    tol_added = Counter()
    for chunk, n in lost.items():
        if tolerated(chunk):
            for line in chunk:
                tol_lost[line] += n
    for chunk, n in added.items():
        if tolerated(chunk):
            for line in chunk:
                tol_added[line] += n
    if tol_lost or tol_added:
        missing = tol_lost - tol_added
        extra = tol_added - tol_lost
        patterns = [re.compile(p) for p in args.allow_line]
        for line, n in sorted(missing.items()):
            ok = False
            print("LINE LOST  x%d: %s" % (n, line.strip()))
        for line, n in sorted(extra.items()):
            if not any(p.search(line) for p in patterns):
                ok = False
                print("LINE ADDED x%d: %s" % (n, line.strip()))

    print("RESULT: PASS" if ok else "RESULT: FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
