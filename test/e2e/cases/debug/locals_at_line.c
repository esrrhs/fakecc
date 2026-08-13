package main;
// Break on a source line and read locals that the line still uses.  This is
// the baseline for -g being useful at all.
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: print a
// gdb: print b
// gdb: print a + b
// gdb: continue
// gdb_expect: Breakpoint 1, main \(\) at .*locals_at_line\.c
// gdb_expect: \$1 = 40
// gdb_expect: \$2 = 2
// gdb_expect: \$3 = 42
// gdb_reject: optimized out
int main(void) {
    int a;
    int b;
    a = 40;
    b = 2;
    return a + b;   // BRK
}
