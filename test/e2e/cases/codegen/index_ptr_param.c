// expect: 0
// expect_stdout: init 42
// Sole-use frame LOAD of a pointer parameter into `p[i]` must stay live
// when the address ADD is folded into the dereference (vsnprintf fmt[i]).
package main;
import runtime;

int sum_chars(const char *s, int n) {
    int i;
    int acc;
    acc = 0;
    for (i = 0; i < n; i = i + 1)
        acc = acc + (int)s[i];
    return acc;
}

int main(void) {
    int acc;
    acc = 42;
    if (sum_chars("ABC", 3) != 198) return 1;
    runtime.printf("init %d\n", acc);
    return 0;
}
