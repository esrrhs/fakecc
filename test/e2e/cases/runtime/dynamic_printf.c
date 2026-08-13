// expect: 0
// expect_stdout: hello 42
// Call libc printf (declared extern) through the PLT.  printf returns the
// number of characters printed; we sanity-check that and return 0 on success.
package main;
extern int printf(const char *fmt, ...);
int main(void) {
    int n = printf("hello %d\n", 42);
    if (n != 9) return 1;  // "hello 42\n" is 9 chars
    return 0;
}
