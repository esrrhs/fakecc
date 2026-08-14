// runtime.putchar: writes a single character to runtime.stdout.  Pin a few characters
// including 'A', 'B', and a newline.
// expect: 0
// expect_stdout: AB
package main;
import runtime;
int main() {
    if (runtime.putchar('A') < 0) return 1;
    if (runtime.putchar('B') < 0) return 2;
    if (runtime.putchar('\n') < 0) return 3;
    return 0;
}
