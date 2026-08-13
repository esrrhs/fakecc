// expect: 7
// Variadic sum of exactly 3 doubles returned as int (no loop — each va_arg
// explicit).  Verifies va_arg walks the FP (XMM) register-save area.
package main;
int dsum3(int n, ...) {
    va_list ap;
    va_start(ap, n);
    double a = va_arg(ap, double);
    double b = va_arg(ap, double);
    double c = va_arg(ap, double);
    va_end(ap);
    return (int)(a + b + c);
}
int main(void) { return dsum3(3, 1.5, 2.5, 3.0); }
