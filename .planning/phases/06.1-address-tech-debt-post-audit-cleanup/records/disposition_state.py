# Checks one review disposition file: frontmatter list, table rows and the open count must agree.
import re
import sys


def parse(path):
    with open(path, encoding="utf-8") as handle:
        lines = handle.read().split("\n")

    # Frontmatter sits between the first two '---' lines.
    marks = [i for i, line in enumerate(lines) if line.strip() == "---"]
    if len(marks) < 2 or marks[0] != 0:
        raise ValueError("no frontmatter in " + path)
    front = lines[marks[0] + 1 : marks[1]]
    body = lines[marks[1] + 1 :]

    entries = []  # (id, disposition) in file order
    current = None
    open_value = None
    for line in front:
        m = re.match(r"^\s*-\s+id:\s*(\S+)\s*$", line)
        if m:
            current = m.group(1)
            continue
        m = re.match(r"^\s+disposition:\s*(\S+)\s*$", line)
        if m and current is not None:
            entries.append((current, m.group(1)))
            current = None
            continue
        m = re.match(r"^open:\s*(\d+)\s*$", line)
        if m:
            open_value = int(m.group(1))
    if open_value is None:
        raise ValueError("no open: value in " + path)

    table = {}
    for line in body:
        m = re.match(r"^\|\s*([A-Z]+-\d+)\s*\|\s*[^|]*\|\s*(\S+)\s*\|", line)
        if m:
            table[m.group(1)] = m.group(2)
    return entries, table, open_value


def main():
    if len(sys.argv) != 2:
        print("usage: disposition_state.py <disposition file>", file=sys.stderr)
        return 2
    try:
        entries, table, open_value = parse(sys.argv[1])
    except (OSError, ValueError) as err:
        print("error: " + str(err), file=sys.stderr)
        return 2

    failed = False
    front = dict(entries)
    for fid, disposition in entries:
        in_table = table.get(fid)
        print(fid, disposition, in_table if in_table is not None else "MISSING")
        if in_table is None or in_table != disposition:
            failed = True
    for fid in table:
        if fid not in front:
            print(fid, "MISSING", table[fid])
            failed = True

    counted = sum(1 for _, disposition in entries if disposition == "open")
    print("open: %d counted %d" % (open_value, counted))
    if open_value != counted:
        failed = True
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
