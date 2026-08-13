package main;
// Each recursive frame must appear separately in the backtrace: unwinding
// four frames of the same function is what .debug_frame is for, and the frame
// chain must not collapse or stop early.
//
// The per-frame parameter value is only checked at -O0.  At -O1 the parameter
// has been promoted into a register, and recovering a caller's register value
// from an outer frame needs call-site information (DW_OP_entry_value /
// DW_TAG_call_site_parameter) that fakecc does not emit yet, so outer frames
// show the innermost value.
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: backtrace
// gdb: info args
// gdb: continue
// gdb_expect: #0 +countdown \(n=0\) at
// gdb_expect: #1 +0x[0-9a-f]+ in countdown \(n=[0-9]+\) at
// gdb_expect: #3 +0x[0-9a-f]+ in countdown \(n=[0-9]+\) at
// gdb_expect: #4 +0x[0-9a-f]+ in main \(\) at
// gdb_expect: n = 0
// gdb_expect_O0: #1 +0x[0-9a-f]+ in countdown \(n=1\) at
// gdb_expect_O0: #3 +0x[0-9a-f]+ in countdown \(n=3\) at
// gdb_reject: Backtrace stopped
int countdown(int n) {
    if (n == 0) {
        return 42;   // BRK
    }
    return countdown(n - 1);
}
int main(void) {
    return countdown(3);
}
