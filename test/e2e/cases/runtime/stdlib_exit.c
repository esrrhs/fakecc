// runtime.exit: terminates the process after flushing buffered stdout.  The
// freestanding entry stub drains stdout via exit() rather than a raw syscall,
// so a buffered printf must be flushed before the process ends.  Pin that the
// exit code propagates (exit() calls exit_group, not a raw stub syscall).
// expect: 7
// expect_stdout: exiting
package main;
import runtime;
int main(void) {
    runtime.printf("exiting\n");   /* buffered: exit()'s fflush drains it */
    runtime.exit(7);
    return 0; /* unreachable */
}
