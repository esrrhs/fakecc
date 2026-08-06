// expect: 60
// Variadic sum of exactly 3 ints (no loop — each va_arg is explicit).
// Verifies va_start/va_arg/va_end walk the register-save area for GP-class
// arguments.  Uses a fixed arity to avoid needing the named param to stay
// live across va_arg calls.
package main;
int sum3(int n, ...) {
    va_list ap;
    va_start(ap, n);
    int a = va_arg(ap, int);
    int b = va_arg(ap, int);
    int c = va_arg(ap, int);
    va_end(ap);
    return a + b + c;
}
int main(void) { return sum3(3, 10, 20, 30); }
