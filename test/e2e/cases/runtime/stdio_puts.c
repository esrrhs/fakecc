// puts: writes a string followed by a newline to stdout.  Pin that it
// returns non-negative and that the string round-trips.  The exact line
// is also checked via expect_stdout.
// expect: 0
// expect_stdout: hello world
package main;
extern int puts(const char *s);
int main() {
    int r = puts("hello world");
    if (r < 0) return 1;
    return 0;
}
