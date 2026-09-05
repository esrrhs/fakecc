// expect_error
// va_arg(ap, void) must be rejected: 'void' is an incomplete type and cannot
// be the type argument to va_arg.  Mirrors the GCC torture test
// test/compile/pr48767.c so the e2e suite catches a regression in the
// semantic check.
package main;
int foo(__builtin_va_list ap) {
    return __builtin_va_arg(ap, void);
}
int main(void) {
    return 0;
}
