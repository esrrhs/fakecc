package main;
// Arrays keep a stack home and carry their element count, so a debugger can
// index them and print the whole object.
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: print a
// gdb: print a[0]
// gdb: print a[3]
// gdb: print sum
// gdb: continue
// gdb_expect: \$1 = \{6, 9, 12, 15\}
// gdb_expect: \$2 = 6
// gdb_expect: \$3 = 15
// gdb_expect: \$4 = 42
int main(void) {
    int a[4];
    int sum;
    a[0] = 6;
    a[1] = 9;
    a[2] = 12;
    a[3] = 15;
    sum = a[0] + a[1] + a[2] + a[3];
    return sum;   // BRK
}
