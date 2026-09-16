// expect: 0
// strtod("inf") must not read past the terminating NUL (4-byte buffer).
package main;
import runtime;
int main(void) {
    char inf[4];
    inf[0] = 'i'; inf[1] = 'n'; inf[2] = 'f'; inf[3] = 0;
    double v = runtime.strtod(inf, 0);
    if (!(v > 1.0e300)) return 1;
    char n[4];
    n[0] = 'n'; n[1] = 'a'; n[2] = 'n'; n[3] = 0;
    double nan = runtime.strtod(n, 0);
    if (nan == nan) return 2;
    return 0;
}
