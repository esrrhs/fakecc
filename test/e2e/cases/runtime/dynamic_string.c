// expect: 5
// runtime.strlen over a string literal, returned as the exit code.
package main;
import runtime;
int main(void) { return (int)runtime.strlen("hello"); }
