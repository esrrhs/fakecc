#!/usr/bin/env bash
# Compare wall-clock of the same programs compiled by fakecc vs host gcc.
#
# The interesting number is the binary fakecc produced, not how long fakecc
# itself took to compile.  Prefer the self-hosted compiler (v0/fakecc-1)
# when it exists so the numbers match a completed bootstrap.
#
# Usage: run_bench.sh [FAKECC]
#
# Exits 0 when every bench's fakecc and gcc binaries print the same output.
# Performance ratios are reported, never used as a pass/fail gate.
set -uo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
CASE_DIR="$ROOT/bench/cases"
GCC=${BENCH_GCC:-gcc}
RUNS=${BENCH_RUNS:-3}

if [ $# -ge 1 ]; then
    FAKECC=$1
elif [ -x "$ROOT/v0/fakecc-1" ]; then
    FAKECC=$ROOT/v0/fakecc-1
elif [ -x "$ROOT/build/fakecc" ]; then
    FAKECC=$ROOT/build/fakecc
else
    echo "run_bench: no fakecc binary (pass a path, or build / bootstrap first)" >&2
    exit 2
fi

if [ ! -x "$FAKECC" ]; then
    echo "run_bench: fakecc not executable: $FAKECC" >&2
    exit 2
fi

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

to_gcc() {
    local src=$1 dst=$2
    {
        echo '#include <stdio.h>'
        sed -E \
            -e 's/^package[[:space:]]+[A-Za-z_][A-Za-z0-9_]*;//' \
            -e 's/^import[[:space:]]+[A-Za-z_][A-Za-z0-9_]*;//' \
            -e 's/\bruntime\.//g' \
            "$src"
    } > "$dst"
}

# Prints median milliseconds on stdout; copies the program's stdout to $2.
time_bin() {
    local bin=$1 out=$2
    python3 - "$bin" "$out" "$RUNS" <<'PY'
import subprocess, sys, statistics, time
bin, out_path, n = sys.argv[1], sys.argv[2], int(sys.argv[3])
subprocess.run([bin], stdout=subprocess.DEVNULL, check=True)
times = []
text = b""
for _ in range(n):
    t0 = time.perf_counter()
    p = subprocess.run([bin], stdout=subprocess.PIPE, check=True)
    times.append((time.perf_counter() - t0) * 1000.0)
    text = p.stdout
open(out_path, "wb").write(text)
print("%.1f" % statistics.median(times))
PY
}

fcc_kind=host
if [ "$(basename "$FAKECC")" = "fakecc-1" ] || [ "$(basename "$FAKECC")" = "fakecc-2" ]; then
    fcc_kind=self-hosted
fi

echo "=== fakecc vs gcc  (generated-binary runtime, not compile time) ==="
echo "fakecc: $FAKECC  ($fcc_kind)"
echo "gcc:    $($GCC --version | head -1)"
echo "runs:   1 warmup + $RUNS timed; median wall-clock"
echo

fail=0
# name fcc_O0 gcc_O0 ratio0 fcc_O1 gcc_O1 ratio1 gcc_O2
declare -a rows=()
gmean0_fcc=1 gmean0_gcc=1
gmean1_fcc=1 gmean1_gcc=1
nbench=0

shopt -s nullglob
for src in "$CASE_DIR"/*.c; do
    name=$(basename "$src" .c)
    gcc_src="$WORK/$name.gcc.c"
    to_gcc "$src" "$gcc_src"

    echo "--- $name ---"
    if ! "$FAKECC" -O0 "$src" -o "$WORK/${name}.fcc.O0" 2>"$WORK/${name}.fcc.O0.err"; then
        echo "FAIL $name: fakecc -O0 compile"
        cat "$WORK/${name}.fcc.O0.err" >&2
        fail=1
        continue
    fi
    if ! "$FAKECC" -O1 "$src" -o "$WORK/${name}.fcc.O1" 2>"$WORK/${name}.fcc.O1.err"; then
        echo "FAIL $name: fakecc -O1 compile"
        cat "$WORK/${name}.fcc.O1.err" >&2
        fail=1
        continue
    fi
    if ! "$GCC" -std=gnu99 -O0 "$gcc_src" -o "$WORK/${name}.gcc.O0" 2>"$WORK/${name}.gcc.O0.err"; then
        echo "FAIL $name: gcc -O0 compile"
        cat "$WORK/${name}.gcc.O0.err" >&2
        fail=1
        continue
    fi
    if ! "$GCC" -std=gnu99 -O1 "$gcc_src" -o "$WORK/${name}.gcc.O1" 2>"$WORK/${name}.gcc.O1.err"; then
        echo "FAIL $name: gcc -O1 compile"
        cat "$WORK/${name}.gcc.O1.err" >&2
        fail=1
        continue
    fi
    if ! "$GCC" -std=gnu99 -O2 "$gcc_src" -o "$WORK/${name}.gcc.O2" 2>"$WORK/${name}.gcc.O2.err"; then
        echo "FAIL $name: gcc -O2 compile"
        cat "$WORK/${name}.gcc.O2.err" >&2
        fail=1
        continue
    fi

    fcc0=$(time_bin "$WORK/${name}.fcc.O0" "$WORK/${name}.fcc.O0.out") || { echo "FAIL $name: fakecc -O0 run"; fail=1; continue; }
    fcc1=$(time_bin "$WORK/${name}.fcc.O1" "$WORK/${name}.fcc.O1.out") || { echo "FAIL $name: fakecc -O1 run"; fail=1; continue; }
    gcc0=$(time_bin "$WORK/${name}.gcc.O0" "$WORK/${name}.gcc.O0.out") || { echo "FAIL $name: gcc -O0 run"; fail=1; continue; }
    gcc1=$(time_bin "$WORK/${name}.gcc.O1" "$WORK/${name}.gcc.O1.out") || { echo "FAIL $name: gcc -O1 run"; fail=1; continue; }
    gcc2=$(time_bin "$WORK/${name}.gcc.O2" "$WORK/${name}.gcc.O2.out") || { echo "FAIL $name: gcc -O2 run"; fail=1; continue; }

    match=ok
    if ! cmp -s "$WORK/${name}.fcc.O0.out" "$WORK/${name}.gcc.O0.out" \
        || ! cmp -s "$WORK/${name}.fcc.O1.out" "$WORK/${name}.gcc.O0.out" \
        || ! cmp -s "$WORK/${name}.gcc.O1.out" "$WORK/${name}.gcc.O0.out" \
        || ! cmp -s "$WORK/${name}.gcc.O2.out" "$WORK/${name}.gcc.O0.out"; then
        match=MISMATCH
        fail=1
        echo "  fakecc -O0: $(tr -d '\n' < "$WORK/${name}.fcc.O0.out")"
        echo "  fakecc -O1: $(tr -d '\n' < "$WORK/${name}.fcc.O1.out")"
        echo "  gcc    -O0: $(tr -d '\n' < "$WORK/${name}.gcc.O0.out")"
        echo "  gcc    -O1: $(tr -d '\n' < "$WORK/${name}.gcc.O1.out")"
        echo "  gcc    -O2: $(tr -d '\n' < "$WORK/${name}.gcc.O2.out")"
    fi

    r0=$(python3 -c "print('%.2f' % ($fcc0 / $gcc0 if $gcc0 else 0))")
    r1=$(python3 -c "print('%.2f' % ($fcc1 / $gcc1 if $gcc1 else 0))")
    echo "  $name  fakecc -O0 ${fcc0} ms / gcc -O0 ${gcc0} ms  (${r0}x)"
    echo "         fakecc -O1 ${fcc1} ms / gcc -O1 ${gcc1} ms  (${r1}x)  gcc -O2 ${gcc2} ms  $match"

    rows+=("$name $fcc0 $gcc0 $r0 $fcc1 $gcc1 $r1 $gcc2 $match")
    gmean0_fcc=$(python3 -c "print($gmean0_fcc * $fcc0)")
    gmean0_gcc=$(python3 -c "print($gmean0_gcc * $gcc0)")
    gmean1_fcc=$(python3 -c "print($gmean1_fcc * $fcc1)")
    gmean1_gcc=$(python3 -c "print($gmean1_gcc * $gcc1)")
    nbench=$((nbench + 1))
done

echo
echo "------------------------------------------------------------------------"
printf '%-8s %10s %10s %7s %10s %10s %7s %10s %s\n' \
    bench 'fcc -O0' 'gcc -O0' 'O0×' 'fcc -O1' 'gcc -O1' 'O1×' 'gcc -O2' match
echo "------------------------------------------------------------------------"
for row in "${rows[@]+"${rows[@]}"}"; do
    set -- $row
    printf '%-8s %8s ms %8s ms %6sx %8s ms %8s ms %6sx %8s ms %s\n' \
        "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9"
done
echo "------------------------------------------------------------------------"
if [ "$nbench" -gt 0 ]; then
    gm0=$(python3 -c "print('%.2f' % (($gmean0_fcc)**(1.0/$nbench) / ($gmean0_gcc)**(1.0/$nbench)))")
    gm1=$(python3 -c "print('%.2f' % (($gmean1_fcc)**(1.0/$nbench) / ($gmean1_gcc)**(1.0/$nbench)))")
    echo "geometric mean  fakecc/gcc  -O0: ${gm0}x   -O1: ${gm1}x"
    echo "(>1 means fakecc's binary is slower)"
fi

if [ "$fail" -ne 0 ]; then
    echo "run_bench: FAILED (compile/run error or checksum mismatch)"
    exit 1
fi
echo "run_bench: outputs match; ratios are informational"
exit 0
