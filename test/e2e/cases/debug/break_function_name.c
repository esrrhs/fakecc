package main;
// `break <function>` must land past the prologue with a usable frame.  That
// needs a line-table row marked DW_LNS_set_prologue_end; otherwise a debugger
// decodes the prologue itself and can stop before rbp is set up, where the
// call-frame info would still describe the caller's frame.
// expect: 42
// gdb: break scale
// gdb: run
// gdb: info args
// gdb: backtrace
// gdb: continue
// gdb_expect: Breakpoint 1 at 0x[0-9a-f]+: file .*break_function_name\.c
// gdb_expect: Breakpoint 1, scale \(v=(v@entry=)?21\)
// gdb_expect: v = 21
// gdb_expect: #0 +scale \(v=(v@entry=)?21\) at
// gdb_expect: #1 +0x[0-9a-f]+ in main \(\) at
int scale(int v) {
    return v * 2;
}
int main(void) {
    return scale(21);
}
