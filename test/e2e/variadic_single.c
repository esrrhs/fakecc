// expect: 5
// Single variadic int argument (no use of the count beyond driving one
// va_arg).  Verifies the simplest variadic walk.
package main;
int first(int n, ...) {
    va_list ap;
    va_start(ap, n);
    int x = va_arg(ap, int);
    va_end(ap);
    return x;
}
int main(void) { return first(1, 5); }
