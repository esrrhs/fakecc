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

export FAKECC CC_FLAGS CC_TIMEOUT WORK COMPILE_DIR

compile_one() {
    local src="$1"
    local base
    base=$(basename "$src" .c)
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
        printf '%-30s CRASH (signal %s: %s)\n' "$base" "$((rc - 128))" "$(head -1 "$err")"
        return 1
    elif [ "$rc" -eq 124 ]; then
        printf '%-30s TIMEOUT (>%ss)\n' "$base" "$CC_TIMEOUT"
        return 1
    elif [ "$rc" -ne 0 ]; then
        # Compile error (e.g. GNU extension not supported)
        printf '%-30s REJECT (%s)\n' "$base" "$(head -1 "$err")"
        return 0
    else
        printf '%-30s OK\n' "$base"
        return 0
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
printf '%s\n' "${files[@]}" | xargs -P "$JOBS" -I {} bash -c 'compile_one "$@"' _ {}
