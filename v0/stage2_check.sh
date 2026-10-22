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
if [ ! -x "$STAGE0" ] && [ -x "${STAGE0}.exe" ]; then
    STAGE0="${STAGE0}.exe"
fi
# Windows executes a PE named fakecc.exe.  The fixed-point copies stay
# extensionless (v0/fakecc-1) so the suite scripts are the same on every host;
# MSYS runs that image, and a sibling .exe is kept for CreateProcess.
OUT_NAME=fakecc
case "$(uname -s 2>/dev/null)" in
    MSYS*|MINGW*|CYGWIN*) OUT_NAME=fakecc.exe ;;
esac
# Derived from src/, not listed here — see build_bootstrap.sh for why.
MODULES=$(cd "$ROOT/src" && LC_ALL=C ls *.c | sed 's/\.c$//' | tr '\n' ' ')
[ -n "$MODULES" ] || { echo "no sources found in $ROOT/src" >&2; exit 1; }

[ -x "$STAGE0" ] || { echo "no Stage 0 compiler at $STAGE0" >&2; exit 1; }

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

# `stat -c%s` is GNU-only; BSD/macOS spells the same query `stat -f%z`.
file_size() {
    stat -f%z "$1" 2>/dev/null || stat -c%s "$1"
}

echo "=== translating src/*.c -> v0/*.c ==="
python3 "$ROOT/v0/translate.py" >/dev/null || exit 1

build_with() {
    local cc="$1" outdir="$2"
    mkdir -p "$outdir"
    # Compile all modules together in one invocation so fakecc's package-main
    # semantics expose sibling files' symbols unqualified (no per-file extern).
    local srcs=""
    for m in $MODULES; do srcs="$srcs $ROOT/v0/$m.c"; done
    "$cc" $srcs -o "$outdir/$OUT_NAME" 2>"$outdir/build.err" || {
        echo "  BUILD FAIL: $(head -1 "$outdir/build.err")" >&2
        return 1
    }
}

echo "=== stage 1: Stage 0 compiles fakecc ==="
build_with "$STAGE0" "$WORK/s1" || exit 1
echo "    -> $WORK/s1/$OUT_NAME"
# Persist stage1 immediately so the workflow uploads the exact
# stage1 binary even if stage2 crashes the compiler before any
# v0/fakecc-1 is written.
cp "$WORK/s1/$OUT_NAME" "$ROOT/v0/fakecc-1"
if [ "$OUT_NAME" != "fakecc" ]; then
    cp "$WORK/s1/$OUT_NAME" "$ROOT/v0/fakecc-1.exe"
fi

echo "=== stage 2: fakecc-1 compiles fakecc ==="
if ! build_with "$WORK/s1/$OUT_NAME" "$WORK/s2"; then
    echo "stage2 compiler failed; v0/fakecc-1 retained for inspection" >&2
    exit 1
fi
echo "    -> $WORK/s2/$OUT_NAME"

echo
echo "=== comparing stage 1 and stage 2 binaries ==="
if cmp -s "$WORK/s1/$OUT_NAME" "$WORK/s2/$OUT_NAME"; then
    echo "binaries byte-identical"
else
    echo "NOT a fixed point — stage 1 and stage 2 binaries differ"
    echo "  stage 1: $(file_size "$WORK/s1/$OUT_NAME") bytes"
    echo "  stage 2: $(file_size "$WORK/s2/$OUT_NAME") bytes"
    echo "  first differing bytes (cmp -l):"
    cmp -l "$WORK/s1/$OUT_NAME" "$WORK/s2/$OUT_NAME" | head -20
    cp "$WORK/s1/$OUT_NAME" "$ROOT/v0/fakecc-1"
    cp "$WORK/s2/$OUT_NAME" "$ROOT/v0/fakecc-2"
    if [ "$OUT_NAME" != "fakecc" ]; then
        cp "$WORK/s1/$OUT_NAME" "$ROOT/v0/fakecc-1.exe"
        cp "$WORK/s2/$OUT_NAME" "$ROOT/v0/fakecc-2.exe"
    fi
    echo "wrote v0/fakecc-1 and v0/fakecc-2 (mismatch)"
    exit 1
fi

echo
echo "FIXED POINT REACHED: linked binaries are byte-identical"
cp "$WORK/s1/$OUT_NAME" "$ROOT/v0/fakecc-1"
cp "$WORK/s2/$OUT_NAME" "$ROOT/v0/fakecc-2"
if [ "$OUT_NAME" != "fakecc" ]; then
    cp "$WORK/s1/$OUT_NAME" "$ROOT/v0/fakecc-1.exe"
    cp "$WORK/s2/$OUT_NAME" "$ROOT/v0/fakecc-2.exe"
fi
echo "wrote v0/fakecc-1 and v0/fakecc-2"
