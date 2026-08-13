package main;
// A local read after its last assignment.  Under -O1 it has been promoted out
// of memory, so its DWARF location is whichever register the allocator chose.
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: print s
// gdb: print s / 2
// gdb: continue
// gdb_expect: \$1 = 42
// gdb_expect: \$2 = 21
// gdb_reject: optimized out
int add(int a, int b) {
    int s;
    s = a + b;
    return s;   // BRK
}
int main(void) {
    return add(40, 2);
}
