#!/usr/bin/env bash
# Run tools/difftest.sh over the curated e2e manifest (gcc as oracle).
set -uo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
FAKECC=${1:-"$ROOT/build/fakecc"}
MANIFEST=${MANIFEST:-"$ROOT/test/e2e/difftest_manifest.txt"}
SUITE_DIR="$ROOT/test/e2e"

if [ ! -x "$FAKECC" ]; then
    echo "difftest: fakecc not executable: $FAKECC" >&2
    exit 2
fi
if [ ! -f "$MANIFEST" ]; then
    echo "difftest: missing manifest: $MANIFEST" >&2
    exit 2
fi

files=()
while IFS= read -r name || [ -n "$name" ]; do
    case "$name" in
        ''|\#*) continue ;;
    esac
    f="$SUITE_DIR/$name"
    if [ ! -f "$f" ]; then
        echo "difftest: missing case: $f" >&2
        exit 2
    fi
    files+=("$f")
done < "$MANIFEST"

exec bash "$ROOT/tools/difftest.sh" -c "$FAKECC" "${files[@]}"
