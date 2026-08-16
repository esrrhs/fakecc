#!/usr/bin/env bash
# Stage 1: build fakecc with fakecc.
#
#   v0/build_bootstrap.sh [-n] [-g]
#     -n  skip translation, reuse the v0/*.c already on disk
#     -g  link with gcc instead of fakecc
#
# Runs v0/translate.py to turn src/*.c into the fakecc dialect, compiles each
# translated module with the Stage 0 compiler, and links the result into
# v0/bootstrap_fakecc.
#
# Both the compile and the link go through fakecc, so no external toolchain
# takes part in producing the binary.  gcc still runs during translation (as
# the preprocessor for src/*.c) — the fakecc dialect has no preprocessor, so
# that is a one-time cost of the mechanical translation, not of the build.
#
# -g swaps in gcc for the link only.  That is what tools/bisect_module.sh uses
# to mix fakecc-built and gcc-built modules in one binary.
set -uo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
OUT="$ROOT/v0"
FAKECC=${FAKECC:-$ROOT/build/fakecc}
# Derived from src/, not listed here: translate.py globs src/*.c, so a
# hardcoded list silently drops a newly added module from the bootstrap while
# the Stage 0 build picks it up.  LC_ALL=C fixes the order across locales,
# which keeps the linked binary byte-reproducible.
MODULES=$(cd "$ROOT/src" && LC_ALL=C ls *.c | sed 's/\.c$//' | tr '\n' ' ')
[ -n "$MODULES" ] || { echo "no sources found in $ROOT/src" >&2; exit 1; }

TRANSLATE=1
LINK_WITH_GCC=0
while getopts "ng" opt; do
    case "$opt" in
        n) TRANSLATE=0 ;;
        g) LINK_WITH_GCC=1 ;;
        *) echo "usage: $0 [-n] [-g]" >&2; exit 2 ;;
    esac
done

[ -x "$FAKECC" ] || { echo "no Stage 0 compiler at $FAKECC" >&2; exit 1; }

if [ "$TRANSLATE" = "1" ]; then
    echo "=== translating src/*.c -> v0/*.c ==="
    python3 "$OUT/translate.py" || exit 1
fi

# Compile all modules together in one invocation.  fakecc's package-main
# semantics make sibling files' symbols visible unqualified only when they
# are compiled together; `-c` on a single file sees none of its siblings and
# would need per-file `extern` declarations.  Compiling as a unit avoids that.
# ir.c is listed first: it owns all ir.h type definitions, and same-package
# sibling types are visible only to files parsed later.
echo "=== compiling and linking with $FAKECC ==="
srcs="$OUT/ir.c"
for m in $MODULES; do
    [ "$m" = "ir" ] && continue
    srcs="$srcs $OUT/$m.c"
done
if [ "$LINK_WITH_GCC" = "1" ]; then
    # gcc cannot link fakecc's .o directly; build fakecc's objects then link.
    # Fall back to per-file compile here (gcc link path is a debug escape hatch).
    ok=0; fail=0
    for m in $MODULES; do
        if "$FAKECC" "$OUT/$m.c" -c -o "$OUT/$m.o" 2>"$OUT/$m.cc.err"; then
            ok=$((ok + 1))
        else
            fail=$((fail + 1))
        fi
    done
    [ "$fail" = "0" ] || { echo "COMPILE FAIL"; exit 1; }
    objs=""; for m in $MODULES; do objs="$objs $OUT/$m.o"; done
    gcc $objs -o "$OUT/bootstrap_fakecc" -lm 2>"$OUT/link.err" || {
        echo "LINK FAILED"; cat "$OUT/link.err"; exit 1; }
else
    "$FAKECC" $srcs -o "$OUT/bootstrap_fakecc" 2>"$OUT/link.err" || {
        echo "BUILD FAILED"; cat "$OUT/link.err"; exit 1; }
fi
echo "=== built $OUT/bootstrap_fakecc ==="
ls -l "$OUT/bootstrap_fakecc"
