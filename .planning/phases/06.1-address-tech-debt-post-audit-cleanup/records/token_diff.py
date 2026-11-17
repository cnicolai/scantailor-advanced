# Token-level comparison of two gate command lists: pairs changed lines by output file and reports moved tokens.
# Usage: python3 -I token_diff.py <ref-cmds.txt> <new-cmds.txt>   (exit 0 after a report, 2 on bad arguments or files)
import sys
from collections import Counter

AR_MODES = ("qc", "cr", "crs")


def line_key(line):
    tokens = line.split()
    if "-o" in tokens:
        index = tokens.index("-o")
        if index + 1 < len(tokens):
            return tokens[index + 1]
    for index, token in enumerate(tokens[:-1]):
        # The archive line is "... ar qc <archive> <members>"; the archive path follows the mode argument.
        if token in AR_MODES:
            return tokens[index + 1]
    return line


def read_lines(path):
    with open(path, encoding="utf-8", errors="replace") as handle:
        return [line.rstrip("\n") for line in handle if line.strip()]


def pair_by_key(lines):
    keyed = {}
    for line in lines:
        keyed.setdefault(line_key(line), []).append(line)
    return keyed


def report(ref_lines, new_lines):
    # Lines present in both lists, counted as multisets, are unchanged and ignored.
    ref_counter = Counter(ref_lines)
    new_counter = Counter(new_lines)
    removed = list((ref_counter - new_counter).elements())
    added = list((new_counter - ref_counter).elements())

    removed_by_key = pair_by_key(removed)
    added_by_key = pair_by_key(added)

    changed = 0
    ref_only = 0
    new_only = 0
    removed_tokens = Counter()
    added_tokens = Counter()
    out = []

    for key in sorted(set(removed_by_key) | set(added_by_key)):
        old_group = removed_by_key.get(key, [])
        new_group = added_by_key.get(key, [])
        for position in range(max(len(old_group), len(new_group))):
            if position >= len(new_group):
                ref_only += 1
                out.append("ONLY-REF " + key)
            elif position >= len(old_group):
                new_only += 1
                out.append("ONLY-NEW " + key)
            else:
                changed += 1
                old_tokens = Counter(old_group[position].split())
                new_tokens = Counter(new_group[position].split())
                gone = old_tokens - new_tokens
                came = new_tokens - old_tokens
                out.append("CHANGED " + key)
                if not gone and not came:
                    out.append("  reordered")
                for token in sorted(gone.elements()):
                    out.append("  - " + token)
                for token in sorted(came.elements()):
                    out.append("  + " + token)
                removed_tokens.update(gone)
                added_tokens.update(came)

    out.append("summary: changed=%d ref-only=%d new-only=%d" % (changed, ref_only, new_only))
    for label, tokens in (("removed", removed_tokens), ("added", added_tokens)):
        if not tokens:
            out.append("%s tokens: none" % label)
            continue
        out.append("%s tokens:" % label)
        for token, count in sorted(tokens.items()):
            out.append("  %s x%d" % (token, count))
    return out


def main(argv):
    if len(argv) != 3:
        sys.stderr.write("usage: token_diff.py <ref-cmds.txt> <new-cmds.txt>\n")
        return 2
    try:
        ref_lines = read_lines(argv[1])
        new_lines = read_lines(argv[2])
    except OSError as error:
        sys.stderr.write("token_diff.py: %s\n" % error)
        return 2
    sys.stdout.write("\n".join(report(ref_lines, new_lines)) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
