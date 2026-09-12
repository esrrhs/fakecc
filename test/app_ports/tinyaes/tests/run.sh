#!/bin/bash
# Build and run the tiny-AES-c FakeCC port test.
# Usage: run.sh [path-to-fakecc] [OPT]
#   path-to-fakecc   compiler binary to use (default: ./build/fakecc)
#   OPT              optimization flag to pass to fakecc (default: -O1)
set -u

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$( cd "${SCRIPT_DIR}/../../../.." &> /dev/null && pwd )"

FAKECC="${1:-${PROJECT_ROOT}/build/fakecc}"
OPT="${2:--O1}"
if [ ! -x "${FAKECC}" ]; then
    echo "FAIL tinyaes: fakecc not found at ${FAKECC}"
    exit 1
fi

SRC="${SCRIPT_DIR}/../aes.c"
TEST_SRC="${SCRIPT_DIR}/test_tinyaes.c"
# Unique output so app_ports_O0 and app_ports_O1 can run in parallel.
SAFE_OPT="${OPT#-}"
OUTPUT="${SCRIPT_DIR}/test_tinyaes.${SAFE_OPT}.$$.bin"
trap 'rm -f "${OUTPUT}"' EXIT

if ! "${FAKECC}" "${OPT}" "${SRC}" "${TEST_SRC}" -o "${OUTPUT}"; then
    echo "FAIL tinyaes: compilation failed"
    exit 1
fi

if "${OUTPUT}"; then
    echo "PASS tinyaes"
else
    echo "FAIL tinyaes: program exited non-zero"
    exit 1
fi
