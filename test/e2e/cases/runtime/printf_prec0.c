// expect: 0
// C99: converting 0 with precision 0 yields no digits; %F/%E/%G inf/nan are uppercase.
// skip_oracle: clang
package main;
import runtime;
int main(void) {
    char buf[64];
    if (runtime.sprintf(buf, "%.0d", 0) != 0) return 1;
    if (buf[0] != 0) return 2;
    if (runtime.sprintf(buf, "%.0d", 1) != 1) return 3;
    if (runtime.strcmp(buf, "1") != 0) return 4;

    double inf = runtime.strtod("inf", 0);
    double nan = runtime.strtod("nan", 0);
    runtime.sprintf(buf, "%F", inf);
    if (runtime.strcmp(buf, "INF") != 0) return 5;
    runtime.sprintf(buf, "%f", inf);
    if (runtime.strcmp(buf, "inf") != 0) return 6;
    /* The sign of a NaN is not pinned down by C and differs by hardware --
     * x86-64 gives "-NAN" where arm64 gives "NAN" -- so accept either. */
    runtime.sprintf(buf, "%F", nan);
    const char *nanp = buf;
    if (*nanp == '-' || *nanp == '+') nanp++;
    if (runtime.strcmp(nanp, "NAN") != 0) return 7;
    return 0;
}
