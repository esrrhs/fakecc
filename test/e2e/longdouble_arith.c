// expect: 0
// long double arithmetic: + - * / and comparison.
package main;
int main() {
    long double a = 6.0L;
    long double b = 2.0L;
    long double q = a / b; /* 3.0 */
    long double d = a - b; /* 4.0 */
    long double m = a * b; /* 12.0 */
    if (q > 2.0L && d > 3.0L && m > 11.0L) return 0;
    return 1;
}
