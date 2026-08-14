// rt/fmt.printf.c's floating-point conversions, plus the flags and width handling
// that all conversions now share.
// expect: 42
// expect_stdout: 3.750000|-0.500000|0.333333|1234.568|0.000000
// expect_stdout: 1.234568e+03|1.230000E-04|0.0001|100000|1.23457e+06|1.5
// expect_stdout: [      3.14][3.14      ][0000003.14][+3.1][ 3.1]
// expect_stdout: [+42][ 42][00042][42    ][00042][0xff][   ab][ab   ][ab]
// expect_stdout: [     7][7     ][2.500]
// expect_stdout: inf|-inf|-nan
// expect_stdout: 2|2.2|12.35
package main;


import fmt;
int main() {
    fmt.printf("%f|%f|%f|%.3f|%f\n", 3.75, -0.5, 1.0 / 3.0, 1234.56789, 0.0);
    fmt.printf("%e|%E|%g|%g|%g|%g\n", 1234.5678, 0.000123, 0.0001, 100000.0,
           1234567.0, 1.5);
    fmt.printf("[%10.2f][%-10.2f][%010.2f][%+.1f][% .1f]\n",
           3.14159, 3.14159, 3.14159, 3.14159, 3.14159);
    fmt.printf("[%+d][% d][%05d][%-6d][%.5d][%#x][%5s][%-5s][%.2s]\n",
           42, 42, 42, 42, 42, 255, "ab", "ab", "abcd");
    fmt.printf("[%*d][%-*d][%.*f]\n", 6, 7, 6, 7, 3, 2.5);

    double huge = 1e308;
    double inf = huge * 10.0;
    fmt.printf("%f|%f|%f\n", inf, -inf, inf - inf);

    /* Ties round to even, the way the default FP rounding mode leaves them,
     * and a value that only looks like a tie in fewer digits does not. */
    char buf[64];
    int n = fmt.snprintf(buf, sizeof buf, "%.2f", 12.345);
    fmt.printf("%.0f|%.1f|%s\n", 2.5, 2.25, buf);
    if (n != 5) return 1;
    return 42;
}
