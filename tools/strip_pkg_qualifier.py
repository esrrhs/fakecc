#!/usr/bin/env python3
"""Strip fakecc package qualifiers (`runtime.`, `fmt.`, ...) for the oracle.

The oracle compiler has no package system, so `runtime.puts(...)` has to
become `puts(...)` before gcc/clang see the source.  This used to be a
`sed -E 's/\\b(runtime|fmt|io|ctype)\\.//g'` pass, which silently no-ops on
BSD sed (macOS): `\\b` is a GNU extension, so every qualified call survived
and gcc rejected the translation unit with `'runtime' undeclared` — 81 of
the difftest cases failed on macOS for that reason alone.

Python's `\\b` is portable, so do the rewrite here instead.
"""
import re
import sys

# Package qualifiers a case may prefix a runtime symbol with.  The word
# boundary keeps `myruntime.foo` (a plain struct member access) intact.
_QUALIFIER = re.compile(r"\b(runtime|fmt|io|ctype)\.")


def main():
    # Ports may contain latin-1 bytes (e.g. `"\0\xff"` as a raw 0xFF).
    raw = sys.stdin.buffer.read()
    out = _QUALIFIER.sub("", raw.decode("latin-1"))
    sys.stdout.buffer.write(out.encode("latin-1"))


if __name__ == "__main__":
    main()
