// expect: 0
// expect_stdout: hello 42
// expect_stdout: second line
// printf must actually write, not just return a plausible count.
//
// Returning from main is not the same as calling exit(): libc buffers stdout
// and drains it from an atexit handler.  The entry stub used to issue a raw
// exit_group syscall, which skips that handler, so every byte printf() had
// buffered was discarded.  dynamic_printf.c did not catch it because it only
// checks printf's return value.
package main;
extern int printf(const char *fmt, ...);
int main(void) {
    int n = printf("hello %d\n", 42);
    if (n != 9) return 1;
    printf("second line\n");
    return 0;
}
