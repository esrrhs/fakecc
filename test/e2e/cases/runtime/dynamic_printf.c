// expect: 0
// expect_stdout: hello 42
// rt fmt.printf via import; check the return value and that stdout is flushed.
package main;
import fmt;
int main(void) {
    int n = fmt.printf("hello %d\n", 42);
    if (n != 9) return 1;  // "hello 42\n" is 9 chars
    return 0;
}
