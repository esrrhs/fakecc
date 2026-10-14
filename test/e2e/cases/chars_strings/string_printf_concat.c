// expect: 11
// expect_stdout: hello world
// skip_oracle: clang
// libc puts (via runtime.printf of a plain string) writes the full payload.
package main;
import runtime;
int main(void) {
    char *s = "hello" " " "world";
    int n = runtime.printf("%s\n", s);
    if (n != 12) return 1; /* "hello world\n" */
    return n - 1;          /* 11 chars without newline */
}
