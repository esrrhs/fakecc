// The x86 int/float conversions are all signed, so both directions need the
// 2^63 split to cover the top half of the unsigned 64-bit range.
// expect: 42
// expect_stdout: 1 1 1 1 3000000000 4000000000
package main;

extern int printf(const char *fmt, ...);

int main() {
    unsigned long long big = 18446744073709551615ULL;
    unsigned long long half = 9300000000000000000ULL;
    double d_big = (double)big;
    long double l_big = (long double)big;
    unsigned long w = 3000000000UL;
    unsigned int ui = 4000000000U;
    printf("%d %d %d %d %lu %u\n",
           d_big > 1.8e19, l_big > 1.8e19,
           (unsigned long long)l_big == big,
           (unsigned long long)(double)half >= 9200000000000000000ULL,
           (unsigned long)(double)w, (unsigned int)(double)ui);
    if ((unsigned long long)l_big != big) return 1;
    if ((unsigned long)(double)w != 3000000000UL) return 2;
    if ((unsigned int)3000000000.0 != 3000000000U) return 3;
    return 42;
}
