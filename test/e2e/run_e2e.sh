#!/usr/bin/env bash
# End-to-end tests: compile each case, run it, compare the exit code with the
# `// expect:` annotation.  Cases annotated `// expect_error` must be rejected
# with a diagnostic.

set -uo pipefail
# Portability helpers (timeout/nproc/readelf shims for non-GNU hosts).
_COMPAT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
# Abort if the shims could not be sourced: a silently missing `timeout` turns
# every case into a confusing "command not found" instead of a clear error.
. "$_COMPAT_DIR/compat.sh" || { echo "cannot source compat.sh from $_COMPAT_DIR" >&2; exit 2; }
# run_e2e fans cases out to `xargs bash -c` workers, which only see exported
# functions, so re-export the shims this script's workers rely on.
export -f timeout nproc cmp 2>/dev/null || true


FAKECC=${1:-./build/fakecc}
shift || true
CC_TIMEOUT=${CC_TIMEOUT:-60}
RUN_TIMEOUT=${RUN_TIMEOUT:-10}
JOBS=${JOBS:-$(nproc 2>/dev/null || echo 4)}
# An empty or non-numeric JOBS used to reach `xargs -P ""`, which fails after
# collecting nothing -- the suite then reported 0 cases and exited 0, so ctest
# showed a green test that had checked nothing.  Never let that happen again.
case "$JOBS" in ''|*[!0-9]*) JOBS=4 ;; esac
[ "$JOBS" -ge 1 ] || JOBS=4

CC_EXTRA="${CC_FLAGS:-} $*"
CC_EXTRA=$(echo "$CC_EXTRA" | tr -s ' ' | sed 's/^ //; s/ $//')

SUITE_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
CASE_DIR="$SUITE_DIR/cases"
# `unsupported_on:` tags are `<os>-<machine>` from uname (darwin-arm64,
# linux-x86_64).  Resolve it once: inside run_single_case it cost two `uname`
# forks per case, ~5.4k of them across the corpus, to compute a value that
# cannot change during the run.
HOST_TAG="$(host_tag)"
export HOST_TAG
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

    # Platform-intrinsic skips, e.g. a test keyed to x87 80-bit long double
    # (memcmp of 10 bytes, literals beyond double range) that the platform
    # ABI genuinely cannot satisfy — the host toolchain fails it the same
    # way.  HOST_TAG is computed once at the top of the script.
    local skipped=0
    for t in $(sed -n 's|^//[[:space:]]*unsupported_on:[[:space:]]*\(.*\)|\1|p' "$src" | head -1); do
        if [ "$t" = "$HOST_TAG" ]; then skipped=1; fi
    done
    if [ "$skipped" = "1" ]; then
        echo "SKIP $name (unsupported on $HOST_TAG)"
        return 0
    fi

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
n_skip=$(grep -c '^SKIP ' "$WORK/results.log" || true)

cat "$WORK/results.log" | sort

n_total=$((n_pass + n_fail + n_skip))
echo "--- e2e${label:+ ($label)}: $n_pass passed, $n_fail failed, $n_skip skipped, $n_total total ---"

# Zero cases is never a passing run: it is what a broken fan-out looks like
# (`xargs -P ""`, a missing case directory, ...).  Reporting it as success is
# how this suite spent weeks green on Linux while testing nothing.
if [ "$n_total" -eq 0 ]; then
    echo "e2e: no cases were collected from $CASE_DIR -- refusing to pass" >&2
    exit 1
fi

if [ "$n_fail" -ne 0 ]; then
    exit 1
fi
