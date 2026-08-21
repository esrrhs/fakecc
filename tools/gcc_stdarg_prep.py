#!/usr/bin/env python3
"""Prepare stripped fakecc sources so gcc can compile them as the oracle.

Drops in-source SysV va_list typedefs in favour of <stdarg.h>, and drops
K&R empty local redeclarations of functions already defined with a typed
prototype (gcc 13 rejects `float fx()` after `float fx(float)`).
"""
import re
import sys


def strip_empty_redecls(src):
    """gcc rejects `float fx();` after `float fx(float)` (default promotion).

    Torture still uses the empty local list; drop those names on the gcc side
    once a typed prototype/definition exists.  The test file is left alone.
    """
    defined = []
    for m in re.finditer(
        r"^(?:static\s+)?[\w\s\*]+\b(\w+)\s*\(([^)]*)\)\s*\{",
        src,
        re.M,
    ):
        name, params = m.group(1), m.group(2).strip()
        if params and params != "void":
            defined.append(name)
    for name in defined:
        src = re.sub(r"\b" + re.escape(name) + r"\s*\(\s*\)\s*,\s*", "", src)
        src = re.sub(r",\s*\b" + re.escape(name) + r"\s*\(\s*\)", "", src)
    return src


def forward_file_scope_funcs(src):
    """Emit prototypes for functions defined in the TU so gcc does not treat
    later-defined `f()` as an implicit int (which we must not accept: that is
    how header-free getenv() became `int getenv()` and SIGSEGV'd)."""
    decls = []
    for m in re.finditer(
        r"^(static\s+)?([\w\s\*]+)\b(\w+)\s*\(([^)]*)\)\s*\{",
        src,
        re.M,
    ):
        static, ret, name, params = m.group(1) or "", m.group(2).strip(), m.group(3), m.group(4)
        if name == "main":
            continue
        decls.append("%s%s %s(%s);" % (static, ret, name, params))
    if not decls:
        return src
    return "\n".join(decls) + "\n" + src


def prep(src):
    src = strip_empty_redecls(src)
    src = re.sub(
        r"typedef\s+struct\s*\{[^{}]*\}\s*__va_list_tag\s*;",
        "",
        src,
        flags=re.S,
    )
    src = re.sub(
        r"typedef\s+struct\s*\{[^{}]*\}\s*va_list\s*\[\s*1\s*\]\s*;",
        "",
        src,
        flags=re.S,
    )
    src = re.sub(
        r"typedef\s+__va_list_tag\s+__builtin_va_list\s*\[[^\]]+\]\s*;",
        "",
        src,
    )
    src = re.sub(r"typedef\s+__builtin_va_list\s+va_list\s*;", "", src)
    src = re.sub(r"typedef\s+__builtin_va_list\s+__gnuc_va_list\s*;", "", src)
    src = re.sub(r"typedef\s+__gnuc_va_list\s+va_list\s*;", "", src)
    src = forward_file_scope_funcs(src)
    return src


def main():
    sys.stdout.write(prep(sys.stdin.read()))


if __name__ == "__main__":
    main()
