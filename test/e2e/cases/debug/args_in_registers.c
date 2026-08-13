package main;
// Parameters at the point they are used.  At -O1 they are still in their
// arrival registers and are described by a location list; at -O0 they live in
// stack slots.  Both must read back the caller's values.
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: info args
// gdb: print a
// gdb: print b
// gdb: continue
// gdb_expect: a = 40
// gdb_expect: b = 2
// gdb_expect: \$1 = 40
// gdb_expect: \$2 = 2
// gdb_reject: optimized out
int add(int a, int b) {
    int s;
    s = a + b;   // BRK
    return s;
}
int main(void) {
    return add(40, 2);
}
