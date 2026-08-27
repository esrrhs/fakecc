#!/usr/bin/env bash
# Compile-only test suite: compile each case with `fakecc -c $file -o /tmp/xxx.o`.
# Verifies compiler robustness (no crash / ICE / segfault / error on edge cases).
#
#   test/compile/run_compile.sh [FAKECC] [FLAGS...]
#
set -uo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
FAKECC=${1:-"$ROOT/build/fakecc"}
shift || true

CC_FLAGS="${CC_FLAGS:-} $*"
CC_TIMEOUT=${CC_TIMEOUT:-60}
JOBS=${JOBS:-$(nproc 2>/dev/null || echo 4)}
COMPILE_DIR="$ROOT/test/compile"

if [ ! -x "$FAKECC" ]; then
    echo "run_compile: fakecc not executable: $FAKECC" >&2
    exit 2
fi

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

export FAKECC CC_FLAGS CC_TIMEOUT WORK COMPILE_DIR

compile_one() {
    local src="$1"
    local base
    base=$(basename "$src" .c)
    local filename
    filename=$(basename "$src")
    local hash
    hash=$(printf '%s' "$src" | md5sum | cut -c1-8)
    local out="$WORK/${base}_${hash}.o"
    local err="$WORK/${base}_${hash}.err"
    local body="$WORK/${base}_${hash}.c"
    
    if grep -q '^package ' "$src"; then
        cp "$src" "$body"
    else
        {
            echo "package main;"
            cat "$src"
        } > "$body"
    fi

    local rc=0
    timeout "$CC_TIMEOUT" "$FAKECC" $CC_FLAGS -c "$body" -o "$out" 2>"$err" || rc=$?

    if [ "$rc" -ge 128 ]; then
        printf 'FAIL_CRASH   %-30s (signal %s: %s)\n' "$filename" "$((rc - 128))" "$(head -1 "$err")" >> "$WORK/results.txt"
    elif [ "$rc" -eq 124 ]; then
        printf 'FAIL_TIMEOUT %-30s (>%ss)\n' "$filename" "$CC_TIMEOUT" >> "$WORK/results.txt"
    elif [ "$rc" -ne 0 ]; then
        printf 'FAIL_REJECT  %-30s (%s)\n' "$filename" "$(head -1 "$err")" >> "$WORK/results.txt"
    else
        printf 'OK           %s\n' "$filename" >> "$WORK/results.txt"
    fi
}
export -f compile_one

files=()
for f in $(find "$COMPILE_DIR" -maxdepth 1 -name '*.c' | sort); do
    files+=("$f")
done

if [ ${#files[@]} -eq 0 ]; then
    echo "run_compile: no cases found in $COMPILE_DIR" >&2
    exit 1
fi

echo "Running compile suite (${#files[@]} cases) with $FAKECC $CC_FLAGS..."
touch "$WORK/results.txt"
printf '%s\n' "${files[@]}" | xargs -P "$JOBS" -I {} bash -c 'compile_one "$@"' _ {}

ok_cnt=$(grep -c '^OK ' "$WORK/results.txt" || true)
fail_cnt=$(grep -c '^FAIL_' "$WORK/results.txt" || true)

if [ "$fail_cnt" -gt 0 ]; then
    echo "FAIL: $fail_cnt regressions detected in compile suite ($ok_cnt passed):"
    grep '^FAIL_' "$WORK/results.txt"
    exit 1
fi

echo "PASS compile suite (${#files[@]} cases passed)"
exit 0
