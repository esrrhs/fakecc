// expect: 0
// IEEE fabs(-0.0) is +0.0; a signed compare `x < 0` leaves the sign bit.
package main;
import runtime;
int main(void) {
    double nz = 0.0;
    nz = -nz;
    double z = runtime.fabs(nz);
    if (z != 0.0) return 1;
    if (1.0 / z < 0.0) return 2;
    if (runtime.fabs(-3.5) != 3.5) return 3;
    float fnz = 0.0f;
    fnz = -fnz;
    float fz = runtime.fabsf(fnz);
    if (fz != 0.0f) return 4;
    if (1.0f / fz < 0.0f) return 5;
    return 0;
}
