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
    for m in $MODULES; do
        "$cc" "$ROOT/v0/$m.c" -c -o "$outdir/$m.o" 2>"$outdir/$m.err" || {
            echo "  FAIL $m: $(head -1 "$outdir/$m.err")" >&2
            return 1
        }
    done
    "$cc" $(for m in $MODULES; do echo "$outdir/$m.o"; done) \
          -o "$outdir/fakecc" 2>"$outdir/link.err" || {
        echo "  LINK FAIL: $(head -1 "$outdir/link.err")" >&2
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
echo "=== comparing stage 1 and stage 2 objects ==="
same=0
diff_mods=""
for m in $MODULES; do
    if cmp -s "$WORK/s1/$m.o" "$WORK/s2/$m.o"; then
        same=$((same + 1))
    else
        diff_mods="$diff_mods $m"
        printf '  %-12s DIFFERS (%s vs %s bytes)\n' "$m" \
               "$(stat -c%s "$WORK/s1/$m.o")" "$(stat -c%s "$WORK/s2/$m.o")"
    fi
done

echo
if [ -n "$diff_mods" ]; then
    echo "NOT a fixed point — $same/$(echo $MODULES | wc -w) identical, differing:$diff_mods"
    exit 1
fi
echo "all $same modules byte-identical"

if ! cmp -s "$WORK/s1/fakecc" "$WORK/s2/fakecc"; then
    echo "NOT a fixed point — objects match but the linked binaries differ"
    exit 1
fi

echo "FIXED POINT REACHED: objects and linked binary are byte-identical"
cp "$WORK/s1/fakecc" "$ROOT/v0/fakecc-1"
cp "$WORK/s2/fakecc" "$ROOT/v0/fakecc-2"
echo "wrote v0/fakecc-1 and v0/fakecc-2"
