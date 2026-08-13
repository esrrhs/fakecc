package main;
// A four-deep call chain: .debug_frame must let a debugger unwind every frame,
// not just the innermost one, and each frame must resolve to a source line.
// expect: 42
// gdb: break {brk}
// gdb: run
// gdb: backtrace
// gdb: continue
// gdb_expect: #0 +inner \(v=39\) at
// gdb_expect: #1 +0x[0-9a-f]+ in middle \(v=39\) at
// gdb_expect: #2 +0x[0-9a-f]+ in outer \(v=39\) at
// gdb_expect: #3 +0x[0-9a-f]+ in main \(\) at
// gdb_reject: Backtrace stopped
// gdb_reject: \?\? \(\)
int inner(int v) {
    return v + 1;   // BRK
}
int middle(int v) { return inner(v) + 1; }
int outer(int v) { return middle(v) + 1; }
int main(void) {
    return outer(39);
}
