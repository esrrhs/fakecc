// Float/double passed as arguments.  foo(2.5, 3.5) returns int (int)(a+b)=6.
// expect: 6
package main;
int foo(double a, double b) { return (int)(a + b); }
int main() { return foo(2.5, 3.5); }
