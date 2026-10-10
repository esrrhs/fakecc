#!/usr/bin/env bash
# Differential fuzzing: random C programs are compiled with gcc (oracle) and
# fakecc (-O0 and -O1); stdout must match.
#
#   run_fuzz.sh [FAKECC] [NUM_SEEDS] [FIRST_SEED]
#
# test/fuzz/known_failures/seed<N>.{gcc,fcc}.c hold programs that currently
# miscompile.  They are reported as XFAIL and do not fail the run; when one
# starts to pass it is reported as XPASS (delete the pair).  A mismatch on any
# freshly generated seed fails the run.
set -uo pipefail
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
FAKECC=${1:-"$ROOT/build/fakecc"}
NUM=${2:-${FUZZ_SEEDS:-300}}
FIRST=${3:-${FUZZ_FIRST_SEED:-1}}
JOBS=${JOBS:-$(nproc 2>/dev/null || echo 4)}
DIR="$ROOT/test/fuzz"
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
export FAKECC DIR WORK

# check_pair NAME GCC_SRC FCC_SRC -> prints "OK|FAIL|CCFAIL <name> <opt>" lines
check_pair() {
    local name=$1 g=$2 f=$3 b="$WORK/$1"
    gcc -w -fwrapv "$g" -o "$b.gcc" 2>"$b.gcc.err" || { echo "SKIP $name (gcc rejected)"; return 0; }
    timeout 10 "$b.gcc" >"$b.gcc.out" 2>/dev/null || true
    local rc=0 opt
    for opt in -O0 -O1; do
        if ! timeout 60 "$FAKECC" $opt "$f" -o "$b.fcc$opt" 2>"$b.err$opt"; then
            echo "FAIL $name $opt (fakecc failed to compile: $(head -1 "$b.err$opt"))"; rc=1; continue
        fi
        timeout 10 "$b.fcc$opt" >"$b.out$opt" 2>/dev/null || true
        if ! cmp -s "$b.gcc.out" "$b.out$opt"; then
            echo "FAIL $name $opt (output differs from gcc)"; rc=1
        fi
    done
    [ $rc = 0 ] && echo "PASS $name"
    return 0
}
export -f check_pair

run_seed() {
    local s=$1
    [ -e "$DIR/known_failures/seed$s.gcc.c" ] && return 0
    python3 "$DIR/gen_random.py" "$s" gcc >"$WORK/seed$s.g.c"
    python3 "$DIR/gen_random.py" "$s" fakecc >"$WORK/seed$s.f.c"
    check_pair "seed$s" "$WORK/seed$s.g.c" "$WORK/seed$s.f.c"
}
export -f run_seed

echo "--- fuzz: seeds $FIRST..$((FIRST + NUM - 1)) ---"
seq "$FIRST" $((FIRST + NUM - 1)) | xargs -P "$JOBS" -n 1 bash -c 'run_seed "$@"' _ >"$WORK/results.log"
sort "$WORK/results.log" | grep -v '^PASS' || true
fail=$(grep -c '^FAIL' "$WORK/results.log")
pass=$(grep -c '^PASS' "$WORK/results.log")

echo "--- known failures ---"
xfail=0; xpass=0
for g in "$DIR"/known_failures/*.gcc.c; do
    [ -e "$g" ] || continue
    n=$(basename "$g" .gcc.c)
    out=$(check_pair "kf_$n" "$g" "${g%.gcc.c}.fcc.c")
    if echo "$out" | grep -q '^FAIL'; then
        echo "XFAIL $n"; xfail=$((xfail + 1))
    else
        echo "XPASS $n (now matches gcc; remove from known_failures)"; xpass=$((xpass + 1))
    fi
done
echo "--- fuzz: $pass passed, $fail failed, $xfail known-failing, $xpass fixed ---"
[ "$fail" = 0 ]
