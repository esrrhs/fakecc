// expect: 1
// Empty-parens function pointer may be called with no arguments.
package main;
int one(void) { return 1; }
int main(void) {
    int (*fp)();
    fp = one;
    return fp();
}
