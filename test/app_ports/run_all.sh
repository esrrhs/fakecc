#!/bin/bash
# Run the test suite for every FakeCC application port under test/app_ports/.
#
# Each port lives in test/app_ports/<name>/ and must provide:
#   tests/run.sh   — builds + runs that port's test; exits non-zero on failure.
# It may optionally accept a fakecc path as its first argument and an optimization flag as second.
#
# Usage: run_all.sh [path-to-fakecc] [OPT]
#   path-to-fakecc   compiler binary to use (default: ./build/fakecc)
#   OPT              optimization flag to pass to fakecc (default: -O1)
#
# The runner aggregates the PASS/FAIL of every port and exits non-zero if any
# of them failed, so it can be wired into CI as a single step.
set -uo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$( cd "${SCRIPT_DIR}/../.." &> /dev/null && pwd )"
FAKECC="${1:-${PROJECT_ROOT}/build/fakecc}"
OPT="${2:--O1}"

if [ ! -x "${FAKECC}" ]; then
    echo "run_app_ports: fakecc not found at ${FAKECC}"
    exit 1
fi

FAIL=0
PORT_COUNT=0

for port in "${SCRIPT_DIR}"/*/; do
    [ -d "$port" ] || continue
    name="$( basename "$port" )"
    # Skip non-port entries such as the README or shared scaffolding.
    run_script="${port}/tests/run.sh"
    [ -f "$run_script" ] || continue

    PORT_COUNT=$(( PORT_COUNT + 1 ))
    echo "=== app port: ${name} ==="
    if bash "$run_script" "${FAKECC}" "${OPT}"; then
        :
    else
        echo "FAIL app port: ${name}"
        FAIL=1
    fi
done

echo ""
echo "app_ports: ran ${PORT_COUNT} port(s)"
if [ "${FAIL}" = "0" ]; then
    echo "app_ports: all ports passed"
else
    echo "app_ports: some ports FAILED"
fi
exit ${FAIL}