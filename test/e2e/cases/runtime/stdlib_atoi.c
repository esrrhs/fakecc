// runtime.atoi: converts a decimal string to int.  Implemented on top of runtime.strtol,
// so it skips leading whitespace and honours a leading sign.  Pin the
// positive, negative, whitespace-prefixed, plus-sign, and zero cases.
// expect: 0
package main;
import runtime;
int main() {
    if (runtime.atoi("42") != 42) return 1;
    if (runtime.atoi("-7") != -7) return 2;
    if (runtime.atoi("  123") != 123) return 3; /* leading whitespace skipped */
    if (runtime.atoi("+5") != 5) return 4;
    if (runtime.atoi("0") != 0) return 5;
    return 0;
}
