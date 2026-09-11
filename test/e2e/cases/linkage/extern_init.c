// expect: 42
// `extern int x = 42;` is a definition (GCC).  The object must be emitted.
package main;
extern int x = 42;
int main(void) { return x; }
