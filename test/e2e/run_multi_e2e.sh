#!/usr/bin/env bash
# Multi-file compile + link end-to-end tests.
set -uo pipefail

FAKECC=${1:-./build/fakecc}
FAIL=0
TMP=$(mktemp -d)

run_multi() {
    local expect="$1"; shift
    local out="$TMP/prog"
    "$FAKECC" "$@" -o "$out"
    local got=0
    "$out" >/dev/null 2>&1 || got=$?
    if [ "$got" = "$expect" ]; then
        echo "PASS multi $* (expect $expect)"
    else
        echo "FAIL multi $* (expected $expect, got $got)"
        FAIL=1
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

rm -rf "$TMP"
exit $FAIL
