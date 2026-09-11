// expect: 1
// Variadic extra arguments get default promotions: float is passed as double
// so `va_arg(ap, double)` of a `1.0f` argument yields 1.0.
package main;
int pick(int n, ...) {
    va_list ap;
    va_start(ap, n);
    double d = va_arg(ap, double);
    va_end(ap);
    return d == 1.0;
}
int main(void) { return pick(1, 1.0f); }
