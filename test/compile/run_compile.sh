#!/usr/bin/env bash
# Compile-only test suite: compile each case with `fakecc -c $file -o /tmp/xxx.o`.
# Verifies compiler robustness (no crash / ICE / segfault / error on edge cases).
#
#   test/compile/run_compile.sh [FAKECC] [FLAGS...]
#
# Test annotation (compatible with GCC's DejaGnu syntax):
#   code();  /* { dg-error   "regex" } */   -- compiler must reject AND stderr
#                                              must contain a line matching regex
#   code();  /* { dg-warning "regex" } */   -- compiler exit code is not
#                                              checked; stderr must contain a
#                                              line matching regex
#
# Target-restricted directives (those containing an extra { ... } block,
# e.g. /* { dg-error "PAT" "" { target ... } } */) are skipped in this
# first pass: the test falls back to the default behavior (compile must
# succeed).
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

# Extract dg-error / dg-warning patterns from a C source file.
#
# Usage: parse_expect_directives <src> <kind> <out_file>
#   kind: "error" or "warning"
#   writes one regex per line to out_file (no leading/trailing whitespace)
#
# Skips target-restricted directives that contain a trailing { ... } block
# (a DejaGnu target selector), e.g.:
#   /* { dg-error "PAT" "" { target { ... } } } */
# These are ignored for now; emit a one-line note to stderr.
parse_expect_directives() {
    local src="$1" kind="$2" out="$3"
    # Match /* { dg-error "PAT" ... } */ or /* { dg-warning "PAT" ... } */.
    grep -oE '/\*[[:space:]]*\{[[:space:]]*dg-'"$kind"'[[:space:]]+"[^"]*".*\}' "$src" 2>/dev/null \
        | sed -nE 's|.*/\*[[:space:]]*\{[[:space:]]*dg-'"$kind"'[[:space:]]+"([^"]*)".*|\1|p' > "$out" || true
}
export -f parse_expect_directives

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
    local err_pats="$WORK/${base}_${hash}.err_pats"
    local warn_pats="$WORK/${base}_${hash}.warn_pats"

    if grep -q '^package ' "$src"; then
        cp "$src" "$body"
    else
        {
            echo "package main;"
            cat "$src"
        } > "$body"
    fi

    parse_expect_directives "$src" error   "$err_pats"
    parse_expect_directives "$src" warning "$warn_pats"
    local n_err=$(wc -l < "$err_pats" 2>/dev/null || echo 0)
    local n_warn=$(wc -l < "$warn_pats" 2>/dev/null || echo 0)

    local rc=0
    timeout "$CC_TIMEOUT" "$FAKECC" $CC_FLAGS -c "$body" -o "$out" 2>"$err" || rc=$?

    if [ "$rc" -ge 128 ]; then
        printf 'FAIL_CRASH   %-30s (signal %s: %s)\n' "$filename" "$((rc - 128))" "$(head -1 "$err")" >> "$WORK/results.txt"
        return
    fi
    if [ "$rc" -eq 124 ]; then
        printf 'FAIL_TIMEOUT %-30s (>%ss)\n' "$filename" "$CC_TIMEOUT" >> "$WORK/results.txt"
        return
    fi

    if [ "$n_err" -gt 0 ]; then
        # dg-error present: compiler must fail AND every pattern must match
        # some line in stderr.
        if [ "$rc" -eq 0 ]; then
            printf 'FAIL_UNEXPECTED %-30s (expected compile error, succeeded)\n' "$filename" >> "$WORK/results.txt"
            return
        fi
        local missing=""
        while IFS= read -r pat; do
            [ -z "$pat" ] && continue
            if ! grep -Eq -- "$pat" "$err"; then
                missing="$missing [$pat]"
            fi
        done < "$err_pats"
        if [ -n "$missing" ]; then
            printf 'FAIL_EXPECT  %-30s (unmatched dg-error:%s)\n' "$filename" "$missing" >> "$WORK/results.txt"
            return
        fi
        # Errors matched. Now verify warnings if any.
        if [ "$n_warn" -gt 0 ]; then
            while IFS= read -r pat; do
                [ -z "$pat" ] && continue
                if ! grep -Eq -- "$pat" "$err"; then
                    printf 'FAIL_EXPECT  %-30s (unmatched dg-warning: %s)\n' "$filename" "$pat" >> "$WORK/results.txt"
                    return
                fi
            done < "$warn_pats"
        fi
        printf 'OK_EXPECT    %s\n' "$filename" >> "$WORK/results.txt"
        return
    fi

    if [ "$n_warn" -gt 0 ]; then
        # dg-warning only: exit code is not checked, but every pattern
        # must match stderr.
        while IFS= read -r pat; do
            [ -z "$pat" ] && continue
            if ! grep -Eq -- "$pat" "$err"; then
                printf 'FAIL_EXPECT  %-30s (unmatched dg-warning: %s)\n' "$filename" "$pat" >> "$WORK/results.txt"
                return
            fi
        done < "$warn_pats"
        printf 'OK_WARN      %s\n' "$filename" >> "$WORK/results.txt"
        return
    fi

    # No expected-error/warning directives: existing behavior.
    if [ "$rc" -ne 0 ]; then
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

ok_cnt=$(grep -cE '^(OK |OK_EXPECT|OK_WARN) ' "$WORK/results.txt" || true)
fail_cnt=$(grep -c '^FAIL_' "$WORK/results.txt" || true)

if [ "$fail_cnt" -gt 0 ]; then
    echo "FAIL: $fail_cnt regressions detected in compile suite ($ok_cnt passed):"
    grep '^FAIL_' "$WORK/results.txt"
    exit 1
fi

echo "PASS compile suite (${#files[@]} cases passed)"
exit 0
