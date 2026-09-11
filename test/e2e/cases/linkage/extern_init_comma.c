// expect: 3
// `extern int a = 1, b;` clears extern only on `a`.  `b` stays a declaration
// and is defined by the later `int b = 2`.
package main;
extern int a = 1, b;
int b = 2;
int main(void) { return a + b; }
