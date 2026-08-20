#!/usr/bin/env bash
# End-to-end tests: compile each case, run it, compare the exit code with the
# `// expect:` annotation.  Cases annotated `// expect_error` must be rejected
# with a diagnostic.

set -uo pipefail

FAKECC=${1:-./build/fakecc}
shift || true
CC_TIMEOUT=${CC_TIMEOUT:-60}
RUN_TIMEOUT=${RUN_TIMEOUT:-10}
JOBS=${JOBS:-$(nproc 2>/dev/null || echo 4)}

CC_EXTRA="${CC_FLAGS:-} $*"
CC_EXTRA=$(echo "$CC_EXTRA" | tr -s ' ' | sed 's/^ //; s/ $//')

SUITE_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
CASE_DIR="$SUITE_DIR/cases"
export FAKECC_PKG="${SUITE_DIR}/pkg_fixtures${FAKECC_PKG:+:$FAKECC_PKG}"
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

cc_status_desc() {
    case "$1" in
        0)   echo "ok" ;;
        124) echo "compiler timed out after ${CC_TIMEOUT}s" ;;
        13[3-9]|1[4-9][0-9]) echo "compiler killed by signal $(($1 - 128))" ;;
        *)   echo "compiler exited $1" ;;
    esac
}
export -f cc_status_desc
export FAKECC CC_EXTRA CC_TIMEOUT RUN_TIMEOUT CASE_DIR WORK

run_single_case() {
    local src="$1"
    local name=${src#"$CASE_DIR"/}
    local out="$WORK/$(echo "${name%.c}" | tr / _)"
    local cc_err="$WORK/$(echo "${name%.c}" | tr / _).cc_err"
    local run_out="$WORK/$(echo "${name%.c}" | tr / _).run_out"
    local run_want="$WORK/$(echo "${name%.c}" | tr / _).run_want"
    local extra_flags
    extra_flags=$(sed -n 's|^//[[:space:]]*link:[[:space:]]*\(.*\)|\1|p; s|^//[[:space:]]*flags:[[:space:]]*\(.*\)|\1|p; s|^//[[:space:]]*libs:[[:space:]]*\(.*\)|\1|p' "$src" | head -1)

    timeout "$CC_TIMEOUT" "$FAKECC" $CC_EXTRA $extra_flags "$src" -o "$out" 2>"$cc_err"
    local cc_rc=$?

    if grep -q '^// expect_error' "$src"; then
        if [ "$cc_rc" = "0" ]; then
            echo "FAIL $name (expected compile error, but succeeded)"
            return 1
        elif [ "$cc_rc" -ge 124 ]; then
            echo "FAIL $name (expected a diagnostic, got: $(cc_status_desc $cc_rc))"
            return 1
        else
            echo "PASS $name"
            return 0
        fi
    fi

    if [ "$cc_rc" != "0" ]; then
        echo "FAIL $name ($(cc_status_desc $cc_rc)): $(head -1 "$cc_err")"
        return 1
    fi
    if [ ! -x "$out" ]; then
        echo "FAIL $name (compiler reported success but produced no executable)"
        return 1
    fi

    local expect
    expect=$(sed -n 's|^// expect: \([0-9]*\)|\1|p' "$src" | head -1)
    local got=0
    timeout "$RUN_TIMEOUT" "$out" >"$run_out" 2>/dev/null || got=$?
    if [ "$got" = "124" ]; then
        echo "FAIL $name (program did not terminate within ${RUN_TIMEOUT}s)"
        return 1
    fi
    if [ "$got" != "$expect" ]; then
        echo "FAIL $name (expected $expect, got $got)"
        return 1
    fi

    if grep -q '^// expect_stdout:' "$src"; then
        sed -n 's|^// expect_stdout: \{0,1\}\(.*\)$|\1|p' "$src" > "$run_want"
        if ! cmp -s "$run_want" "$run_out"; then
            echo "FAIL $name (stdout mismatch)"
            echo "  want: $(tr '\n' '|' < "$run_want")"
            echo "  got:  $(tr '\n' '|' < "$run_out")"
            return 1
        fi
    fi

    echo "PASS $name"
    return 0
}
export -f run_single_case

label="$CC_EXTRA"
echo "--- e2e: $(basename "$CASE_DIR")/**  flags: ${label:-(default)} ---"

find "$CASE_DIR" -name '*.c' | sort | xargs -P "$JOBS" -n 1 bash -c 'run_single_case "$@"' _ > "$WORK/results.log"

n_pass=$(grep -c '^PASS ' "$WORK/results.log" || true)
n_fail=$(grep -c '^FAIL ' "$WORK/results.log" || true)

cat "$WORK/results.log" | sort

echo "--- e2e${label:+ ($label)}: $n_pass passed, $n_fail failed, $((n_pass + n_fail)) total ---"

if [ "$n_fail" -ne 0 ]; then
    exit 1
fi
