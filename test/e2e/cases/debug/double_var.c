package main;
// Float locals are allocated out of the XMM register file, so their DWARF
// register numbers come from a different bank than integers (xmm0 is DWARF
// 17).  Getting the bank wrong prints an unrelated integer register.
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: print d
// gdb: print d * 2
// gdb: continue
// gdb_expect: \$1 = 21
// gdb_expect: \$2 = 42
int main(void) {
    double d;
    int r;
    d = 21.0;
    r = (int)(d * 2.0);
    return r;   // BRK
}
