// expect: 0
// long double: declare, initialize, arithmetic, compare.
package main;
int main() {
    long double x = 1.5L;
    long double y = 2.5L;
    long double z = x + y;
    if (z > 3.0L) return 0;
    return 1;
}
