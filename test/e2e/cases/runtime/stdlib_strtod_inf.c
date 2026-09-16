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
    return 0;
}
