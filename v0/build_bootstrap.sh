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
MODULES="ast cfg codegen common domtree emit ir lexer link main mem2reg opt parser regalloc scalar_opt sema"

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

echo "=== compiling with $FAKECC ==="
ok=0
fail=0
for m in $MODULES; do
    if "$FAKECC" "$OUT/$m.c" -c -o "$OUT/$m.o" 2>"$OUT/$m.cc.err"; then
        ok=$((ok + 1))
    else
        rc=$?
        if [ "$rc" -ge 128 ]; then
            echo "  COMPILER CRASH (signal $((rc - 128))): $m"
        else
            echo "  COMPILE FAIL: $m: $(head -1 "$OUT/$m.cc.err")"
        fi
        fail=$((fail + 1))
    fi
done
echo "=== compiled $ok/$((ok + fail)) modules ==="
[ "$fail" = "0" ] || exit 1

objs=""
for m in $MODULES; do objs="$objs $OUT/$m.o"; done
if [ "$LINK_WITH_GCC" = "1" ]; then
    echo "=== linking with gcc ==="
    linker_ok=0
    gcc $objs -o "$OUT/bootstrap_fakecc" -lm 2>"$OUT/link.err" && linker_ok=1
else
    echo "=== linking with $FAKECC ==="
    linker_ok=0
    "$FAKECC" $objs -o "$OUT/bootstrap_fakecc" 2>"$OUT/link.err" && linker_ok=1
fi
if [ "$linker_ok" != "1" ]; then
    echo "LINK FAILED"; cat "$OUT/link.err"; exit 1
fi
echo "=== linked $OUT/bootstrap_fakecc ==="
ls -l "$OUT/bootstrap_fakecc"
