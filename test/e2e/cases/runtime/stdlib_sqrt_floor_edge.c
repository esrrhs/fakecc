// expect: 0
// Annex F: sqrt(+Inf) is +Inf, sqrt(±0) keeps the sign; floor(±0) keeps the sign.
package main;
import runtime;
int main(void) {
    double inf = runtime.strtod("inf", 0);
    double s = runtime.sqrt(inf);
    if (!(s > 1.0e300) || s != s) return 1;

    double nz = 0.0;
    nz = -nz;
    double z = runtime.sqrt(nz);
    if (z != 0.0) return 2;
    if (1.0 / z > 0.0) return 3;

    z = runtime.floor(nz);
    if (z != 0.0) return 4;
    if (1.0 / z > 0.0) return 5;
    z = runtime.ceil(nz);
    if (z != 0.0) return 6;
    if (1.0 / z > 0.0) return 7;

    if (runtime.floor(3.7) != 3.0) return 8;
    if (runtime.sqrt(0.0) != 0.0) return 9;
    return 0;
}
