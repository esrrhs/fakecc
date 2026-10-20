#!/usr/bin/env bash
# Compare runtime of the binaries fakecc and gcc produce.
#
# The interesting number is the binary fakecc produced, not how long fakecc
# itself took to compile.  Prefer the self-hosted compiler (v0/fakecc-1)
# when it exists so the numbers match a completed bootstrap.
#
# Usage: run_bench.sh [FAKECC] [measure.py flags...]
#
# Exits 0 when every bench's fakecc and gcc binaries print the same output.
# Performance ratios are reported, never used as a pass/fail gate.
#
# BENCH_GCC selects the oracle.  BENCH_RUNS, when set, forces that many
# measured repetitions (the old fixed-count knob).  Otherwise measure.py
# picks a count from --min-time.
set -uo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
GCC=${BENCH_GCC:-gcc}

FAKECC=""
if [ $# -ge 1 ] && [ "${1#-}" = "$1" ]; then
    FAKECC=$1
    shift
fi
if [ -z "$FAKECC" ]; then
    if [ -x "$ROOT/v0/fakecc-1" ]; then
        FAKECC=$ROOT/v0/fakecc-1
    elif [ -x "$ROOT/build/fakecc" ]; then
        FAKECC=$ROOT/build/fakecc
    else
        echo "run_bench: no fakecc binary (pass a path, or build / bootstrap first)" >&2
        exit 2
    fi
fi
if [ ! -x "$FAKECC" ]; then
    echo "run_bench: fakecc not executable: $FAKECC" >&2
    exit 2
fi

args=(--fakecc "$FAKECC" --gcc "$GCC")
if [ -n "${BENCH_RUNS:-}" ]; then
    args+=(--repetitions "$BENCH_RUNS")
fi
exec python3 "$ROOT/bench/measure.py" "${args[@]}" "$@"
