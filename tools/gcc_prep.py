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


def main():
    if len(sys.argv) != 3:
        sys.stderr.write("usage: gcc_prep.py OUT.body.c OUT.prep.c\n")
        return 2
    # Ports may contain latin-1 bytes (e.g. `"\0\xff"` as a raw 0xFF), so both
    # modules decode with latin-1 and write it back unchanged.
    raw = sys.stdin.buffer.read().decode("latin-1")
    body = strip_pkg_qualifier.strip(raw)
    prep = gcc_stdarg_prep.prep(body)
    with open(sys.argv[1], "wb") as fh:
        fh.write(body.encode("latin-1"))
    with open(sys.argv[2], "wb") as fh:
        fh.write(prep.encode("latin-1"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
