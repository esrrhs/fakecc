// putchar: writes a single character to stdout.  Pin a few characters
// including 'A', 'B', and a newline.
// expect: 0
// expect_stdout: AB
package main;
extern int putchar(int c);
int main() {
    if (putchar('A') < 0) return 1;
    if (putchar('B') < 0) return 2;
    if (putchar('\n') < 0) return 3;
    return 0;
}
