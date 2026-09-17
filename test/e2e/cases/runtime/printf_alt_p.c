// expect: 0
// `#` on f/e always prints a decimal point; on g it keeps trailing zeros.
// `%#.0o` of 0 is `0`.  glibc `%p` of NULL is `(nil)`.
package main;
import runtime;
int main(void) {
    char buf[80];

    runtime.sprintf(buf, "%#.0f", 1.0);
    if (runtime.strcmp(buf, "1.") != 0) return 1;

    runtime.sprintf(buf, "%#.0e", 1.0);
    if (runtime.strcmp(buf, "1.e+00") != 0) return 2;

    runtime.sprintf(buf, "%#g", 1.0);
    if (runtime.strcmp(buf, "1.00000") != 0) return 3;

    runtime.sprintf(buf, "%g", 1.0);
    if (runtime.strcmp(buf, "1") != 0) return 4;

    runtime.sprintf(buf, "%#.0o", 0);
    if (runtime.strcmp(buf, "0") != 0) return 5;

    runtime.sprintf(buf, "%.0o", 0);
    if (runtime.strcmp(buf, "") != 0) return 6;

    runtime.sprintf(buf, "%p", (void *)0);
    if (runtime.strcmp(buf, "(nil)") != 0) return 7;

    runtime.sprintf(buf, "%#f", 1.0);
    if (runtime.strcmp(buf, "1.000000") != 0) return 8;
    return 0;
}
