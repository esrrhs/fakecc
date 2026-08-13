package main;
// Each recursive frame must appear separately in the backtrace: unwinding
// four frames of the same function is what .debug_frame is for, and the frame
// chain must not collapse or stop early.
//
// Per-frame parameter values are recovered at every -O level via
// DW_OP_entry_value + DW_TAG_call_site_parameter.  gdb may print them as
// `n=n@entry=N` when the live home was recovered from the entry value.
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: backtrace
// gdb: info args
// gdb: continue
// gdb_expect: #0 +countdown \(n=(n@entry=)?0\) at
// gdb_expect: #1 +0x[0-9a-f]+ in countdown \(n=(n@entry=)?1\) at
// gdb_expect: #3 +0x[0-9a-f]+ in countdown \(n=(n@entry=)?3\) at
// gdb_expect: #4 +0x[0-9a-f]+ in main \(\) at
// gdb_expect: n = 0
// gdb_reject: Backtrace stopped
// gdb_reject: optimized out
int countdown(int n) {
    if (n == 0) {
        return 42;   // BRK
    }
    return countdown(n - 1);
}
int main(void) {
    return countdown(3);
}
