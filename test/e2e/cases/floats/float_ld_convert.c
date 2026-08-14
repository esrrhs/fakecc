// Converting between long double and the SSE float types goes through a stack
// slot: the x87 store and the SSE reload must agree on the width, or a
// (float)long_double round trip reads a double-sized slot as a float.
// expect: 42
// expect_stdout: 325 7125 350 -125 650 14250 0 25 50
package main;


import fmt;
static long double widen_f(float f) { return (long double)f; }
static float narrow_ld(long double x) { return (float)x; }

int main() {
    float f = 3.25f;
    double d = 7.125;
    long double a = (long double)f;
    long double b = (long double)d;
    float nf = narrow_ld(3.5L);
    float neg = (float)(-1.25L);

    fmt.printf("%ld %ld %ld %ld ",
           (long)(a * 100), (long)(b * 1000),
           (long)(nf * 100), (long)(neg * 100));

    float back = (float)(widen_f(f) * 2);
    double dback = (double)(b * 2);
    fmt.printf("%ld %ld ", (long)(back * 100), (long)(dback * 1000));

    float arr[3];
    for (int i = 0; i < 3; i++) arr[i] = (float)((long double)i / 4.0L);
    fmt.printf("%ld %ld %ld\n",
           (long)(arr[0] * 100), (long)(arr[1] * 100), (long)(arr[2] * 100));

    if ((long)(nf * 100) != 350) return 1;
    if ((long)(a * 100) != 325) return 2;
    return 42;
}
