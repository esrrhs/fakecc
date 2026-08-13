// Compound assignment and ++/-- on floating types stay in the float domain:
// lowering them to the integer ADD/SUB would operate on the bit pattern.
// expect: 42
// expect_stdout: 25 80 20 75 125 150 25 50
package main;

extern int printf(const char *fmt, ...);

double g_d = 2.5;
struct S { double x; float y; };

int main() {
    double a = 1.5;
    a++;
    ++a;
    a--;                    /* 2.5 */
    float f = 2.5f;
    f += 1.5f;
    f *= 2.0f;              /* 8.0 */
    g_d -= 0.5;             /* 2.0 */
    double arr[3];
    arr[1] = 3.0;
    arr[1] /= 4.0;          /* 0.75 */
    struct S s;
    s.x = 1.0;
    s.y = 2.0f;
    s.x += 0.25;            /* 1.25 */
    s.y -= 0.5f;            /* 1.5 */
    long double ld = 4.5L;
    ld += 0.5L;
    ld /= 2.0L;             /* 2.5 */
    double m = 2.0;
    int i = 3;
    m += i;                 /* 5.0 */
    printf("%ld %ld %ld %ld %ld %ld %ld %ld\n",
           (long)(a * 10), (long)(f * 10), (long)(g_d * 10),
           (long)(arr[1] * 100), (long)(s.x * 100), (long)(s.y * 100),
           (long)(ld * 10), (long)(m * 10));
    if ((long)(a * 10) != 25) return 1;
    if ((long)(f * 10) != 80) return 2;
    if ((long)(ld * 10) != 25) return 3;
    return 42;
}
