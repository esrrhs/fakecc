// expect_error
// `void f(void)` must be called with no arguments.
package main;
void f(void) {}
int main(void) { f(1); return 0; }
