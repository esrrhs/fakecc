// expect_error
// Empty-parens `f()` is unprototyped, but FakeCC still requires zero arguments.
package main;
void f();
int main(void) { f(1); return 0; }
