#!/usr/bin/env bash
# End-to-end tests: compile each case, run it, compare the exit code with the
# `// expect:` annotation.  Cases annotated `// expect_error` must be rejected
# with a diagnostic.
#
# A case may also assert its output, one line per annotation:
#
#     // expect_stdout: hello 42
#
# Checking only the exit code is not enough on its own.  It let a real bug sit
# in the suite unnoticed: the entry stub exited via a raw syscall, so libc never
# flushed stdout and printf() wrote nothing — while the test that "covered"
# printf passed, because it only inspected the return value.
#
# Both the compile and the run are bounded by a timeout, and the compiler's
# exit status is checked before its output is executed.  That matters when the
# compiler under test is a bootstrap build that may crash or loop: without it,
# a failed compile leaves the previous case's binary in place and the suite
# either reports a stale result or hangs forever.
set -uo pipefail

FAKECC=${1:-./build/fakecc}
CC_TIMEOUT=${CC_TIMEOUT:-30}
RUN_TIMEOUT=${RUN_TIMEOUT:-10}

SUITE_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

FAIL=0
n_pass=0 n_fail=0

# Describe a compiler exit status: ok / diagnostic / timeout / signal.
cc_status_desc() {
    case "$1" in
        0)   echo "ok" ;;
        124) echo "compiler timed out after ${CC_TIMEOUT}s" ;;
        13[3-9]|1[4-9][0-9]) echo "compiler killed by signal $(($1 - 128))" ;;
        *)   echo "compiler exited $1" ;;
    esac
}

for src in "$SUITE_DIR"/*.c; do
    name=${src#"$SUITE_DIR"/}
    out="$WORK/${name%.c}"
    rm -f "$out"

    timeout "$CC_TIMEOUT" "$FAKECC" "$src" -o "$out" 2>"$WORK/cc.err"
    cc_rc=$?

    if grep -q '^// expect_error' "$src"; then
        # A rejected program must produce a diagnostic, not a crash or a hang.
        if [ "$cc_rc" = "0" ]; then
            echo "FAIL $name (expected compile error, but succeeded)"
            n_fail=$((n_fail + 1)); FAIL=1
        elif [ "$cc_rc" -ge 124 ]; then
            echo "FAIL $name (expected a diagnostic, got: $(cc_status_desc $cc_rc))"
            n_fail=$((n_fail + 1)); FAIL=1
        else
            echo "PASS $name"
            n_pass=$((n_pass + 1))
        fi
        continue
    fi

    if [ "$cc_rc" != "0" ]; then
        echo "FAIL $name ($(cc_status_desc $cc_rc)): $(head -1 "$WORK/cc.err")"
        n_fail=$((n_fail + 1)); FAIL=1
        continue
    fi
    if [ ! -x "$out" ]; then
        echo "FAIL $name (compiler reported success but produced no executable)"
        n_fail=$((n_fail + 1)); FAIL=1
        continue
    fi

    expect=$(sed -n 's|^// expect: \([0-9]*\)|\1|p' "$src" | head -1)
    got=0
    timeout "$RUN_TIMEOUT" "$out" >"$WORK/run.out" 2>/dev/null || got=$?
    if [ "$got" = "124" ]; then
        echo "FAIL $name (program did not terminate within ${RUN_TIMEOUT}s)"
        n_fail=$((n_fail + 1)); FAIL=1
        continue
    fi
    if [ "$got" != "$expect" ]; then
        echo "FAIL $name (expected $expect, got $got)"
        n_fail=$((n_fail + 1)); FAIL=1
        continue
    fi

    # Optional stdout assertion: one `// expect_stdout:` line per output line.
    if grep -q '^// expect_stdout:' "$src"; then
        sed -n 's|^// expect_stdout: \{0,1\}\(.*\)$|\1|p' "$src" > "$WORK/run.want"
        if ! cmp -s "$WORK/run.want" "$WORK/run.out"; then
            echo "FAIL $name (stdout mismatch)"
            echo "  want: $(tr '\n' '|' < "$WORK/run.want")"
            echo "  got:  $(tr '\n' '|' < "$WORK/run.out")"
            n_fail=$((n_fail + 1)); FAIL=1
            continue
        fi
    fi

    echo "PASS $name"
    n_pass=$((n_pass + 1))
done

echo "--- e2e: $n_pass passed, $n_fail failed, $((n_pass + n_fail)) total ---"
exit $FAIL
