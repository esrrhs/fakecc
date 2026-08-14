// atoi: converts a decimal string to int.  Implemented on top of strtol,
// so it skips leading whitespace and honours a leading sign.  Pin the
// positive, negative, whitespace-prefixed, plus-sign, and zero cases.
// expect: 0
package main;
extern int atoi(const char *s);
int main() {
    if (atoi("42") != 42) return 1;
    if (atoi("-7") != -7) return 2;
    if (atoi("  123") != 123) return 3; /* leading whitespace skipped */
    if (atoi("+5") != 5) return 4;
    if (atoi("0") != 0) return 5;
    return 0;
}
