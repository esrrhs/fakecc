#!/usr/bin/env bash
# Locate which compiler module fakecc miscompiles, by hybrid linking.
#
#   tools/bisect_module.sh [-p probe.c] [-t]
#     -p  probe program (fakecc dialect, `// expect: N` annotated)
#     -t  re-run v0/translate.py before building
#
# A bootstrap compiler that misbehaves gives you no handle: the fault could be
# anywhere in 14k lines.  This narrows it to one file.  Build every module with
# gcc except one, take that one from fakecc, and link the mix.  If the hybrid
# misbehaves, fakecc miscompiles that module.
#
# The mix links because fakecc's own .o files are ordinary ET_REL objects with
# the same symbol names as the gcc-built ones — v0/*.c is a mechanical
# translation of src/*.c, so the two halves agree on every interface.
#
# LIMITATION — false positives from remaining ABI gaps.  Small structs
# (≤16 bytes) now follow SysV register pass/return, and MEMORY-class
# aggregates travel on the stack.  Hybrid linking can still disagree on
# less-common corner cases (e.g. nested X87 fields); reverse the swap to
# tell a real miscompilation apart from a calling-convention mismatch.
set -uo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
# Derived from src/, not listed here — see v0/build_bootstrap.sh for why.
MODULES=$(cd "$ROOT/src" && LC_ALL=C ls *.c | sed 's/\.c$//' | tr '\n' ' ')
[ -n "$MODULES" ] || { echo "no sources found in $ROOT/src" >&2; exit 1; }
PROBE=""
TRANSLATE=0

while getopts "p:t" opt; do
    case "$opt" in
        p) PROBE=$OPTARG ;;
        t) TRANSLATE=1 ;;
        *) echo "usage: $0 [-p probe.c] [-t]" >&2; exit 2 ;;
    esac
done

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

# Default probe: a counted loop.  It needs a φ node at the loop header, which
# is the first thing to break when the dominator tree is wrong.
if [ -z "$PROBE" ]; then
    PROBE="$WORK/probe.c"
    cat > "$PROBE" <<'EOF'
// expect: 3
package main;
int main() { int i = 0; while (i < 3) { i = i + 1; } return i; }
EOF
fi
EXPECT=$(sed -n 's|^// expect: \([0-9]*\)|\1|p' "$PROBE" | head -1)
[ -n "$EXPECT" ] || { echo "probe has no '// expect: N' annotation" >&2; exit 2; }

STAGE0="$ROOT/build/fakecc"
[ -x "$STAGE0" ] || { echo "need a Stage 0 build at $STAGE0" >&2; exit 2; }

if [ "$TRANSLATE" = "1" ]; then
    python3 "$ROOT/v0/translate.py" >/dev/null || exit 1
fi

echo "building reference objects with gcc ..."
for m in $MODULES; do
    gcc -std=c99 -c -O0 -w -I "$ROOT/include" -o "$WORK/gcc_$m.o" "$ROOT/src/$m.c" \
        || { echo "gcc failed on src/$m.c" >&2; exit 1; }
done

echo "building objects with fakecc ..."
for m in $MODULES; do
    [ -f "$ROOT/v0/$m.c" ] || { echo "missing v0/$m.c — run with -t" >&2; exit 1; }
    "$STAGE0" "$ROOT/v0/$m.c" -c -o "$WORK/fcc_$m.o" 2>"$WORK/cc_$m.err" \
        || { echo "fakecc failed on v0/$m.c: $(head -1 "$WORK/cc_$m.err")" >&2; exit 1; }
done

# An all-gcc build must pass the probe, otherwise the probe itself is at fault.
gcc $(for m in $MODULES; do echo "$WORK/gcc_$m.o"; done) -o "$WORK/ref" -lm 2>/dev/null
ref_got=0
"$WORK/ref" "$PROBE" -o "$WORK/ref.out" >/dev/null 2>&1 && { "$WORK/ref.out" >/dev/null 2>&1 || ref_got=$?; }
if [ "$ref_got" != "$EXPECT" ]; then
    echo "all-gcc reference does not pass the probe (expected $EXPECT, got $ref_got) — fix the probe first" >&2
    exit 1
fi
echo "reference OK; swapping one module at a time"
echo

bad=""
for swap in $MODULES; do
    objs=""
    for m in $MODULES; do
        if [ "$m" = "$swap" ]; then objs="$objs $WORK/fcc_$m.o"; else objs="$objs $WORK/gcc_$m.o"; fi
    done
    gcc $objs -o "$WORK/hy" -lm 2>/dev/null
    if [ ! -x "$WORK/hy" ]; then printf '  %-12s LINK FAILED\n' "$swap"; bad="$bad $swap"; continue; fi

    timeout 30 "$WORK/hy" "$PROBE" -o "$WORK/hy.out" >/dev/null 2>"$WORK/hy.err"
    rc=$?
    if [ "$rc" -ge 128 ]; then
        printf '  %-12s MISCOMPILED (compiler killed by signal %s)\n' "$swap" "$((rc - 128))"
        bad="$bad $swap"
    elif [ "$rc" = "124" ]; then
        printf '  %-12s MISCOMPILED (compiler did not terminate)\n' "$swap"
        bad="$bad $swap"
    elif [ "$rc" != "0" ]; then
        printf '  %-12s MISCOMPILED (rejects the probe: %s)\n' "$swap" "$(head -1 "$WORK/hy.err")"
        bad="$bad $swap"
    else
        got=0
        timeout 10 "$WORK/hy.out" >/dev/null 2>&1 || got=$?
        if [ "$got" = "124" ]; then
            printf '  %-12s MISCOMPILED (probe build does not terminate)\n' "$swap"
            bad="$bad $swap"
        elif [ "$got" != "$EXPECT" ]; then
            printf '  %-12s MISCOMPILED (probe returns %s, expected %s)\n' "$swap" "$got" "$EXPECT"
            bad="$bad $swap"
        else
            printf '  %-12s ok\n' "$swap"
        fi
    fi
done

echo
if [ -z "$bad" ]; then
    echo "all $(echo $MODULES | wc -w) modules compile correctly for this probe"
    exit 0
fi
echo "miscompiled:$bad"
exit 1
