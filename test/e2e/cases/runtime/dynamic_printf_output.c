// expect: 0
// expect_stdout: hello 42
// expect_stdout: second line
// runtime.printf must actually write, not just return a plausible count.
//
// Returning from main is not the same as calling exit(): libc buffers stdout
// and drains it from an atexit handler.  The entry stub used to issue a raw
// exit_group syscall, which skips that handler, so every byte runtime.printf() had
// buffered was discarded.  dynamic_printf.c did not catch it because it only
// checks runtime.printf's return value.
package main;
import runtime;
int main(void) {
    int n = runtime.printf("hello %d\n", 42);
    if (n != 9) return 1;
    runtime.printf("second line\n");
    return 0;
}
