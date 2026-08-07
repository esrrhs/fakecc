// expect_error
// Calling a function that was never declared (not even extern) must fail to
// compile — sema should reject the undeclared callee before codegen.
package main;
int main(void) { return nosuch(1); }
