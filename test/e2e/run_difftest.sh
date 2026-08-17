#!/usr/bin/env bash
# Differential testing: run every e2e case through both gcc and fakecc,
# compare exit codes.  gcc is the oracle, so a case never needs a
# hand-computed expected value.
set -uo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
FAKECC=${1:-"$ROOT/build/fakecc"}
shift || true
MANIFEST=${MANIFEST:-"$ROOT/test/e2e/difftest_manifest.txt"}
SUITE_DIR="$ROOT/test/e2e/cases"
# Extra fakecc flags (e.g. -O0); gcc remains the unmodified oracle.
FCC_FLAGS="${CC_FLAGS:-} $*"

if [ ! -x "$FAKECC" ]; then
    echo "difftest: fakecc not executable: $FAKECC" >&2
    exit 2
fi
# Collect every .c case except the gdb/debug suite (which uses gdb
# breakpoints, not exit codes) and the expect_error cases (rejected
# by design, gcc would accept them).
files=()
for f in $(find "$SUITE_DIR" -name '*.c' -not -path '*/debug/*' | sort); do
    if grep -q '^// expect_error' "$f"; then
        continue
    fi
    files+=("$f")
done

if [ ${#files[@]} -eq 0 ]; then
    echo "difftest: no cases found in $SUITE_DIR" >&2
    exit 1
fi

exec bash "$ROOT/tools/difftest.sh" -c "$FAKECC" -f "$FCC_FLAGS" "${files[@]}"
