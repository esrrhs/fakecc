// expect: 60
// A VLA allocated before va_start must not move the register-save area:
// va_arg still reads the incoming GP arguments (10+20+30).
package main;
int sum3(int n, ...) {
    int dummy[n];
    dummy[0] = 0;
    va_list ap;
    va_start(ap, n);
    int a = va_arg(ap, int);
    int b = va_arg(ap, int);
    int c = va_arg(ap, int);
    va_end(ap);
    return a + b + c + dummy[0];
}
int main(void) { return sum3(3, 10, 20, 30); }
