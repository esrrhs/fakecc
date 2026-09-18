// expect: 0
// %n must consume the int* so later arguments stay aligned; %a is a hex
// float conversion, not an unknown specifier that reprints the next %f.
package main;
import runtime;
int main(void) {
    char buf[80];
    int n = -1;
    runtime.sprintf(buf, "%d%n%d", 1, &n, 2);
    if (buf[0] != '1' || buf[1] != '2' || buf[2] != '\0') return 1;
    if (n != 1) return 2;

    runtime.sprintf(buf, "%a %f", 1.0, 2.0);
    /* glibc: "0x1p+0 2.000000" */
    if (runtime.strcmp(buf, "0x1p+0 2.000000") != 0) return 3;

    runtime.sprintf(buf, "%a", 3.0);
    if (runtime.strcmp(buf, "0x1.8p+1") != 0) return 4;

    runtime.sprintf(buf, "%A", 0.5);
    if (runtime.strcmp(buf, "0X1P-1") != 0) return 5;

    runtime.sprintf(buf, "%a", 0.0);
    if (runtime.strcmp(buf, "0x0p+0") != 0) return 6;

    runtime.sprintf(buf, "%.2a", 1.0);
    if (runtime.strcmp(buf, "0x1.00p+0") != 0) return 7;

    runtime.sprintf(buf, "%a", -2.0);
    if (runtime.strcmp(buf, "-0x1p+1") != 0) return 8;

    n = -1;
    runtime.sprintf(buf, "%s%n", "hi", &n);
    if (n != 2) return 9;

    /* Round-to-nearest-even: 1.5 = 0x1.8p+0, %.0a ties up to 0x2p+0. */
    runtime.sprintf(buf, "%.0a", 1.5);
    if (runtime.strcmp(buf, "0x2p+0") != 0) return 10;

    runtime.sprintf(buf, "%.1a", 1.09375);
    if (runtime.strcmp(buf, "0x1.2p+0") != 0) return 11;

    /* Tie-to-even: 0x1.08p+0 keeps the even 0. */
    runtime.sprintf(buf, "%.1a", 1.03125);
    if (runtime.strcmp(buf, "0x1.0p+0") != 0) return 12;

    runtime.sprintf(buf, "%.0La", 1.09375L);
    if (runtime.strcmp(buf, "0x9p-3") != 0) return 13;

    return 0;
}
