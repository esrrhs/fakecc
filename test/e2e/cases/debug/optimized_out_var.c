package main;
// A variable the optimizer deleted must be reported as unavailable, never as a
// stale stack slot: printing leftover bytes as a live value is worse than
// admitting the value is gone.  At -O0 nothing is deleted, so the same case
// also checks the variable is readable there.
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: print kept
// gdb: continue
// gdb_expect: \$1 = 42
int main(void) {
    int kept;
    int dropped;
    dropped = 99;
    kept = 42;
    return kept;   // BRK
}
