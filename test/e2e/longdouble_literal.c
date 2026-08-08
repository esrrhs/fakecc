// expect: 0
// long double literals: 1.5L, .5L, suffix classification.
package main;
int main() {
    long double a = 1.5L;
    long double b = .5L;
    if (a > b) return 0;
    return 1;
}
