// expect_error
// A void function cannot return a value.
package main;
void f(void) { return 5; }
int main() { return 0; }
