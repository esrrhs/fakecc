// expect: 0
// expect_stdout: ab
// expect_stdout: cd
// Multiple runtime.printf calls must each appear on stdout after exit flushes.
package main;
import runtime;
int main(void) {
    if (runtime.printf("ab\n") != 3) return 1;
    if (runtime.printf("cd\n") != 3) return 2;
    return 0;
}
