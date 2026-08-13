package main;
// Taking a local's address must keep it in memory: a promoted value has no
// address for the pointer to hold, and a debugger has to be able to read
// through the pointer as well as write to the variable.
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: print v
// gdb: print *p
// gdb: print p == &v
// gdb: continue
// gdb_expect: \$1 = 42
// gdb_expect: \$2 = 42
// gdb_expect: \$3 = 1
int main(void) {
    int v;
    int *p;
    v = 42;
    p = &v;
    return *p;   // BRK
}
