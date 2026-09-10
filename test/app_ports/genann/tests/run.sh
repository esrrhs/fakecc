#!/bin/bash
# Build and run the genann FakeCC port test.
# Usage: run.sh [path-to-fakecc]   (defaults to ./build/fakecc)
set -u

# Directory of this script.
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
# tests/ -> genann -> app_ports -> test -> project root (4 levels up).
PROJECT_ROOT="$( cd "${SCRIPT_DIR}/../../../.." &> /dev/null && pwd )"

# fakecc binary: $1, else the local build tree, else the self-hosted stage-2.
FAKECC="${1:-${PROJECT_ROOT}/build/fakecc}"
if [ ! -x "${FAKECC}" ]; then
    echo "FAIL genann: fakecc not found at ${FAKECC}"
    exit 1
fi

GEN_ANN_SRC="${SCRIPT_DIR}/../genann.c"
TEST_SRC="${SCRIPT_DIR}/test_genann.c"
OUTPUT="${SCRIPT_DIR}/test_genann.bin"

# genann links against -lc -lm (rand/exp/tanh come from the system libc).
if ! "${FAKECC}" "${GEN_ANN_SRC}" "${TEST_SRC}" -o "${OUTPUT}" -lc -lm; then
    echo "FAIL genann: compilation failed"
    exit 1
fi

if "${OUTPUT}"; then
    echo "PASS genann"
else
    echo "FAIL genann: program exited non-zero"
    exit 1
fi