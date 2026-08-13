package main;
// A variable updated in a loop has several definitions merged at the loop
// header, so its home changes as the loop runs.  This is exactly what a single
// DW_OP_fbreg cannot express and location lists exist for: stop on the fifth
// iteration and both the counter and the accumulator must read correctly.
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: continue 4
// gdb: print i
// gdb: print sum
// gdb: delete 1
// gdb: continue
// gdb_expect: \$1 = 4
// gdb_expect: \$2 = 12
// gdb_reject: optimized out
int main(void) {
    int i;
    int sum;
    sum = 0;
    i = 0;
    while (i < 7) {
        sum = sum + i * 2;   // BRK
        i = i + 1;
    }
    return sum;
}
