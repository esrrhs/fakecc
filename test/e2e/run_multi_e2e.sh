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

pass() { echo "PASS $*"; }
fail() { echo "FAIL $*"; FAIL=1; }

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

# Package Pre-scan (P3) test: multi-file imported package where file A uses struct/typedef defined in file B.
mkdir -p "$TMP/pkg/prescantest"
echo 'package prescantest; typedef int prescan_int; prescan_int get_val(void) { return 42; }' > "$TMP/pkg/prescantest/b.c"
echo 'package prescantest; int get_val(void);' > "$TMP/pkg/prescantest/a.c"
echo 'package main; import prescantest; int main(void) { return prescantest.get_val(); }' > "$TMP/ps_main.c"
FAKECC_PKG="$TMP/pkg:$FAKECC_PKG" run_multi 42 "$TMP/ps_main.c"

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

# Negative: 3-level cyclic package import (cycA -> cycB -> cycC -> cycA)
mkdir -p "$TMP/pkg/cycA" "$TMP/pkg/cycB" "$TMP/pkg/cycC"
echo 'package cycA; import cycB; int a_func(void) { return cycB.b_func(); }' > "$TMP/pkg/cycA/a.c"
echo 'package cycB; import cycC; int b_func(void) { return cycC.c_func(); }' > "$TMP/pkg/cycB/b.c"
echo 'package cycC; import cycA; int c_func(void) { return cycA.a_func(); }' > "$TMP/pkg/cycC/c.c"
echo 'package main; import cycA; int main(void) { return cycA.a_func(); }' > "$TMP/cyc_main.c"
FAKECC_PKG="$TMP/pkg:$FAKECC_PKG" run_multi_fail "$TMP/cyc_main.c"

# Cross-file TLS: file 1 defines __thread, file 2 accesses it
echo 'package main; __thread int tls_counter; int inc_tls(int v) { tls_counter += v; return tls_counter; }' > "$TMP/tls1.c"
echo 'package main; extern int inc_tls(int v); int main(void) { inc_tls(10); return inc_tls(5) - 15; }' > "$TMP/tls_main.c"
run_multi 0 "$TMP/tls1.c" "$TMP/tls_main.c"

# Separate compilation (-c) with TLS object file
"$FAKECC" $CC_EXTRA -c "$TMP/tls1.c" -o "$TMP/tls1.o"
"$FAKECC" $CC_EXTRA -c "$TMP/tls_main.c" -o "$TMP/tls_main.o"
run_multi 0 "$TMP/tls1.o" "$TMP/tls_main.o"

# TLS Initial-Exec in .o: GOTTPOFF, not Local-Exec TPOFF32
if LANG=C readelf -rW "$TMP/tls1.o" 2>/dev/null | grep -q 'R_X86_64_GOTTPOFF'; then
    pass "TLS .o has GOTTPOFF"
else
    fail "TLS .o missing GOTTPOFF: $(LANG=C readelf -rW "$TMP/tls1.o" 2>/dev/null | head -20)"
fi
if LANG=C readelf -rW "$TMP/tls1.o" 2>/dev/null | grep -q 'R_X86_64_TPOFF32'; then
    fail "TLS .o still has TPOFF32"
else
    pass "TLS .o has no TPOFF32"
fi

# Pointer initializer in __thread storage: .rela.tdata must round-trip through -c
echo 'package main; __thread const char *tls_msg = "xy"; const char *get_tls_msg(void) { return tls_msg; }' > "$TMP/tls_ptr.c"
echo 'package main; extern const char *get_tls_msg(void); int main(void) { const char *p = get_tls_msg(); if (p[0] != '"'"'x'"'"') return 1; if (p[1] != '"'"'y'"'"') return 2; return 0; }' > "$TMP/tls_ptr_main.c"
run_multi 0 "$TMP/tls_ptr.c" "$TMP/tls_ptr_main.c"
"$FAKECC" $CC_EXTRA -c "$TMP/tls_ptr.c" -o "$TMP/tls_ptr.o"
"$FAKECC" $CC_EXTRA -c "$TMP/tls_ptr_main.c" -o "$TMP/tls_ptr_main.o"
if LANG=C readelf -S "$TMP/tls_ptr.o" 2>/dev/null | grep -q '\.rela\.tdata'; then
    pass "TLS pointer .o has .rela.tdata"
else
    fail "TLS pointer .o missing .rela.tdata"
fi
if LANG=C readelf -rW "$TMP/tls_ptr.o" 2>/dev/null | grep -A20 '\.rela.tdata' | grep -q '__str'; then
    pass "TLS pointer .o .rela.tdata relocates string"
else
    fail "TLS pointer .o .rela.tdata missing string reloc: $(LANG=C readelf -rW "$TMP/tls_ptr.o" 2>/dev/null)"
fi
run_multi 0 "$TMP/tls_ptr.o" "$TMP/tls_ptr_main.o"

# static __thread in two TUs must not share an IE GOT slot (same name, distinct vars)
echo 'package main; static __thread int x = 3; int get_a(void) { return x; } void set_a(int v) { x = v; }' > "$TMP/tls_la.c"
echo 'package main; static __thread int x = 9; int get_b(void) { return x; }' > "$TMP/tls_lb.c"
echo 'package main; extern int get_a(void); extern int get_b(void); extern void set_a(int); int main(void) { if (get_a() != 3) return 1; if (get_b() != 9) return 2; set_a(5); if (get_a() != 5) return 3; if (get_b() != 9) return 4; return 0; }' > "$TMP/tls_lm.c"
run_multi 0 "$TMP/tls_la.c" "$TMP/tls_lb.c" "$TMP/tls_lm.c"
"$FAKECC" $CC_EXTRA -c "$TMP/tls_la.c" -o "$TMP/tls_la.o"
"$FAKECC" $CC_EXTRA -c "$TMP/tls_lb.c" -o "$TMP/tls_lb.o"
"$FAKECC" $CC_EXTRA -c "$TMP/tls_lm.c" -o "$TMP/tls_lm.o"
run_multi 0 "$TMP/tls_la.o" "$TMP/tls_lb.o" "$TMP/tls_lm.o"

# ELF R_X86_64_64 in .rela.data (function pointer initializer).  The old
# type-10 encoding was R_X86_64_32 and overflowed gcc PIE / libm.
echo 'package main; int f(void) { return 42; } int (*fp)(void) = f; int main(void) { return fp(); }' > "$TMP/fp_data.c"
"$FAKECC" $CC_EXTRA -c "$TMP/fp_data.c" -o "$TMP/fp_data.o"
if LANG=C readelf -rW "$TMP/fp_data.o" 2>/dev/null | grep -q 'R_X86_64_64'; then
    pass "fnptr .data reloc is R_X86_64_64"
else
    fail "fnptr .data reloc not R_X86_64_64: $(LANG=C readelf -rW "$TMP/fp_data.o" 2>/dev/null | head -20)"
fi
if LANG=C readelf -rW "$TMP/fp_data.o" 2>/dev/null | grep -q 'R_X86_64_32'; then
    fail "fnptr .data still has R_X86_64_32"
else
    pass "fnptr .data has no R_X86_64_32"
fi
if gcc -pie -o "$TMP/fp_pie" "$TMP/fp_data.o" 2>"$TMP/fp_pie.err"; then
    got=0
    timeout "$RUN_TIMEOUT" "$TMP/fp_pie" >/dev/null 2>&1 || got=$?
    if [ "$got" = "42" ]; then
        pass "gcc -pie links fakecc fnptr .o"
    else
        fail "gcc -pie fnptr ran with $got"
    fi
else
    fail "gcc -pie failed: $(head -1 "$TMP/fp_pie.err")"
fi

# .note.GNU-stack in .o; PT_GNU_STACK RW (not executable) in the linked image.
if LANG=C readelf -S "$TMP/fp_data.o" 2>/dev/null | grep -q '\.note\.GNU-stack'; then
    pass ".o has .note.GNU-stack"
else
    fail ".o missing .note.GNU-stack"
fi
echo 'package main; int main(void) { return 7; }' > "$TMP/gs.c"
run_multi 7 "$TMP/gs.c"
if LANG=C readelf -l "$TMP/prog" 2>/dev/null | grep -q 'GNU_STACK'; then
    if LANG=C readelf -l "$TMP/prog" 2>/dev/null | grep 'GNU_STACK' | grep -qE 'RWE|E '; then
        fail "GNU_STACK is executable: $(LANG=C readelf -l "$TMP/prog" | grep GNU_STACK)"
    else
        pass "GNU_STACK is non-executable"
    fi
else
    fail "linked image missing GNU_STACK"
fi

# MEMORY blob interop: fakecc callee + gcc caller (24-byte and 200-byte).
echo 'package main; struct M { long a; long b; long c; }; long sum3(struct M s) { return s.a + s.b + s.c; }' > "$TMP/fc_mem.c"
echo 'struct M { long a; long b; long c; }; long sum3(struct M s); int main(void) { struct M s; s.a = 1; s.b = 2; s.c = 4; return (int)sum3(s); }' > "$TMP/gcc_mem_main.c"
"$FAKECC" $CC_EXTRA -c "$TMP/fc_mem.c" -o "$TMP/fc_mem.o"
gcc -std=c99 -c "$TMP/gcc_mem_main.c" -o "$TMP/gcc_mem_main.o"
run_multi 7 "$TMP/fc_mem.o" "$TMP/gcc_mem_main.o"

echo 'package main; struct Big { unsigned char x[200]; }; int pick(struct Big b, int i) { return b.x[i]; }' > "$TMP/fc_big.c"
echo 'struct Big { unsigned char x[200]; }; int pick(struct Big b, int i); int main(void) { struct Big b; int i; for (i = 0; i < 200; i++) b.x[i] = (unsigned char)i; return pick(b, 199) - 199; }' > "$TMP/gcc_big_main.c"
"$FAKECC" $CC_EXTRA -c "$TMP/fc_big.c" -o "$TMP/fc_big.o"
gcc -std=c99 -c "$TMP/gcc_big_main.c" -o "$TMP/gcc_big_main.o"
run_multi 0 "$TMP/fc_big.o" "$TMP/gcc_big_main.o"

echo 'package main; struct Big { unsigned char x[200]; }; int pick(struct Big b, int i); int main(void) { struct Big b; int i; for (i = 0; i < 200; i++) b.x[i] = (unsigned char)i; return pick(b, 7); }' > "$TMP/fc_big_main.c"
echo 'struct Big { unsigned char x[200]; }; int pick(struct Big b, int i) { return b.x[i]; }' > "$TMP/gcc_pick.c"
gcc -std=c99 -c "$TMP/gcc_pick.c" -o "$TMP/gcc_pick.o"
run_multi 7 "$TMP/fc_big_main.c" "$TMP/gcc_pick.o"

exit $FAIL
