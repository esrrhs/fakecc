// std.atoi: converts a decimal string to int.  Implemented on top of std.strtol,
// so it skips leading whitespace and honours a leading sign.  Pin the
// positive, negative, whitespace-prefixed, plus-sign, and zero cases.
// expect: 0
package main;
import std;
int main() {
    if (std.atoi("42") != 42) return 1;
    if (std.atoi("-7") != -7) return 2;
    if (std.atoi("  123") != 123) return 3; /* leading whitespace skipped */
    if (std.atoi("+5") != 5) return 4;
    if (std.atoi("0") != 0) return 5;
    return 0;
}
