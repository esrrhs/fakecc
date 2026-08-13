// expect: 100
// Function taking narrow-typed params; passing wider ints.
package main;
int square_short(short x) { return x * x; }
int main() { return square_short(10); }
