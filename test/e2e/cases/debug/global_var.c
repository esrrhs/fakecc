package main;
// Globals are described by absolute address, resolved from the symbol table at
// link time rather than relative to any frame — so they stay readable from any
// function, including before main's frame exists.
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: print counter
// gdb: print counter * 2
// gdb: print &counter != 0
// gdb: continue
// gdb_expect: \$1 = 21
// gdb_expect: \$2 = 42
// gdb_expect: \$3 = 1
int counter = 21;
int main(void) {
    int local;
    local = counter * 2;
    return local;   // BRK
}
