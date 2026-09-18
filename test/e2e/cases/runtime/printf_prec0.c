// expect: 0
// C99: converting 0 with precision 0 yields no digits; %F/%E/%G inf/nan are uppercase.
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
    runtime.sprintf(buf, "%F", nan);
    if (runtime.strcmp(buf, "NAN") != 0) return 7;
    return 0;
}
