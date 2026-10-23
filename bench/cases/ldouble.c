/* long double (x87 80-bit) loop-carried dependence; dyadic so rounding matches. */
package main;
import runtime;

enum { N = 8000000 };

int main(void) {
    int i;
    long double x, y;
    int scaled;

    x = 1.0L;
    y = 0.5L;
    for (i = 0; i < N; i++) {
        x = (x + (long double)(i & 15)) * 0.5L;
        y = (y * 0.5L) + (long double)(i & 7);
        /* Keep y from growing without bound. */
        y = y * 0.25L + x;
    }
    if (x < 0.0L)
        x = -x;
    if (y < 0.0L)
        y = -y;
    scaled = (int)(x * 1024.0L) + (int)(y * 64.0L);
    runtime.printf("ldouble %d\n", scaled);
    return 0;
}
