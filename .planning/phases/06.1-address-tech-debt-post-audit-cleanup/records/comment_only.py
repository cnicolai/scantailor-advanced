# Proves that an edit changed only comments: compares the comment-free token streams of <base-rev>:<file> and the working file.
#
# Usage: python3 -I comment_only.py [--strings] <base-rev> <file> ...
#   C, C++, Objective-C++ (.c .cc .cpp .h .hpp .mm): // and /* */ comments are removed; string literals (with escapes),
#     character literals and raw strings stay as single tokens. With --strings their contents are replaced by one
#     placeholder, so a changed message text does not count as a difference.
#   CMake (CMakeLists.txt, *.cmake): # line comments and #[[ ]] bracket comments are removed outside quoted arguments.
# Prints "EQUAL <file>" or "DIFFERENT <file>" (with the first differing tokens); exit 0 only if every file is EQUAL.

import os
import subprocess
import sys

CPP_SUFFIXES = (".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hh", ".mm", ".m")
STRING_PLACEHOLDER = '"<string>"'
CHAR_PLACEHOLDER = "'<char>'"


def is_cmake(path):
    base = os.path.basename(path)
    return base == "CMakeLists.txt" or base.endswith(".cmake")


def flush(buf, out):
    if buf:
        out.extend("".join(buf).split())
        del buf[:]


def keep_literal(text, placeholder, strings_mode):
    if strings_mode:
        return placeholder
    # one token per literal: whitespace inside it must not split it
    return "".join("\x00%02x" % ord(c) if c.isspace() else c for c in text)


def tokens_cpp(text, strings_mode):
    out = []
    buf = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if c == "/" and nxt == "/":
            # line comment; a trailing backslash continues it onto the next line
            j = i + 2
            while j < n:
                if text[j] == "\n" and text[j - 1] != "\\":
                    break
                j += 1
            buf.append(" ")
            i = j
            continue
        if c == "/" and nxt == "*":
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            buf.append(" ")
            i = j
            continue
        if c == '"':
            if i > 0 and text[i - 1] == "R":
                # raw string R"delim( ... )delim"
                k = text.find("(", i + 1)
                if k > 0 and "\n" not in text[i + 1:k] and len(text[i + 1:k]) <= 16:
                    delim = text[i + 1:k]
                    end = text.find(")" + delim + '"', k + 1)
                    if end >= 0:
                        end += len(delim) + 2
                        flush(buf, out)
                        out.append(keep_literal(text[i:end], STRING_PLACEHOLDER, strings_mode))
                        i = end
                        continue
            j = i + 1
            while j < n and text[j] != "\n":
                if text[j] == "\\":
                    j += 2
                    continue
                if text[j] == '"':
                    break
                j += 1
            if j < n and text[j] == '"':
                flush(buf, out)
                out.append(keep_literal(text[i:j + 1], STRING_PLACEHOLDER, strings_mode))
                i = j + 1
                continue
            buf.append(c)
            i += 1
            continue
        if c == "'":
            # digit separator (1'000) or character literal
            k = i
            while k > 0 and (text[k - 1].isalnum() or text[k - 1] in "_.'"):
                k -= 1
            if k < i and text[k].isdigit():
                buf.append(c)
                i += 1
                continue
            j = i + 1
            while j < n and text[j] != "\n":
                if text[j] == "\\":
                    j += 2
                    continue
                if text[j] == "'":
                    break
                j += 1
            if j < n and text[j] == "'":
                flush(buf, out)
                out.append(keep_literal(text[i:j + 1], CHAR_PLACEHOLDER, strings_mode))
                i = j + 1
                continue
            buf.append(c)
            i += 1
            continue
        buf.append(c)
        i += 1
    flush(buf, out)
    return out


def tokens_cmake(text, strings_mode):
    out = []
    buf = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if c == "\\":
            buf.append(text[i:i + 2])
            i += 2
            continue
        if c == "#":
            # bracket comment #[=*[ ... ]=*] or line comment
            j = i + 1
            if j < n and text[j] == "[":
                k = j + 1
                while k < n and text[k] == "=":
                    k += 1
                if k < n and text[k] == "[":
                    close = "]" + "=" * (k - j - 1) + "]"
                    end = text.find(close, k + 1)
                    end = n if end < 0 else end + len(close)
                    buf.append(" ")
                    i = end
                    continue
            end = text.find("\n", i)
            buf.append(" ")
            i = n if end < 0 else end
            continue
        if c == "[":
            # bracket argument [=*[ ... ]=*]: kept as one token
            k = i + 1
            while k < n and text[k] == "=":
                k += 1
            if k < n and text[k] == "[":
                close = "]" + "=" * (k - i - 1) + "]"
                end = text.find(close, k + 1)
                end = n if end < 0 else end + len(close)
                flush(buf, out)
                out.append(keep_literal(text[i:end], STRING_PLACEHOLDER, strings_mode))
                i = end
                continue
        if c == '"':
            j = i + 1
            while j < n:
                if text[j] == "\\":
                    j += 2
                    continue
                if text[j] == '"':
                    break
                j += 1
            end = min(j + 1, n)
            flush(buf, out)
            out.append(keep_literal(text[i:end], STRING_PLACEHOLDER, strings_mode))
            i = end
            continue
        buf.append(c)
        i += 1
    flush(buf, out)
    return out


def tokens_for(path, text, strings_mode):
    if is_cmake(path):
        return tokens_cmake(text, strings_mode)
    if path.endswith(CPP_SUFFIXES):
        return tokens_cpp(text, strings_mode)
    raise ValueError("unsupported file type: " + path)


def read_base(rev, path):
    res = subprocess.run(["git", "show", "%s:%s" % (rev, path)], capture_output=True)
    if res.returncode != 0:
        raise ValueError("git show %s:%s failed: %s" % (rev, path, res.stderr.decode("utf-8", "replace").strip()))
    return res.stdout.decode("utf-8")


def compare(rev, path, strings_mode):
    try:
        base = tokens_for(path, read_base(rev, path), strings_mode)
        with open(path, encoding="utf-8") as handle:
            now = tokens_for(path, handle.read(), strings_mode)
    except (ValueError, OSError) as err:
        print("DIFFERENT %s (%s)" % (path, err))
        return False
    if base == now:
        print("EQUAL %s" % path)
        return True
    k = 0
    while k < len(base) and k < len(now) and base[k] == now[k]:
        k += 1
    print("DIFFERENT %s" % path)
    print("  first difference at token %d (base has %d tokens, working file %d)" % (k, len(base), len(now)))
    print("  base: %s" % " ".join(base[max(0, k - 2):k + 4]))
    print("  now:  %s" % " ".join(now[max(0, k - 2):k + 4]))
    return False


def main(argv):
    args = argv[1:]
    strings_mode = False
    if args and args[0] == "--strings":
        strings_mode = True
        args = args[1:]
    if len(args) < 2:
        print("usage: comment_only.py [--strings] <base-rev> <file> ...", file=sys.stderr)
        return 2
    rev = args[0]
    ok = True
    for path in args[1:]:
        ok = compare(rev, path, strings_mode) and ok
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
