// expect: 0
// `inline` function specifier: define and call an inline function.  In this
// single-TU model inline is a no-op hint, so the function must lower and run
// exactly like an ordinary definition.  Returns 0 on success or a failing
// sentinel.
package main;
static inline int square(int n) {
    return n * n;
}
int main() {
    if (square(6) != 36) return 1;
    return 0;
}
