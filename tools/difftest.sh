#!/usr/bin/env bash
# Differential testing: compile the same program with gcc and with fakecc,
# run both, compare exit codes.
#
#   tools/difftest.sh [-c FAKECC] [-f "FAKECC_FLAGS"] file.c [file.c ...]
#
# gcc is the oracle, so a case never needs a hand-computed expected value.
# That matters more than it sounds: a wrong expectation looks exactly like a
# compiler bug, and chasing one costs more than writing the whole harness.
#
# Input files are in the fakecc dialect (a leading `package main;`); the line
# is stripped before handing the source to gcc.
set -uo pipefail

FAKECC=${FAKECC:-./build/fakecc}
RUN_TIMEOUT=${RUN_TIMEOUT:-10}
# Flags for fakecc only: gcc stays the unmodified oracle regardless of the
# optimization level we are exercising.
FCC_FLAGS=${FCC_FLAGS:-}

while getopts "c:f:" opt; do
    case "$opt" in
        c) FAKECC=$OPTARG ;;
        f) FCC_FLAGS=$OPTARG ;;
        *) echo "usage: $0 [-c FAKECC] [-f FLAGS] file.c ..." >&2; exit 2 ;;
    esac
done
shift $((OPTIND - 1))
[ $# -gt 0 ] || { echo "usage: $0 [-c FAKECC] file.c ..." >&2; exit 2; }

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
FAIL=0

for src in "$@"; do
    name=$(basename "$src" .c)

    sed 's/^package main;//' "$src" > "$WORK/$name.gcc.c"
    if ! gcc -std=c99 -w -o "$WORK/$name.gcc" "$WORK/$name.gcc.c" 2>"$WORK/$name.gcc.err"; then
        printf '%-28s SKIP (gcc rejected: %s)\n' "$name" "$(head -1 "$WORK/$name.gcc.err")"
        continue
    fi
    gcc_rc=0
    timeout "$RUN_TIMEOUT" "$WORK/$name.gcc" >"$WORK/$name.gcc.out" 2>&1 || gcc_rc=$?

    if ! "$FAKECC" $FCC_FLAGS "$src" -o "$WORK/$name.fcc" 2>"$WORK/$name.fcc.err"; then
        rc=$?
        if [ "$rc" -ge 128 ]; then
            printf '%-28s DIFF (fakecc killed by signal %s; gcc exits %s)\n' \
                   "$name" "$((rc - 128))" "$gcc_rc"
        else
            printf '%-28s DIFF (fakecc rejected: %s)\n' \
                   "$name" "$(head -1 "$WORK/$name.fcc.err")"
        fi
        FAIL=1
        continue
    fi
    fcc_rc=0
    timeout "$RUN_TIMEOUT" "$WORK/$name.fcc" >"$WORK/$name.fcc.out" 2>&1 || fcc_rc=$?

    if [ "$fcc_rc" = "124" ] && [ "$gcc_rc" != "124" ]; then
        printf '%-28s DIFF (fakecc build did not terminate; gcc exits %s)\n' "$name" "$gcc_rc"
        FAIL=1
    elif [ "$gcc_rc" != "$fcc_rc" ]; then
        printf '%-28s DIFF (gcc exits %s, fakecc exits %s)\n' "$name" "$gcc_rc" "$fcc_rc"
        FAIL=1
    elif ! cmp -s "$WORK/$name.gcc.out" "$WORK/$name.fcc.out"; then
        printf '%-28s DIFF (same exit %s, different stdout)\n' "$name" "$gcc_rc"
        FAIL=1
    else
        printf '%-28s OK (exit %s)\n' "$name" "$gcc_rc"
    fi
done

exit $FAIL
