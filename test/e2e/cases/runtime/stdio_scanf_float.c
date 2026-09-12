// runtime.sscanf / runtime.fscanf floating conversions: %f/%e/%g/%le/%lf/%a,
// field width, leftover input, inf/nan, and a fprintf/fscanf file round-trip.
// expect: 0
package main;
import runtime;
int main() {
    float f = 0;
    double d = 0;
    long double ld = 0;
    int r, n, a;
    char rest[8];

    r = runtime.sscanf("1.5", "%f", &f);
    if (r != 1 || f != 1.5f) return 1;

    r = runtime.sscanf("-2.5", "%e", &f);
    if (r != 1 || f != -2.5f) return 2;

    r = runtime.sscanf("1.25e3", "%le", &d);
    if (r != 1 || d != 1250.0) return 3;

    r = runtime.sscanf("0.5", "%lf", &d);
    if (r != 1 || d != 0.5) return 4;

    r = runtime.sscanf("1.5", "%Lf", &ld);
    if (r != 1 || ld != 1.5L) return 5;

    r = runtime.sscanf("1e3 xyz", "%g%n%s", &f, &n, rest);
    if (r != 2) return 6;
    if ((int)f != 1000) return 7;
    if (n != 3) return 8;
    if (rest[0] != 'x') return 9;

    r = runtime.sscanf("123.45", "%4f", &f);
    if (r != 1 || f != 123.0f) return 10;

    r = runtime.sscanf("  7.125", "%f", &f);
    if (r != 1 || f != 7.125f) return 11;

    r = runtime.sscanf("1.2e+2", "%f", &f);
    if (r != 1 || f != 120.0f) return 12;

    r = runtime.sscanf("0x1.0p4", "%a", &f);
    if (r != 1 || f != 16.0f) return 13;

    r = runtime.sscanf("1.25xyz", "%le%s", &d, rest);
    if (r != 2 || d != 1.25) return 14;
    if (rest[0] != 'x') return 15;

    r = runtime.sscanf("inf", "%f", &f);
    if (r != 1 || f + f != f || f == 0) return 16;

    r = runtime.sscanf("-INFINITY", "%le", &d);
    if (r != 1 || d + d != d || d >= 0) return 17;

    r = runtime.sscanf("nan", "%f", &f);
    if (r != 1 || f == f) return 18;

    r = runtime.sscanf("ix", "%f", &f);
    if (r != 0) return 19;

    r = runtime.sscanf("9abc", "%*f%d", &a);
    if (r != 0) return 20;

    r = runtime.sscanf("9 8", "%*f%d", &a);
    if (r != 1 || a != 8) return 21;

    {
        runtime.FILE *fp = runtime.fopen("/tmp/fakecc_scanf_float.dat", "w");
        if (fp == 0) return 22;
        if (runtime.fprintf(fp, " %.20e %.20e", 1.25, -3.5) < 0) {
            runtime.fclose(fp);
            return 23;
        }
        if (runtime.fclose(fp) != 0) return 24;

        fp = runtime.fopen("/tmp/fakecc_scanf_float.dat", "r");
        if (fp == 0) return 25;
        double x = 0, y = 0;
        r = runtime.fscanf(fp, " %le %le", &x, &y);
        runtime.fclose(fp);
        runtime.remove("/tmp/fakecc_scanf_float.dat");
        if (r != 2) return 26;
        if (x != 1.25) return 27;
        if (y != -3.5) return 28;
    }

    return 0;
}
