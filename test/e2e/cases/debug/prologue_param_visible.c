package main;
// At -O0 a parameter arrives in a register and is spilled to a stack slot
// during the prologue.  A debugger can stop inside the prologue (before the
// spill), so the parameter must be described by a location list — register
// during the prologue, stack slot after.  Break on the function's first
// statement and read the parameter back; it must show the caller's value.
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: info args
// gdb: print v
// gdb: continue
// gdb_expect: v = 21
// gdb_expect: \$1 = 21
// gdb_reject: optimized out
// gdb_reject: <error
int scale(int v) {
    return v * 2;   // BRK
}
int main(void) {
    return scale(21);
}
