package main;
// Narrow and unsigned types must carry their width and signedness, or a
// debugger reads the right bytes and prints the wrong value.  `c` and `sh`
// are parameters (register-resident at -O1) and `u` is a local, so both
// location paths are covered in one stop.
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: print c
// gdb: print sh
// gdb: print u
// gdb: print sizeof(c)
// gdb: print sizeof(sh)
// gdb: print sizeof(u)
// gdb: continue
// gdb_expect: \$1 = 65 'A'
// gdb_expect: \$2 = -3
// gdb_expect: \$3 = 4000000000
// gdb_expect: \$4 = 1
// gdb_expect: \$5 = 2
// gdb_expect: \$6 = 4
int narrow(char c, short sh) {
    unsigned int u;
    u = 4000000000;
    return (int)c + (int)sh - (int)(u / 200000000);   // BRK
}
int main(void) {
    return narrow(65, -3);
}
