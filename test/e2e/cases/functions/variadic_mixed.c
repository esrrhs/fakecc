// expect: 8
// Mixed int and double varargs (no loop — each va_arg explicit).  Verifies the
// GP and FP save-area walks are independent: an int from the GP area, a
// double from the FP area.
package main;
int mixed(int n, ...) {
    va_list ap;
    va_start(ap, n);
    int a = va_arg(ap, int);
    double b = va_arg(ap, double);
    va_end(ap);
    return a + (int)b;
}
int main(void) { return mixed(2, 5, 3.5); }
