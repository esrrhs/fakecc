#!/usr/bin/env bash
# Multi-file compile + link end-to-end tests.
set -uo pipefail

FAKECC=${1:-./build/fakecc}
shift || true
# Extra compiler flags (e.g. -O0) so the suite can run once per opt level.
CC_EXTRA="${CC_FLAGS:-} $*"
CC_TIMEOUT=${CC_TIMEOUT:-30}
RUN_TIMEOUT=${RUN_TIMEOUT:-10}
FAIL=0
SUITE_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
# Package fixtures for import tests (same role as in run_e2e.sh).
export FAKECC_PKG="${SUITE_DIR}/pkg_fixtures${FAKECC_PKG:+:$FAKECC_PKG}"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

run_multi() {
    local expect="$1"; shift
    local out="$TMP/prog"
    rm -f "$out"
    local cc_rc=0
    timeout "$CC_TIMEOUT" "$FAKECC" $CC_EXTRA "$@" -o "$out" 2>"$TMP/cc.err" || cc_rc=$?
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
    timeout "$CC_TIMEOUT" "$FAKECC" $CC_EXTRA "$@" -o "$out" 2>"$TMP/cc.err" || cc_rc=$?
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
"$FAKECC" $CC_EXTRA -c "$TMP/mul.c" -o "$TMP/mul.o"
"$FAKECC" $CC_EXTRA -c "$TMP/main.c" -o "$TMP/main.o"
run_multi 42 "$TMP/mul.o" "$TMP/main.o"

# Cross-file libc call (PLT).  (`return;` required — void functions without
# an explicit return hit a pre-existing codegen bug, unrelated to linking.)
echo 'package main; import runtime; void hi(){ runtime.printf("x"); return; }' > "$TMP/p.c"
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
"$FAKECC" $CC_EXTRA -c "$TMP/twice.c" -o "$TMP/twice.o"
run_multi 42 "$TMP/twice.o" "$TMP/use_twice.c"

# SysV small-struct interop: gcc callee + fakecc caller.
echo 'struct S { int x; int y; }; int sum(struct S s) { return s.x + s.y; }' > "$TMP/gcc_sum.c"
echo 'package main; struct S { int x; int y; }; int sum(struct S s); int main(void) { struct S p; p.x = 3; p.y = 4; return sum(p); }' > "$TMP/fc_main_sum.c"
gcc -std=c99 -c "$TMP/gcc_sum.c" -o "$TMP/gcc_sum.o"
run_multi 7 "$TMP/fc_main_sum.c" "$TMP/gcc_sum.o"

# SysV MEMORY-class (>16) interop: gcc callee + fakecc caller.
echo 'struct Big { long a; long b; long c; }; long sum3(struct Big s) { return s.a + s.b + s.c; }' > "$TMP/gcc_big.c"
echo 'package main; struct Big { long a; long b; long c; }; long sum3(struct Big s); int main(void) { struct Big s; s.a = 1; s.b = 2; s.c = 4; return (int)sum3(s); }' > "$TMP/fc_main_big.c"
gcc -std=c99 -c "$TMP/gcc_big.c" -o "$TMP/gcc_big.o"
run_multi 7 "$TMP/fc_main_big.c" "$TMP/gcc_big.o"

# SysV small-struct return interop: gcc callee returns in RAX.
echo 'struct S { int a; int b; }; struct S make(int x, int y) { struct S s; s.a = x; s.b = y; return s; }' > "$TMP/gcc_make.c"
echo 'package main; struct S { int a; int b; }; struct S make(int x, int y); int main(void) { struct S s = make(3, 4); return s.a + s.b; }' > "$TMP/fc_main_make.c"
gcc -std=c99 -c "$TMP/gcc_make.c" -o "$TMP/gcc_make.o"
run_multi 7 "$TMP/fc_main_make.c" "$TMP/gcc_make.o"

# Two static helpers with the same local name, both called from main.
echo 'package main; static int helper(void) { return 3; } int left(void) { return helper(); }' > "$TMP/st_a.c"
echo 'package main; static int helper(void) { return 4; } int right(void) { return helper(); }' > "$TMP/st_b.c"
echo 'package main; int left(void); int right(void); int main(void) { return left() * 10 + right(); }' > "$TMP/st_m.c"
run_multi 34 "$TMP/st_a.c" "$TMP/st_b.c" "$TMP/st_m.c"

# Same-package multi-file: no `extern` — siblings are visible via package scope.
echo 'package main; int add(int a, int b) { return a + b; }' > "$TMP/pkg_a.c"
echo 'package main; int main(void) { return add(20, 22); }' > "$TMP/pkg_b.c"
run_multi 42 "$TMP/pkg_a.c" "$TMP/pkg_b.c"

# Same-package typedef from an earlier CLI file must be visible at parse time.
echo 'package main; typedef int my_i; my_i val(void) { return 7; }' > "$TMP/td_a.c"
echo 'package main; my_i val(void); int main(void) { return val(); }' > "$TMP/td_b.c"
run_multi 7 "$TMP/td_a.c" "$TMP/td_b.c"

# Command-line files that reuse the builtin `runtime` package name still see each
# other (exports are merged into the preloaded package, not dropped).
echo 'package runtime; int helper(void) { return 40; }' > "$TMP/rt_a.c"
echo 'package runtime; int main(void) { return helper() + 2; }' > "$TMP/rt_b.c"
run_multi 42 "$TMP/rt_a.c" "$TMP/rt_b.c"

# Negative: no main → linker must reject.
echo 'package main; int foo(void) { return 1; }' > "$TMP/nomain.c"
run_multi_fail "$TMP/nomain.c"

# Negative: two object files, still no main.
echo 'package main; int a(void) { return 1; }' > "$TMP/nm1.c"
echo 'package main; int b(void) { return 2; }' > "$TMP/nm2.c"
"$FAKECC" $CC_EXTRA -c "$TMP/nm1.c" -o "$TMP/nm1.o"
"$FAKECC" $CC_EXTRA -c "$TMP/nm2.c" -o "$TMP/nm2.o"
run_multi_fail "$TMP/nm1.o" "$TMP/nm2.o"

# --- User-package imports.  `import` now auto-links the package's code: only
# the user's own source is passed on the command line.  FAKECC_PKG points at the
# fixtures directory so `import <name>` resolves. ---

# Import a user package and call its functions (calc.c auto-linked).
echo 'package main; import calc; int main(void) { return calc.add(2, 3) + calc.mul(6, 7) - 47; }' > "$TMP/up_main.c"
run_multi 0 "$TMP/up_main.c"

# Import a package's typedef'd type, use it as a local, pass it by value.
echo 'package main; import point; int main(void) { point.Pt p; p.x = 3; p.y = 4; return point.sum(p) - 7; }' > "$TMP/upt_main.c"
run_multi 0 "$TMP/upt_main.c"

# Import a multi-file package: one import must expose both files' exports.
echo 'package main; import vec; int main(void) { return vec.scale(5) + vec.add(10, 20) - 45; }' > "$TMP/upm_main.c"
run_multi 0 "$TMP/upm_main.c"

# Negative: directory whose files disagree on the package name.
echo 'package main; import dup; int main(void) { return 0; }' > "$TMP/updup_main.c"
run_multi_fail "$TMP/updup_main.c"

exit $FAIL
