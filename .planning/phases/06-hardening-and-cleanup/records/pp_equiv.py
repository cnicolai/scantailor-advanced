# Proves an edit cannot change object code: the Release preprocessor output (comments dropped) of the base revision and the working file are equal.
# Usage: python3 -I pp_equiv.py <base-rev> <file> [<file> ...]   (run from the repository root; exit 0 only if every file is EQUAL)
import json
import os
import shlex
import subprocess
import sys

COMPILE_COMMANDS = "build/macos-release/compile_commands.json"


def find_entry(entries, path):
    want = os.path.realpath(path)
    for entry in entries:
        if os.path.realpath(os.path.join(entry["directory"], entry["file"])) == want:
            return entry
    return None


def preprocessor_command(entry, path):
    # Keep the flags (defines, include paths, language level); drop the output,
    # compile-only and dependency-file options and the source path itself.
    words = shlex.split(entry["command"])
    src = os.path.realpath(os.path.join(entry["directory"], entry["file"]))
    kept = []
    skip = 0
    for i, word in enumerate(words):
        if skip:
            skip -= 1
            continue
        if word in ("-o", "-MT", "-MF", "-MQ"):
            skip = 1
            continue
        if word in ("-c", "-MD", "-MMD"):
            continue
        if os.path.realpath(os.path.join(entry["directory"], word)) == src:
            continue
        kept.append(word)
    kept += ["-E", "-P", "-x", "c++", "-", "-iquote", os.path.dirname(os.path.realpath(path))]
    return kept


def preprocess(command, directory, source):
    result = subprocess.run(command, cwd=directory, input=source, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        sys.stderr.write(result.stderr.decode("utf-8", "replace"))
        raise RuntimeError("preprocessor failed")
    lines = result.stdout.decode("utf-8", "replace").splitlines()
    return [line for line in lines if line.strip()]


def main(argv):
    if len(argv) < 3:
        sys.stderr.write("usage: pp_equiv.py <base-rev> <file> [<file> ...]\n")
        return 2
    base_rev = argv[1]
    with open(COMPILE_COMMANDS) as handle:
        entries = json.load(handle)
    all_equal = True
    for path in argv[2:]:
        entry = find_entry(entries, path)
        if entry is None:
            print("NOENTRY " + path)
            all_equal = False
            continue
        command = preprocessor_command(entry, path)
        base_source = subprocess.run(["git", "show", base_rev + ":" + path], stdout=subprocess.PIPE, check=True).stdout
        with open(path, "rb") as handle:
            work_source = handle.read()
        base_out = preprocess(command, entry["directory"], base_source)
        work_out = preprocess(command, entry["directory"], work_source)
        if base_out == work_out:
            print("EQUAL " + path + " (" + str(len(base_out)) + " preprocessed lines)")
            continue
        all_equal = False
        print("DIFFERENT " + path)
        shown = 0
        for index in range(max(len(base_out), len(work_out))):
            a = base_out[index] if index < len(base_out) else "<end>"
            b = work_out[index] if index < len(work_out) else "<end>"
            if a != b:
                print("  base: " + a[:200])
                print("  work: " + b[:200])
                shown += 1
                if shown == 3:
                    break
    return 0 if all_equal else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
