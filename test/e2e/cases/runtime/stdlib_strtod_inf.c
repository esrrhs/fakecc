// expect: 0
// strtod must accept INF/INFINITY/NAN; sqrt of a negative is a NaN, not 0.
package main;
import runtime;
int main(void) {
    double inf = runtime.strtod("inf", 0);
    double ninf = runtime.strtod("-INFINITY", 0);
    double nan = runtime.strtod("nan", 0);
    if (!(inf > 1.0e300)) return 1;
    if (!(ninf < -1.0e300)) return 2;
    if (nan == nan) return 3;

    double s = runtime.sqrt(-1.0);
    if (s == s) return 4;
    if (runtime.sqrt(0.0) != 0.0) return 5;

    char *end;
    double z = runtime.strtod("0x", &end);
    if (z != 0.0) return 6;
    if (end[0] != 'x') return 7;

    /* NAN( without a closing paren is just "nan", remainder at '(' */
    double n2 = runtime.strtod("nan(foo", &end);
    if (n2 == n2) return 8;
    if (end[0] != '(') return 9;
    n2 = runtime.strtod("nan(foo)", &end);
    if (n2 == n2) return 10;
    if (end[0] != '\0') return 11;
    return 0;
}
