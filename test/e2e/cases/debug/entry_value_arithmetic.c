package main;
// A recursive call whose argument is an expression of the parameter
// (countdown(n - 1)).  gdb recovers each frame's parameter via the call
// site's DW_AT_call_value (DW_OP_entry_value).  Every frame must report the
// correct n@entry — computed from the outer frame's entry value, not the
// live home (which is clobbered by the recursive call).
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: backtrace
// gdb: info args
// gdb: continue
// gdb_expect: #0 +countdown \(n=(n@entry=)?0\) at
// gdb_expect: #1 +0x[0-9a-f]+ in countdown \(n=(n@entry=)?1\) at
// gdb_expect: #2 +0x[0-9a-f]+ in countdown \(n=(n@entry=)?2\) at
// gdb_expect: #3 +0x[0-9a-f]+ in countdown \(n=(n@entry=)?3\) at
// gdb_expect: #4 +0x[0-9a-f]+ in main \(\) at
// gdb_expect: n = 0
// gdb_reject: <error
int countdown(int n) {
    if (n == 0) {
        return 42;   // BRK
    }
    return countdown(n - 1);
}
int main(void) {
    return countdown(3);
}
