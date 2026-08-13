// expect: 0
// expect_stdout: ab
// expect_stdout: cd
// Multiple printf calls must each appear on stdout after exit flushes.
package main;
extern int printf(const char *fmt, ...);
int main(void) {
    if (printf("ab\n") != 3) return 1;
    if (printf("cd\n") != 3) return 2;
    return 0;
}
