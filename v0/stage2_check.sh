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

# Succeeds (exit 0) when $1 is a 64-bit Mach-O image carrying no
# LC_CODE_SIGNATURE, so `if macho_unsigned f` reads as "sign it".  Note the
# polarity: a shell `if` treats status 0 as true, so "needs signing" has to be
# the success status, not a failure one.
macho_unsigned() {
    python3 - "$1" <<'PY'
import struct, sys
d = open(sys.argv[1], 'rb').read()
if d[:4] != b'\xcf\xfa\xed\xfe':
    sys.exit(1)          # not a little-endian 64-bit Mach-O: leave it alone
ncmds = struct.unpack_from('<I', d, 16)[0]
off = 32
for _ in range(ncmds):
    cmd, cmdsize = struct.unpack_from('<II', d, off)
    if cmd == 0x1d:      # LC_CODE_SIGNATURE
        sys.exit(1)      # already signed: nothing to do
    off += cmdsize
sys.exit(0)              # unsigned: the caller should stamp it
PY
}

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
    "$cc" $srcs -o "$outdir/fakecc" 2>"$outdir/build.err" || {
        echo "  BUILD FAIL: $(head -1 "$outdir/build.err")" >&2
        return 1
    }
    # An arm64-macos image will not run until it carries an ad-hoc signature,
    # and a selfhosted fakecc has no fork/execl to invoke codesign(1) itself,
    # so stage-2 output arrives unsigned.  Stamp it here with the very
    # invocation the stage-0 driver uses, which puts both binaries in the same
    # state before they are compared.  Only an image that is still unsigned
    # gets signed: codesign is not idempotent, because the signature it writes
    # covers the file length, so re-signing stage 1 would alter it.
    if [ "$(uname -s)" = "Darwin" ] && macho_unsigned "$outdir/fakecc"; then
        /usr/bin/codesign -s - -f "$outdir/fakecc" 2>>"$outdir/build.err" || {
            echo "  CODESIGN FAIL: $(tail -1 "$outdir/build.err")" >&2
            return 1
        }
    fi
}

echo "=== stage 1: Stage 0 compiles fakecc ==="
build_with "$STAGE0" "$WORK/s1" || exit 1
echo "    -> $WORK/s1/fakecc"
# Persist stage1 immediately so the workflow uploads the exact
# stage1 binary even if stage2 crashes the compiler before any
# v0/fakecc-1 is written.
cp "$WORK/s1/fakecc" "$ROOT/v0/fakecc-1"

echo "=== stage 2: fakecc-1 compiles fakecc ==="
if ! build_with "$WORK/s1/fakecc" "$WORK/s2"; then
    echo "stage2 compiler failed; v0/fakecc-1 retained for inspection" >&2
    exit 1
fi
echo "    -> $WORK/s2/fakecc"

echo
echo "=== comparing stage 1 and stage 2 binaries ==="
if cmp -s "$WORK/s1/fakecc" "$WORK/s2/fakecc"; then
    echo "binaries byte-identical"
else
    echo "NOT a fixed point — stage 1 and stage 2 binaries differ"
    echo "  stage 1: $(file_size "$WORK/s1/fakecc") bytes"
    echo "  stage 2: $(file_size "$WORK/s2/fakecc") bytes"
    echo "  first differing bytes (cmp -l):"
    cmp -l "$WORK/s1/fakecc" "$WORK/s2/fakecc" | head -20
    cp "$WORK/s1/fakecc" "$ROOT/v0/fakecc-1"
    cp "$WORK/s2/fakecc" "$ROOT/v0/fakecc-2"
    echo "wrote v0/fakecc-1 and v0/fakecc-2 (mismatch)"
    exit 1
fi

echo
echo "FIXED POINT REACHED: linked binaries are byte-identical"
cp "$WORK/s1/fakecc" "$ROOT/v0/fakecc-1"
cp "$WORK/s2/fakecc" "$ROOT/v0/fakecc-2"
echo "wrote v0/fakecc-1 and v0/fakecc-2"
