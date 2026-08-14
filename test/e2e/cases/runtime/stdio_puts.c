// runtime.puts: writes a string followed by a newline to runtime.stdout.  Pin that it
// returns non-negative and that the string round-trips.  The exact line
// is also checked via expect_stdout.
// expect: 0
// expect_stdout: hello world
package main;
import runtime;
int main() {
    int r = runtime.puts("hello world");
    if (r < 0) return 1;
    return 0;
}
