#!/usr/bin/env bash
# Compile-only test suite: compile each case with `fakecc -c $file -o /tmp/xxx.o`.
# Verifies compiler robustness (no crash / ICE / segfault on edge cases).
#
#   test/compile/run_compile.sh [FAKECC] [FLAGS...]
#
set -uo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
FAKECC=${1:-"$ROOT/build/fakecc"}
shift || true

CC_FLAGS="${CC_FLAGS:-} $*"
CC_TIMEOUT=${CC_TIMEOUT:-15}
JOBS=${JOBS:-$(nproc 2>/dev/null || echo 4)}
COMPILE_DIR="$ROOT/test/compile"

if [ ! -x "$FAKECC" ]; then
    echo "run_compile: fakecc not executable: $FAKECC" >&2
    exit 2
fi

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

# Extract known non-OK cases from need_faithful.txt
KNOWN_FILE="$WORK/known_fails.txt"
if [ -f "$COMPILE_DIR/need_faithful.txt" ]; then
    grep -E '^[A-Za-z0-9_-]+\.c' "$COMPILE_DIR/need_faithful.txt" | sort -u > "$KNOWN_FILE"
else
    touch "$KNOWN_FILE"
fi

export FAKECC CC_FLAGS CC_TIMEOUT WORK COMPILE_DIR KNOWN_FILE

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

    local is_known=0
    if grep -qx "$filename" "$KNOWN_FILE" 2>/dev/null; then
        is_known=1
    fi

    local rc=0
    timeout "$CC_TIMEOUT" "$FAKECC" $CC_FLAGS -c "$body" -o "$out" 2>"$err" || rc=$?

    if [ "$rc" -ge 128 ]; then
        if [ "$is_known" -eq 1 ]; then
            printf 'KNOWN_CRASH %s (signal %s)\n' "$filename" "$((rc - 128))" >> "$WORK/results.txt"
        else
            printf 'FAIL_CRASH  %s (signal %s: %s)\n' "$filename" "$((rc - 128))" "$(head -1 "$err")" >> "$WORK/results.txt"
        fi
    elif [ "$rc" -eq 124 ]; then
        if [ "$is_known" -eq 1 ]; then
            printf 'KNOWN_TIMEOUT %s\n' "$filename" >> "$WORK/results.txt"
        else
            printf 'FAIL_TIMEOUT  %s\n' "$filename" >> "$WORK/results.txt"
        fi
    elif [ "$rc" -ne 0 ]; then
        if [ "$is_known" -eq 1 ]; then
            printf 'KNOWN_REJECT %s\n' "$filename" >> "$WORK/results.txt"
        else
            printf 'FAIL_REJECT  %s (%s)\n' "$filename" "$(head -1 "$err")" >> "$WORK/results.txt"
        fi
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

echo "Running compile robustness suite (${#files[@]} cases) with $FAKECC $CC_FLAGS..."
touch "$WORK/results.txt"
printf '%s\n' "${files[@]}" | xargs -P "$JOBS" -I {} bash -c 'compile_one "$@"' _ {}

ok_cnt=$(grep -c '^OK ' "$WORK/results.txt" || true)
known_cnt=$(grep -c '^KNOWN_' "$WORK/results.txt" || true)
fail_cnt=$(grep -c '^FAIL_' "$WORK/results.txt" || true)

echo "--- Compile Suite Summary ---"
echo "  Passed (OK):       $ok_cnt"
echo "  Known Non-OK:      $known_cnt"
echo "  Unexpected Fails:  $fail_cnt"

if [ "$fail_cnt" -gt 0 ]; then
    echo "Regressions detected in compile suite:"
    grep '^FAIL_' "$WORK/results.txt"
    exit 1
fi

echo "PASS compile suite (${#files[@]} cases)"
exit 0
