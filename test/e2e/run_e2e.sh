#!/usr/bin/env bash
set -uo pipefail

FAKECC=${1:-./build/fakecc}
FAIL=0

for src in test/e2e/*.c; do
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
