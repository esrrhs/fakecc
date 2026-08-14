// expect: 0
// expect_stdout: ab
// expect_stdout: cd
// Multiple fmt.printf calls must each appear on stdout after exit flushes.
package main;
import fmt;
int main(void) {
    if (fmt.printf("ab\n") != 3) return 1;
    if (fmt.printf("cd\n") != 3) return 2;
    return 0;
}
