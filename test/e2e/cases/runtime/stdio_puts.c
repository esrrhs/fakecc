// io.puts: writes a string followed by a newline to io.stdout.  Pin that it
// returns non-negative and that the string round-trips.  The exact line
// is also checked via expect_stdout.
// expect: 0
// expect_stdout: hello world
package main;
import io;
int main() {
    int r = io.puts("hello world");
    if (r < 0) return 1;
    return 0;
}
