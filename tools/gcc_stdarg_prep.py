#!/usr/bin/env python3
"""Rewrite fakecc SysV va_list typedefs so gcc can use <stdarg.h>.

Torture ports spell the AMD64 va_list layout in-source because fakecc
predeclares it.  gcc's va_arg only accepts the compiler's own va_list, and
the local typedef makes a header-free translation fail, after which the
fallback #include <stdio.h> clashes with `fprintf(void*, ...)`.
"""
import re
import sys


def prep(src):
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
    return src


def main():
    sys.stdout.write(prep(sys.stdin.read()))


if __name__ == "__main__":
    main()
