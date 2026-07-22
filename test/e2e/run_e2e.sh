#!/usr/bin/env bash
set -uo pipefail

FAKECC=${1:-./build/fakecc}
FAIL=0

for src in test/e2e/*.c; do
    # Error-path tests: expect fakecc to fail (non-zero exit).
    if grep -q '^// expect_error' "$src"; then
        if "$FAKECC" "$src" -o /tmp/fakecc_e2e.out 2>/dev/null; then
            echo "FAIL $src (expected compile error, but succeeded)"
            FAIL=1
        else
            echo "PASS $src"
        fi
        continue
    fi

    # Normal tests: expect a specific exit code.
    expect=$(sed -n 's/^\/\/ expect: \([0-9]*\)/\1/p' "$src" | head -1)
    "$FAKECC" "$src" -o /tmp/fakecc_e2e.out
    got=0
    /tmp/fakecc_e2e.out || got=$?
    if [ "$got" = "$expect" ]; then
        echo "PASS $src"
    else
        echo "FAIL $src (expected $expect, got $got)"
        FAIL=1
    fi
done

exit $FAIL
