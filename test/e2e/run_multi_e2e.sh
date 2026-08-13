#!/usr/bin/env bash
# Multi-file compile + link end-to-end tests.
set -uo pipefail

FAKECC=${1:-./build/fakecc}
CC_TIMEOUT=${CC_TIMEOUT:-30}
RUN_TIMEOUT=${RUN_TIMEOUT:-10}
FAIL=0
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

run_multi() {
    local expect="$1"; shift
    local out="$TMP/prog"
    rm -f "$out"
    local cc_rc=0
    timeout "$CC_TIMEOUT" "$FAKECC" "$@" -o "$out" 2>"$TMP/cc.err" || cc_rc=$?
    if [ "$cc_rc" != "0" ]; then
        echo "FAIL multi $* (compile exited $cc_rc: $(head -1 "$TMP/cc.err"))"
        FAIL=1
        return
    fi
    local got=0
    timeout "$RUN_TIMEOUT" "$out" >/dev/null 2>&1 || got=$?
    if [ "$got" = "124" ]; then
        echo "FAIL multi $* (program timed out)"
        FAIL=1
    elif [ "$got" = "$expect" ]; then
        echo "PASS multi $* (expect $expect)"
    else
        echo "FAIL multi $* (expected $expect, got $got)"
        FAIL=1
    fi
}

# Expect the compiler/linker to reject the inputs.
run_multi_fail() {
    local out="$TMP/prog"
    rm -f "$out"
    local cc_rc=0
    timeout "$CC_TIMEOUT" "$FAKECC" "$@" -o "$out" 2>"$TMP/cc.err" || cc_rc=$?
    if [ "$cc_rc" = "0" ]; then
        echo "FAIL multi-fail $* (expected link/compile error, but succeeded)"
        FAIL=1
    elif [ "$cc_rc" -ge 124 ]; then
        echo "FAIL multi-fail $* (compiler timed out / killed: $cc_rc)"
        FAIL=1
    else
        echo "PASS multi-fail $* (rejected)"
    fi
}

# Cross-file function call.
echo 'package main; int add(int a, int b) { return a + b; }' > "$TMP/a.c"
echo 'package main; int add(int a, int b); int main() { return add(10, 32); }' > "$TMP/b.c"
run_multi 42 "$TMP/a.c" "$TMP/b.c"

# Cross-file global variable (extern declaration in the using TU).
echo 'package main; int g = 7;' > "$TMP/g.c"
echo 'package main; extern int g; int main() { return g + 3; }' > "$TMP/m.c"
run_multi 10 "$TMP/g.c" "$TMP/m.c"

# Static symbols with same name in different files do not collide.
echo 'package main; static int x = 10; int get_x() { return x; }' > "$TMP/s1.c"
echo 'package main; static int x = 20; int get_x(); int main() { return get_x(); }' > "$TMP/s2.c"
run_multi 10 "$TMP/s1.c" "$TMP/s2.c"

# Two-stage: compile to .o then link.
echo 'package main; int mul(int a, int b) { return a * b; }' > "$TMP/mul.c"
echo 'package main; int mul(int a, int b); int main() { return mul(6, 7); }' > "$TMP/main.c"
"$FAKECC" -c "$TMP/mul.c" -o "$TMP/mul.o"
"$FAKECC" -c "$TMP/main.c" -o "$TMP/main.o"
run_multi 42 "$TMP/mul.o" "$TMP/main.o"

# Cross-file libc call (PLT).  (`return;` required — void functions without
# an explicit return hit a pre-existing codegen bug, unrelated to linking.)
echo 'package main; extern int printf(const char *f,...); void hi(){ printf("x"); return; }' > "$TMP/p.c"
echo 'package main; void hi(); int main(){ hi(); return 5; }' > "$TMP/q.c"
run_multi 5 "$TMP/p.c" "$TMP/q.c"

# Three translation units: leaf → mid → main.
echo 'package main; int leaf(void) { return 2; }' > "$TMP/t1.c"
echo 'package main; int leaf(void); int mid(void) { return leaf() + 3; }' > "$TMP/t2.c"
echo 'package main; int mid(void); int main(void) { return mid() * 7; }' > "$TMP/t3.c"
run_multi 35 "$TMP/t1.c" "$TMP/t2.c" "$TMP/t3.c"

# Cross-file global mutation: writer TU updates, reader TU observes.
echo 'package main; int g; void bump(void) { g = g + 1; return; }' > "$TMP/w.c"
echo 'package main; extern int g; void bump(void); int main(void) { g = 40; bump(); bump(); return g; }' > "$TMP/r.c"
run_multi 42 "$TMP/w.c" "$TMP/r.c"

# Mixed .o + .c link.
echo 'package main; int twice(int x) { return x + x; }' > "$TMP/twice.c"
echo 'package main; int twice(int x); int main(void) { return twice(21); }' > "$TMP/use_twice.c"
"$FAKECC" -c "$TMP/twice.c" -o "$TMP/twice.o"
run_multi 42 "$TMP/twice.o" "$TMP/use_twice.c"

# Two static helpers with the same local name, both called from main.
echo 'package main; static int helper(void) { return 3; } int left(void) { return helper(); }' > "$TMP/st_a.c"
echo 'package main; static int helper(void) { return 4; } int right(void) { return helper(); }' > "$TMP/st_b.c"
echo 'package main; int left(void); int right(void); int main(void) { return left() * 10 + right(); }' > "$TMP/st_m.c"
run_multi 34 "$TMP/st_a.c" "$TMP/st_b.c" "$TMP/st_m.c"

# Negative: no main → linker must reject.
echo 'package main; int foo(void) { return 1; }' > "$TMP/nomain.c"
run_multi_fail "$TMP/nomain.c"

# Negative: two object files, still no main.
echo 'package main; int a(void) { return 1; }' > "$TMP/nm1.c"
echo 'package main; int b(void) { return 2; }' > "$TMP/nm2.c"
"$FAKECC" -c "$TMP/nm1.c" -o "$TMP/nm1.o"
"$FAKECC" -c "$TMP/nm2.c" -o "$TMP/nm2.o"
run_multi_fail "$TMP/nm1.o" "$TMP/nm2.o"

exit $FAIL
