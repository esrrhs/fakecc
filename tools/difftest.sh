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

JOBS=${JOBS:-$(nproc 2>/dev/null || echo 4)}

while getopts "c:f:j:" opt; do
    case "$opt" in
        c) FAKECC=$OPTARG ;;
        f) FCC_FLAGS=$OPTARG ;;
        j) JOBS=$OPTARG ;;
        *) echo "usage: $0 [-c FAKECC] [-f FLAGS] [-j JOBS] file.c ..." >&2; exit 2 ;;
    esac
done
shift $((OPTIND - 1))
[ $# -gt 0 ] || { echo "usage: $0 [-c FAKECC] file.c ..." >&2; exit 2; }

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

export FAKECC RUN_TIMEOUT FCC_FLAGS WORK

difftest_one() {
    local src="$1"
    local base
    base=$(basename "$src" .c)
    local hash
    hash=$(printf '%s' "$src" | md5sum | cut -c1-8)
    local name="${base}_${hash}"

    {
        echo '#define _GNU_SOURCE 1'
        echo '#include <stdio.h>'
        echo '#include <stdlib.h>'
        echo '#include <string.h>'
        echo '#include <ctype.h>'
        echo '#include <stdarg.h>'
        echo '#include <stdint.h>'
        echo '#include <unistd.h>'
        echo '#include <sys/stat.h>'
        echo '#define __syscall syscall'
        sed -E \
            -e 's/^package[[:space:]]+[A-Za-z_][A-Za-z0-9_]*;//' \
            -e 's/^import[[:space:]]+[A-Za-z_][A-Za-z0-9_]*;//' \
            -e 's/\b(runtime|fmt|io|ctype)\.//g' \
            "$src"
    } > "$WORK/$name.gcc.c"
    if ! gcc -std=gnu99 -D_GNU_SOURCE -w -o "$WORK/$name.gcc" "$WORK/$name.gcc.c" 2>"$WORK/$name.gcc.err"; then
        printf '%-28s SKIP (gcc rejected: %s)\n' "$base" "$(head -1 "$WORK/$name.gcc.err")"
        return 0
    fi
    local gcc_rc=0
    timeout "$RUN_TIMEOUT" "$WORK/$name.gcc" >"$WORK/$name.gcc.out" 2>"$WORK/$name.gcc.stderr" || gcc_rc=$?

    if ! "$FAKECC" $FCC_FLAGS "$src" -o "$WORK/$name.fcc" 2>"$WORK/$name.fcc.err"; then
        local rc=$?
        if [ "$rc" -ge 128 ]; then
            printf '%-28s DIFF (fakecc killed by signal %s; gcc exits %s)\n' \
                   "$base" "$((rc - 128))" "$gcc_rc"
        else
            printf '%-28s DIFF (fakecc rejected: %s)\n' \
                   "$base" "$(head -1 "$WORK/$name.fcc.err")"
        fi
        return 1
    fi
    local fcc_rc=0
    timeout "$RUN_TIMEOUT" "$WORK/$name.fcc" >"$WORK/$name.fcc.out" 2>"$WORK/$name.fcc.stderr" || fcc_rc=$?

    if [ "$fcc_rc" = "124" ] && [ "$gcc_rc" != "124" ]; then
        printf '%-28s DIFF (fakecc build did not terminate; gcc exits %s)\n' "$base" "$gcc_rc"
        return 1
    elif [ "$gcc_rc" != "$fcc_rc" ]; then
        printf '%-28s DIFF (gcc exits %s, fakecc exits %s)\n' "$base" "$gcc_rc" "$fcc_rc"
        return 1
    elif ! cmp -s "$WORK/$name.gcc.out" "$WORK/$name.fcc.out"; then
        printf '%-28s DIFF (same exit %s, different stdout)\n' "$base" "$gcc_rc"
        return 1
    else
        printf '%-28s OK (exit %s)\n' "$base" "$gcc_rc"
        return 0
    fi
}
export -f difftest_one

printf "%s\n" "$@" | xargs -P "$JOBS" -n 1 bash -c 'difftest_one "$@"' _ > "$WORK/results.log"
cat "$WORK/results.log" | sort

if grep -q 'DIFF' "$WORK/results.log"; then
    exit 1
fi
exit 0
