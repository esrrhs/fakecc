// expect_error
// va_arg's second argument must be a type, not an expression.  Passing an
// integer expression should fail to parse.
package main;
int main(void) {
    va_list ap;
    va_start(ap, 0);
    int x = va_arg(ap, 42);
    va_end(ap);
    return x;
}
