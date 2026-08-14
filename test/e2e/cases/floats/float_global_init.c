// A float-typed global is packed in its own binary format: folding the
// initializer as an integer would store 1 as the bit pattern 0x1, not 1.0.
// expect: 42
// expect_stdout: 15 -25 10 35 20 45
package main;


import fmt;
double g_a = 1.5;
double g_b = -2.5;
double g_int = 1;
float  g_f = 3.5f;
long double g_ld = 2.0L;
double g_arr[2] = { 1.0, 3.5 };

int main() {
    fmt.printf("%ld %ld %ld %ld %ld %ld\n",
           (long)(g_a * 10), (long)(g_b * 10), (long)(g_int * 10),
           (long)(g_f * 10), (long)(g_ld * 10),
           (long)((g_arr[0] + g_arr[1]) * 10));
    if ((long)(g_int * 10) != 10) return 1;
    if ((long)(g_b * 10) != -25) return 2;
    return 42;
}
