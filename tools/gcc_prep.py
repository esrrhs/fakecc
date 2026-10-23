#!/usr/bin/env python3
"""Both oracle-side source rewrites, in one process.

   sed -E 's/^package ...;//; s/^import ...;//' case.c \
       | python3 tools/gcc_prep.py OUT.body.c OUT.prep.c

tools/difftest.sh used to run these as two separate python processes per case:
strip_pkg_qualifier.py, then gcc_stdarg_prep.py.  A bare `python3` start-up
(interpreter + `import re`) costs ~165 ms here, so at ~2.6k cases that was
~440 s of pure process spawning per difftest leg -- more than the fakecc
compiles the suite exists to perform.  Doing both passes in one process halves
it and leaves the output byte-identical.

The two source modules remain runnable on their own; this is only a driver.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import strip_pkg_qualifier
import gcc_stdarg_prep


def _ident_at(src, i):
    return i < len(src) and (src[i].isalnum() or src[i] == "_")


def _widen_fmt(s):
    """Inside a string, a single `l` length on an integer conversion is
    `long`.  After the keyword rewrite that is 64-bit, so `%ld` must be
    `%lld`.  `%lf`, `%ls`, and an existing `%lld` stay put."""
    out = []
    i = 0
    n = len(s)
    while i < n:
        if s[i] != "%" or (i + 1 < n and s[i + 1] == "%"):
            if s[i] == "%" and i + 1 < n and s[i + 1] == "%":
                out.append("%%")
                i += 2
                continue
            out.append(s[i])
            i += 1
            continue
        j = i + 1
        while j < n and s[j] in "-+ #0'":
            j += 1
        while j < n and (s[j].isdigit() or s[j] == "*"):
            j += 1
        if j < n and s[j] == ".":
            j += 1
            while j < n and (s[j].isdigit() or s[j] == "*"):
                j += 1
        if j < n and s[j] == "l" and not (j + 1 < n and s[j + 1] == "l"):
            if j + 1 < n and s[j + 1] in "diuoxXn":
                out.append(s[i:j])
                out.append("ll")
                out.append(s[j + 1])
                i = j + 2
                continue
        if j < n:
            j += 1
        out.append(s[i:j])
        i = j
    return "".join(out)
    while i < len(src) and src[i] in " \t\n\r":
        i += 1
    return i


def _skip_ws(src, i):
    while i < len(src) and src[i] in " \t\n\r":
        i += 1
    return i


def widen_long(src):
    """Rewrite LP64 `long` so a MinGW oracle matches fakecc.

    fakecc's Windows target keeps the 64-bit `long` the shared suite was
    written against.  MinGW's `long` is 32 bits.  `long double` and an
    existing `long long` stay put.  Integer suffixes `l`/`L` become `ll`
    so `3000000000L` is not a 32-bit literal; a floating suffix (`1.0L`)
    stays, because long double is already 64-bit under -mlong-double-64.
    Strings and comments are copied through unchanged.
    """
    out = []
    i = 0
    n = len(src)
    while i < n:
        c = src[i]
        if c == "/" and i + 1 < n and src[i + 1] == "/":
            j = src.find("\n", i)
            if j < 0:
                j = n
            out.append(src[i:j])
            i = j
            continue
        if c == "/" and i + 1 < n and src[i + 1] == "*":
            j = src.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append(src[i:j])
            i = j
            continue
        if c == '"':
            j = i + 1
            while j < n:
                if src[j] == "\\":
                    j += 2
                    continue
                if src[j] == c:
                    j += 1
                    break
                j += 1
            out.append('"')
            out.append(_widen_fmt(src[i + 1:j - 1]))
            out.append('"')
            i = j
            continue
        if c == "'":
            j = i + 1
            while j < n:
                if src[j] == "\\":
                    j += 2
                    continue
                if src[j] == c:
                    j += 1
                    break
                j += 1
            out.append(src[i:j])
            i = j
            continue
        if c.isdigit():
            # Leading-dot float: ".5L" -- the dot was already emitted as a
            # character, so the digit path must not treat "5L" as an integer
            # with an L suffix (that becomes ".5ll", which MinGW rejects).
            if out and out[-1] == ".":
                j = i + 1
                while j < n and src[j].isdigit():
                    j += 1
                if j < n and src[j] in "eE":
                    j += 1
                    if j < n and src[j] in "+-":
                        j += 1
                    while j < n and src[j].isdigit():
                        j += 1
                while j < n and src[j] in "fFlL":
                    j += 1
                out.append(src[i:j])
                i = j
                continue
            j = i + 1
            hexnum = False
            if src[i] == "0" and j < n and src[j] in "xX":
                hexnum = True
                j += 1
                while j < n and src[j] in "0123456789abcdefABCDEF":
                    j += 1
            else:
                while j < n and src[j].isdigit():
                    j += 1
            if j < n and (
                src[j] == "."
                or (hexnum and src[j] in "pP")
                or (not hexnum and src[j] in "eE")
            ):
                if src[j] == ".":
                    j += 1
                    digits = "0123456789abcdefABCDEF" if hexnum else "0123456789"
                    while j < n and src[j] in digits:
                        j += 1
                if j < n and ((hexnum and src[j] in "pP") or (not hexnum and src[j] in "eE")):
                    j += 1
                    if j < n and src[j] in "+-":
                        j += 1
                    while j < n and src[j].isdigit():
                        j += 1
                while j < n and src[j] in "fFlL":
                    j += 1
                out.append(src[i:j])
                i = j
                continue
            lit = src[i:j]
            if j < n and src[j] in "uU":
                lit += src[j]
                j += 1
            if j < n and src[j] in "lL":
                first = src[j]
                j += 1
                if j < n and src[j] in "lL":
                    lit += first + src[j]
                    j += 1
                else:
                    lit += "ll"
                if j < n and src[j] in "uU":
                    lit += src[j]
                    j += 1
            out.append(lit)
            i = j
            continue
        if src.startswith("long", i) and not _ident_at(src, i + 4) and (i == 0 or not _ident_at(src, i - 1)):
            nxt = _skip_ws(src, i + 4)
            if src.startswith("double", nxt) and not _ident_at(src, nxt + 6):
                out.append(src[i:nxt + 6])
                i = nxt + 6
                continue
            if src.startswith("long", nxt) and not _ident_at(src, nxt + 4):
                out.append(src[i:nxt + 4])
                i = nxt + 4
                continue
            out.append("long long")
            i += 4
            continue
        out.append(c)
        i += 1
    return "".join(out)



def remap_long_libcalls(src):
    """Fix MinGW builtin width after widen_long for abs/uabs torture ports.

    `-fno-builtin-abs` keeps `labs` as a builtin while `abs` is a real call.
    After LP64 widening the tripwire `labs` has a long-long signature, but
    MinGW's builtin `labs` still takes 32-bit `long`, so the oracle calls the
    tripwire and aborts (exit 3) while fakecc folds and exits 0.  Remap call
    sites only; leave the definition as a tripwire.  Same idea for `ulabs`
    under `-fno-builtin-uabs`.  abs-2 defines its own labs/llabs and must not
    be rewritten -- gate on the flags line.
    """
    import re
    flags_m = re.search(r"(?m)^//\s*flags:\s*(.*)$", src)
    flags = flags_m.group(1) if flags_m else ""
    out = src

    def protect_and_remap(text, name, builtin):
        pat_def = r"(\b(?:unsigned\s+)?long(?:\s+long)?\s+)(" + name + r")(\s*\()"
        text = re.sub(pat_def, r"\1__FAKECC_DEF_" + name + r"\3", text)
        text = re.sub(r"\b" + name + r"\s*\(", builtin + "(", text)
        text = text.replace("__FAKECC_DEF_" + name, name)
        return text

    if "-fno-builtin-abs" in flags:
        out = protect_and_remap(out, "labs", "__builtin_llabs")
    if "-fno-builtin-uabs" in flags:
        out = protect_and_remap(out, "ulabs", "__builtin_ulabs")
    return out


def _drop_host_idents(src):
    """MinGW's stdin/stdout/stderr are macros, not linkable objects."""
    import re
    src = re.sub(
        r"(?m)^[ \t]*extern[ \t]+(?:FILE[ \t]*\*|void[ \t]*\*)[ \t]*(?:stdin|stdout|stderr)[ \t]*;[ \t]*\n?",
        "",
        src,
    )
    return src


def main():
    if len(sys.argv) != 3:
        sys.stderr.write("usage: gcc_prep.py OUT.body.c OUT.prep.c\n")
        return 2
    # Ports may contain latin-1 bytes (e.g. `"\0\xff"` as a raw 0xFF), so both
    # modules decode with latin-1 and write it back unchanged.
    raw = sys.stdin.buffer.read().decode("latin-1")
    body = strip_pkg_qualifier.strip(raw)
    prep = gcc_stdarg_prep.prep(body)
    if os.environ.get("FAKECC_ORACLE_LP64") == "1":
        prep = widen_long(prep)
        body = widen_long(body)
        prep = remap_long_libcalls(prep)
        body = remap_long_libcalls(body)
        prep = _drop_host_idents(prep)
        body = _drop_host_idents(body)
    with open(sys.argv[1], "wb") as fh:
        fh.write(body.encode("latin-1"))
    with open(sys.argv[2], "wb") as fh:
        fh.write(prep.encode("latin-1"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
