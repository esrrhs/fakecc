// long double parameters arrive in a 16-byte stack slot and move through the
// x87 stack, so they must not be promoted into the GP register file — a
// promoted copy would be reloaded as an integer and refilled with fild.
// expect: 42
// expect_stdout: 375 375 375 -375 1000
package main;


import runtime;
static long double id1(long double a) { return a; }
static long double id2(int x, long double a) { return a; }
static long double id3(char *s, long double a, int prec) { (void)s; (void)prec; return a; }
static long double negate(long double a) { return -a; }
static long double from_u64(unsigned long long u) { return (long double)u; }

int main() {
    char buf[8];
    double d = 3.75;
    long double v = (long double)d;
    runtime.printf("%ld %ld %ld %ld %ld\n",
           (long)(id1(v) * 100), (long)(id2(7, v) * 100),
           (long)(id3(buf, v, 6) * 100), (long)(negate(v) * 100),
           (long)(from_u64(1000ULL)));
    if ((long)(id1(v) * 100) != 375) return 1;
    if ((long)(negate(v) * 100) != -375) return 2;
    return 42;
}
