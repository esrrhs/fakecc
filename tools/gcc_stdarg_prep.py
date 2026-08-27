#!/usr/bin/env python3
"""Prepare stripped fakecc sources so gcc can compile them as the oracle.

Drops in-source SysV va_list typedefs in favour of <stdarg.h>, maps
__builtin_c23_va_start to __builtin_va_start (gcc 13 has no C23 builtin),
drops K&R empty local redeclarations, and avoids treating
`void __attribute__((noinline))\\nfoo(...)` as implicit-int K&R (which
emitted a conflicting `int foo()` prototype).
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


def _prev_nonempty_line(src, pos):
    i = pos
    while i > 0 and src[i - 1] != "\n":
        i -= 1
    j = i - 1
    while j >= 0:
        prev_nl = src.rfind("\n", 0, j)
        line = src[prev_nl + 1 : j].strip() if prev_nl >= 0 else src[:j].strip()
        if line:
            return line
        if prev_nl < 0:
            break
        j = prev_nl
    return ""


def _is_header_continuation(line):
    """`void __attribute__((noinline))\\nbar (...)` is not K&R `int bar()`."""
    if not line:
        return False
    if line.endswith(";") or line.endswith("{") or line.endswith("}"):
        return False
    if line.startswith("#") or line.startswith("//"):
        return False
    return True


_STMT_KW = {
    "if", "for", "while", "switch", "return", "sizeof", "do", "else", "case",
}


def _is_inline_ret(ret):
    toks = ret.replace("\n", " ").split()
    return "inline" in toks or "__inline" in toks or "__inline__" in toks


def forward_file_scope_funcs(src):
    """Emit prototypes just before `main` (or the first non-inline function).

    Faithful stdlib preambles define `static inline` helpers first.  Inserting
    later functions' prototypes there (before `typedef host_addr_t`) made gcc
    treat `host_addr_t f()` as `int f()` and fail the gcc translation.  `main` is after
    those typedefs.

    K&R `foo (){}` has no return type; gcc 13 still compiles it (implicit int)
    but warns 隐式声明, and the harness then falls back to stdio.h which
    clashes with `fprintf(void*, ...)`.  Forward those as `int foo();`.
    """
    decls = []
    seen = set()
    first_non_inline = None

    def note(pos):
        nonlocal first_non_inline
        if first_non_inline is None or pos < first_non_inline:
            first_non_inline = pos

    for m in re.finditer(
        r"^(static\s+)?([\w\s\*]+)\b(\w+)\s*\(([^)]*)\)\s*\{",
        src,
        re.M,
    ):
        static, ret, name, params = m.group(1) or "", m.group(2).strip(), m.group(3), m.group(4)
        if name in _STMT_KW or ";" in params:
            continue
        if not ret or ret in _STMT_KW:
            continue
        if _is_inline_ret(ret):
            seen.add(name)
            continue
        note(m.start())
        if name == "main" or name in seen:
            continue
        seen.add(name)
        # K&R `int g(x, y)` has identifier-only params; emitting `int g(x, y);`
        # is a GCC 16 error (-Wdeclaration-missing-parameter-type).  Use an
        # unprototyped declaration instead.
        raw = params.strip()
        if raw and raw != "void" and not re.search(
            r"\b(?:void|int|char|short|long|float|double|signed|unsigned|"
            r"struct|union|enum|_Bool|const|volatile|restrict)\b|\*",
            raw,
        ):
            decls.append("%s%s %s();" % (static, ret, name))
        else:
            decls.append("%s%s %s(%s);" % (static, ret, name, params))

    for m in re.finditer(r"^(static\s+)?(\w+)\s*\(([^)]*)\)\s*\{", src, re.M):
        static, name, params = m.group(1) or "", m.group(2), m.group(3)
        if name in _STMT_KW or ";" in params:
            continue
        prev = _prev_nonempty_line(src, m.start())
        if _is_header_continuation(prev):
            continue
        note(m.start())
        if name == "main" or name in seen:
            continue
        seen.add(name)
        decls.append("%sint %s();" % (static, name))

    if not decls:
        return src
    m_main = re.search(r"^int\s+main\s*\(", src, re.M)
    pos = m_main.start() if m_main else (first_non_inline if first_non_inline is not None else 0)
    return src[:pos] + "\n".join(decls) + "\n" + src[pos:]


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
    src = re.sub(r"typedef\s+[^;]*\bfpos_t\s*;", "", src)
    src = re.sub(r"typedef\s+struct\s+_?IO_FILE\s+FILE\s*;", "", src)
    # fakecc accepts the C23 builtin; gcc 13 does not.  Same ABI as va_start.
    src = re.sub(r"\b__builtin_c23_va_start\b", "__builtin_va_start", src)
    src = forward_file_scope_funcs(src)
    return src


def main():
    # Ports may contain latin-1 bytes (e.g. `"\0\xff"` as a raw 0xFF).
    # Decoding as UTF-8 aborted the gcc-side rewrite and failed the case.
    raw = sys.stdin.buffer.read()
    sys.stdout.buffer.write(prep(raw.decode("latin-1")).encode("latin-1"))


if __name__ == "__main__":
    main()
