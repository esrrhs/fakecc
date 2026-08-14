// io.putchar: writes a single character to io.stdout.  Pin a few characters
// including 'A', 'B', and a newline.
// expect: 0
// expect_stdout: AB
package main;
import io;
int main() {
    if (io.putchar('A') < 0) return 1;
    if (io.putchar('B') < 0) return 2;
    if (io.putchar('\n') < 0) return 3;
    return 0;
}
