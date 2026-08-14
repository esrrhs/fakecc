// expect: 11
// expect_stdout: hello world
// libc puts (via fmt.printf of a plain string) writes the full payload.
package main;
import fmt;
int main(void) {
    char *s = "hello" " " "world";
    int n = fmt.printf("%s\n", s);
    if (n != 12) return 1; /* "hello world\n" */
    return n - 1;          /* 11 chars without newline */
}
