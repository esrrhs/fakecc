package main;
// Stepping needs distinct code offsets mapped to distinct lines; a collapsed
// line table makes `next` jump over several statements at once.
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: next
// gdb: next
// gdb: print b
// gdb: continue
// gdb_expect: b = a \* 2;
// gdb_expect: c = b \+ 22;
// gdb_expect: \$1 = 20
int main(void) {
    int a;
    int b;
    int c;
    a = 10;         // BRK
    b = a * 2;
    c = b + 22;
    return c;
}
