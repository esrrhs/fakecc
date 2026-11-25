// expect_error
// A brace initializer must not allocate one AST slot per element.
// fuzz_compile: `char buf[2000000000] = {1}` asked for ~16 GB and the
// OOM killer took down the runner (exit 137 / lost communication).
package main;
char buf[2000000000] = { 1 };
int main(void) { return buf[0]; }
