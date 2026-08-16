#!/usr/bin/env bash
# Stage 2: the bootstrap fixed point.
#
#   v0/stage2_check.sh
#
# Stage 0 (gcc-built fakecc) compiles v0/*.c    -> fakecc-1
# fakecc-1                   compiles v0/*.c    -> fakecc-2
#
# If fakecc-1 and fakecc-2 are byte-identical, the compiler reproduces itself:
# compiling the same source with two different compilers whose only difference
# is which compiler built them yields the same output.  That is the strongest
# available evidence that fakecc compiles its own source correctly, because any
# miscompilation would have to be an exact fixed point to stay invisible.
#
# Both stages compile and link entirely with fakecc, so the comparison covers
# the object files and the linked binary.  The object comparison localizes a
# mismatch to one module; the binary comparison additionally covers the
# linker.
set -uo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
STAGE0=${FAKECC:-$ROOT/build/fakecc}
# Derived from src/, not listed here — see build_bootstrap.sh for why.
MODULES=$(cd "$ROOT/src" && LC_ALL=C ls *.c | sed 's/\.c$//' | tr '\n' ' ')
[ -n "$MODULES" ] || { echo "no sources found in $ROOT/src" >&2; exit 1; }

[ -x "$STAGE0" ] || { echo "no Stage 0 compiler at $STAGE0" >&2; exit 1; }

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

echo "=== translating src/*.c -> v0/*.c ==="
python3 "$ROOT/v0/translate.py" >/dev/null || exit 1

build_with() {
    local cc="$1" outdir="$2"
    mkdir -p "$outdir"
    # Compile all modules together in one invocation so fakecc's package-main
    # semantics expose sibling files' symbols unqualified (no per-file extern).
    local srcs=""
    for m in $MODULES; do srcs="$srcs $ROOT/v0/$m.c"; done
    "$cc" $srcs -o "$outdir/fakecc" 2>"$outdir/build.err" || {
        echo "  BUILD FAIL: $(head -1 "$outdir/build.err")" >&2
        return 1
    }
}

echo "=== stage 1: Stage 0 compiles fakecc ==="
build_with "$STAGE0" "$WORK/s1" || exit 1
echo "    -> $WORK/s1/fakecc"

echo "=== stage 2: fakecc-1 compiles fakecc ==="
build_with "$WORK/s1/fakecc" "$WORK/s2" || exit 1
echo "    -> $WORK/s2/fakecc"

echo
echo "=== comparing stage 1 and stage 2 binaries ==="
if cmp -s "$WORK/s1/fakecc" "$WORK/s2/fakecc"; then
    echo "binaries byte-identical"
else
    echo "NOT a fixed point — stage 1 and stage 2 binaries differ"
    echo "  stage 1: $(stat -c%s "$WORK/s1/fakecc") bytes"
    echo "  stage 2: $(stat -c%s "$WORK/s2/fakecc") bytes"
    exit 1
fi

echo
echo "FIXED POINT REACHED: linked binaries are byte-identical"
cp "$WORK/s1/fakecc" "$ROOT/v0/fakecc-1"
cp "$WORK/s2/fakecc" "$ROOT/v0/fakecc-2"
echo "wrote v0/fakecc-1 and v0/fakecc-2"
