// expect: 0
// Scalar float in `if` / `!` / `&&` / `||` is an FCMP against +0.0, not a GP test.
package main;
int main(void) {
    double z = 0.0;
    double nz = 2.5;
    double n0 = -0.0;
    if (z) return 1;
    if (!nz) return 2;
    if (n0) return 3;
    if (!(nz && !z)) return 4;
    if (z || n0) return 5;
    if (!(nz || z)) return 6;
    float fz = 0.0f;
    float fn = 1.0f;
    if (fz) return 7;
    if (!fn) return 8;
    return 0;
}
